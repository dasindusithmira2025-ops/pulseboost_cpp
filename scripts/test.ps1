param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$exe = Join-Path $PSScriptRoot "..\build\$Configuration\PulseBoost.exe"
$exe = [System.IO.Path]::GetFullPath($exe)

if (-not (Test-Path $exe)) {
    throw "Executable not found: $exe"
}

function Invoke-PulseBoost {
    param(
        [string[]]$Arguments
    )

    $process = Start-Process -FilePath $exe -ArgumentList $Arguments -PassThru -Wait
    if ($process.ExitCode -ne 0) {
        throw "PulseBoost failed for arguments: $($Arguments -join ' ')"
    }
}

Invoke-PulseBoost -Arguments @("--scan")
Invoke-PulseBoost -Arguments @("--chat", "analyze", "performance")
Invoke-PulseBoost -Arguments @("--chat", "clean", "my", "pc")
Invoke-PulseBoost -Arguments @("--self-test")

Write-Host "Smoke tests completed. Inspect logs\ for telemetry and self_test output."
