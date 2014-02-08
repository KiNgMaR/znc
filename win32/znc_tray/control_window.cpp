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
#include "znc_tray.hpp"
#include "control_window.hpp"
#include "resource.h"

using namespace ZNCTray;


CControlWindow::CControlWindow() :
	m_hwndDlg(0),
	m_hIconSmall(0), m_hIconBig(0),
	m_hwndStatusBar(0),
	m_statusFlag(ZS_UNKNOWN)
{
}


bool CControlWindow::CreateDlg()
{
	// load some resources:

	m_hIconSmall = ::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICONZBLUE));
	m_hIconBig = (HICON)::LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICONZNC), IMAGE_ICON,
		::GetSystemMetrics(SM_CXICON),::GetSystemMetrics(SM_CYICON), 0);

	m_bmpLogo = std::shared_ptr<CResourceBitmap>(new CResourceBitmap(IDB_ZNCLOGO));

	m_serviceStatus = std::shared_ptr<CServiceStatus>(new CServiceStatus(ZNC_SERVICE_NAME));

	// our main window is a simple #32770 dialog!

	::CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_DLGCONTROL), NULL,
		DialogProcStatic, reinterpret_cast<LONG_PTR>(this));

	if(!m_hwndDlg) // set by message proc, CreateDialogParam won't return until WM_INITDIALOG has run
	{
		return false;
	}

	return true;
}


void CControlWindow::InitialSetup()
{
	if(!WM_TASKBARCREATED)
	{
		WM_TASKBARCREATED = ::RegisterWindowMessage(L"TaskbarCreated");
	}

	// set up window icons:
	::SendMessage(m_hwndDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(m_hIconSmall));
	::SendMessage(m_hwndDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(m_hIconBig));
	
	// create status bar:
	m_hwndStatusBar = ::CreateWindowEx(0, STATUSCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, m_hwndDlg, NULL, g_hInstance, NULL);
	::SendMessage(m_hwndStatusBar, SB_SIMPLE, TRUE, 0);

	// set up UAC button icons:
	if(CWinUtils::WinVerAtLeast(6, 0))
	{
		::SendDlgItemMessage(m_hwndDlg, IDC_BTN_START, BCM_SETSHIELD, 0, TRUE);
		::SendDlgItemMessage(m_hwndDlg, IDC_BTN_STOP, BCM_SETSHIELD, 0, TRUE);
	}

	// set up tray icon:
	m_trayIcon = std::shared_ptr<CTrayIcon>(new CTrayIcon(m_hwndDlg, L"ZNC"));

	m_trayIcon->Add('B');

	// set up service manager interface:
	m_serviceStatus->Init();

	DetectServiceStatus();
	UpdateUIWithServiceStatus();
}


void CControlWindow::Show()
{
	m_serviceStatus->StartWatchingStatus(m_hwndDlg);

	::ShowWindow(m_hwndDlg, SW_SHOWNORMAL);

	// check whether we need to run the initial setup:
	if(m_serviceStatus->IsInstalled() && m_statusFlag == ZS_STOPPED)
	{
		bool l_continueWizard = true;

		if(CZNCSetupWizard::GetServiceConfDirPath().empty())
		{
			int l_msgBoxResult = ::MessageBox(m_hwndDlg, L"Warning: No service configuration folder path has been set. Do you want to use the default path? Otherwise, running ZNC won't be possible.",
				L"ZNC Config", MB_ICONEXCLAMATION | MB_YESNOCANCEL);

			l_continueWizard = false;

			if(l_msgBoxResult == IDYES)
			{
				if(!CZNCSetupWizard::WriteDefaultServiceConfDirPath())
				{
					::MessageBox(m_hwndDlg, L"Error: Writing to the HKLM Registry failed. Please restart the tray program with administrative permissions to have this fixed.", L"ZNC Config", MB_ICONSTOP);

					::PostMessage(m_hwndDlg, WM_DESTROY, 0, 0);
				}
				else
				{
					// everything went well.
					l_continueWizard = true;
				}
			}
			else if(l_msgBoxResult == IDCANCEL)
			{
				::PostMessage(m_hwndDlg, WM_DESTROY, 0, 0);
			}
		}

		if(l_continueWizard && !CZNCSetupWizard::DoesServiceConfigExist())
		{
			std::wstring l_msg = L"The ZNC service has not been configured yet. Do you wish to create a config file at the following location now?\r\n\r\n"
				+ CZNCSetupWizard::GetServiceConfDirPath() + L"\r\n\r\nZNC can not run without a config file. If you already have an existing .znc configuration folder, "
				L"please move it to this location and click No.";

			if(::MessageBox(m_hwndDlg, l_msg.c_str(), L"ZNC Config", MB_ICONQUESTION | MB_YESNO) == IDYES)
			{
				m_initialConf = std::shared_ptr<CInitialZncConf>(new CInitialZncConf());
				const std::wstring l_confDirPath = CZNCSetupWizard::GetServiceConfDirPath() + L"\\configs";

				// ignore any failures, we handle errors from WriteZncConf below:
				CWinUtils::CreateFolderPath(l_confDirPath);

				if(m_initialConf->WriteZncConf(CZNCSetupWizard::GetServiceZncConfPath()))
				{
					m_serviceStatus->StartService(m_hwndDlg);
				}
				else
				{
					// :TODO: improve error message
					::MessageBox(m_hwndDlg, L"Writing to the config file failed.", L"ZNC Config", MB_ICONEXCLAMATION);
				}
			}
			else if(CZNCSetupWizard::DoesServiceConfigExist())
			{
				m_serviceStatus->StartService(m_hwndDlg);
			}
		}
	}
}


