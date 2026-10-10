; Compile after the native CMake package with Inno Setup 6.
[Setup]
AppId={{C91A1556-C4A1-4DAA-8EC8-B5D5FC7A6F10}
AppName=Open Kitten
AppVersion=2.0.0
AppPublisher=Open Kitten
DefaultDirName={localappdata}\Programs\OpenKitten
DefaultGroupName=Open Kitten
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\release
OutputBaseFilename=OpenKitten-2.0.0-setup
SetupIconFile=..\assets\icon.ico
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
UninstallDisplayIcon={app}\OpenKitten.exe
VersionInfoVersion=2.0.0.0
VersionInfoProductName=Open Kitten
[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
[Files]
Source: "..\dist\OpenKitten\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\Open Kitten"; Filename: "{app}\OpenKitten.exe"
[Run]
Filename: "{app}\OpenKitten.exe"; Description: "Ouvrir Open Kitten"; Flags: nowait postinstall skipifsilent
; User data is intentionally retained on uninstall.
