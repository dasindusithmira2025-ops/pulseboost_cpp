[Setup]
AppName=PulseBoost AI
AppVersion=1.0.0
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
Source: "build\Release\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
Source: "build\Release\qml\*"; DestDir: "{app}\qml"; Flags: ignoreversion recursesubdirs
Source: "build\Release\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs
Source: "build\Release\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\PulseBoost AI"; Filename: "{app}\PulseBoostAI.exe"
Name: "{group}\{cm:UninstallProgram,PulseBoost AI}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\PulseBoost AI"; Filename: "{app}\PulseBoostAI.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\PulseBoostAI.exe"; Description: "{cm:LaunchProgram,PulseBoost AI}"; Flags: nowait postinstall skipifsilent

[Registry]
; Standard uninstaller entries handled automatically by Inno Setup
; Proactive alerts or startup handling can optionally be toggled via CLI on subsequent runs.
