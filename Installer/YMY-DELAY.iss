; YMY DELAY - Instalador de Windows (Inno Setup 6)
; Se compila desde la raiz del repositorio:  iscc Installer\YMY-DELAY.iss
; Requiere que el proyecto ya este compilado en build\ (Release x64).

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

#define BuildDir "..\build\YMYDelay_artefacts\Release"

[Setup]
AppId={{B7D2C1A4-5E3F-4A8B-9C61-2F4D8E10A7C3}
AppName=YMY DELAY
AppVersion={#AppVersion}
AppVerName=YMY DELAY {#AppVersion}
AppPublisher=ORYMY - YMY RECORDS
DefaultDirName={autopf}\ORYMY\YMY DELAY
DefaultGroupName=ORYMY
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=YMY DELAY Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=YMY DELAY
UninstallDisplayIcon={app}\YMY DELAY.exe

[Types]
Name: "full";   Description: "Full installation (VST3 + Standalone)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3";       Description: "YMY DELAY VST3 plugin"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "YMY DELAY Standalone application"; Types: full custom

[Tasks]
Name: "startmenuicon"; Description: "Create a Start Menu shortcut (Standalone)"; Components: standalone
Name: "desktopicon";   Description: "Create a desktop shortcut (Standalone)"; Components: standalone; Flags: unchecked

[Files]
; VST3: solo se instala la carpeta "YMY DELAY.vst3"; no se toca nada mas de la carpeta VST3.
Source: "{#BuildDir}\VST3\YMY DELAY.vst3\*"; DestDir: "{code:GetVst3Dir}\YMY DELAY.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\YMY DELAY.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\YMY DELAY"; Filename: "{app}\YMY DELAY.exe"; Components: standalone; Tasks: startmenuicon
Name: "{autodesktop}\YMY DELAY"; Filename: "{app}\YMY DELAY.exe"; Components: standalone; Tasks: desktopicon

[Run]
Filename: "{app}\YMY DELAY.exe"; Description: "Launch YMY DELAY (Standalone)"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

[Code]
var
  Vst3Page: TInputDirWizardPage;

procedure InitializeWizard;
begin
  Vst3Page := CreateInputDirPage(wpSelectComponents,
    'VST3 plugin folder',
    'Where should the VST3 plugin be installed?',
    'The standard Windows VST3 folder is selected by default. Choose another folder only if your DAW scans a custom location.',
    False, '');
  Vst3Page.Add('');
  Vst3Page.Values[0] := ExpandConstant('{commoncf64}\VST3');
end;

function GetVst3Dir(Param: String): String;
begin
  Result := RemoveBackslash(Vst3Page.Values[0]);
end;
