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
 * Windows utility class
 **/
class ZNC_API CWinUtils
{
public:
	static bool WinVerAtLeast(DWORD dwMajor, DWORD dwMinor, WORD dwServicePack = -1);

	static bool CreateFolderPath(const std::wstring& a_path);

	static std::wstring GetExePath();
	static std::wstring GetExeDir();

	static void HardenDLLSearchPath();
	static void HardenHeap();
	static void EnforceDEP();
};
