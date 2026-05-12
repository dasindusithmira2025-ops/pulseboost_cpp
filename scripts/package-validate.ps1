param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$releaseDir = Join-Path $root "build\$Configuration"
$installerScript = Join-Path $root "PulseBoost_Installer.iss"
$exe = Join-Path $releaseDir "PulseBoostAI.exe"

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Description
    )

    if (-not (Test-Path $Path)) {
        throw "Package validation failed: $Description not found at $Path"
    }
}

function Get-RelativePath {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $base = [System.IO.Path]::GetFullPath($BasePath).TrimEnd('\') + '\'
    $target = [System.IO.Path]::GetFullPath($TargetPath)
    if ($target.StartsWith($base, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $target.Substring($base.Length)
    }

    return $target
}

Push-Location $root
try {
    Assert-PathExists -Path $exe -Description "release executable"
    Assert-PathExists -Path $installerScript -Description "installer script"

    $installer = Get-Content $installerScript -Raw
    $requiredInstallerText = @(
        'AppName=PulseBoost AI',
        'AppVersion=1.0.0',
        'DefaultDirName={autopf}\PulseBoost AI',
        'Source: "build\Release\PulseBoostAI.exe"',
        'Filename: "{app}\PulseBoostAI.exe"'
    )

    foreach ($text in $requiredInstallerText) {
        if ($installer -notmatch [regex]::Escape($text)) {
            throw "Package validation failed: installer script is missing '$text'"
        }
    }

    $installerSourcePaths = @()
    foreach ($line in ($installer -split "`r?`n")) {
        if ($line -match '^\s*Source:\s*"([^"]+)"') {
            $installerSourcePaths += $Matches[1]
        }
    }

    $forbiddenInstallerSourcePatterns = @(
        'PulseBoost\.exe',
        'tauri_ui',
        'design-reference',
        'node_modules',
        '\\dist\\',
        '\\.vite\\',
        '\.env',
        'build\\Release\\data',
        'build\\Release\\logs',
        '\.db',
        '\.sqlite',
        '\.sqlite3',
        '\.log'
    )

    foreach ($sourcePath in $installerSourcePaths) {
        foreach ($pattern in $forbiddenInstallerSourcePatterns) {
            if ($sourcePath -match $pattern) {
                throw "Package validation failed: installer source '$sourcePath' matched forbidden pattern '$pattern'"
            }
        }
    }

    $expectedDlls = @(
        "Qt6Core.dll",
        "Qt6Gui.dll",
        "Qt6Qml.dll",
        "Qt6Quick.dll",
        "Qt6QuickControls2.dll",
        "Qt6Sql.dll",
        "Qt6Network.dll"
    )
    foreach ($dll in $expectedDlls) {
        Assert-PathExists -Path (Join-Path $releaseDir $dll) -Description "Qt runtime DLL $dll"
    }

    $expectedPluginDirs = @(
        "platforms",
        "styles",
        "imageformats",
        "sqldrivers",
        "tls",
        "iconengines",
        "networkinformation",
        "generic",
        "translations",
        "qml"
    )
    foreach ($dir in $expectedPluginDirs) {
        Assert-PathExists -Path (Join-Path $releaseDir $dir) -Description "Qt runtime directory $dir"
    }

    $payloadRoots = $expectedPluginDirs | ForEach-Object { Join-Path $releaseDir $_ }
    $payloadFiles = @()
    foreach ($payloadRoot in $payloadRoots) {
        if (Test-Path $payloadRoot) {
            $payloadFiles += Get-ChildItem $payloadRoot -Recurse -File -Force
        }
    }
    $payloadFiles += Get-ChildItem $releaseDir -File -Force | Where-Object {
        $_.Name -eq "PulseBoostAI.exe" -or $_.Extension -ieq ".dll"
    }

    $forbiddenPayloadPatterns = @(
        '\\tauri_ui\\',
        '\\design-reference\\',
        '\\node_modules\\',
        '\\dist\\',
        '\\\.vite\\',
        '\\data\\',
        '\\logs\\',
        '\.db$',
        '\.sqlite$',
        '\.sqlite3$',
        '\.log$',
        '\.env($|\.)',
        '\.pfx$',
        '\.p12$',
        '\.pem$',
        '\.key$',
        'secret',
        'certificate'
    )

    foreach ($file in $payloadFiles) {
        $relative = Get-RelativePath -BasePath $root -TargetPath $file.FullName
        foreach ($pattern in $forbiddenPayloadPatterns) {
            if ($relative -match $pattern) {
                throw "Package validation failed: package payload file '$relative' matched forbidden pattern '$pattern'"
            }
        }
    }

    & $exe --validate-qml
    & "$PSScriptRoot\test.ps1" -Configuration $Configuration
    & "$PSScriptRoot\release-validate.ps1" -Configuration $Configuration

    Write-Host "Package validation passed."
} finally {
    Pop-Location
}
