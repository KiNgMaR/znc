/*
* Copyright (C) 2004-2013 ZNC-MSVC
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

// Win32 utility + feature code:

class ZNC_API CZNCWin32Helpers
{
public:
	static int RuntimeStartUp();

	static bool IsWindowsService() {
		return ms_serviceMode;
	}
	static void SetWindowsServiceMode() {
		ms_serviceMode = true;
	}

	// this is the port that is used for IPC between ZNC_Tray
	// and ZNC_Service...
	static unsigned short GetServiceControlPort() {
		return 962;
	}

private:
	static bool ms_serviceMode;
};

// Shared service definitions:

#define ZNC_SERVICE_NAME L"ZNC"
#define ZNC_EVENT_PROVIDER L"ZNCService"
#define ZNC_SERVICE_DESCRIPTION L"ZNC is an advanced IRC network bouncer (BNC)."
