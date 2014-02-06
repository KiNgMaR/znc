; // stupidly complex stuff...
; // http://msdn.microsoft.com/en-us/library/aa363680%28VS.85%29.aspx

; // This is the header section.

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )

LanguageNames=(English=0x409:MSG00409)

; // The following are the categories of events.

MessageIdTypedef=WORD

MessageId=0x1
SymbolicName=INIT_CATEGORY
Language=English
Initialization Events
.

MessageId=0x2
SymbolicName=CONFIG_CATEGORY
Language=English
Configuration Events
.

MessageId=0x3
SymbolicName=RUNTIME_CATEGORY
Language=English
Runtime Events
.

; // The following are the message definitions.

MessageIdTypedef=DWORD

MessageId=0x100
Severity=Error
Facility=Runtime
SymbolicName=MSG_DLL_VERSION_MISMATCH
Language=English
The version number in ZNC.dll doesn't match the service binary.
.

MessageId=0x101
Severity=Error
Facility=System
SymbolicName=MSG_CSOCKET_FAILED
Language=English
Failed to initialize Csocket!
.

MessageId=0x102
Severity=Error
Facility=System
SymbolicName=MSG_CONFIG_CORRUPTED
Language=English
The configuration could not be loaded or parsed successfully.
DataDir: %1
%2.
.

MessageId=0x103
Severity=Error
Facility=System
SymbolicName=MSG_MODULE_BOOT_ERROR
Language=English
A module returned an error from OnBoot.
.

MessageId=0x104
Severity=Error
Facility=System
SymbolicName=MSG_RUNTIME_ERROR
Language=English
ZNC Error: %1
.

MessageId=0x105
Severity=Error
Facility=System
SymbolicName=MSG_RUNTIME_RESTART
Language=English
A service restart has been requested from within ZNC.
.

MessageId=0x106
Severity=Error
Facility=System
SymbolicName=MSG_RUNTIME_SHUTDOWN
Language=English
A service shutdown has been requested from within ZNC.
.
