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

/**
 * Registry access class
 **/
class ZNC_API CRegistryKey
{
public:
	CRegistryKey::CRegistryKey(HKEY a_hive) :
		m_hive(a_hive), m_key(0) {}

	virtual ~CRegistryKey() {
		CloseKey();
	}

	bool OpenForReading(const std::wstring& a_keyPath);
	bool OpenForWriting(const std::wstring& a_keyPath);

	DWORD ReadDword(const wchar_t* a_szName, DWORD a_default);
	std::wstring ReadString(const wchar_t* a_szName, std::wstring a_default);
	bool ReadBool(const wchar_t* a_szKey, bool a_default);

	bool WriteDword(const wchar_t* a_szName, DWORD a_newValue);
	bool WriteString(const wchar_t* a_szName, std::wstring a_newValue);
	bool WriteBool(const wchar_t* a_szKey, bool a_newValue);
	
	bool DeleteValue(const wchar_t* a_szKey);
	
	static bool DeleteKey(const std::wstring& a_parentKeyPath, const std::wstring& a_keyName);
protected:
	HKEY m_hive;
	HKEY m_key;

	void CloseKey();
};
