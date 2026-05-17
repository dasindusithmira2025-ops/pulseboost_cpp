const { app, BrowserWindow, globalShortcut, ipcMain, shell } = require("electron");
const { spawn } = require("child_process");
const http = require("http");
const net = require("net");
const path = require("path");
const fs = require("fs");

const APP_VERSION = "0.3.0";
const ROOT_DIR = path.resolve(__dirname, "..");
const WORKSPACE_ROOT = path.resolve(ROOT_DIR, "..");
const PYTHON_EXE = path.join(ROOT_DIR, "tools", "python", "python.exe");
const LOG_DIR = path.join(ROOT_DIR, "run");
const DATA_DIR = path.join(WORKSPACE_ROOT, "data");
const DEFAULT_BACKEND_PORT = 18400;
const MAX_BACKEND_PORT = 18420;

let backendPort = DEFAULT_BACKEND_PORT;
let backendUrl = `http://127.0.0.1:${backendPort}`;

let backendProcess = null;
let mainWindow = null;
let backendOwnedByElectron = false;
let backendStdoutStream = null;
let backendStderrStream = null;
let appLogStream = null;

function ensurePath(targetPath) {
  fs.mkdirSync(targetPath, { recursive: true });
}

function ensureAppLogStream() {
  ensurePath(LOG_DIR);
  if (!appLogStream) {
    appLogStream = fs.createWriteStream(path.join(LOG_DIR, "pulseboost.log"), { flags: "a" });
  }
}

function getBackendUrl(port) {
  return `http://127.0.0.1:${port}`;
}

function waitForBackend(url, timeoutMs = 30000) {
  const deadline = Date.now() + timeoutMs;
  return new Promise((resolve, reject) => {
    const attempt = () => {
      const request = http.get(`${url}/healthz`, (response) => {
        response.resume();
        if (response.statusCode === 200) {
          resolve();
          return;
        }
        if (Date.now() > deadline) {
          reject(new Error("PulseBoost backend did not become ready in time."));
          return;
        }
        setTimeout(attempt, 400);
      });

      request.on("error", () => {
        if (Date.now() > deadline) {
          reject(new Error("PulseBoost backend did not become ready in time."));
          return;
        }
        setTimeout(attempt, 400);
      });
    };

    attempt();
  });
}

function isBackendReachable(url, timeoutMs = 1500) {
  return new Promise((resolve) => {
    const request = http.get(`${url}/healthz`, (response) => {
      response.resume();
      resolve(response.statusCode === 200);
    });
    request.setTimeout(timeoutMs, () => {
      request.destroy();
      resolve(false);
    });
    request.on("error", () => resolve(false));
  });
}

function servesDesktopFrontend(url, timeoutMs = 2000) {
  return new Promise((resolve) => {
    const request = http.get(`${url}/`, (response) => {
      const contentType = String(response.headers?.["content-type"] || "").toLowerCase();
      let body = "";
      response.setEncoding("utf8");
      response.on("data", (chunk) => {
        if (body.length < 4096) {
          body += chunk;
        }
      });
      response.on("end", () => {
        const looksLikeSpa = response.statusCode === 200
          && contentType.includes("text/html")
          && body.includes('id="root"');
        resolve(looksLikeSpa);
      });
    });
    request.setTimeout(timeoutMs, () => {
      request.destroy();
      resolve(false);
    });
    request.on("error", () => resolve(false));
  });
}

function canListenOnPort(port) {
  return new Promise((resolve) => {
    const server = net.createServer();
    server.once("error", () => resolve(false));
    server.once("listening", () => {
      server.close(() => resolve(true));
    });
    server.listen(port, "127.0.0.1");
  });
}

async function findAvailableBackendPort(startPort, endPort) {
  for (let port = startPort; port <= endPort; port += 1) {
    // Probe candidate ports and keep startup deterministic.
    // eslint-disable-next-line no-await-in-loop
    if (await canListenOnPort(port)) {
      return port;
    }
  }
  return null;
}