void CControlWindow::OnServiceFirstStarted(bool a_started)
{
	if(!a_started)
	{
		::MessageBox(m_hwndDlg, L"The config file has been written, but starting the service failed for some reason. Please make sure you installed the entire package correctly.",
			L"ZNC Config", MB_ICONEXCLAMATION);

		return;
	}

	const std::string l_url = m_initialConf->GetWebUrl();

	::ShellExecuteA(m_hwndDlg, "open", l_url.c_str(), NULL, NULL, SW_SHOWNORMAL);

	m_initialConf.reset();
}


void CControlWindow::Hide()
{
	m_serviceStatus->StopWatchingStatus();

	::ShowWindow(m_hwndDlg, SW_HIDE);
}


void CControlWindow::DetectServiceStatus()
{
	if(!m_serviceStatus->IsInstalled())
	{
		m_statusFlag = ZS_NOT_INSTALLED;
	}
	else if(m_serviceStatus->IsRunning())
	{
		m_statusFlag = ZS_RUNNING;
	}
	else
	{
		m_statusFlag = ZS_STOPPED;
	}
}


void CControlWindow::UpdateUIWithServiceStatus()
{
	std::wstring sStatus;

	switch(m_statusFlag)
	{
	case ZS_NOT_INSTALLED:
		sStatus = L"Service not installed.";
		break;
	case ZS_STANDALONE:
		sStatus = L"Running in stand-alone mode.";
		break;
	case ZS_STARTING:
		sStatus = L"Service is starting...";
		break;
	case ZS_RUNNING:
		sStatus = L"Service is up and running.";
		break;
	case ZS_STOPPING:
		sStatus = L"Service is stopping...";
		break;
	case ZS_STOPPED:
		sStatus = L"Service is stopped.";
		break;
	default:
		sStatus = L"Unknown :(";
	}

	::SetDlgItemText(m_hwndDlg, IDC_LBL_STATUS, sStatus.c_str());

	::EnableDlgItem(m_hwndDlg, IDC_BTN_START, m_statusFlag == ZS_STOPPED);
	::EnableDlgItem(m_hwndDlg, IDC_BTN_STOP, m_statusFlag == ZS_RUNNING);
}


void CControlWindow::OnBeforeDestroy()
{
}


INT_PTR CALLBACK CControlWindow::DialogProcStatic(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LONG_PTR l_ptr = ::GetWindowLongPtr(hDlg, GWLP_USERDATA);

	// set up GWLP_USERDATA during init:
	if(!l_ptr && uMsg == WM_INITDIALOG)
	{
		l_ptr = lParam;
		::SetWindowLongPtr(hDlg, GWLP_USERDATA, l_ptr);
	}

	// extract ptr to CControlWindow instance:
	CControlWindow *l_wnd = reinterpret_cast<CControlWindow*>(l_ptr);

	if(l_wnd)
	{
		// store hwnd, may not present because we get here before returning from CreateDialogParam:
		if(!l_wnd->m_hwndDlg) l_wnd->m_hwndDlg = hDlg;

		// hand over to instance proc:
		return (l_wnd->DialogProc(uMsg, wParam, lParam) ? TRUE : FALSE);
	}

	return FALSE;
}


bool CControlWindow::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		this->InitialSetup();
		this->Show();
		return true;

	case WM_PAINT:
		this->OnPaint();
		return true;

	case WM_COMMAND:
		this->OnWmCommand(LOWORD(wParam));
		return true;

	case WM_DESTROY:
		this->OnBeforeDestroy();
		::PostQuitMessage(0);
		return true;

	case WM_CLOSE:
		this->Hide();
		return true;

	// custom messages below this line

	case WM_TRAYICONEVENT:
		m_trayIcon->OnWmTrayIconEvent(wParam, lParam);
		return true;

	case WM_SERVICESTARTED:
		m_statusFlag = ZS_RUNNING;
		UpdateUIWithServiceStatus();
		break;

	case WM_SERVICESTARTING:
		m_statusFlag = ZS_STARTING;
		UpdateUIWithServiceStatus();
		break;

	case WM_SERVICESTOPPED:
		m_statusFlag = ZS_STOPPED;
		UpdateUIWithServiceStatus();
		break;

	case WM_SERVICESTOPPING:
		m_statusFlag = ZS_STOPPING;
		UpdateUIWithServiceStatus();
		break;

	case WM_SERVICECONTROL_RESULT:
		{
			bool bOpStart = (wParam != 0), bSuccess = (lParam != 0);

			if(bOpStart && m_initialConf)
			{
				OnServiceFirstStarted(bSuccess);
			}
			else if(!bSuccess)
			{
				::MessageBox(m_hwndDlg,	(bOpStart ?
					L"Starting the service failed. Make sure it's installed correctly." :
					L"Stopping the service failed."), L"Error", MB_ICONEXCLAMATION);
			}
		}
		break;

	// end of custom messages

	}

	// catch explorer.exe restarts:
	if(uMsg == WM_TASKBARCREATED)
	{
		m_trayIcon->OnWmTaskbarCreated();
	}

	return false;
}


