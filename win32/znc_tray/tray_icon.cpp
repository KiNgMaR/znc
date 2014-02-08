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
#include "tray_icon.hpp"
#include "znc_tray.hpp"
#include "resource.h"

using namespace ZNCTray;


CTrayIcon::CTrayIcon(HWND a_hwndMainWin, const wchar_t* a_szAppTitle) :
	m_hwndMainWin(a_hwndMainWin),
	m_szAppTitle(a_szAppTitle),
	m_iconColor(0),
	m_hIcon(0),
	m_addedToTna(false)
{
}


bool CTrayIcon::Add(char a_color)
{
	if(m_addedToTna)
	{
		return this->UpdateColor(a_color);
	}

	NOTIFYICONDATA l_nid;
	this->_InitNotifyIconData(&l_nid);

	l_nid.uFlags |= NIF_MESSAGE | NIF_ICON | NIF_TIP;

	l_nid.uCallbackMessage = WM_TRAYICONEVENT;

	// set icon
	this->LoadIconForColor(a_color);
	l_nid.hIcon = m_hIcon;

	// add tooltip/title:
	wcscpy_s(l_nid.szTip, 128, m_szAppTitle);

	// go (retry some times because of known windows weirdness, at least on XP):
	for(int retry = 0; retry < 10; retry++)
	{
		m_addedToTna = (::Shell_NotifyIcon(NIM_ADD, &l_nid) != FALSE);

		if(m_addedToTna)
		{
			break;
		}

		::Sleep(100);
	}

	if(m_addedToTna)
	{
		// use most current available feature set...

		this->_InitNotifyIconData(&l_nid);

		l_nid.uVersion = (CWinUtils::WinVerAtLeast(6, 0) ? NOTIFYICON_VERSION_4 : NOTIFYICON_VERSION);

		::Shell_NotifyIcon(NIM_SETVERSION, &l_nid);
	}

	return m_addedToTna;
}


bool CTrayIcon::Remove()
{
	if(!m_addedToTna)
	{
		// already removed
		return true;
	}

	NOTIFYICONDATA l_nid;
	this->_InitNotifyIconData(&l_nid);

	m_addedToTna = false;

	return (::Shell_NotifyIcon(NIM_DELETE, &l_nid) != FALSE);
}


bool CTrayIcon::UpdateColor(char a_color)
{
	if(!m_addedToTna)
	{
		return false;
	}

	if(a_color != m_iconColor)
	{
		NOTIFYICONDATA l_nid;
		this->_InitNotifyIconData(&l_nid);

		this->LoadIconForColor(a_color);

		l_nid.hIcon = m_hIcon;
		l_nid.uFlags |= NIF_ICON;

		bool success;

		// retry some times because of known windows weirdness, at least on XP:
		for(int retry = 0; retry < 10; retry++)
		{
			success = (::Shell_NotifyIcon(NIM_MODIFY, &l_nid) != FALSE);

			if(success)
			{
				break;
			}

			::Sleep(100);
		}

		return success;
	}

	return true;
}


void CTrayIcon::OnWmTaskbarCreated()
{
	// re-add after explorer.exe restarts e.g.
	if(m_addedToTna)
	{
		m_addedToTna = false;
		this->Add(m_iconColor);
	}
}


void CTrayIcon::LoadIconForColor(char a_color)
{
	if(m_hIcon == 0 || a_color != m_iconColor)
	{
		if(m_hIcon) ::DestroyIcon(m_hIcon);

		m_hIcon = ::LoadIcon(g_hInstance, MAKEINTRESOURCE(
			(a_color == 'R' ? IDI_ICONZRED :
			(a_color == 'G' ? IDI_ICONZGREEN :
			IDI_ICONZBLUE))));
		
		m_iconColor = a_color;
	}
}


void CTrayIcon::_InitNotifyIconData(PNOTIFYICONDATA nid)
{
	// find target structure size by checking OS version...
	// the actually officially recommended way would be using DllGetVersion from Shell32.dll...
	// ...but that would require an assload of code, so we take a shortcut.

	RtlZeroMemory(nid, sizeof(NOTIFYICONDATA));

	if(CWinUtils::WinVerAtLeast(6, 0))
	{
		nid->cbSize = sizeof(NOTIFYICONDATA);
	}
	else
	{
		nid->cbSize = NOTIFYICONDATA_V3_SIZE;
	}

	nid->hWnd = m_hwndMainWin;

#if 0 /* actually don't do this, because once someone decides to move the .exe file,
	the icon will stop working ...
	ref: http://msdn.microsoft.com/en-us/library/windows/desktop/bb773352%28v=vs.85%29.aspx */

	if(CWinUtils::WinVerAtLeast(6, 1)) // identify icon by GUID on Windows 7
	{
		static const GUID ls_iconGuid = // {C28F9A28-C95E-4E99-ADFD-9A7EFA8530DF}
			{ 0xc28f9a28, 0xc95e, 0x4e99, { 0xad, 0xfd, 0x9a, 0x7e, 0xfa, 0x85, 0x30, 0xdf } };

		nid->uFlags = NIF_GUID;
		nid->guidItem = ls_iconGuid;
	}
	else // regular boring ID on XP, Vista
#endif
	{
		nid->uID = 0x962;
	}
}


void CTrayIcon::OnWmTrayIconEvent(WPARAM wParam, LPARAM lParam)
{
	UINT uMsg;
	POINT pt;

	// ref: http://msdn.microsoft.com/en-us/library/windows/desktop/bb773352%28v=vs.85%29.aspx

	if(CWinUtils::WinVerAtLeast(6, 0)) // NOTIFYICON_VERSION_4
	{
		uMsg = LOWORD(lParam);

		pt.x = GET_X_LPARAM(wParam);
		pt.y = GET_Y_LPARAM(wParam);
	}
	else // NOTIFYICON_VERSION
	{
		uMsg = static_cast<UINT>(lParam);

		::GetCursorPos(&pt);
	}

	switch(uMsg)
	{
	case WM_CONTEXTMENU:
		this->ShowContextMenu(&pt);
		break;

	case WM_LBUTTONDBLCLK:
		// send WMC_SHOW_CONTROL_WIN:
		::PostMessage(m_hwndMainWin, WM_COMMAND, MAKEWPARAM(WMC_SHOW_CONTROL_WIN, 0), 0);
		break;
	}
}


#define _APPENDMENU(FLAG, CMD, TXT) InsertMenu(hMenu, -1, MF_BYPOSITION | FLAG, CMD, TXT)

void CTrayIcon::ShowContextMenu(LPPOINT pt)
{
	HMENU hMenu = ::CreatePopupMenu();

	// put together menu at runtime:

	::_APPENDMENU(0, WMC_SHOW_CONTROL_WIN, L"Show");

	::_APPENDMENU(MF_SEPARATOR, 0, NULL);
	::_APPENDMENU(0, WMC_CLOSE_TRAY, L"Quit");

	// as per MSDN recommendation:
	::SetForegroundWindow(m_hwndMainWin);

	// Display the menu (modally):
	::TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt->x, pt->y, 0, m_hwndMainWin, NULL);

	// as per MSDN recommendation:
	::PostMessage(m_hwndMainWin, WM_NULL, 0, 0);

	// recursively free menu:
	::DestroyMenu(hMenu);
}


CTrayIcon::~CTrayIcon()
{
	Remove();
}