function spawnManagedBackend(port) {
  const processRef = spawn(
    PYTHON_EXE,
    ["-m", "uvicorn", "serve_app:app", "--host", "127.0.0.1", "--port", String(port)],
    {
      cwd: WORKSPACE_ROOT,
      env: { ...process.env, DESKTOP_RUNTIME: "electron" },
      stdio: ["ignore", "pipe", "pipe"],
      windowsHide: true,
    },
  );
  if (processRef.stdout && backendStdoutStream) {
    processRef.stdout.pipe(backendStdoutStream);
  }
  if (processRef.stderr && backendStderrStream) {
    processRef.stderr.pipe(backendStderrStream);
  }
  return processRef;
}

function waitForManagedBackend(processRef, url, timeoutMs = 30000) {
  const deadline = Date.now() + timeoutMs;
  return new Promise((resolve, reject) => {
    const attempt = async () => {
      if (!processRef || processRef.exitCode !== null) {
        reject(new Error("PulseBoost desktop backend exited before becoming ready."));
        return;
      }

      try {
        const [healthy, servesFrontend] = await Promise.all([
          isBackendReachable(url, 1000),
          servesDesktopFrontend(url, 1200),
        ]);
        if (healthy && servesFrontend) {
          resolve();
          return;
        }
      } catch (_) {
        // Retry path below.
      }

      if (Date.now() > deadline) {
        reject(new Error("PulseBoost desktop backend did not become ready in time."));
        return;
      }
      setTimeout(attempt, 400);
    };

    attempt().catch((error) => reject(error));
  });
}

async function startBackend() {
  if (backendProcess && backendProcess.exitCode === null) {
    await waitForBackend(backendUrl);
    return;
  }

  const defaultUrl = getBackendUrl(DEFAULT_BACKEND_PORT);
  const reachable = await isBackendReachable(defaultUrl);
  if (reachable) {
    const hasDesktopFrontend = await servesDesktopFrontend(defaultUrl);
    if (hasDesktopFrontend) {
      backendPort = DEFAULT_BACKEND_PORT;
      backendUrl = defaultUrl;
      backendProcess = null;
      backendOwnedByElectron = false;
      closeLogStreams();
      return;
    }

    const fallbackPort = await findAvailableBackendPort(DEFAULT_BACKEND_PORT + 1, MAX_BACKEND_PORT);
    if (!fallbackPort) {
      throw new Error(`PulseBoost could not allocate a desktop backend port between ${DEFAULT_BACKEND_PORT + 1} and ${MAX_BACKEND_PORT}.`);
    }
    backendPort = fallbackPort;
    backendUrl = getBackendUrl(backendPort);
  } else {
    backendPort = DEFAULT_BACKEND_PORT;
    backendUrl = defaultUrl;
  }

  ensurePath(LOG_DIR);
  closeLogStreams();
  backendStdoutStream = fs.createWriteStream(path.join(LOG_DIR, "desktop-backend.out.log"), { flags: "a" });
  backendStderrStream = fs.createWriteStream(path.join(LOG_DIR, "desktop-backend.err.log"), { flags: "a" });

  backendProcess = spawnManagedBackend(backendPort);
  backendOwnedByElectron = true;
  try {
    await waitForManagedBackend(backendProcess, backendUrl);
  } catch (error) {
    if (backendPort === DEFAULT_BACKEND_PORT) {
      const fallbackPort = await findAvailableBackendPort(DEFAULT_BACKEND_PORT + 1, MAX_BACKEND_PORT);
      if (!fallbackPort) {
        throw error;
      }
      if (backendProcess && backendProcess.exitCode === null) {
        backendProcess.kill();
      }
      backendPort = fallbackPort;
      backendUrl = getBackendUrl(backendPort);
      backendProcess = spawnManagedBackend(backendPort);
      await waitForManagedBackend(backendProcess, backendUrl);
      return;
    }
    throw error;
  }

  await waitForBackend(backendUrl);
}

async function stopBackend() {
  if (!backendOwnedByElectron || !backendProcess || backendProcess.exitCode !== null) {
    backendProcess = null;
    backendOwnedByElectron = false;
    closeLogStreams();
    return;
  }

  const processRef = backendProcess;
  backendProcess = null;
  backendOwnedByElectron = false;
  processRef.kill();

  await new Promise((resolve) => {
    const timer = setTimeout(() => {
      if (processRef.exitCode === null) {
        processRef.kill("SIGKILL");
      }
      resolve();
    }, 5000);
    processRef.once("exit", () => {
      clearTimeout(timer);
      resolve();
    });
  });
  closeLogStreams();
}

