/*
* Copyright (C) 2012-2014 ZNC-MSVC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "stdafx.hpp"
#include "znc_setup_wizard.h"
#include "win_registry.h"


std::wstring CZNCSetupWizard::GetServiceConfDirPath()
{
	CRegistryKey l_key(HKEY_LOCAL_MACHINE);

	if(l_key.OpenForReading(L"SOFTWARE\\ZNC"))
	{
		return l_key.ReadString(L"ServiceDataDir", L"");
	}

	return L"";
}


bool CZNCSetupWizard::WriteDefaultServiceConfDirPath()
{
	CRegistryKey l_key(HKEY_LOCAL_MACHINE);

	if(l_key.OpenForWriting(L"SOFTWARE\\ZNC"))
	{
		wchar_t l_buf[1024] = {0};

		if(SUCCEEDED(::SHGetFolderPath(0, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, l_buf)) && ::PathIsDirectory(l_buf))
		{
			::PathAddBackslash(l_buf);

			std::wstring l_path(l_buf);

			l_path += L"ZNC\\Configuration";

			return l_key.WriteString(L"ServiceDataDir", l_path);
		}
	}

	return false;
}


std::wstring CZNCSetupWizard::GetServiceZncConfPath()
{
	return GetServiceConfDirPath() + L"\\configs\\znc.conf";
}


bool CZNCSetupWizard::DoesServiceConfigExist()
{
	std::wstring l_dataDir = GetServiceConfDirPath(),
		l_configFile = l_dataDir + L"\\configs\\znc.conf";

	return (!l_dataDir.empty() && ::PathFileExists(l_configFile.c_str()));
}


CInitialZncConf::CInitialZncConf()
{
	m_adminPass = GenerateRandomPassword();
	m_port = FindFreeListenerPort();
}


const std::string CInitialZncConf::GetWebUrl() const
{
	std::stringstream l_url;
	l_url << "http://127.0.0.1:" << m_port << "/";
	return l_url.str();
}


const std::string CInitialZncConf::GetAdminPass() const
{
	return m_adminPass;
}


const std::string CInitialZncConf::GetZncConf() const
{
	std::stringstream l_conf;

	l_conf << "// Auto-generated config by ZNC-MSVC" << std::endl;
	l_conf << "// Please remember to change the admin user's password" << std::endl;
	l_conf << std::endl;
	l_conf << "Listener4 = 127.0.0.1 " << m_port << std::endl;
	l_conf << "LoadModule = webadmin" << std::endl;
	l_conf << std::endl;
	l_conf << "<User zncadmin>" << std::endl;
	l_conf << "\tPass = " << m_adminPass << std::endl;
	l_conf << "\tAdmin = true" << std::endl;
	l_conf << "\tIRCConnectEnabled = false" << std::endl;
	l_conf << "</User>" << std::endl;

	return l_conf.str();
}


bool CInitialZncConf::WriteZncConf(const std::wstring a_path) const
{
	FILE *pf;
	bool success = false;

	if(_wfopen_s(&pf, a_path.c_str(), L"w") == 0)
	{
		const std::string l_cfg = this->GetZncConf();

		if(fwrite(l_cfg.c_str(), 1, l_cfg.size(), pf) == l_cfg.size())
		{
			success = true;
		}

		fclose(pf);
	}

	return success;
}


unsigned short CInitialZncConf::FindFreeListenerPort()
{
    SOCKET sock = INVALID_SOCKET;
    sockaddr_in service;

    sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(sock == INVALID_SOCKET)
	{
        return 0;
    }

	const int START_PORT = 963;
	const int MAX_PORT = 1024;

	unsigned short port = START_PORT;

	do
	{
		service.sin_family = AF_INET;
		service.sin_addr.s_addr = inet_addr("127.0.0.1");
		service.sin_port = htons(port);

		if(::bind(sock, (SOCKADDR*)&service, sizeof(service)) != SOCKET_ERROR)
		{
			break;
		}

		port++;
	} while(port <= MAX_PORT);

	::closesocket(sock);

	return (port <= MAX_PORT ? port : 0);
}


std::string CInitialZncConf::GenerateRandomPassword(size_t a_length)
{
	std::string l_pass;

	for(size_t i = 0; i < a_length; i++)
	{
		switch(rand() % 3)
		{
		case 0:
			l_pass += (char)('0' + (rand() % 10));
			break;
		case 1:
			l_pass += (char)('a' + (rand() % 26));
			break;
		case 2:
			l_pass += (char)('A' + (rand() % 26));
			break;
		}
	}

	return l_pass;
}
