param(
    [string]$Configuration = "Release",
    [string]$QtPath = $env:CMAKE_PREFIX_PATH
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
