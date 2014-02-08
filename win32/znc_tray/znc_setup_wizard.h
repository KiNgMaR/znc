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

#pragma once

class CZNCSetupWizard
{
public:
	
	static std::wstring GetServiceConfDirPath();
	static bool WriteDefaultServiceConfDirPath();
	static std::wstring GetServiceZncConfPath();
	static bool DoesServiceConfigExist();

protected:
	
};

class CInitialZncConf
{
public:
	CInitialZncConf();

	bool GotFreePort() const { return m_port != 0; }

	const std::string GetWebUrl() const;
	const std::string GetAdminPass() const;

	const std::string GetZncConf() const;
	bool WriteZncConf(const std::wstring a_path) const;

protected:
	unsigned short m_port;
	std::string m_adminPass;

	static unsigned short FindFreeListenerPort();
	static std::string GenerateRandomPassword(size_t a_length = 10);
};
