; based on http://code.google.com/p/infekt/source/browse/trunk/project/setup-script.iss
; GPLv2

#define SourceFileDir32 "..\build\Win32-Release"
#define SourceFileDir64 "..\build\x64-Release"
#define SourceCodeDir "..\.."
#define COMServiceControlCLSID "{{DC2BF05E-2451-435E-A24C-1B9BA804B5F0}"
#define COMServiceControlPreCompiledDir "C:\Dev\COMServiceControl_Signed"
#define MSVCRedist GetEnv('VS120COMNTOOLS') + "..\..\VC\redist"

[Setup]
AppId={{819933F3-A105-4B40-8920-6D74139B5464}
AppName=ZNC IRC Bouncer
AppVerName=ZNC 1.2a
AppVersion=1.2.0
AppPublisher=Ingmar Runge
AppPublisherURL=http://znc-msvc.googlecode.com/
DefaultDirName={pf}\ZNC
DefaultGroupName=ZNC IRC Bouncer
AllowNoIcons=yes
OutputDir=..\build
OutputBaseFilename=znc-setup
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
MinVersion=0,5.1sp2
UninstallDisplayIcon={app}\ZNC_Tray.exe
UninstallDisplayName=ZNC IRC Bouncer
SignTool=kSign /d $qZNC IRC Bouncer$q /du $qhttp://znc-msvc.googlecode.com/$q $f
WizardImageFile=compiler:WizModernImage-IS.bmp
WizardSmallImageFile=compiler:WizModernSmallImage-IS.bmp

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Components]
Name: "core"; Description: "Core Files (required)"; Types: full compact custom; Flags: fixed
Name: "service"; Description: "ZNC Service"; Types: full compact
Name: "service/autorun"; Description: "Set Service to Run on Startup"; Types: full
Name: "service/firewall"; Description: "Add Service Firewall Exception"; Types: full
Name: "service/tray"; Description: "Install Tray Control"; Types: full compact; Flags: checkablealone
Name: "service/tray/desktop"; Description: "Create a Desktop Icon"; Types: full
Name: "service/tray/autorun"; Description: "Launch Tray Control on Startup"; Types: full
Name: "modules"; Description: "Modules"; Types: full compact
Name: "modules/core"; Description: "Core Modules"; Types: full compact
Name: "modules/extra_win32"; Description: "Useful third-party Modules"; Types: full
Name: "webskins"; Description: "Web Interface Themes"; Types: full compact custom; Flags: fixed
Name: "webskins\default"; Description: "Default Theme"; Types: full compact custom; Flags: fixed
Name: "webskins\darkclouds"; Description: "Dark Clouds Theme"; Types: full
Name: "webskins\forest"; Description: "Forest Theme"; Types: full
Name: "webskins\ice"; Description: "Ice Theme"; Types: full

[Files]
Source: "{#SourceFileDir32}\ZNC.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\ZNC.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\ZNC_CLI.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\ZNC_CLI.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode

Source: "{#MSVCRedist}\x86\Microsoft.VC120.CRT\msvcp120.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#MSVCRedist}\x64\Microsoft.VC120.CRT\msvcp120.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#MSVCRedist}\x86\Microsoft.VC120.CRT\msvcr120.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#MSVCRedist}\x64\Microsoft.VC120.CRT\msvcr120.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode

Source: "{#SourceFileDir32}\libeay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\libeay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\ssleay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\ssleay32.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode

Source: "{#SourceFileDir32}\icudt51.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\icudt51.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\icuin51.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\icuin51.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode
Source: "{#SourceFileDir32}\icuuc51.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode
Source: "{#SourceFileDir64}\icuuc51.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode

Source: "{#SourceFileDir32}\ZNC_Service.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode; Components: service
Source: "{#SourceFileDir64}\ZNC_Service.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode; Components: service
Source: "{#SourceFileDir32}\znc_service_provider.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode; Components: service
Source: "{#SourceFileDir64}\znc_service_provider.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode; Components: service
Source: "{#SourceFileDir32}\ZNC_Tray.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode; Components: service/tray
Source: "{#SourceFileDir64}\ZNC_Tray.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode; Components: service/tray

Source: "{#COMServiceControlPreCompiledDir}\Release\COMServiceControl.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: not Is64BitInstallMode; Components: service/tray
Source: "{#COMServiceControlPreCompiledDir}\x64\Release\COMServiceControl.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode; Components: service/tray

#include "..\solution\modules\ModulesList.iss"

Source: "{#SourceCodeDir}\webskins\_default_\*"; DestDir: "{app}\webskins\_default_"; Excludes: ".svn"; Flags: recursesubdirs
Source: "{#SourceCodeDir}\webskins\dark-clouds\*"; DestDir: "{app}\webskins\dark-clouds"; Excludes: ".svn"; Flags: recursesubdirs; Components: webskins\darkclouds
Source: "{#SourceCodeDir}\webskins\forest\*"; DestDir: "{app}\webskins\forest"; Excludes: ".svn"; Flags: recursesubdirs; Components: webskins\forest
Source: "{#SourceCodeDir}\webskins\ice\*"; DestDir: "{app}\webskins\ice"; Excludes: ".svn"; Flags: recursesubdirs; Components: webskins\ice

