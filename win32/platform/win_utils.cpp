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
