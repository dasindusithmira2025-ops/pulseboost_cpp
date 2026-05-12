[Setup]
AppName=PulseBoost AI
AppVersion=1.0.0
AppPublisher=PulseBoost AI
VersionInfoVersion=1.0.0
VersionInfoProductName=PulseBoost AI
VersionInfoProductVersion=1.0.0
DefaultDirName={autopf}\PulseBoost AI
DefaultGroupName=PulseBoost AI
UninstallDisplayIcon={app}\PulseBoostAI.exe
Compression=lzma2
SolidCompression=yes
OutputDir=build\Installer
OutputBaseFilename=PulseBoostAI_Setup
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "build\Release\PulseBoostAI.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Release\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Release\qml\*"; DestDir: "{app}\qml"; Flags: ignoreversion recursesubdirs; Excludes: "*.pdb,*.log,*.db,*.sqlite,*.sqlite3,.env,.env.*,*secret*,*certificate*,*.pfx,*.p12,*.pem,*.key,node_modules\*,dist\*,.vite\*,tauri_ui\*,design-reference\*"
Source: "build\Release\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs
Source: "build\Release\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs
Source: "build\Release\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs
Source: "build\Release\sqldrivers\*"; DestDir: "{app}\sqldrivers"; Flags: ignoreversion recursesubdirs
Source: "build\Release\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs
Source: "build\Release\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs
Source: "build\Release\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs
Source: "build\Release\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs
Source: "build\Release\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\PulseBoost AI"; Filename: "{app}\PulseBoostAI.exe"
Name: "{group}\{cm:UninstallProgram,PulseBoost AI}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\PulseBoost AI"; Filename: "{app}\PulseBoostAI.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\PulseBoostAI.exe"; Description: "{cm:LaunchProgram,PulseBoost AI}"; Flags: nowait postinstall skipifsilent

[Registry]
; Standard uninstaller entries handled automatically by Inno Setup
; Proactive alerts or startup handling can optionally be toggled via CLI on subsequent runs.
