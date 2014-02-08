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
#include "service_status.hpp"
#ifdef HAVE_COM_SERVICE_CONTROL
#include "COMServiceControl_i.h"
#include "Strsafe.h"
#endif

// for QueryServiceStatus & friends:
#pragma comment(lib, "Advapi32.lib")


CServiceStatus::CServiceStatus(const wchar_t* a_serviceName) :
	m_serviceName(a_serviceName),
	m_scm(NULL), m_hService(NULL),
	m_hwndWatch(NULL), m_hWatchThread(NULL), m_doWatch(false)
{
}


bool CServiceStatus::Init()
{
	if(m_scm)
	{
		// already initialized
		return true;
	}

	m_scm = ::OpenSCManager(NULL, NULL, STANDARD_RIGHTS_READ | SC_MANAGER_ENUMERATE_SERVICE);

	return (!!m_scm);
}


bool CServiceStatus::IsInstalled()
{
	if(this->OpenService())
	{
		// service can be opened, so it must be installed

		this->CloseService();

		return true;
	}

	return false;
}


bool CServiceStatus::IsRunning()
{
	if(this->OpenService())
	{
		SERVICE_STATUS ss = {0};

		if(::QueryServiceStatus(m_hService, &ss))
		{
			return (ss.dwCurrentState == SERVICE_RUNNING ||
				ss.dwCurrentState == SERVICE_START_PENDING);
		}

		// we are lazy and don't CloseService here,
		// destructor or the next call will take care of it
	}

	return false;
}


bool CServiceStatus::OpenService()
{
	if(!m_scm)
	{
		return false;
	}

	if(m_hService)
	{
		// already open
		return true;
	}

	// read-only handle!
	m_hService = ::OpenService(m_scm, m_serviceName, SERVICE_QUERY_STATUS);

	return (m_hService != NULL);
}


bool CServiceStatus::CanStartStop()
{
	// try to obtain handle with start/stop privilege
	SC_HANDLE hService = ::OpenService(m_scm, m_serviceName,
		SERVICE_QUERY_STATUS | SERVICE_START | SERVICE_STOP);

	if(hService != NULL)
	{
		::CloseServiceHandle(hService);

		return true;
	}

#ifdef HAVE_COM_SERVICE_CONTROL

	if(CServiceStatus::IsNT6())
	{
		IServiceControlSTA *ppSS = NULL;

		HRESULT hr = ::CoCreateInstance(CLSID_ServiceControlSTA, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&ppSS));

		if(SUCCEEDED(hr))
		{
			// class registered, so it's hopefully usable

			ppSS->Release();

			return true;
		}
	}

#endif

	return false;
}


struct start_stop_service_proc_data_t
{
	int action;
	CServiceStatus *p;
	HWND hwnd;
	HANDLE signal;
};


void CServiceStatus::StartService(HWND a_hwnd)
{
	if(this->IsRunning()) // implicit OpenService()
	{
		return;
	}

	// StartService may block for up to 30 seconds, so we use a separate fire-and-forget thread

	start_stop_service_proc_data_t data;
	data.action = 1;
	data.p = this;
	data.hwnd = a_hwnd;
	data.signal = ::CreateEvent(NULL, TRUE, FALSE, NULL);

	if(_beginthread(&StartStopperThread, 0, &data) != (uintptr_t)-1)
	{
		::WaitForSingleObject(data.signal, INFINITE);
	}

	::CloseHandle(data.signal);
}


void CServiceStatus::StopService(HWND a_hwnd)
{
	SERVICE_STATUS ss = {0};

	if(!this->OpenService())
	{
		return;
	}

	if(::QueryServiceStatus(m_hService, &ss) &&
		(ss.dwCurrentState == SERVICE_STOPPED || ss.dwCurrentState == SERVICE_STOP_PENDING))
	{
		return;
	}

	// ControlService may block for up to 30 seconds, so we use a separate fire-and-forget thread

	start_stop_service_proc_data_t data;
	data.action = 0;
	data.p = this;
	data.hwnd = a_hwnd;
	data.signal = ::CreateEvent(NULL, TRUE, FALSE, NULL);

	if(_beginthread(&StartStopperThread, 0, &data) != (uintptr_t)-1)
	{
		::WaitForSingleObject(data.signal, INFINITE);
	}

	::CloseHandle(data.signal);
}


void __cdecl CServiceStatus::StartStopperThread(void *ptr)
{
	start_stop_service_proc_data_t data;
	memcpy_s(&data, sizeof(start_stop_service_proc_data_t), ptr, sizeof(start_stop_service_proc_data_t));
	// ptr is safe to release now:
	::SetEvent(data.signal);
	data.signal = NULL;

	data.p->DoStartStopInternal(data.action == 1, data.hwnd);
}


