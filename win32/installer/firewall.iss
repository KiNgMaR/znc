// Utility functions for Inno Setup
//   used to add/remove programs from the windows firewall rules
// Code originally from http://news.jrsoftware.org/news/innosetup/msg43799.html
// Added "Windows Firewall with Advanced Security" code for Vista and later.

// could use some error handling+feedback...

const
	NET_FW_SCOPE_ALL = 0;
	NET_FW_IP_VERSION_ANY = 2;
	NET_FW_PROFILE_DOMAIN = 0;
	NET_FW_PROFILE_STANDARD = 1;
	NET_FW_IP_PROTOCOL_TCP = 6;
	NET_FW_ACTION_ALLOW = 1;

function IsWindowsNT6: Boolean;
var
	Version: TWindowsVersion;
begin
	GetWindowsVersionEx(Version);
	Result := (Version.NTPlatform and (Version.Major >= 6));
end;

procedure AddFirewallException(AppName, FileName: string);
var
	FirewallObject: Variant;
	FirewallManager: Variant;
	FirewallProfile: Variant;
	PolicyObject: Variant;
	NewRule: Variant;
begin
	try
		if IsWindowsNT6() then
		begin
			// create new inbound firewall exception:
			PolicyObject := CreateOleObject('HNetCfg.FwPolicy2');
			NewRule := CreateOleObject('HNetCfg.FWRule');
			NewRule.Name := AppName + ' - inbound';
			NewRule.Applicationname := FileName;
			NewRule.Protocol := NET_FW_IP_PROTOCOL_TCP;
			NewRule.EdgeTraversal := True;
			NewRule.Action := NET_FW_ACTION_ALLOW;
			NewRule.Enabled := True;
			PolicyObject.Rules.Add(NewRule);
		end
		else
		begin
			FirewallObject := CreateOleObject('HNetCfg.FwAuthorizedApplication');
			FirewallObject.ProcessImageFileName := FileName;
			FirewallObject.Name := AppName;
			FirewallObject.Scope := NET_FW_SCOPE_ALL;
			FirewallObject.IpVersion := NET_FW_IP_VERSION_ANY;
			FirewallObject.Enabled := True;

			FirewallManager := CreateOleObject('HNetCfg.FwMgr');

			FirewallProfile := FirewallManager.LocalPolicy.GetProfileByType(NET_FW_PROFILE_STANDARD);
			FirewallProfile.AuthorizedApplications.Add(FirewallObject);

			FirewallProfile := FirewallManager.LocalPolicy.GetProfileByType(NET_FW_PROFILE_DOMAIN);
			FirewallProfile.AuthorizedApplications.Add(FirewallObject);
		end;
	except
	end;
end;

procedure RemoveFirewallException(AppName, FileName: string);
var
	FirewallManager: Variant;
	FirewallProfile: Variant;
	PolicyObject: Variant;
begin
	try
		if IsWindowsNT6() then
		begin
			PolicyObject := CreateOleObject('HNetCfg.FwPolicy2');
			PolicyObject.Rules.Remove(AppName + ' - inbound');
		end
		else
		begin
			FirewallManager := CreateOleObject('HNetCfg.FwMgr');

			FirewallProfile := FirewallManager.LocalPolicy.GetProfileByType(NET_FW_PROFILE_STANDARD);
			FireWallProfile.AuthorizedApplications.Remove(FileName);

			FirewallProfile := FirewallManager.LocalPolicy.GetProfileByType(NET_FW_PROFILE_DOMAIN);
			FireWallProfile.AuthorizedApplications.Remove(FileName);
		end;
	except
	end;
end;
