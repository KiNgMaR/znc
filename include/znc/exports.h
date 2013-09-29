// Generic helper definitions for shared library support
// cf. http://gcc.gnu.org/wiki/Visibility
#if defined(WIN_MSVC) || defined(__CYGWIN__)
  #define ZNC_HELPER_DLL_IMPORT __declspec(dllimport)
  #define ZNC_HELPER_DLL_EXPORT __declspec(dllexport)
  #define ZNC_HELPER_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define ZNC_HELPER_DLL_IMPORT __attribute__ ((visibility("default")))
    #define ZNC_HELPER_DLL_EXPORT __attribute__ ((visibility("default")))
    #define ZNC_HELPER_DLL_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define ZNC_HELPER_DLL_IMPORT
    #define ZNC_HELPER_DLL_EXPORT
    #define ZNC_HELPER_DLL_LOCAL
  #endif
#endif

// Now we use the generic helper definitions above to define ZNC_API and ZNC_LOCAL.
// ZNC_API is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
// ZNC_LOCAL is used for non-api symbols.

#ifdef WIN_MSVC // we build a znc.dll on Windows that znc.exe and all modules link to.
  #ifdef ZNC_EXPORTS // defined if we are building znc.dll (instead of using it)
    #define ZNC_API ZNC_HELPER_DLL_EXPORT
  #else
    #define ZNC_API ZNC_HELPER_DLL_IMPORT
  #endif // ZNC_EXPORTS
  #define ZNC_LOCAL ZNC_HELPER_DLL_LOCAL
#else // normal binary with "inbound linking" for non-MSVC systems.
  #define ZNC_API
  #define ZNC_LOCAL
#endif // WIN_MSVC
