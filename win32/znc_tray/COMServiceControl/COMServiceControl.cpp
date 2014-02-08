// COMServiceControl.cpp : Implementation of WinMain


#include "stdafx.h"
#include "resource.h"
#include "COMServiceControl_i.h"
#include "xdlldata.h"



class CCOMServiceControlModule : public ATL::CAtlExeModuleT< CCOMServiceControlModule >
	{
public :
	DECLARE_LIBID(LIBID_COMServiceControlLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_COMSERVICECONTROL, "{029CCDA9-293E-4994-94FE-D1F5B5F7F445}")
	};

CCOMServiceControlModule _AtlModule;



//
extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, 
								LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	return _AtlModule.WinMain(nShowCmd);
}

