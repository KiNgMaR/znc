/*****************************************************************************
 *                                                                           *
 * DH_TIME.C                                                                 *
 *                                                                           *
 * Freely redistributable and modifiable.  Use at your own risk.             *
 *                                                                           *
 * Copyright 1994 The Downhill Project                                       *
 * 
 * Modified by Shane Caraveo for use with PHP
 *
 *****************************************************************************/

#include <time.h>
#include <tchar.h>
#include <windows.h>
#include <winbase.h>
#include <mmsystem.h>
#include <errno.h>


struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};


typedef VOID (WINAPI *MyGetSystemTimeAsFileTime)(LPFILETIME lpSystemTimeAsFileTime);

static MyGetSystemTimeAsFileTime get_time_func(void)
{
	MyGetSystemTimeAsFileTime timefunc = NULL;
	HMODULE hMod = LoadLibrary(_T("kernel32.dll"));

	if (hMod) {
		/* Max possible resolution <1us, win8/server2012 */
		timefunc = (MyGetSystemTimeAsFileTime)GetProcAddress(hMod, "GetSystemTimePreciseAsFileTime");

		if(!timefunc) {
			/* 100ns blocks since 01-Jan-1641 */
			timefunc = (MyGetSystemTimeAsFileTime)GetProcAddress(hMod, "GetSystemTimeAsFileTime");
		}
	}

	return timefunc;
}

static int getfilesystemtime(struct timeval *tv)
{
	FILETIME ft;
	unsigned __int64 ff = 0;
	MyGetSystemTimeAsFileTime timefunc;
	ULARGE_INTEGER fft;

	timefunc = get_time_func();
	if (timefunc) {
		timefunc(&ft);
	} else {
		GetSystemTimeAsFileTime(&ft);
	}

        /*
	 * Do not cast a pointer to a FILETIME structure to either a 
	 * ULARGE_INTEGER* or __int64* value because it can cause alignment faults on 64-bit Windows.
	 * via  http://technet.microsoft.com/en-us/library/ms724284(v=vs.85).aspx
	 */
	fft.HighPart = ft.dwHighDateTime;
	fft.LowPart = ft.dwLowDateTime;
	ff = fft.QuadPart;

	ff /= 10Ui64; /* convert to microseconds */
	ff -= 11644473600000000Ui64; /* convert to unix epoch */

	tv->tv_sec = (long)(ff / 1000000Ui64);
	tv->tv_usec = (long)(ff % 1000000Ui64);

	return 0;
}

int gettimeofday(struct timeval *time_Info, struct timezone *timezone_Info)
{
	/* Get the time, if they want it */
	if (time_Info != NULL) {
		getfilesystemtime(time_Info);
	}
	/* Get the timezone, if they want it */
	if (timezone_Info != NULL) {
		_tzset();
		timezone_Info->tz_minuteswest = _timezone;
		timezone_Info->tz_dsttime = _daylight;
	}
	/* And return */
	return 0;
}
