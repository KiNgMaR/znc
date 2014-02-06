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

#ifndef UNICODE
// ZNC hack...
#define UNICODE
#endif

#include "win_registry.h"


CRegistryKey::CRegistryKey(HKEY a_hive) :
	m_hive(a_hive), m_key(0)
{
}


bool CRegistryKey::OpenForReading(const std::wstring& a_keyPath)
{
	HKEY l_key;

	CloseKey();

	// disable registry redirection on x64 Windows (KEY_WOW64_64KEY).
	if(::RegOpenKeyExW(m_hive, a_keyPath.c_str(), 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &l_key) != ERROR_SUCCESS)
	{
		return false;
	}

	m_key = l_key;

	return true;
}


bool CRegistryKey::OpenForWriting(const std::wstring& a_keyPath)
{
	HKEY l_key;

	CloseKey();

	// disable registry redirection on x64 Windows (KEY_WOW64_64KEY).
	if(::RegCreateKeyExW(m_hive, a_keyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &l_key, NULL) != ERROR_SUCCESS)
	{
		return false;
	}

	m_key = l_key;

	return true;
}


DWORD CRegistryKey::ReadDword(const wchar_t* a_szName, DWORD a_default)
{
	DWORD l_dwType;
	DWORD l_dwBuf = 0;
	DWORD l_dwBufSz = sizeof(DWORD);

	if(::RegQueryValueExW(m_key, a_szName, NULL, &l_dwType,
		(LPBYTE)&l_dwBuf, &l_dwBufSz) == ERROR_SUCCESS && l_dwType == REG_DWORD)
	{
		return l_dwBuf;
	}

	return a_default;
}


std::wstring CRegistryKey::ReadString(const wchar_t* a_szName, std::wstring a_default)
{
	wchar_t l_buffer[1000] = {0};
	DWORD l_dwType, l_dwSize = 999 * sizeof(wchar_t);

	if(::RegQueryValueExW(m_key, a_szName, 0, &l_dwType, (LPBYTE)l_buffer,
		&l_dwSize) == ERROR_SUCCESS && l_dwType == REG_SZ)
	{
		return l_buffer;
	}

	return a_default;
}


bool CRegistryKey::ReadBool(const wchar_t* a_szKey, bool a_default)
{
	return (ReadDword(a_szKey, (a_default ? 1 : 0)) != 0 ? 1 : 0);
}


bool CRegistryKey::WriteDword(const wchar_t* a_szName, DWORD a_newValue)
{
	bool l_success = (::RegSetValueExW(m_key, a_szName, 0, REG_DWORD,
		(LPBYTE)&a_newValue, sizeof(DWORD)) == ERROR_SUCCESS);

	_ASSERT(l_success);

	return l_success;
}


bool CRegistryKey::WriteString(const wchar_t* a_szName, std::wstring a_newValue)
{
	bool l_success = (::RegSetValueExW(m_key, a_szName, 0, REG_SZ, (LPBYTE)a_newValue.c_str(),
		(DWORD)(a_newValue.size() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS);

	_ASSERT(l_success);

	return l_success;
}


bool CRegistryKey::WriteBool(const wchar_t* a_szKey, bool a_newValue)
{
	return WriteDword(a_szKey, (a_newValue ? 1 : 0));
}


bool CRegistryKey::DeleteValue(const wchar_t* a_szKey)
{
	LSTATUS l_result = ::RegDeleteValueW(m_key, a_szKey);

	return (l_result == ERROR_SUCCESS || l_result == ERROR_FILE_NOT_FOUND);
}


void CRegistryKey::CloseKey()
{
	if(m_key)
	{
		::RegCloseKey(m_key);
		m_key = 0;
	}
}


bool CRegistryKey::DeleteKey(const std::wstring& a_parentKeyPath, const std::wstring& a_keyName)
{
	HKEY l_key;
	LSTATUS l_result = ~ERROR_SUCCESS;

	if(::RegOpenKeyExW(HKEY_LOCAL_MACHINE, a_parentKeyPath.c_str(), 0,
		KEY_ALL_ACCESS | KEY_WOW64_64KEY, &l_key) == ERROR_SUCCESS)
	{
		// RegDeleteKeyEx is not available on Windows XP 32 bit...
		// ...however required only for proper WOW64 treatement, so we do this:
		typedef LSTATUS (APIENTRY *frdkex)(HKEY hKey, LPCWSTR lpSubKey,  REGSAM samDesired, DWORD Reserved);
		frdkex l_RegDeleteKeyEx = (frdkex)::GetProcAddress(::GetModuleHandleW(L"Advapi32.dll"), "RegDeleteKeyExW");
		if(l_RegDeleteKeyEx)
		{
			l_result = l_RegDeleteKeyEx(l_key, a_keyName.c_str(), KEY_WOW64_64KEY, 0);
		}
		else
		{
			// fall back to RegDeleteKey...
			l_result = ::RegDeleteKeyW(l_key, a_keyName.c_str());
		}

		::RegCloseKey(l_key);
	}

	return (l_result == ERROR_SUCCESS);
}


CRegistryKey::~CRegistryKey()
{
	CloseKey();
}
