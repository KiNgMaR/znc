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

// ZNC_Tray.exe is also supposed to work fine on Windows XP,
// but we need some constants etc. that are only defined with
// WINVER >= 0x0600.

#define WINVER 0x0600 // Windows Vista / Server 2008
#define _WIN32_IE 0x0700 // Internet Explorer 7
#define _WIN32_WINNT WINVER
#define _WIN32_WINDOWS WINVER

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define _CRT_RAND_S
#define _WINSOCK_DEPRECATED_NO_WARNINGS

// define to empty:
#define ZNC_API

#include "znc_msvc_shared.h"

#include <Windows.h>
#include <Windowsx.h>
#include <Shellapi.h>
#include <Shlwapi.h>
#include <Shlobj.h>
#include <process.h>

#include <memory>
#include <string>
#include <sstream>

#include <Commctrl.h>
#pragma comment(lib, "Comctl32.lib")

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define EnableDlgItem(HWND, ITEM, ENABLE) EnableWindow(::GetDlgItem(HWND, ITEM), (ENABLE) ? TRUE : FALSE)

#define HAVE_COM_SERVICE_CONTROL