bool CControlWindow::OnWmCommand(UINT uCmd)
{
	switch(uCmd)
	{
	case WMC_SHOW_CONTROL_WIN:
		this->Show();
		return true;

	case IDM_HIDE:
		this->Hide();
		return true;

	case WMC_CLOSE_TRAY:
	case IDM_QUIT:
		if(m_statusFlag == ZS_NOT_INSTALLED || ::MessageBox(m_hwndDlg,
			L"You are about to close ZNC's tray icon. The ZNC service will keep running, if started. "
			L"Do you want to continue?", L"Confirm", MB_ICONQUESTION | MB_YESNO)
			== IDYES)
			// :TODO: put together message based on m_statusFlag
		{
			::DestroyWindow(m_hwndDlg);
		}
		return true;

	case IDM_ABOUT:
		::MessageBox(m_hwndDlg, L"ZNC is © 2004-2014 all contributors, please see the AUTHORS file for details. Licensed under the Apache License, Version 2.0.\r\n\r\nhttp://www.znc.in/"
			L"\r\n\r\nZNC Tray © Ingmar Runge 2012-2014",
			L"About", MB_ICONINFORMATION | MB_OK);
		return true;

	case IDC_BTN_START:
		m_serviceStatus->StartService(m_hwndDlg);
		break;

	case IDC_BTN_STOP:
		m_serviceStatus->StopService(m_hwndDlg);
		break;
	}

	return false;
}


void CControlWindow::OnPaint()
{
	// let's make it nice
	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(m_hwndDlg, &ps);

	// white background:
	RECT rectClient;
	RECT rectStatusBar;
	if(::GetClientRect(m_hwndDlg, &rectClient) && ::GetWindowRect(m_hwndStatusBar, &rectStatusBar))
	{
		// remove status bar from client rect:
		rectClient.bottom -= (rectStatusBar.bottom - rectStatusBar.top);

		HBRUSH hbWhite = ::CreateSolidBrush(RGB(255, 255, 255));
		::FillRect(hDC, &rectClient, hbWhite);
		::DeleteObject(hbWhite);
	}
	
	// draw PNG logo:
	if(m_bmpLogo && m_bmpLogo->GetBmp())
	{
		Gdiplus::Graphics gr(hDC);

		gr.DrawImage(m_bmpLogo->GetBmp().get(), 10,
			(rectClient.bottom - rectClient.top) / 2 - m_bmpLogo->GetBmp()->GetHeight() / 2,
			m_bmpLogo->GetBmp()->GetWidth(), m_bmpLogo->GetBmp()->GetHeight());
	}
	
	// add a background for the group box control...
	RECT rectStatusGroupBox;
	if(::GetWindowRect(::GetDlgItem(m_hwndDlg, IDC_STATUS_GROUPBOX), &rectStatusGroupBox))
	{
		::ScreenToClient(m_hwndDlg, (LPPOINT)&rectStatusGroupBox.left);
		::ScreenToClient(m_hwndDlg, (LPPOINT)&rectStatusGroupBox.right);
		HBRUSH hbWindowColor = ::GetSysColorBrush(COLOR_BTNFACE);
		HPEN hPen = ::CreatePen(PS_NULL, 0, 0);
		::SelectObject(hDC, hbWindowColor);
		::SelectObject(hDC, hPen);
		::RoundRect(hDC, rectStatusGroupBox.left - 6, rectStatusGroupBox.top - 1,
			rectStatusGroupBox.right + 6, rectStatusGroupBox.bottom + 5, 5, 5);
		::DeleteObject(hPen);
	}

	::EndPaint(m_hwndDlg, &ps);
}


int CControlWindow::MessageLoop()
{
	MSG msg;
	int status;

	while((status = ::GetMessage(&msg, 0, 0, 0)) != 0)
	{
		if(status == -1)
			return -1;

		// YOU SPIN ME RIGHT ROUND, BABY
		// RIGHT ROUND
		// LIKE A RECORD BABY
		// RIGHT ROUND ROUND ROUND
		// love this song

		if(!::IsDialogMessage(m_hwndDlg, &msg))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return static_cast<int>(msg.wParam);
}


int CControlWindow::Run()
{
	if(!this->CreateDlg())
	{
		return 1;
	}

	return this->MessageLoop();
}


CControlWindow::~CControlWindow()
{
	::DestroyIcon(m_hIconSmall);
	::DestroyIcon(m_hIconBig);
}


/* global var */
UINT WM_TASKBARCREATED = 0;
