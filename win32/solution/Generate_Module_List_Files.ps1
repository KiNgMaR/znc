# Updates ModulesList.make and ModulesList.iss from
#  filesystem (module dirs with .cpp files)
# no beauty.

$mod_package_dirs = @("..\..\modules\", "..\extra_modules\")
$blacklist = @("charset.cpp", "cyrusauth.cpp", "modperl.cpp", "modpython.cpp", "modtcl.cpp", "shell.cpp")

filter script:moduleblacklist { if([array]::IndexOf($blacklist, $_.name) -eq -1) { $_ } }
filter script:skip-null { $_|?{ $_ } }

Function GetMakeFileFileList([string]$Extension, [bool]$SSL)
{
	[string]$result = ""
	foreach($p in $mod_package_dirs)
	{
		$list = Get-Childitem "$p*.cpp" | moduleblacklist
        if($SSL)
        {
            $list = $list | skip-null | ForEach-Object { if( ( $(Get-Content $_) |
                            select-string -pattern "REQUIRESSL") )
                     { $_.Name } }
        }
        else
        {
            $list = $list | skip-null | ForEach-Object { if( !( $(Get-Content $_) |
                            select-string -pattern "REQUIRESSL") )
                     { $_.Name } }
        }
        $list = $list | skip-null |
			% { $_ -replace "cpp$", $Extension } |
			% { $_ -replace "^", "`$(MAKDIR)\" } |
			% { $_ -replace "$", " \`r`n" }
		$result += " " + $list
	}
	return ($result | % { $_ -replace "\s*\\\s*$", "" })
}

"DLLS=" + (GetMakeFileFileList "dll" $false) + "`r`n" | Out-File -Encoding "UTF8" ModulesList.make
"DLLS_SSL=" + (GetMakeFileFileList "dll" $true) + "`r`n" | Out-File -Encoding "UTF8" -Append ModulesList.make

"OBJS=" + (GetMakeFileFileList "obj" $false) + "`r`n" | Out-File -Encoding "UTF8" -Append ModulesList.make
"OBJS_SSL=" + (GetMakeFileFileList "obj" $true) + "`r`n" | Out-File -Encoding "UTF8" -Append ModulesList.make

Function GetInnoSetupFileList()
{
	[string]$result = ""
	foreach($p in $mod_package_dirs)
	{
		Get-Childitem "$p*.cpp" | moduleblacklist | ForEach-Object {
			$name = ($_.name -replace "\.cpp$", "")
			if($p -eq "..\..\modules\") { $compo = "core" } else { $compo = "extra_win32" }
			$result += "`r`nSource: `"{#SourceFileDir32}\modules\$name.dll`"; DestDir: `"{app}\modules`"; Flags: ignoreversion; Check: not Is64BitInstallMode; "
			if($_.name -ne "win32_service_helper.cpp") { $result += "Components: modules/$compo" }
			$result += "`r`nSource: `"{#SourceFileDir64}\modules\$name.dll`"; DestDir: `"{app}\modules`"; Flags: ignoreversion; Check: Is64BitInstallMode; "
			if($_.name -ne "win32_service_helper.cpp") { $result += "Components: modules/$compo" }

			if(Test-Path -Path ("$p" + "data\$name") -PathType Container)
			{
				$result += "`r`nSource: `"{#SourceCodeDir}\win32\solution\$p" + "data\$name\*`"; DestDir: `"{app}\modules\data\$name`"; Flags: recursesubdirs; Components: modules/$compo"
			}
		}
	}
	return $result
}

(GetInnoSetupFileList) | Out-File -Encoding "UTF8" "ModulesList.iss"
