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

#include "tray_icon.hpp"
#include "service_status.hpp"
#include "znc_setup_wizard.h"

namespace ZNCTray
{

typedef enum _service_status
{
	ZS_UNKNOWN = 0,
	ZS_NOT_INSTALLED,
	ZS_STANDALONE,
	ZS_STARTING,
	ZS_RUNNING,
	ZS_STOPPING,
	ZS_STOPPED
} EServiceStatus;


class CControlWindow
{
public:
	CControlWindow();
	virtual ~CControlWindow();

	int Run();
	void Show();
	void Hide();

protected:
	// main properties:
	HWND m_hwndDlg;
	std::shared_ptr<CTrayIcon> m_trayIcon;

	HICON m_hIconSmall, m_hIconBig;
	std::shared_ptr<CResourceBitmap> m_bmpLogo;

	HWND m_hwndStatusBar;

	std::shared_ptr<CServiceStatus> m_serviceStatus;
	EServiceStatus m_statusFlag;

	// main methods:
	bool CreateDlg();
	int MessageLoop();
	static INT_PTR CALLBACK DialogProcStatic(HWND, UINT, WPARAM, LPARAM);
	bool DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool OnWmCommand(UINT uCmd);
	void OnPaint();

	void InitialSetup();
	void OnBeforeDestroy();

	void DetectServiceStatus();
	void UpdateUIWithServiceStatus();

	// initial wizard handling members:
	std::shared_ptr<CInitialZncConf> m_initialConf;
	void OnServiceFirstStarted(bool a_started);
};


}; // end of namespace
