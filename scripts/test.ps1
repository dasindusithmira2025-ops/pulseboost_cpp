param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$candidates = @(
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

    $outputFile = Join-Path ([System.IO.Path]::GetTempPath()) ("pulseboost-cli-" + [System.Guid]::NewGuid().ToString("N") + ".json")
    $process = Start-Process -FilePath $exe -ArgumentList $Arguments -PassThru -Wait -RedirectStandardOutput $outputFile
    $output = Get-Content $outputFile -Raw
    Remove-Item -Force $outputFile
    if ($process.ExitCode -ne 0) {
        throw "PulseBoost failed for arguments: $($Arguments -join ' ')"
    }
    if ([string]::IsNullOrWhiteSpace($output)) {
        throw "PulseBoost returned empty output for arguments: $($Arguments -join ' ')"
    }
    $json = $output | ConvertFrom-Json
    if ($null -eq $json.ok -or $json.ok -ne $true) {
        throw "PulseBoost returned invalid success JSON for arguments: $($Arguments -join ' ')"
    }
}

Invoke-PulseBoost -Arguments @("--scan")
Invoke-PulseBoost -Arguments @("--chat", "analyze", "performance")
Invoke-PulseBoost -Arguments @("--clean", "--dry-run")
Invoke-PulseBoost -Arguments @("--self-test")
Invoke-PulseBoost -Arguments @("--validate-qml")

Write-Host "Smoke tests completed with strict JSON validation."
