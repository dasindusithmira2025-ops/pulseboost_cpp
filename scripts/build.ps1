param(
    [string]$Configuration = "Release",
    [string]$QtPath = $env:CMAKE_PREFIX_PATH,
    [switch]$DeployRuntime = $true
)

$ErrorActionPreference = "Stop"

if (-not $QtPath) {
    throw "CMAKE_PREFIX_PATH / QtPath is not set."
}

$vswhere = "$Env:ProgramFiles(x86)\Microsoft Visual Studio\Installer\vswhere.exe"
$vcvarsCandidates = @()

if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsPath) {
        $vcvarsCandidates += (Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat")
    }
}

$vcvarsCandidates += @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

$vcvars = $vcvarsCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $vcvars) {
    throw "vcvars64.bat not found. Install Visual Studio 2022 C++ tools first."
}

$command = "`"$vcvars`" && cmake -S . -B build -G `"Visual Studio 17 2022`" -DCMAKE_PREFIX_PATH=`"$QtPath`" && cmake --build build --config $Configuration"
cmd /c $command

if (-not $DeployRuntime) {
    return
}

$qtRoot = $QtPath
if (-not (Test-Path (Join-Path $qtRoot "bin\windeployqt.exe"))) {
    $candidate = Split-Path $qtRoot -Parent
    if (Test-Path (Join-Path $candidate "bin\windeployqt.exe")) {
        $qtRoot = $candidate
    }
}

$windeployqt = Join-Path $qtRoot "bin\windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    Write-Warning "windeployqt.exe not found under '$QtPath'. Skipping runtime deployment."
    return
}

$releaseDir = Join-Path "build" $Configuration
$primaryExe = Join-Path $releaseDir "PulseBoostAI.exe"
$fallbackExe = Join-Path $releaseDir "PulseBoost.exe"
$exePath = if (Test-Path $primaryExe) { $primaryExe } else { $fallbackExe }
if (-not (Test-Path $exePath)) {
    throw "Build succeeded but executable not found in '$releaseDir'."
}

& $windeployqt --release --qmldir "ui\qml" $exePath
