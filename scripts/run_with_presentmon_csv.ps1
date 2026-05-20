param(
    [Parameter(Mandatory = $true)]
    [string]$CsvPath,
    [string]$GameProcessName
)

$launcherScript = Join-Path $PSScriptRoot "..\run_app.ps1"

if ([string]::IsNullOrWhiteSpace($CsvPath)) {
    Write-Error "CsvPath is required. Pass the path to a real PresentMon-compatible CSV file."
    exit 1
}

try {
    $resolvedCsvPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($CsvPath)
}
catch {
    $resolvedCsvPath = [System.IO.Path]::GetFullPath($CsvPath)
}

if (-not (Test-Path -LiteralPath $resolvedCsvPath -PathType Leaf)) {
    Write-Error "PresentMon CSV file was not found: $resolvedCsvPath"
    Write-Host "Start PresentMon separately and point this script at the real CSV it is writing." -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path -LiteralPath $launcherScript -PathType Leaf)) {
    Write-Error "PulseBoost launcher was not found: $launcherScript"
    exit 1
}

$env:PRESENTMON_CSV_PATH = $resolvedCsvPath

Write-Host "PRESENTMON_CSV_PATH configured for this PowerShell process only." -ForegroundColor Green
Write-Host "CSV path: $resolvedCsvPath"
if (-not [string]::IsNullOrWhiteSpace($GameProcessName)) {
    Write-Host "Reminder: PresentMon must be actively capturing process '$GameProcessName' while the game is rendering frames." -ForegroundColor Yellow
}
Write-Host "Launching PulseBoost through the existing desktop launcher..." -ForegroundColor Cyan

& $launcherScript
exit $LASTEXITCODE
