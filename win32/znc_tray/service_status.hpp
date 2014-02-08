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

// instances of this class are NOT safe to call from multiple threads!

class CServiceStatus
{
public:
	// watch this service:
	CServiceStatus(const wchar_t* a_serviceName);
	virtual ~CServiceStatus();

	// call this before doing anything else:
	bool Init();
	// probes:
	bool IsInstalled();
	bool IsRunning();
	bool CanStartStop();

	// watching functionality:
	bool StartWatchingStatus(HWND a_targetWindow);
	void StopWatchingStatus();

	// control functionality:
	void StartService(HWND a_hwnd);
	void StopService(HWND a_hwnd);

protected:
	const wchar_t* m_serviceName;
	// valid from calling Init to the instance being destroyed:
	SC_HANDLE m_scm;
	// access this after calling OpenService only:
	// it's a read-only (SERVICE_QUERY_STATUS) handle.
	SC_HANDLE m_hService;

	// for watching:
	HWND m_hwndWatch;
	HANDLE m_hWatchThread;
	bool m_doWatch;

	// used in most of the other calls:
	bool OpenService();
	void CloseService();

	// utility method:
	static bool IsNT6();

	// watching implementation internals:
	static unsigned __stdcall WatchThreadProc(void *ptr);
	void WatchWait();
	bool WatchWaitNT6(SC_HANDLE hService);
	void WatchWaitFallback(SC_HANDLE hService);
	static void CALLBACK ServiceNotifyCallback(void *ptr);
	void OnWatchEvent(const LPSERVICE_STATUS_PROCESS pss);

	// starting/stopping thread proc:
	static void __cdecl StartStopperThread(void *ptr);
	void DoStartStopInternal(bool start, HWND a_hwnd);
};


// Notifications sent to the window registered with CServiceStatus::StartWatchingStatus:

#define WM_SERVICESTOPPED (WM_USER + 101)
#define WM_SERVICESTARTED (WM_USER + 102)
#define WM_SERVICESTARTING (WM_USER + 103)
#define WM_SERVICESTOPPING (WM_USER + 104)
#define WM_SERVICECONTROL_RESULT (WM_USER + 105)
