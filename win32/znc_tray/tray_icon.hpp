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

namespace ZNCTray
{


class CTrayIcon
{
public:
	CTrayIcon(HWND a_hwndMainWin, const wchar_t* a_szAppTitle);
	virtual ~CTrayIcon();

	bool Add(char a_color);
	void OnWmTaskbarCreated();
	void OnWmTrayIconEvent(WPARAM wParam, LPARAM lParam);
	bool UpdateColor(char a_color);
	bool Remove();
protected:
	HWND m_hwndMainWin;
	const wchar_t* m_szAppTitle;
	char m_iconColor;
	HICON m_hIcon;
	bool m_addedToTna;

	void _InitNotifyIconData(PNOTIFYICONDATA nid);
	void LoadIconForColor(char a_color);

	void ShowContextMenu(LPPOINT pt);
};


}; // end of namespace
