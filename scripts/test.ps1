param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$candidates = @(
    (Join-Path $PSScriptRoot "..\build_run\$Configuration\PulseBoostAI.exe"),
    (Join-Path $PSScriptRoot "..\build\$Configuration\PulseBoostAI.exe"),
    (Join-Path $PSScriptRoot "..\build\$Configuration\PulseBoost.exe")
)

$resolved = $null
foreach ($candidate in $candidates) {
    $full = [System.IO.Path]::GetFullPath($candidate)
    if (Test-Path $full) {
        $resolved = $full
        break
    }
}

if (-not $resolved) {
    throw "Executable not found. Checked:`n$($candidates -join "`n")"
}

$exe = $resolved

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
