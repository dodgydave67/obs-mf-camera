; SPDX-License-Identifier: GPL-2.0-or-later
#ifndef StageDir
  #error Supply /DStageDir=<staged runtime directory>
#endif
#ifndef InstallerVersion
  #define InstallerVersion "0.1.0"
#endif
#ifndef OutputDir
  #define OutputDir "output"
#endif
#ifndef BuildRevision
  #define BuildRevision "local"
#endif

[Setup]
AppId={{73E7BCC1-686D-45BE-87BA-EDAAAE05849A}
AppName=OBS Studio MF (Experimental)
AppVersion={#InstallerVersion}
AppVerName=OBS Studio MF {#InstallerVersion} (Experimental)
AppPublisher=OBS MF Camera fork contributors
DefaultDirName={autopf}\OBS Studio MF (Experimental)
UsePreviousAppDir=no
DisableDirPage=yes
DefaultGroupName=OBS Studio MF (Experimental)
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0.22000
AppMutex=OBSStudioMFCore
LicenseFile={#StageDir}\COPYING
InfoBeforeFile=experimental-notice.txt
UninstallDisplayIcon={app}\bin\64bit\obs64.exe
OutputDir={#OutputDir}
OutputBaseFilename=OBS-MF-Camera-{#InstallerVersion}-{#BuildRevision}-Windows11-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
SetupLogging=yes

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "core\win-dshow\data\obs-virtualcam-module32.dll,core\win-dshow\data\obs-virtualcam-module64.dll,prerequisites\*"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\core\win-dshow\data\obs-virtualcam-module32.dll"; DestDir: "{app}\core\win-dshow\data"; Flags: ignoreversion regserver 32bit
Source: "{#StageDir}\core\win-dshow\data\obs-virtualcam-module64.dll"; DestDir: "{app}\core\win-dshow\data"; Flags: ignoreversion regserver 64bit
Source: "{#StageDir}\prerequisites\VC_redist.x64.exe"; Flags: dontcopy

[Registry]
Root: HKLM64; Subkey: "Software\Classes\CLSID\{{BD34B55A-51CA-4A96-BEB4-601C35D572A9}"; ValueType: string; ValueData: "OBS Media Foundation Camera Source"; Flags: uninsdeletekey
Root: HKLM64; Subkey: "Software\Classes\CLSID\{{BD34B55A-51CA-4A96-BEB4-601C35D572A9}\InprocServer32"; ValueType: string; ValueData: "{app}\mf-camera\obs-mf-camera-source.dll"
Root: HKLM64; Subkey: "Software\Classes\CLSID\{{BD34B55A-51CA-4A96-BEB4-601C35D572A9}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Both"

[Icons]
Name: "{group}\OBS Studio MF (Experimental)"; Filename: "{app}\bin\64bit\obs64.exe"; Parameters: "--disable-updater"; WorkingDir: "{app}\bin\64bit"
Name: "{group}\Camera setup instructions"; Filename: "{app}\INSTALL-MF-CAMERA.txt"
Name: "{group}\Uninstall OBS Studio MF"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\bin\64bit\obs64.exe"; Parameters: "--disable-updater"; WorkingDir: "{app}\bin\64bit"; Description: "Open OBS Studio MF (Experimental)"; Flags: nowait postinstall skipifsilent runasoriginaluser

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Result := '';
  ExtractTemporaryFile('VC_redist.x64.exe');
  if not Exec(ExpandConstant('{tmp}\VC_redist.x64.exe'), '/install /quiet /norestart', '',
    SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    Result := 'Could not start the Microsoft Visual C++ runtime installer.';
    exit;
  end;
  if ResultCode = 3010 then
    NeedsRestart := True
  else if (ResultCode <> 0) and (ResultCode <> 1638) then
    Result := 'Microsoft Visual C++ runtime installation failed (code ' + IntToStr(ResultCode) + ').';
end;
