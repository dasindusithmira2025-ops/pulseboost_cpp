$root = $PSScriptRoot
$uiRoot = Join-Path $root "pulseboost\ui"
$nodeRoot = Join-Path $root "pulseboost\tools\node-tmp\node-v20.15.1-win-x64"
$npm = Join-Path $nodeRoot "npm.cmd"
$python = Join-Path $root "pulseboost\tools\python\python.exe"
$electronCmd = Join-Path $uiRoot "node_modules\.bin\electron.cmd"
$desktopApp = Join-Path $root "pulseboost\desktop_app.py"

$env:Path = "$nodeRoot;$env:Path"

if (!(Test-Path $npm)) {
    throw "Portable Node runtime was not found. Expected: $npm"
}
if (!(Test-Path $python)) {
    throw "Portable Python runtime was not found. Expected: $python"
}

# Ensure Electron runs in desktop mode even if the shell inherited node-only mode.
$env:ELECTRON_RUN_AS_NODE = ""

Push-Location $uiRoot
try {
    & $npm run build
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    if (Test-Path $electronCmd) {
        & $npm run desktop:electron
    } elseif (Test-Path $desktopApp) {
        & $npm run desktop:webview
    } else {
        throw "No supported desktop launcher was found. Expected Electron under $electronCmd or WebView entrypoint at $desktopApp"
    }
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
