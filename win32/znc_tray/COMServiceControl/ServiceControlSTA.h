// ServiceControlSTA.h : Declaration of the CServiceControlSTA

#pragma once
#include "resource.h"       // main symbols



#include "COMServiceControl_i.h"



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// CServiceControlSTA

class ATL_NO_VTABLE CServiceControlSTA :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CServiceControlSTA, &CLSID_ServiceControlSTA>,
	public IDispatchImpl<IServiceControlSTA, &IID_IServiceControlSTA, &LIBID_COMServiceControlLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CServiceControlSTA()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SERVICECONTROLSTA)

DECLARE_NOT_AGGREGATABLE(CServiceControlSTA)

BEGIN_COM_MAP(CServiceControlSTA)
	COM_INTERFACE_ENTRY(IServiceControlSTA)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:

	STDMETHOD(DoStartService)(BSTR bStrServiceName);
	STDMETHOD(DoStopService)(BSTR bStrServiceName);
};

OBJECT_ENTRY_AUTO(__uuidof(ServiceControlSTA), CServiceControlSTA)
