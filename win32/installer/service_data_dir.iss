[Code]
var
  DataDirPage: TInputDirWizardPage;

procedure Service_Data_Dir_InitializeWizard();
begin
  DataDirPage := CreateInputDirPage(wpSelectComponents,
    'Select Service Configuration Data Directory', 'Where should the service keep the ZNC configuration data?',
    'Select the folder where the service should keep (and look for) ZNC configuration data, then click Next.' + #13#10 + #13#10 +
    'This only applies to the Windows service (which you opted to install.)' + #13#10 + #13#10 +
    'Corresponds to the ".znc" and "AppData\ZNC" directories.',
    False, 'znc-service-config');
  DataDirPage.Add('');

  DataDirPage.Values[0] := GetPreviousData('ServiceDataDir', ExpandConstant('{commonappdata}\ZNC\Configuration'));
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
  if PageID = DataDirPage.ID then
  begin
    Result := not IsComponentSelected('service');
  end;
end;

function GetServiceDataDir(Param: String): String;
begin
  Result := DataDirPage.Values[0];
end;
