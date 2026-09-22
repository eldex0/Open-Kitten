; Compile after the PyInstaller build with Inno Setup 6.
[Setup]
AppId=MiniBrowserV9
AppName=Open Kitten
AppVersion=1.0.0
DefaultDirName={localappdata}\Programs\OpenKitten
DefaultGroupName=Open Kitten
PrivilegesRequired=lowest
OutputDir=..\release
OutputBaseFilename=OpenKitten-1.0.0-setup
Compression=lzma2
SolidCompression=yes
UninstallDisplayIcon={app}\OpenKitten.exe
[Files]
Source: "..\dist\OpenKitten\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\Open Kitten"; Filename: "{app}\OpenKitten.exe"
[Run]
Filename: "{app}\OpenKitten.exe"; Description: "Ouvrir Open Kitten"; Flags: nowait postinstall skipifsilent
; User data is intentionally retained on uninstall.
