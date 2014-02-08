

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Sat Feb 08 20:47:18 2014
 */
/* Compiler settings for COMServiceControl.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __COMServiceControl_i_h__
#define __COMServiceControl_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IServiceControlSTA_FWD_DEFINED__
#define __IServiceControlSTA_FWD_DEFINED__
typedef interface IServiceControlSTA IServiceControlSTA;
#endif 	/* __IServiceControlSTA_FWD_DEFINED__ */


#ifndef __ServiceControlSTA_FWD_DEFINED__
#define __ServiceControlSTA_FWD_DEFINED__

#ifdef __cplusplus
typedef class ServiceControlSTA ServiceControlSTA;
#else
typedef struct ServiceControlSTA ServiceControlSTA;
#endif /* __cplusplus */

#endif 	/* __ServiceControlSTA_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IServiceControlSTA_INTERFACE_DEFINED__
#define __IServiceControlSTA_INTERFACE_DEFINED__

/* interface IServiceControlSTA */
/* [unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IServiceControlSTA;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B2E93418-924E-4DFC-A110-46473991B05F")
    IServiceControlSTA : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoStartService( 
            /* [in] */ BSTR bStrServiceName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoStopService( 
            /* [in] */ BSTR bStrServiceName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IServiceControlSTAVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IServiceControlSTA * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IServiceControlSTA * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IServiceControlSTA * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IServiceControlSTA * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IServiceControlSTA * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IServiceControlSTA * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IServiceControlSTA * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DoStartService )( 
            IServiceControlSTA * This,
            /* [in] */ BSTR bStrServiceName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DoStopService )( 
            IServiceControlSTA * This,
            /* [in] */ BSTR bStrServiceName);
        
        END_INTERFACE
    } IServiceControlSTAVtbl;

    interface IServiceControlSTA
    {
        CONST_VTBL struct IServiceControlSTAVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IServiceControlSTA_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IServiceControlSTA_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IServiceControlSTA_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IServiceControlSTA_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IServiceControlSTA_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IServiceControlSTA_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IServiceControlSTA_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IServiceControlSTA_DoStartService(This,bStrServiceName)	\
    ( (This)->lpVtbl -> DoStartService(This,bStrServiceName) ) 

#define IServiceControlSTA_DoStopService(This,bStrServiceName)	\
    ( (This)->lpVtbl -> DoStopService(This,bStrServiceName) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IServiceControlSTA_INTERFACE_DEFINED__ */



#ifndef __COMServiceControlLib_LIBRARY_DEFINED__
#define __COMServiceControlLib_LIBRARY_DEFINED__

/* library COMServiceControlLib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_COMServiceControlLib;

EXTERN_C const CLSID CLSID_ServiceControlSTA;

#ifdef __cplusplus

class DECLSPEC_UUID("DC2BF05E-2451-435E-A24C-1B9BA804B5F0")
ServiceControlSTA;
#endif
#endif /* __COMServiceControlLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


