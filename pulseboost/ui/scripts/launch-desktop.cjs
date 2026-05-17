const { spawn } = require("child_process");
const path = require("path");

const UI_ROOT = path.resolve(__dirname, "..");
const ROOT_DIR = path.resolve(UI_ROOT, "..");
const PYTHON_EXE = path.join(ROOT_DIR, "tools", "python", "python.exe");
const PYTHON_DESKTOP = path.join(ROOT_DIR, "desktop_app.py");
const ELECTRON_MAIN = path.join(ROOT_DIR, "electron", "main.cjs");

function spawnProcess(command, args) {
  const child = spawn(command, args, {
    cwd: UI_ROOT,
    stdio: "inherit",
    windowsHide: false,
  });
  child.on("exit", (code) => {
    process.exit(code ?? 0);
  });
}

try {
  require.resolve("electron");
  spawnProcess(process.execPath, [require.resolve("electron/cli.js"), ELECTRON_MAIN]);
} catch {
  spawnProcess(PYTHON_EXE, [PYTHON_DESKTOP]);
}