// utility function copied from: http://msdn.microsoft.com/en-us/library/ms679687
static HRESULT CoCreateInstanceAsAdmin(HWND hwnd, REFCLSID rclsid, REFIID riid, __out void ** ppv)
{
	BIND_OPTS3 bo;
    WCHAR wszCLSID[50];
    WCHAR wszMonikerName[100];

    ::StringFromGUID2(rclsid, wszCLSID, sizeof(wszCLSID) / sizeof(wszCLSID[0]));

    HRESULT hr = ::StringCchPrintf(wszMonikerName, sizeof(wszMonikerName) / sizeof(wszMonikerName[0]),
		L"Elevation:Administrator!new:%s", wszCLSID);

    if(FAILED(hr))
	{
        return hr;
	}
    
	ZeroMemory(&bo, sizeof(bo));
    bo.cbStruct = sizeof(bo);
    bo.hwnd = hwnd;
    bo.dwClassContext = CLSCTX_LOCAL_SERVER;

    return ::CoGetObject(wszMonikerName, &bo, riid, ppv);
}


void CServiceStatus::DoStartStopInternal(bool start, HWND a_hwnd)
{
	SC_HANDLE hService = ::OpenService(m_scm, m_serviceName,
		SERVICE_QUERY_STATUS | (start ? SERVICE_START : SERVICE_STOP));

	BOOL bResult = FALSE;

	if(hService)
	{
		// for XP, admin users without UAC etc., try "locally"

		if(start)
		{
			::StartService(hService, 0, NULL);
		}
		else
		{
			SERVICE_STATUS dummy;

			bResult = ::ControlService(hService, SERVICE_CONTROL_STOP, &dummy);
		}

		// handle is closed later on
	}
	else if(CServiceStatus::IsNT6())
	{
		// woo-hoo moniking action!
		::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

		IServiceControlSTA *ppSS = NULL;

		HRESULT hr = ::CoCreateInstanceAsAdmin(a_hwnd, CLSID_ServiceControlSTA, IID_PPV_ARGS(&ppSS));

		if(SUCCEEDED(hr))
		{
			BSTR bStrServiceName = ::SysAllocString(m_serviceName);

			if(start)
			{
				hr = ppSS->DoStartService(bStrServiceName);
			}
			else
			{
				hr = ppSS->DoStopService(bStrServiceName);
			}

			bResult = (SUCCEEDED(hr));

			ppSS->Release();

			::SysFreeString(bStrServiceName);
		}

		::CoUninitialize();
	}
	else
		;// shit out of luck...

	if(bResult)
	{
		// based on http://msdn.microsoft.com/en-us/library/windows/desktop/ms686315%28v=vs.85%29.aspx

		SERVICE_STATUS_PROCESS ssStatus = {0}; 
		DWORD dwOldCheckPoint; 
		DWORD dwStartTickCount;
		DWORD dwWaitTime;
		DWORD dwBytesNeeded = 0;
		
		if(!hService)
		{
			hService = ::OpenService(m_scm, m_serviceName, SERVICE_QUERY_STATUS);
		}

		if(::QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus,
			sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
		{
			dwStartTickCount = ::GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;

			while(ssStatus.dwCurrentState == (start ? SERVICE_START_PENDING : SERVICE_STOP_PENDING))
			{
				// Do not wait longer than the wait hint. A good interval i
				// one-tenth of the wait hint but not less than 1 second
				// and not more than 10 seconds.
 
				dwWaitTime = ssStatus.dwWaitHint / 10;

				if(dwWaitTime < 1000)
					dwWaitTime = 1000;
				else if(dwWaitTime > 10000)
					dwWaitTime = 10000;

				::Sleep(dwWaitTime);

				if(!::QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus,
					sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
				{
					break;
				}

				if(ssStatus.dwCheckPoint > dwOldCheckPoint)
				{
					// Continue to wait and check.
					dwStartTickCount = ::GetTickCount();
					dwOldCheckPoint = ssStatus.dwCheckPoint;
				}
				else if(::GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
				{
					// Timeout waiting for service
					bResult = false;
					break;
				}
			}
			
			bResult = (ssStatus.dwCurrentState == (start ? SERVICE_RUNNING : SERVICE_STOPPED));
		}
	}

	if(hService)
	{
		::CloseServiceHandle(hService);
	}

	::SendMessage(a_hwnd, WM_SERVICECONTROL_RESULT, (start ? 1 : 0), (bResult ? 1 : 0));
}


void CServiceStatus::CloseService()
{
	if(m_hService)
	{
		::CloseServiceHandle(m_hService);
		m_hService = NULL;
	}
}


bool CServiceStatus::StartWatchingStatus(HWND a_targetWindow)
{
	if(m_hWatchThread != NULL)
	{
		// already watching
		return false;
	}

	m_hwndWatch = a_targetWindow;
	m_doWatch = true;

	m_hWatchThread = (HANDLE)_beginthreadex(NULL, 0, &WatchThreadProc, static_cast<void*>(this), 0, NULL);

	return (!!m_hWatchThread);
}


unsigned __stdcall CServiceStatus::WatchThreadProc(void *ptr)
{
	// get instance:
	CServiceStatus *instance = static_cast<CServiceStatus*>(ptr);

	if(!instance)
		return -1;

	instance->WatchWait();

	return 0;
}


void CServiceStatus::WatchWait()
{
	// get a separate service handle for this thread:
	SC_HANDLE l_service = ::OpenService(m_scm, m_serviceName, SERVICE_QUERY_STATUS);

	if(l_service)
	{
		bool bFallback;

		if(CServiceStatus::IsNT6())
		{
			bFallback = (this->WatchWaitNT6(l_service) == false);
		}
		else
		{
			// legacy OS (pre-Vista):
			bFallback = true;
		}

		if(bFallback)
		{
			this->WatchWaitFallback(l_service);
		}

		::CloseServiceHandle(l_service);
	}
}


bool CServiceStatus::WatchWaitNT6(SC_HANDLE hService)
{
	// the good version: NotifyServiceStatusChange
	typedef DWORD (WINAPI *tfnssc)(SC_HANDLE hService, DWORD dwNotifyMask, PSERVICE_NOTIFY pNotifyBuffer);
	tfnssc nssc = (tfnssc)::GetProcAddress(::GetModuleHandle(L"Advapi32.dll"), "NotifyServiceStatusChange");

	if(!nssc)
	{
		return false;
	}

	SERVICE_NOTIFY serviceNotify = { 0 };
	serviceNotify.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
	serviceNotify.pfnNotifyCallback = &ServiceNotifyCallback;
	serviceNotify.pContext = static_cast<void*>(this);

	while(m_doWatch)
	{
		if(nssc(hService, SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_START_PENDING |
			SERVICE_NOTIFY_STOPPED | SERVICE_NOTIFY_STOP_PENDING, &serviceNotify) == ERROR_SUCCESS)
		{
			::SleepEx(INFINITE, TRUE); // Wait for the notification
		}
		else
			break;
	}

	return true;
}


void CServiceStatus::WatchWaitFallback(SC_HANDLE hService)
{
	SERVICE_STATUS_PROCESS ssp = {0};
	DWORD dwBytesNeeded = 0;
	DWORD dwState = 0;

	do
	{
		if(!::QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
			break;

		if(ssp.dwCurrentState != dwState)
		{
			this->OnWatchEvent(&ssp);

			dwState = ssp.dwCurrentState;
		}

		::SleepEx(1000, TRUE); // probe every second
	}
	while(m_doWatch);
}


void CALLBACK CServiceStatus::ServiceNotifyCallback(void *ptr)
{
	SERVICE_NOTIFY* serviceNotify = static_cast<SERVICE_NOTIFY*>(ptr);
	CServiceStatus *instance = static_cast<CServiceStatus*>(serviceNotify->pContext);

	if(instance && serviceNotify->dwNotificationStatus == ERROR_SUCCESS)
	{
		instance->OnWatchEvent(&serviceNotify->ServiceStatus);
	}
}


void CServiceStatus::OnWatchEvent(const LPSERVICE_STATUS_PROCESS pssp)
{
	// notify handler window:
	switch(pssp->dwCurrentState)
	{
	case SERVICE_RUNNING:
		::PostMessage(m_hwndWatch, WM_SERVICESTARTED, 0, 0);
		break;
	case SERVICE_START_PENDING:
		::PostMessage(m_hwndWatch, WM_SERVICESTARTING, 0, 0);
		break;
	case SERVICE_STOP_PENDING:
		::PostMessage(m_hwndWatch, WM_SERVICESTOPPING, 0, 0);
		break;
	case SERVICE_STOPPED:
		::PostMessage(m_hwndWatch, WM_SERVICESTOPPED, 0, 0);
		break;
	}
}


static VOID CALLBACK APCDummyProc(ULONG_PTR) {}

void CServiceStatus::StopWatchingStatus()
{
	if(m_doWatch && m_hWatchThread)
	{
		m_doWatch = false;

		// post APC shit to wake up SleepEx:
		::QueueUserAPC(&APCDummyProc, m_hWatchThread, NULL);

		// wait till the thread has terminated:
		::WaitForSingleObject(m_hWatchThread, INFINITE);
		// ... then close the handle:
		::CloseHandle(m_hWatchThread);
		m_hWatchThread = NULL;
	}
}


bool CServiceStatus::IsNT6()
{
	// check for OS support:
	OSVERSIONINFOEXW l_osver = { sizeof(OSVERSIONINFOEXW), 0 };
	l_osver.dwPlatformId = VER_PLATFORM_WIN32_NT;
	l_osver.dwMajorVersion = 6;

	DWORDLONG dwlVerCond = 0;
	VER_SET_CONDITION(dwlVerCond, VER_PLATFORMID, VER_EQUAL);
	VER_SET_CONDITION(dwlVerCond, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlVerCond, VER_MINORVERSION, VER_GREATER_EQUAL);

	return (::VerifyVersionInfo(&l_osver, VER_PLATFORMID | VER_MAJORVERSION | VER_MINORVERSION, dwlVerCond) == TRUE);
}


CServiceStatus::~CServiceStatus()
{
	this->StopWatchingStatus();

	this->CloseService();

	if(m_scm)
	{
		::CloseServiceHandle(m_scm);
	}
}
