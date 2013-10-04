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

#include "znc_msvc.h"
#include <stdio.h>
#include <fcntl.h>
#ifdef HAVE_LIBSSL
#include <openssl/crypto.h>
#endif

const char *ZNC_VERSION_EXTRA =
#ifdef _WIN64
	"-Win-x64";
#else
	"-Win";
#endif

// some utils:

static void HardenDLLSearchPath()
{
	// this removes `pwd` from the DLL search path.

	OSVERSIONINFOEX winver = { sizeof(OSVERSIONINFOEX), 0 };

	::GetVersionEx((LPOSVERSIONINFO)&winver);

	// available from Windows XP SP1 and higher:

	if (winver.dwMajorVersion > 5 || (winver.dwMajorVersion == 5 && winver.wServicePackMajor >= 1))
	{
		typedef BOOL (WINAPI *pSetDllDirectory)(LPCTSTR);

		pSetDllDirectory SetDllDirectory = (pSetDllDirectory)::GetProcAddress(::GetModuleHandle("Kernel32.dll"), "SetDllDirectory");

		if (SetDllDirectory != nullptr)
		{
			SetDllDirectory("");
		}
	}
}

static void HardenHeap()
{
#ifndef _DEBUG
	// Activate program termination on heap corruption.
	// http://msdn.microsoft.com/en-us/library/aa366705%28VS.85%29.aspx
	typedef BOOL (WINAPI *pHeapSetInformation)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
	pHeapSetInformation HeapSetInformation = (pHeapSetInformation)::GetProcAddress(::GetModuleHandle("Kernel32.dll"), "HeapSetInformation");
	if (HeapSetInformation != nullptr)
	{
		HeapSetInformation(::GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);
	}
#endif
}

#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x00000001
#endif

static void EnsureDEPIsEnabled()
{
#ifndef _WIN64
	// Explicitly activate DEP, only necessary on 32 bit.
	// http://msdn.microsoft.com/en-us/library/bb736299%28VS.85%29.aspx
	typedef BOOL (WINAPI *pSetProcessDEPPolicy)(DWORD);
	pSetProcessDEPPolicy SetProcessDEPPolicy = (pSetProcessDEPPolicy)::GetProcAddress(::GetModuleHandle("Kernel32.dll"), "SetProcessDEPPolicy");
	if (SetProcessDEPPolicy != nullptr)
	{
		SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
	}
#endif
}

int CZNCWin32Helpers::RuntimeStartUp()
{
	EnsureDEPIsEnabled();
	HardenHeap();
	HardenDLLSearchPath();

	// this prevents open()/read() and friends from stripping \r from files... simply
	// adding _O_BINARY to the modes doesn't seem to be enough for some reason...
	// if we don't do this, Template.cpp will break because it uses string.size() for
	// file position calculations.
	if (_set_fmode(_O_BINARY) != 0) {
		return -1;
	}

	if (!setlocale(LC_CTYPE, "C")) {
		return -2;
	}

#ifdef HAVE_LIBSSL
	if (!CRYPTO_malloc_init()) {
		return -3;
	}
#endif

	return 0;
}
