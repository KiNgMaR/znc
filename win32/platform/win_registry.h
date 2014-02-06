/*
 * Copyright (C) 2011 Ingmar Runge
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
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
	CRegistryKey(HKEY a_hive);
	virtual ~CRegistryKey();

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
