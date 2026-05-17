$root = $PSScriptRoot
$uiRoot = Join-Path $root "pulseboost\ui"
$nodeRoot = Join-Path $root "pulseboost\tools\node-tmp\node-v20.15.1-win-x64"
$npm = Join-Path $nodeRoot "npm.cmd"
$electronCmd = Join-Path $uiRoot "node_modules\.bin\electron.cmd"
$python = Join-Path $root "pulseboost\tools\python\python.exe"
$desktopApp = Join-Path $root "pulseboost\desktop_app.py"
$desktopBackendPorts = 18400..18420

function Get-DesktopLauncher {
    if (Test-Path $electronCmd) {
        return @{
            Name = "electron"
            Script = "desktop:electron"
            ProcessName = "electron"
            CommandLinePattern = "electron[\\/]+main\.cjs"
        }
    }

    if (Test-Path $desktopApp) {
        return @{
            Name = "pywebview"
            Script = "desktop:webview"
            ProcessName = "python"
            CommandLinePattern = "pulseboost[\\/]+desktop_app\.py"
        }
    }

    throw "No supported desktop launcher was found. Expected Electron under $electronCmd or WebView entrypoint at $desktopApp"
}

function Test-BackendReady {
    foreach ($port in $desktopBackendPorts) {
        $healthUrl = "http://127.0.0.1:$port/healthz"
        $rootUrl = "http://127.0.0.1:$port/"
        try {
            $health = Invoke-WebRequest -UseBasicParsing $healthUrl -TimeoutSec 2
            if ($health.StatusCode -ne 200) {
                continue
            }

            $index = Invoke-WebRequest -UseBasicParsing $rootUrl -TimeoutSec 2
            $contentType = [string]$index.Headers["Content-Type"]
            $looksLikeDesktopSpa = $index.StatusCode -eq 200 -and $contentType.ToLower().Contains("text/html") -and [string]$index.Content -match 'id="root"'
            if ($looksLikeDesktopSpa) {
                return @{
                    Ready = $true
                    Url = "http://127.0.0.1:$port"
                }
            }
        }
        catch {
            continue
        }
    }

    return @{
        Ready = $false
        Url = $null
    }
}

function Get-DesktopShellProcess([hashtable]$launcher) {
    $processes = Get-CimInstance Win32_Process -Filter ("Name = '{0}.exe'" -f $launcher.ProcessName) -ErrorAction SilentlyContinue
    if (-not $processes) {
        return $null
    }

    $matching = @()
    foreach ($processInfo in $processes) {
        if ($processInfo.CommandLine -and $processInfo.CommandLine -notmatch $launcher.CommandLinePattern) {
            continue
        }

        $process = Get-Process -Id $processInfo.ProcessId -ErrorAction SilentlyContinue
        if (-not $process) {
            continue
        }

        $matching += $process

        if ($process.MainWindowHandle -ne 0 -or $process.MainWindowTitle -like "*PulseBoost*" -or $process.MainWindowTitle -like "*Electron*") {
            return $process
        }
    }

    if ($launcher.Name -eq "electron" -and $matching.Count -gt 0) {
        return $matching[0]
    }

    return $null
}

New-Item -ItemType Directory -Force -Path (Join-Path $root "pulseboost\run") | Out-Null

if (!(Test-Path $npm)) {
    throw "Portable Node runtime was not found. Expected: $npm"
}
if (!(Test-Path $python)) {
    throw "Portable Python runtime was not found. Expected: $python"
}

# Ensure Electron runs in desktop mode even if the shell inherited node-only mode.
$env:ELECTRON_RUN_AS_NODE = ""

$launcher = Get-DesktopLauncher
$env:Path = "$nodeRoot;$env:Path"

Push-Location $uiRoot
try {
    & $npm run build
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    $desktopProcess = Start-Process `
        -FilePath $npm `
        -ArgumentList "run", $launcher.Script `
        -WorkingDirectory $uiRoot `
        -PassThru

    $deadline = (Get-Date).AddSeconds(90)
    $windowProcess = $null
    $backendReady = $false
    $backendUrl = $null
    while ((Get-Date) -lt $deadline) {
        if (-not $backendReady) {
            $backendProbe = Test-BackendReady
            $backendReady = [bool]$backendProbe.Ready
            $backendUrl = $backendProbe.Url
        }

        $windowProcess = Get-DesktopShellProcess -launcher $launcher
        if ($backendReady -and $windowProcess) {
            Write-Output ("desktop_launcher=" + $launcher.Name)
            Write-Output ("desktop_process_name=" + $windowProcess.ProcessName)
            Write-Output ("desktop_process_id=" + $windowProcess.Id)
            Write-Output ("desktop_window_title=" + $windowProcess.MainWindowTitle)
            Write-Output ("backend_url=" + $backendUrl)
            exit 0
        }

        Start-Sleep -Milliseconds 750
    }

    Write-Output ("desktop_launcher=" + $launcher.Name)
    Write-Output ("launcher_pid=" + $desktopProcess.Id)
    Write-Output ("backend_ready=" + $backendReady)
    Write-Output ("desktop_window_detected=" + [bool]$windowProcess)
    Write-Output ("desktop_launcher_exited=" + $desktopProcess.HasExited)
    if ($desktopProcess.HasExited) {
        Write-Output ("desktop_launcher_exit_code=" + $desktopProcess.ExitCode)
    }
    exit 1
}
finally {
    Pop-Location
}
