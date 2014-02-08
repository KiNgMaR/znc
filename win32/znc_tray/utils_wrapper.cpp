#include "stdafx.hpp"

// We don't want to load ZNC.DLL into ZNC_Tray.exe.
// Therefore, statically bake in some of the utils here.

#pragma comment(lib, "Shlwapi.lib")

#include "win_registry.cpp"
#include "win_utils.cpp"
