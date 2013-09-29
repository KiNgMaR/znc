#ifndef _ZNC_MSVC_H
#define _ZNC_MSVC_H

// this is something like a config.h for MSVC
// it's force-included using /FI in upstream ZNC code

//#if defined(_WIN32) && defined(_MSC_VER)
//#define WIN_MSVC
//#endif

// some basic Windows headers:
#ifndef _TARGET_WINVER_H
#error target_winver.h has not been included before znc_msvc.h!
#endif

#include <Windows.h>
#include <Winsock2.h> // do not use Winsock v1.
#include <Shlwapi.h>
#include <Shlobj.h>

// C(++) headers:
#include <stdint.h>
#include <direct.h>
#include <time.h>
#include <process.h>

// map some general types:
#define ssize_t SSIZE_T
typedef long suseconds_t;

// map some POSIX function calls to ISO underscore or Win32 equivalents:
#define getcwd _getcwd
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strtoll _strtoi64
#define strtoull _strtoui64
#define getpid _getpid
#define isatty _isatty
#define ftruncate _chsize
#define mkdir(a, b) mkdir(a)
#define fsync(a) _commit(a)

__inline void usleep(int useconds) { ::Sleep(useconds / 1000); }
__inline void sleep(int seconds) { ::Sleep(seconds * 1000); }

// getopt and getopt_long are provided via getopt.c, a Win32 getopt implementation
#define HAVE_GETOPT_LONG

// set up some basics that are not provided in the MinGW headers that we ship:
typedef int _mode_t;
typedef _mode_t mode_t;
typedef short uid_t;
typedef short gid_t;

// define this as empty:
#define __MINGW_NOTHROW

// fake some includes that just appear in other headers on POSIX:
#include <sys/stat.h>

// some other things that are not provided by MinGW headers:
#define S_ISLNK(x) (0 == 1)
#define S_ISSOCK(x) (0 == 1)
#define O_NOCTTY 0 // unsupported
__inline int fchmod(int, mode_t) { return 0; } // always pretend it worked
__inline int chmod(const char*, mode_t) { return 0; } // always pretend it worked


#define ZNC_API // *TODO*

// gettimeofday from dh_time.c:
int ZNC_API gettimeofday(struct timeval *, struct timezone *);

#endif // _ZNC_MSVC_H
