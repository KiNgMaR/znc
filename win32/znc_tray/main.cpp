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
#include "znc_tray.hpp"
#include "control_window.hpp"

static ULONG_PTR s_gdiplusToken;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR wszCommandLine, int nShowCmd)
{
	g_hInstance = hInstance;

	// enforce some things:
	CWinUtils::EnforceDEP();
	CWinUtils::HardenHeap();
	CWinUtils::HardenDLLSearchPath();

	// init COM:
	::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	
	// init GDI+:
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&s_gdiplusToken, &gdiplusStartupInput, NULL);

	// init CC:
	INITCOMMONCONTROLSEX icce = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES };
	::InitCommonControlsEx(&icce);

	// init winsock (for communication with service helper module):
	WSADATA wsaData;
	int iResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

	// srand:
	int seed = ::GetTickCount();
	seed ^= ::GetCurrentProcessId();
	seed ^= rand();
	::srand(seed);

	// run main app:
	ZNCTray::CControlWindow *l_mainWin = new ZNCTray::CControlWindow();

	int ret = l_mainWin->Run();

	// make sure this gets destroyed before the following shutdown code is invoked:
	delete l_mainWin;

	// shut down winsock:
	::WSACleanup();

	// shut down GDI+:
	Gdiplus::GdiplusShutdown(s_gdiplusToken);

	// shut down COM:
	::CoUninitialize();

	return ret;
}


// enable "visual styles":
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
	processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


// global var, extern'd in znc_tray.hpp
HINSTANCE g_hInstance;
