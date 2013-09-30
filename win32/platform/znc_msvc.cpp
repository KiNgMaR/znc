#include "znc_msvc.h"
#include <stdio.h>
#include <fcntl.h>

const char *ZNC_VERSION_EXTRA =
#ifdef _WIN64
	"-Win-x64";
#else
	"-Win";
#endif

// some utils:

static void HardenDLLSearchPath()
{
	// this removes `pwd` from the DLL search path.

	OSVERSIONINFOEX winver = { sizeof(OSVERSIONINFOEX), 0 };

	::GetVersionEx((LPOSVERSIONINFO)&winver);

	// available from Windows XP SP1 and higher:

	if (winver.dwMajorVersion > 5 || (winver.dwMajorVersion == 5 && winver.wServicePackMajor >= 1))
	{
		typedef BOOL (WINAPI *pSetDllDirectory)(LPCTSTR);

		pSetDllDirectory SetDllDirectory = (pSetDllDirectory)::GetProcAddress(::GetModuleHandle("Kernel32.dll"), "SetDllDirectory");

		if (SetDllDirectory != nullptr)
		{
			SetDllDirectory("");
		}
	}
}

static void HardenHeap()
{
#ifndef _DEBUG
	// Activate program termination on heap corruption.
	// http://msdn.microsoft.com/en-us/library/aa366705%28VS.85%29.aspx
	typedef BOOL (WINAPI *pHeapSetInformation)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
	pHeapSetInformation HeapSetInformation = (pHeapSetInformation)::GetProcAddress(::GetModuleHandle("Kernel32.dll"), "HeapSetInformation");
	if (HeapSetInformation != nullptr)
	{
		HeapSetInformation(::GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);
	}
#endif
}

#ifndef PROCESS_DEP_ENABLE
#define PROCESS_DEP_ENABLE 0x00000001
#endif

static void EnsureDEPIsEnabled()
{
#ifndef _WIN64
	// Explicitly activate DEP, only necessary on 32 bit.
	// http://msdn.microsoft.com/en-us/library/bb736299%28VS.85%29.aspx
	typedef BOOL (WINAPI *pSetProcessDEPPolicy)(DWORD);
	pSetProcessDEPPolicy SetProcessDEPPolicy = (pSetProcessDEPPolicy)::GetProcAddress(::GetModuleHandle("Kernel32.dll"), "SetProcessDEPPolicy");
	if (SetProcessDEPPolicy != nullptr)
	{
		SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
	}
#endif
}

int CZNCWin32Helpers::RuntimeStartUp()
{
	EnsureDEPIsEnabled();
	HardenHeap();
	HardenDLLSearchPath();

	// this prevents open()/read() and friends from stripping \r from files... simply
	// adding _O_BINARY to the modes doesn't seem to be enough for some reason...
	// if we don't do this, Template.cpp will break because it uses string.size() for
	// file position calculations.
	_set_fmode(_O_BINARY);

#ifdef HAVE_LIBSSL
	CRYPTO_malloc_init();
#endif

	setlocale(LC_CTYPE, "C");

	return 0;
}
