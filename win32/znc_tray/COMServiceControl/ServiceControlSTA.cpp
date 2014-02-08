// ServiceControlSTA.cpp : Implementation of CServiceControlSTA

#include "stdafx.h"
#include "ServiceControlSTA.h"


// CServiceControlSTA



STDMETHODIMP CServiceControlSTA::DoStartService(BSTR bStrServiceName)
{
	HRESULT ret = E_FAIL;
	const CComBSTR sServiceName(bStrServiceName);
		
	SC_HANDLE hScm = 0, hService = 0;
		
	hScm = ::OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, STANDARD_RIGHTS_READ);

	if(hScm)
	{
		hService = ::OpenService(hScm, sServiceName,
			SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_START);

		if(hService && ::StartService(hService, 0, NULL))
		{
			ret = S_OK;
		}
	}

	if(ret != S_OK)
	{
		ret = HRESULT_FROM_WIN32(::GetLastError());
	}

	if(hService)
		::CloseServiceHandle(hService);

	if(hScm)
		::CloseServiceHandle(hScm);

	return ret;
}


STDMETHODIMP CServiceControlSTA::DoStopService(BSTR bStrServiceName)
{
	HRESULT ret = E_FAIL;
	const CComBSTR sServiceName(bStrServiceName);
		
	SC_HANDLE hScm = 0, hService = 0;
		
	hScm = ::OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, STANDARD_RIGHTS_READ);

	if(hScm)
	{
		hService = ::OpenService(hScm, sServiceName,
			SERVICE_QUERY_STATUS | SERVICE_INTERROGATE | SERVICE_STOP);

		SERVICE_STATUS dummy;

		if(hService && // may take some time till the service actually stops,
			// should account for that in a future version
			::ControlService(hService, SERVICE_CONTROL_STOP, &dummy))
		{
			ret = S_OK;
		}
	}

	if(ret != S_OK)
	{
		ret = HRESULT_FROM_WIN32(::GetLastError());
	}

	if(hService)
		::CloseServiceHandle(hService);

	if(hScm)
		::CloseServiceHandle(hScm);

	return ret;
}