async function restartBackend() {
  await stopBackend();
  await startBackend();
  if (mainWindow) {
    await mainWindow.loadURL(backendUrl);
  }
}

function createWindow() {
  mainWindow = new BrowserWindow({
    title: "PulseBoost",
    width: 1640,
    height: 1040,
    minWidth: 1280,
    minHeight: 820,
    frame: false,
    titleBarStyle: "hidden",
    backgroundColor: "#0f1117",
    show: false,
    autoHideMenuBar: true,
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false,
      preload: path.join(__dirname, "preload.cjs"),
    },
  });

  mainWindow.once("ready-to-show", () => {
    mainWindow.show();
  });

  mainWindow.on("closed", () => {
    mainWindow = null;
  });

  mainWindow.loadURL(backendUrl);
}

function runtimeMeta() {
  return {
    isDesktop: true,
    backendUrl,
    logDir: LOG_DIR,
    dataDir: DATA_DIR,
    runtime: "electron",
    version: APP_VERSION,
    windowControls: true,
  };
}

function emitShortcut(action) {
  if (!mainWindow || mainWindow.isDestroyed()) return;
  mainWindow.webContents.send("pulseboost:shortcut", { action });
}

function registerGlobalShortcuts() {
  globalShortcut.unregisterAll();
  globalShortcut.register("CommandOrControl+Shift+B", () => emitShortcut("boost_now"));
  globalShortcut.register("CommandOrControl+Shift+A", () => emitShortcut("focus_chat"));
  globalShortcut.register("CommandOrControl+Shift+H", () => emitShortcut("go_pulsecore"));
  globalShortcut.register("Escape", () => emitShortcut("dismiss_dialog"));
}

function closeLogStreams() {
  if (backendStdoutStream) {
    try {
      backendStdoutStream.end();
      backendStdoutStream.destroy();
    } catch (_) {
      // Best effort shutdown path.
    }
    backendStdoutStream = null;
  }
  if (backendStderrStream) {
    try {
      backendStderrStream.end();
      backendStderrStream.destroy();
    } catch (_) {
      // Best effort shutdown path.
    }
    backendStderrStream = null;
  }
  if (appLogStream) {
    try {
      appLogStream.end();
      appLogStream.destroy();
    } catch (_) {
      // Best effort shutdown path.
    }
    appLogStream = null;
  }
}

ipcMain.handle("pulseboost:get-meta", async () => runtimeMeta());
ipcMain.handle("pulseboost:open-logs", async () => {
  ensurePath(LOG_DIR);
  return shell.openPath(LOG_DIR);
});
ipcMain.handle("pulseboost:open-data-dir", async () => {
  ensurePath(DATA_DIR);
  return shell.openPath(DATA_DIR);
});
ipcMain.handle("pulseboost:open-external", async (_event, url) => {
  if (!url || typeof url !== "string") {
    return false;
  }
  await shell.openExternal(url);
  return true;
});
ipcMain.handle("pulseboost:restart-backend", async () => {
  await restartBackend();
  return { ok: true };
});
ipcMain.handle("pulseboost:minimize-window", async () => {
  if (!mainWindow) return false;
  mainWindow.minimize();
  return true;
});
ipcMain.handle("pulseboost:toggle-maximize-window", async () => {
  if (!mainWindow) {
    return { ok: false, maximized: false };
  }
  if (mainWindow.isMaximized()) {
    mainWindow.unmaximize();
  } else {
    mainWindow.maximize();
  }
  return { ok: true, maximized: mainWindow.isMaximized() };
});
ipcMain.handle("pulseboost:close-window", async () => {
  if (!mainWindow) return false;
  mainWindow.close();
  return true;
});

app.whenReady().then(async () => {
  app.setAppUserModelId("PulseBoost");
  ensureAppLogStream();
  await startBackend();
  createWindow();
  registerGlobalShortcuts();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
}).catch((error) => {
  console.error("[PulseBoost Electron]", error);
  app.quit();
});

app.on("will-quit", () => {
  globalShortcut.unregisterAll();
});

app.on("window-all-closed", async () => {
  await stopBackend();
  if (process.platform !== "darwin") {
    app.quit();
  }
});

process.on("exit", () => {
  closeLogStreams();
});

process.on("SIGINT", () => {
  closeLogStreams();
  process.exit();
});
