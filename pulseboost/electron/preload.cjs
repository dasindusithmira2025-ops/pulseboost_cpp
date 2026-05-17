const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("pulseboostDesktop", {
  isDesktop: true,
  getMeta: () => ipcRenderer.invoke("pulseboost:get-meta"),
  openLogs: () => ipcRenderer.invoke("pulseboost:open-logs"),
  openDataDir: () => ipcRenderer.invoke("pulseboost:open-data-dir"),
  openExternal: (url) => ipcRenderer.invoke("pulseboost:open-external", url),
  restartGuardian: () => ipcRenderer.invoke("pulseboost:restart-backend"),
  minimizeWindow: () => ipcRenderer.invoke("pulseboost:minimize-window"),
  toggleMaximizeWindow: () => ipcRenderer.invoke("pulseboost:toggle-maximize-window"),
  closeWindow: () => ipcRenderer.invoke("pulseboost:close-window"),
  onShortcut: (callback) => {
    if (typeof callback !== "function") return () => {};
    const handler = (_event, payload) => callback(payload);
    ipcRenderer.on("pulseboost:shortcut", handler);
    return () => ipcRenderer.removeListener("pulseboost:shortcut", handler);
  },
});
