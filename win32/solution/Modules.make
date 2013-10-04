# ---------------------------------
# Makefile for building ZNC modules
# ---------------------------------

# don't display commands invoked by NMAKE
.SILENT :

# -------------------------
# Configuration validation
# -------------------------

VALID_CFG = FALSE
!IF "$(CFG)" == "Win32-Release" || "$(CFG)" == "Win32-Debug" || \
  "$(CFG)" == "x64-Release" || "$(CFG)" == "x64-Debug"
VALID_CFG = TRUE
!ENDIF

!IF "$(VALID_CFG)" == "FALSE" && "$(CFG)" != ""
!  MESSAGE ---------------------------------
!  MESSAGE Makefile for building ZNC modules
!  MESSAGE ---------------------------------
!  MESSAGE
!  MESSAGE Usage: nmake /f Modules.make CFG=<config> <target>
!  MESSAGE
!  MESSAGE <config> must be one of: [ Win32-Release | Win32-Debug | x64-Release | x64-Debug ]
!  MESSAGE <target> must be one of: [ ALL | clean ]
!  MESSAGE
!  MESSAGE If neither <config> nor <target> are specified this results in
!  MESSAGE the default Win32-Release config being built.
!  MESSAGE
!  ERROR Choose a valid configuration.
!ENDIF

# set default config
!IF "$(CFG)" == ""
CFG = Win32-Release
!ENDIF

# -------------
# Common macros
# -------------

SSL=1

INCLUDES=/I "..\..\include" /I "..\platform" /I "..\dependencies\include"
LIBS=kernel32.lib advapi32.lib shell32.lib ws2_32.lib ZNC.lib
LIBPATHS=/LIBPATH:"..\build\$(CFG)"
DEFINES=/D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D "_WINDLL"
CXXFLAGS=/c /GS /W3 /Zc:wchar_t /FI"target_winver.h" /FI"znc_msvc.h" /Zi /sdl /fp:precise /errorReport:prompt /WX- /Zc:forScope /Gd /EHsc /nologo
LINKFLAGS=/DLL /SUBSYSTEM:WINDOWS /NOLOGO /DYNAMICBASE /NXCOMPAT

RSP=_ZNCModules.rsp

# Intermediate directory for .obj and .dll files
INTDIR=..\build\temp\Modules\$(CFG)\ 
# same as INTDIR, but without the trailing '\'
MAKDIR=..\build\temp\Modules\$(CFG)
# WARNING! SRCDIR pathes must NOT end with '\' else NMAKE will complain!
SRCDIR=..\..\modules
SRCDIR_EXTRA=..\extra_modules

BUILDOUT=..\build\$(CFG)\modules

# -----------------------------------
# Split configuration specific macros
# -----------------------------------

# Win32-Release configuration (default)
PLATFORM=x86
PLATFORM_CFG=release

# Win32-Debug configuration
!IF "$(CFG)" == "Win32-Debug"
PLATFORM_CFG=debug
!ENDIF

# x64-Release configuration
!IF "$(CFG)" == "x64-Release"
PLATFORM=x64
!ENDIF

# x64-Debug configuration
!IF "$(CFG)" == "x64-Debug"
PLATFORM=x64
PLATFORM_CFG=debug
!ENDIF

# -----------------------------
# Configuration specific macros
# -----------------------------

!IF "$(PLATFORM_CFG)" == "release"
DEFINES=$(DEFINES) /D "NDEBUG"
CXXFLAGS=$(CXXFLAGS) /GL /Gy /Gm- /O2 /Oi /MD
LINKFLAGS=$(LINKFLAGS) /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG /MACHINE:$(PLATFORM)
!ENDIF

!IF "$(PLATFORM_CFG)" == "debug"
DEFINES=$(DEFINES) /D "_DEBUG"
CXXFLAGS=$(CXXFLAGS) /Gm /Od /RTC1 /MDd
LINKFLAGS=$(LINKFLAGS) /INCREMENTAL /DEBUG /MACHINE:$(PLATFORM)
!ENDIF

!IF "$(SSL)" == "1"
DEFINES=$(DEFINES) /D "HAVE_LIBSSL"
!ENDIF

LIBPATHS=$(LIBPATHS) /LIBPATH:"..\dependencies\lib_$(PLATFORM)"

# --------------------
# List of target files
# --------------------

!INCLUDE ModulesList.make

!IF "$(SSL)" == "1"
OBJS=$(OBJS) $(OBJS_SSL)
DLLS=$(DLLS) $(DLLS_SSL)
!ENDIF

# ----------------
# Makefile targets
# ----------------

build: _dir_check $(DLLS)

clean:
  echo Deleting intermediate files for configuration: $(CFG)
  if exist $(INTDIR) rmdir /S /Q $(INTDIR)

rebuild: clean build

_dir_check:
  if not exist $(INTDIR) md $(INTDIR)
  if not exist $(INTDIR)\extra_win32 md $(INTDIR)\extra_win32
  if not exist $(BUILDOUT) md $(BUILDOUT)

# compile .obj files using inference rules
$(OBJS):

# ---------------
# Inference rules
# ---------------

# cpp => obj
{$(SRCDIR)}.cpp{$(MAKDIR)}.obj:
  if exist $(RSP) del $(RSP)
  echo $(CXXFLAGS) >>$(RSP)
  echo $(INCLUDES) >>$(RSP)
  echo $(DEFINES) >>$(RSP)
#  echo /Yc"stdafx.hpp" >>$(RSP)
#  echo /Fp$(INTDIR)ZNC_mods.pch >>$(RSP)
  echo /Fo$(INTDIR) >>$(RSP)
  echo /Fd$(INTDIR)vc110.pdb >>$(RSP)
  echo $< >>$(RSP)
  cl @$(RSP)
  del $(RSP)

# extra_win32\ cpp => obj
{$(SRCDIR_EXTRA)}.cpp{$(MAKDIR)}.obj:
  if exist $(RSP) del $(RSP)
  echo $(CXXFLAGS) >>$(RSP)
  echo $(INCLUDES) >>$(RSP)
  echo $(DEFINES) >>$(RSP)
#  echo /Yc"stdafx.hpp" >>$(RSP)
#  echo /Fp$(INTDIR)ZNC_mods.pch >>$(RSP)
  echo /Fo$(INTDIR) >>$(RSP)
  echo /Fd$(INTDIR)vc110.pdb >>$(RSP)
  echo $< >>$(RSP)
  cl @$(RSP)
  del $(RSP)

# obj => dll
{$(MAKDIR)}.obj{$(MAKDIR)}.dll:
  if exist $(RSP) del $(RSP)
  echo /OUT:$@ >>$(RSP)
  echo $(LIBPATHS) >>$(RSP)
  echo $(LINKFLAGS) >>$(RSP)
  echo /PDB:$(INTDIR)$(@B).pdb >>$(RSP)
  echo $(LIBS) >>$(RSP)
  echo $< >>$(RSP)
  link @$(RSP)
  copy /Y $(@R).dll $(BUILDOUT) >NUL
  if exist $(@R).pdb copy /Y $(@R).pdb $(BUILDOUT) >NUL
  del $(RSP)
