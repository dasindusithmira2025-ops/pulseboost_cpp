param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))

Push-Location $root
try {
    cmake --build --preset build-release
    ctest --test-dir build -C $Configuration --output-on-failure
    & "$PSScriptRoot\test.ps1" -Configuration $Configuration

    $tracked = git ls-files
    $blockedPatterns = @(
        '^tauri_ui/',
        '^build/',
        '^build_run/',
        '^out/',
        '^logs/',
        '\.sqlite3?$',
        '\.db$',
        '^data/.*\.jsonl$',
        '^logs/.*\.jsonl$',
        '\.pyc$',
        '__pycache__/',
        '(^|/)\.env($|\.)',
        'secret',
        'api[_-]?key'
    )

    foreach ($file in $tracked) {
        foreach ($pattern in $blockedPatterns) {
            if ($file -match $pattern) {
                throw "Release validation failed: tracked forbidden file '$file' matched '$pattern'"
            }
        }
    }

    $qrc = Get-Content (Join-Path $root "qml.qrc") -Raw
    $requiredScreens = @(
        "ui/qml/screens/ActionCenter.qml",
        "ui/qml/screens/AuditLog.qml",
        "ui/qml/screens/RestoreCenter.qml",
        "ui/qml/screens/Home.qml",
        "ui/qml/screens/Optimizations.qml",
        "ui/qml/screens/AiChat.qml"
    )
    foreach ($screen in $requiredScreens) {
        if ($qrc -notmatch [regex]::Escape($screen)) {
            throw "Release validation failed: qml.qrc does not include $screen"
        }
    }
    if ($qrc -match "tauri_ui") {
        throw "Release validation failed: deprecated Tauri path is included in QML resources"
    }

    $exe = Join-Path $root "build\$Configuration\PulseBoostAI.exe"
    if (-not (Test-Path $exe)) {
        throw "Release validation failed: executable not found at $exe"
    }

    Write-Host "Release validation passed."
} finally {
    Pop-Location
}
