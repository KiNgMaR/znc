#ifndef _TARGET_WINVER_H
#define _TARGET_WINVER_H

// the following definitions match MSVC runtime's
// mininum system requirements.

#ifdef _WIN64
	#define WINVER 0x0600 // Windows Vista / Server 2008
	#define _WIN32_IE 0x0700 // Internet Explorer 7
#endif

#if defined(_WIN32) && !defined(_WIN64)
	#define WINVER 0x0501 // Windows XP
	#define _WIN32_IE 0x0600 // Internet Explorer 6
#endif

#ifdef _WIN32
	#define _WIN32_WINNT WINVER
	#define _WIN32_WINDOWS WINVER

	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#define NOGDI // not required, plus may cause conflicts
	#define _CRT_SECURE_NO_WARNINGS // not ideal, but ok for now
	#define _CRT_RAND_S
#endif

#endif // _TARGET_WINVER_H
