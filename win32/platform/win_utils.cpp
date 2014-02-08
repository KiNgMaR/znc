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

#include "win_utils.h"


bool CWinUtils::WinVerAtLeast(DWORD dwMajor, DWORD dwMinor, WORD dwServicePack)
{
	OSVERSIONINFOEXW l_osver = { sizeof(OSVERSIONINFOEXW), 0 };
	l_osver.dwPlatformId = VER_PLATFORM_WIN32_NT;
	l_osver.dwMajorVersion = dwMajor;
	l_osver.dwMinorVersion = dwMinor;

	DWORDLONG dwlConditionMask = 0;
	VER_SET_CONDITION(dwlConditionMask, VER_PLATFORMID, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

	DWORD dwlInfo = VER_PLATFORMID | VER_MAJORVERSION | VER_MINORVERSION;

	if (dwServicePack != -1)
	{
		l_osver.wServicePackMajor = dwServicePack;

		VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

		dwlInfo |= VER_SERVICEPACKMAJOR;
	}

	return (::VerifyVersionInfoW(&l_osver, dwlInfo, dwlConditionMask) != FALSE);
}


void CWinUtils::HardenDLLSearchPath()
{
	// this removes `pwd` from the DLL search path.

	// available from Windows XP SP1 and higher:

	if (CWinUtils::WinVerAtLeast(5, 1, 1))
	{
		typedef BOOL(WINAPI *fSetDllDirectory)(LPCWSTR);

		fSetDllDirectory pSetDllDirectory = (fSetDllDirectory)::GetProcAddress(
			::GetModuleHandleW(L"Kernel32.dll"), "SetDllDirectoryW");

		if (pSetDllDirectory != nullptr)
		{
			pSetDllDirectory(L"");
		}
	}
}


void CWinUtils::HardenHeap()
{
#ifndef _DEBUG
	// Activate program termination on heap corruption.
	// http://msdn.microsoft.com/en-us/library/aa366705%28VS.85%29.aspx
	typedef BOOL(WINAPI *pHeapSetInformation)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
	pHeapSetInformation HeapSetInformation = (pHeapSetInformation)::GetProcAddress(
		::GetModuleHandleW(L"Kernel32.dll"), "HeapSetInformation");
	if (HeapSetInformation != nullptr)
	{
		HeapSetInformation(::GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);
	}
#endif
}


#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x00000001
#endif

void CWinUtils::EnforceDEP()
{
#ifndef _WIN64
	// Explicitly activate DEP, only necessary on 32 bit.
	// http://msdn.microsoft.com/en-us/library/bb736299%28VS.85%29.aspx
	typedef BOOL(WINAPI *pSetProcessDEPPolicy)(DWORD);
	pSetProcessDEPPolicy SetProcessDEPPolicy = (pSetProcessDEPPolicy)::GetProcAddress(
		::GetModuleHandleW(L"Kernel32.dll"), "SetProcessDEPPolicy");
	if (SetProcessDEPPolicy != nullptr)
	{
		SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
	}
#endif
}


bool CWinUtils::CreateFolderPath(const std::wstring& a_path)
{
	int r = ::SHCreateDirectoryExW(0, a_path.c_str(), NULL);

	return (r == ERROR_SUCCESS || r == ERROR_FILE_EXISTS || r == ERROR_ALREADY_EXISTS);
}


std::wstring CWinUtils::GetExePath()
{
	WCHAR l_buf[1000] = { 0 };
	WCHAR l_buf2[1000] = { 0 };

	::GetModuleFileNameW(NULL, (LPWCH)l_buf, 999);
	::GetLongPathNameW(l_buf, l_buf2, 999);

	return l_buf2;
}


std::wstring CWinUtils::GetExeDir()
{
	WCHAR l_buf[1000] = { 0 };
	WCHAR l_buf2[1000] = { 0 };

	::GetModuleFileNameW(NULL, (LPWCH)l_buf, 999);
	::GetLongPathNameW(l_buf, l_buf2, 999);
	::PathRemoveFileSpecW(l_buf2);
	::PathRemoveBackslashW(l_buf2);

	return l_buf2;
}
