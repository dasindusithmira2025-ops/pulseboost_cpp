param(
    [Parameter(Mandatory = $true)]
    [string]$CsvPath
)

$frameTimeColumns = @(
    "msBetweenPresents",
    "MsBetweenPresents",
    "msBetweenDisplayChange",
    "FrameTime",
    "FrameTimeMs",
    "frame_time_ms"
)
$processColumns = @("Application", "ProcessName", "Process", "ProcessName.exe", "app")
$pidColumns = @("ProcessID", "PID", "Pid", "process_id")
$staleThresholdSeconds = 120

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
    exit 1
}

$file = Get-Item -LiteralPath $resolvedCsvPath
$lines = Get-Content -LiteralPath $resolvedCsvPath
$lineCount = @($lines).Count
$headerLine = if ($lineCount -gt 0) { [string]$lines[0] } else { "" }
$headerColumns = if ([string]::IsNullOrWhiteSpace($headerLine)) { @() } else { $headerLine.Split(",") | ForEach-Object { $_.Trim() } }
$dataRowCount = [Math]::Max($lineCount - 1, 0)
$now = Get-Date
$ageSeconds = ($now - $file.LastWriteTime).TotalSeconds
$detectedFrameColumns = @($frameTimeColumns | Where-Object { $headerColumns -contains $_ })
$detectedProcessColumns = @($processColumns | Where-Object { $headerColumns -contains $_ })
$detectedPidColumns = @($pidColumns | Where-Object { $headerColumns -contains $_ })

Write-Host "PresentMon CSV check" -ForegroundColor Cyan
Write-Host "Path: $resolvedCsvPath"
Write-Host ("File size: {0} bytes" -f $file.Length)
Write-Host ("Last modified: {0}" -f $file.LastWriteTime)
Write-Host ("Line count: {0}" -f $lineCount)
Write-Host ("Detected header columns: {0}" -f $(if ($headerColumns.Count) { $headerColumns -join ", " } else { "<none>" }))
Write-Host ("Frame-time columns present: {0}" -f $(if ($detectedFrameColumns.Count) { $detectedFrameColumns -join ", " } else { "<none>" }))
Write-Host ("Process identifier columns present: {0}" -f $(if (($detectedProcessColumns + $detectedPidColumns).Count) { ($detectedProcessColumns + $detectedPidColumns) -join ", " } else { "<none>" }))

if ($dataRowCount -le 0) {
    Write-Warning "The CSV has only headers or no rows. Benchmark frame-time evidence will stay unavailable until real sample rows are appended."
}

if ($ageSeconds -gt $staleThresholdSeconds) {
    Write-Warning ("The CSV has not changed in the last {0:N0} seconds and may be stale." -f $ageSeconds)
}

if (-not $detectedFrameColumns.Count) {
    Write-Warning "No common PresentMon-compatible frame-time columns were detected. PulseBoost will treat this CSV as unsupported."
}

if (-not (($detectedProcessColumns + $detectedPidColumns).Count)) {
    Write-Warning "No common process-name or PID columns were detected. PulseBoost may not be able to match rows to the foreground game."
}
