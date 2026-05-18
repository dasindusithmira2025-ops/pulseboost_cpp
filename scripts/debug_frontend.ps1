$repoRoot = Split-Path -Parent $PSScriptRoot
$root = Join-Path $repoRoot "pulseboost"
$logDir = Join-Path $root "run"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$nodeDir = Join-Path $root "tools\node-tmp\node-v20.15.1-win-x64"
$npm = Join-Path $nodeDir "npm.cmd"
$env:PATH = $nodeDir + ";" + $env:PATH
$stdout = Join-Path $logDir "frontend.debug.out.log"
$stderr = Join-Path $logDir "frontend.debug.err.log"

$proc = Start-Process `
    -FilePath $npm `
    -ArgumentList "run", "dev", "--", "--host", "127.0.0.1", "--port", "3000" `
    -WorkingDirectory (Join-Path $root "ui") `
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
