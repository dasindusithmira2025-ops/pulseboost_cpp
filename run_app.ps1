$launcherScript = Join-Path $PSScriptRoot "start_app_and_wait.ps1"

if (!(Test-Path $launcherScript)) {
    throw "Desktop launcher script was not found. Expected: $launcherScript"
}

& $launcherScript
exit $LASTEXITCODE
