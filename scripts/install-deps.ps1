param(
    [string]$QtVersion = "6.6.3",
    [string]$QtCompiler = "win64_msvc2019_64",
    [string]$QtRoot = "C:\Qt"
)

$ErrorActionPreference = "Stop"

function Install-WingetPackage {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [string]$Override = ""
    )

    $found = winget list --exact --id $Id --accept-source-agreements 2>$null
    if ($LASTEXITCODE -eq 0 -and $found) {
        Write-Host "$Id already installed"
        return
    }

    $args = @("install", "--exact", "--id", $Id, "--accept-package-agreements", "--accept-source-agreements")
    if ($Override) {
        $args += @("--override", $Override)
    }

    winget @args
    if ($LASTEXITCODE -ne 0) {
        throw "winget install failed for $Id"
    }
}

function Test-VsCppToolchain {
    $vswhere = "$Env:ProgramFiles(x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath -and (Test-Path (Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"))) {
            return $true
        }
    }

    foreach ($candidate in @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )) {
        if (Test-Path $candidate) {
            return $true
        }
    }

    return $false
}

Install-WingetPackage -Id "Git.Git"
Install-WingetPackage -Id "Kitware.CMake"
Install-WingetPackage -Id "Python.Python.3.11"
Install-WingetPackage -Id "Microsoft.VisualStudio.2022.Community" -Override "--wait --quiet --norestart --nocache --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"

if (-not (Test-VsCppToolchain)) {
    Install-WingetPackage -Id "Microsoft.VisualStudio.2022.BuildTools" -Override "--wait --quiet --norestart --nocache --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
}

if (-not (Test-VsCppToolchain)) {
    throw "Visual Studio C++ toolchain was not detected after installation."
}

python -m pip install --upgrade pip
if ($LASTEXITCODE -ne 0) {
    throw "pip upgrade failed"
}

python -m pip install aqtinstall
if ($LASTEXITCODE -ne 0) {
    throw "aqtinstall package install failed"
}

python -m aqt install-qt windows desktop $QtVersion $QtCompiler -O $QtRoot
if ($LASTEXITCODE -ne 0) {
    throw "Qt installation failed for $QtVersion / $QtCompiler"
}

$qtPath = Join-Path $QtRoot "$QtVersion\$QtCompiler"
[Environment]::SetEnvironmentVariable("CMAKE_PREFIX_PATH", $qtPath, "User")
[Environment]::SetEnvironmentVariable("Path", [Environment]::GetEnvironmentVariable("Path", "User") + ";$qtPath\bin", "User")

Write-Host "Qt installed at $qtPath"
Write-Host "CMAKE_PREFIX_PATH updated for current user"
