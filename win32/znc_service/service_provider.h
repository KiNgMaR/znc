 // stupidly complex stuff...
 // http://msdn.microsoft.com/en-us/library/aa363680%28VS.85%29.aspx
 // This is the header section.
 // The following are the categories of events.
//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SYSTEM                  0x0
#define FACILITY_STUBS                   0x3
#define FACILITY_RUNTIME                 0x2
#define FACILITY_IO_ERROR_CODE           0x4


//
// Define the severity codes
//
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: INIT_CATEGORY
//
// MessageText:
//
// Initialization Events
//
#define INIT_CATEGORY                    ((WORD)0x00000001L)

//
// MessageId: CONFIG_CATEGORY
//
// MessageText:
//
// Configuration Events
//
#define CONFIG_CATEGORY                  ((WORD)0x00000002L)

//
// MessageId: RUNTIME_CATEGORY
//
// MessageText:
//
// Runtime Events
//
#define RUNTIME_CATEGORY                 ((WORD)0x00000003L)

 // The following are the message definitions.
//
// MessageId: MSG_DLL_VERSION_MISMATCH
//
// MessageText:
//
// The version number in ZNC.dll doesn't match the service binary.
//
#define MSG_DLL_VERSION_MISMATCH         ((DWORD)0xC0020100L)

//
// MessageId: MSG_CSOCKET_FAILED
//
// MessageText:
//
// Failed to initialize Csocket!
//
#define MSG_CSOCKET_FAILED               ((DWORD)0xC0000101L)

//
// MessageId: MSG_CONFIG_CORRUPTED
//
// MessageText:
//
// The configuration could not be loaded or parsed successfully.
// DataDir: %1
// %2.
//
#define MSG_CONFIG_CORRUPTED             ((DWORD)0xC0000102L)

//
// MessageId: MSG_MODULE_BOOT_ERROR
//
// MessageText:
//
// A module returned an error from OnBoot.
//
#define MSG_MODULE_BOOT_ERROR            ((DWORD)0xC0000103L)

//
// MessageId: MSG_RUNTIME_ERROR
//
// MessageText:
//
// ZNC Error: %1
//
#define MSG_RUNTIME_ERROR                ((DWORD)0xC0000104L)

//
// MessageId: MSG_RUNTIME_RESTART
//
// MessageText:
//
// A service restart has been requested from within ZNC.
//
#define MSG_RUNTIME_RESTART              ((DWORD)0xC0000105L)

//
// MessageId: MSG_RUNTIME_SHUTDOWN
//
// MessageText:
//
// A service shutdown has been requested from within ZNC.
//
#define MSG_RUNTIME_SHUTDOWN             ((DWORD)0xC0000106L)

