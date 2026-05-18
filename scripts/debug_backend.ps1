$repoRoot = Split-Path -Parent $PSScriptRoot
$root = Join-Path $repoRoot "pulseboost"
$logDir = Join-Path $root "run"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$python = Join-Path $root "tools\python\python.exe"
$stdout = Join-Path $logDir "backend.debug.out.log"
$stderr = Join-Path $logDir "backend.debug.err.log"

$proc = Start-Process `
    -FilePath $python `
    -ArgumentList "-m", "uvicorn", "api.main:app", "--host", "127.0.0.1", "--port", "8000" `
    -WorkingDirectory $root `
    -RedirectStandardOutput $stdout `
    -RedirectStandardError $stderr `
    -PassThru

Start-Sleep -Seconds 8

Write-Output ("has_exited=" + $proc.HasExited)
if ($proc.HasExited) {
    Write-Output ("exit_code=" + $proc.ExitCode)
}

Write-Output "STDOUT:"
if (Test-Path $stdout) {
    Get-Content $stdout
}

Write-Output "STDERR:"
if (Test-Path $stderr) {
    Get-Content $stderr
}

if (!$proc.HasExited) {
    Stop-Process -Id $proc.Id -Force
}
