/*
* Copyright (C) 2011-2014 ZNC-MSVC
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

#include <Windows.h>
#include <string>
#include "service_provider.h"

class CString;

class CZNCWindowsService
{
public:
	CZNCWindowsService();
	~CZNCWindowsService();
	static VOID WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);
	static VOID WINAPI ControlHandler(DWORD dwControl);
	void SetDataDir(const char *dataDir) { sDataDir = dataDir; };

	static DWORD InstallService(bool a_startTypeManual);
	static DWORD UninstallService();
protected:	
	DWORD Init();
	DWORD Loop();
	VOID ReportServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);

	void RedirectStdStreams();

	SERVICE_STATUS serviceStatus;
	SERVICE_STATUS_HANDLE hServiceStatus;
	// checkpoint value, incremented periodically if SERVICE_*_PENDING state
	// is sent to the SCM with SetServiceStatus
	DWORD dwCheckPoint;
	HANDLE hEventLog;
	// for exiting the ZNC loop when SERVICE_CONTROL_STOP is received
	bool bMainLoop;
	std::string sDataDir;

	static CZNCWindowsService *thisSvc;
};