Source: "{#SourceCodeDir}\NOTICE"; DestDir: "{app}"; DestName: "NOTICE.txt"
Source: "{#SourceCodeDir}\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"
Source: "{#SourceCodeDir}\README.md"; DestDir: "{app}"; DestName: "README-ZNC.txt"

[Icons]
Name: "{group}\ZNC Service Control"; Filename: "{app}\ZNC_Tray.exe"; Components: service/tray
Name: "{group}\ZNC (CLI)"; Filename: "{app}\ZNC_CLI.exe"
Name: "{group}\More\{cm:ProgramOnTheWeb,ZNC-MSVC}"; Filename: "http://znc-msvc.googlecode.com/"; Flags: excludefromshowinnewinstall
Name: "{group}\More\{cm:ProgramOnTheWeb,ZNC}"; Filename: "http://znc.in/"; Flags: excludefromshowinnewinstall
Name: "{group}\More\{cm:UninstallProgram,ZNC}"; Filename: "{uninstallexe}"; Flags: excludefromshowinnewinstall
Name: "{userdesktop}\ZNC Service Control"; Filename: "{app}\ZNC_Tray.exe"; Components: service/tray/desktop
Name: "{userstartup}\ZNC Service Control"; Filename: "{app}\ZNC_Tray.exe"; Parameters: "--autorun"; Components: service/tray/autorun

[Registry]
Root: HKLM; Subkey: "SOFTWARE\ZNC"; ValueType: string; ValueName: "ServiceDataDir"; ValueData: "{code:GetServiceDataDir}"; Components: service; Flags: uninsdeletekey

[Dirs]
Name: "{code:GetServiceDataDir}"
Name: "{commonappdata}\ZNC"

[Run]
Filename: "{app}\ZNC_Service.exe"; Parameters: "--install"; Flags: runhidden; Components: service and service/autorun
Filename: "{app}\ZNC_Service.exe"; Parameters: "--install --manual"; Flags: runhidden; Components: service and not service/autorun
Filename: "{app}\COMServiceControl.exe"; Parameters: "/RegServer"; Flags: runhidden; Components: service/tray
; can't use [Registry] because this needs to be done *after* /RegServer (same during uninstall where it needs to be done before /UnRegServer):
Filename: "{cmd}"; Parameters: "/c reg add HKLM\SOFTWARE\Classes\CLSID\{#COMServiceControlCLSID} /v LocalizedString /t REG_EXPAND_SZ /d ""@{app}\COMServiceControl.exe,-101"" /f"; Flags: runhidden; Components: service/tray
Filename: "{cmd}"; Parameters: "/c reg add HKLM\SOFTWARE\Classes\CLSID\{#COMServiceControlCLSID}\Elevation /v Enabled /t REG_DWORD /d 1 /f"; Flags: runhidden; Components: service/tray

[UninstallRun]
; could use something less brutal than taskkill in the future:
Filename: "{cmd}"; parameters: "/c taskkill /f /im ZNC_Tray.exe"; Flags: runhidden; Components: service/tray
Filename: "{cmd}"; parameters: "/c net stop ZNC"; Flags: runhidden; Components: service
Filename: "{app}\ZNC_Service.exe"; Flags: runhidden; Parameters: "--uninstall"; Components: service
Filename: "{cmd}"; Parameters: "/c reg delete HKLM\SOFTWARE\Classes\CLSID\{#COMServiceControlCLSID} /v LocalizedString /f"; Flags: runhidden; Components: service/tray
Filename: "{cmd}"; Parameters: "/c reg delete HKLM\SOFTWARE\Classes\CLSID\{#COMServiceControlCLSID}\Elevation /f"; Flags: runhidden; Components: service/tray
Filename: "{app}\COMServiceControl.exe"; Parameters: "/UnRegServer"; Flags: runhidden; Components: service/tray

[UninstallDelete]
Type: files; Name: "{commonappdata}\ZNC\Service.*.log"
Type: dirifempty; Name: "{commonappdata}\ZNC"

#include "service_data_dir.iss"
#include "firewall.iss"

[Code]
procedure InitializeWizard();
begin
  Service_Data_Dir_InitializeWizard();
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
	if (CurStep = ssPostInstall) and IsComponentSelected('service') and IsComponentSelected('service/firewall') then
	begin
		AddFirewallException('ZNC IRC Bouncer Service', ExpandConstant('{app}\ZNC_Service.exe'));
		AddFirewallException('ZNC IRC Bouncer CLI', ExpandConstant('{app}\ZNC_CLI.exe'));
	end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
	if CurUninstallStep = usUninstall then
	begin
		RemoveFirewallException('ZNC IRC Bouncer Service', ExpandConstant('{app}\ZNC_Service.exe'));
		RemoveFirewallException('ZNC IRC Bouncer CLI', ExpandConstant('{app}\ZNC_CLI.exe'));
	end;
end;
