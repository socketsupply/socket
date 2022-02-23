

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../edge_embedded_browser/client/win/current/webview2experimental.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.xx.xxxx 
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
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __webview2experimental_h__
#define __webview2experimental_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICoreWebView2Experimental5_FWD_DEFINED__
#define __ICoreWebView2Experimental5_FWD_DEFINED__
typedef interface ICoreWebView2Experimental5 ICoreWebView2Experimental5;

#endif 	/* __ICoreWebView2Experimental5_FWD_DEFINED__ */


#ifndef __ICoreWebView2Experimental6_FWD_DEFINED__
#define __ICoreWebView2Experimental6_FWD_DEFINED__
typedef interface ICoreWebView2Experimental6 ICoreWebView2Experimental6;

#endif 	/* __ICoreWebView2Experimental6_FWD_DEFINED__ */


#ifndef __ICoreWebView2Experimental8_FWD_DEFINED__
#define __ICoreWebView2Experimental8_FWD_DEFINED__
typedef interface ICoreWebView2Experimental8 ICoreWebView2Experimental8;

#endif 	/* __ICoreWebView2Experimental8_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler;

#endif 	/* __ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionController3_FWD_DEFINED__
#define __ICoreWebView2ExperimentalCompositionController3_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalCompositionController3 ICoreWebView2ExperimentalCompositionController3;

#endif 	/* __ICoreWebView2ExperimentalCompositionController3_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionController4_FWD_DEFINED__
#define __ICoreWebView2ExperimentalCompositionController4_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalCompositionController4 ICoreWebView2ExperimentalCompositionController4;

#endif 	/* __ICoreWebView2ExperimentalCompositionController4_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuItem_FWD_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuItem_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalContextMenuItem ICoreWebView2ExperimentalContextMenuItem;

#endif 	/* __ICoreWebView2ExperimentalContextMenuItem_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuItemCollection_FWD_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuItemCollection_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalContextMenuItemCollection ICoreWebView2ExperimentalContextMenuItemCollection;

#endif 	/* __ICoreWebView2ExperimentalContextMenuItemCollection_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuRequestedEventArgs_FWD_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuRequestedEventArgs_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalContextMenuRequestedEventArgs ICoreWebView2ExperimentalContextMenuRequestedEventArgs;

#endif 	/* __ICoreWebView2ExperimentalContextMenuRequestedEventArgs_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuRequestedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuRequestedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalContextMenuRequestedEventHandler ICoreWebView2ExperimentalContextMenuRequestedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalContextMenuRequestedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuTarget_FWD_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuTarget_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalContextMenuTarget ICoreWebView2ExperimentalContextMenuTarget;

#endif 	/* __ICoreWebView2ExperimentalContextMenuTarget_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalControllerOptions_FWD_DEFINED__
#define __ICoreWebView2ExperimentalControllerOptions_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalControllerOptions ICoreWebView2ExperimentalControllerOptions;

#endif 	/* __ICoreWebView2ExperimentalControllerOptions_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCustomItemSelectedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalCustomItemSelectedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalCustomItemSelectedEventHandler ICoreWebView2ExperimentalCustomItemSelectedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalCustomItemSelectedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalController2_FWD_DEFINED__
#define __ICoreWebView2ExperimentalController2_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalController2 ICoreWebView2ExperimentalController2;

#endif 	/* __ICoreWebView2ExperimentalController2_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment3_FWD_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment3_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalEnvironment3 ICoreWebView2ExperimentalEnvironment3;

#endif 	/* __ICoreWebView2ExperimentalEnvironment3_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment6_FWD_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment6_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalEnvironment6 ICoreWebView2ExperimentalEnvironment6;

#endif 	/* __ICoreWebView2ExperimentalEnvironment6_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment8_FWD_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment8_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalEnvironment8 ICoreWebView2ExperimentalEnvironment8;

#endif 	/* __ICoreWebView2ExperimentalEnvironment8_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFrame3_FWD_DEFINED__
#define __ICoreWebView2ExperimentalFrame3_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalFrame3 ICoreWebView2ExperimentalFrame3;

#endif 	/* __ICoreWebView2ExperimentalFrame3_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalFramePermissionRequestedEventHandler ICoreWebView2ExperimentalFramePermissionRequestedEventHandler;

#endif 	/* __ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2PermissionRequestedEventArgs_FWD_DEFINED__
#define __ICoreWebView2PermissionRequestedEventArgs_FWD_DEFINED__
typedef interface ICoreWebView2PermissionRequestedEventArgs ICoreWebView2PermissionRequestedEventArgs;

#endif 	/* __ICoreWebView2PermissionRequestedEventArgs_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalPermissionRequestedEventArgs_FWD_DEFINED__
#define __ICoreWebView2ExperimentalPermissionRequestedEventArgs_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalPermissionRequestedEventArgs ICoreWebView2ExperimentalPermissionRequestedEventArgs;

#endif 	/* __ICoreWebView2ExperimentalPermissionRequestedEventArgs_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile_FWD_DEFINED__
#define __ICoreWebView2ExperimentalProfile_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalProfile ICoreWebView2ExperimentalProfile;

#endif 	/* __ICoreWebView2ExperimentalProfile_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile2_FWD_DEFINED__
#define __ICoreWebView2ExperimentalProfile2_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalProfile2 ICoreWebView2ExperimentalProfile2;

#endif 	/* __ICoreWebView2ExperimentalProfile2_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile3_FWD_DEFINED__
#define __ICoreWebView2ExperimentalProfile3_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalProfile3 ICoreWebView2ExperimentalProfile3;

#endif 	/* __ICoreWebView2ExperimentalProfile3_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile4_FWD_DEFINED__
#define __ICoreWebView2ExperimentalProfile4_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalProfile4 ICoreWebView2ExperimentalProfile4;

#endif 	/* __ICoreWebView2ExperimentalProfile4_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_FWD_DEFINED__
#define __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler;

#endif 	/* __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalUpdateRuntimeResult_FWD_DEFINED__
#define __ICoreWebView2ExperimentalUpdateRuntimeResult_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalUpdateRuntimeResult ICoreWebView2ExperimentalUpdateRuntimeResult;

#endif 	/* __ICoreWebView2ExperimentalUpdateRuntimeResult_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalSettings6_FWD_DEFINED__
#define __ICoreWebView2ExperimentalSettings6_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalSettings6 ICoreWebView2ExperimentalSettings6;

#endif 	/* __ICoreWebView2ExperimentalSettings6_FWD_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironmentOptions_FWD_DEFINED__
#define __ICoreWebView2ExperimentalEnvironmentOptions_FWD_DEFINED__
typedef interface ICoreWebView2ExperimentalEnvironmentOptions ICoreWebView2ExperimentalEnvironmentOptions;

#endif 	/* __ICoreWebView2ExperimentalEnvironmentOptions_FWD_DEFINED__ */


/* header files for imported files */
#include "webview2.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __WebView2Experimental_LIBRARY_DEFINED__
#define __WebView2Experimental_LIBRARY_DEFINED__

/* library WebView2Experimental */
/* [version][uuid] */ 




























typedef struct COREWEBVIEW2_MATRIX_4X4
    {
    FLOAT _11;
    FLOAT _12;
    FLOAT _13;
    FLOAT _14;
    FLOAT _21;
    FLOAT _22;
    FLOAT _23;
    FLOAT _24;
    FLOAT _31;
    FLOAT _32;
    FLOAT _33;
    FLOAT _34;
    FLOAT _41;
    FLOAT _42;
    FLOAT _43;
    FLOAT _44;
    } 	COREWEBVIEW2_MATRIX_4X4;

typedef /* [v1_enum] */ 
enum COREWEBVIEW2_UPDATE_RUNTIME_STATUS
    {
        COREWEBVIEW2_UPDATE_RUNTIME_STATUS_LATEST_VERSION_INSTALLED	= 0,
        COREWEBVIEW2_UPDATE_RUNTIME_STATUS_UPDATE_ALREADY_RUNNING	= ( COREWEBVIEW2_UPDATE_RUNTIME_STATUS_LATEST_VERSION_INSTALLED + 1 ) ,
        COREWEBVIEW2_UPDATE_RUNTIME_STATUS_BLOCKED_BY_POLICY	= ( COREWEBVIEW2_UPDATE_RUNTIME_STATUS_UPDATE_ALREADY_RUNNING + 1 ) ,
        COREWEBVIEW2_UPDATE_RUNTIME_STATUS_FAILED	= ( COREWEBVIEW2_UPDATE_RUNTIME_STATUS_BLOCKED_BY_POLICY + 1 ) 
    } 	COREWEBVIEW2_UPDATE_RUNTIME_STATUS;

typedef /* [v1_enum] */ 
enum COREWEBVIEW2_PDF_TOOLBAR_ITEMS
    {
        COREWEBVIEW2_PDF_TOOLBAR_ITEMS_NONE	= 0,
        COREWEBVIEW2_PDF_TOOLBAR_ITEMS_SAVE	= 0x1,
        COREWEBVIEW2_PDF_TOOLBAR_ITEMS_PRINT	= 0x2,
        COREWEBVIEW2_PDF_TOOLBAR_ITEMS_SAVE_AS	= 0x4
    } 	COREWEBVIEW2_PDF_TOOLBAR_ITEMS;

DEFINE_ENUM_FLAG_OPERATORS(COREWEBVIEW2_PDF_TOOLBAR_ITEMS);
typedef /* [v1_enum] */ 
enum COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL
    {
        COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_NORMAL	= 0,
        COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_LOW	= ( COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL_NORMAL + 1 ) 
    } 	COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL;

typedef /* [v1_enum] */ 
enum COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND
    {
        COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_PAGE	= 0,
        COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_IMAGE	= ( COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_PAGE + 1 ) ,
        COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_SELECTED_TEXT	= ( COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_IMAGE + 1 ) ,
        COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_AUDIO	= ( COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_SELECTED_TEXT + 1 ) ,
        COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_VIDEO	= ( COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND_AUDIO + 1 ) 
    } 	COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND;

typedef /* [v1_enum] */ 
enum COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND
    {
        COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_COMMAND	= 0,
        COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_CHECK_BOX	= ( COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_COMMAND + 1 ) ,
        COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_RADIO	= ( COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_CHECK_BOX + 1 ) ,
        COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_SEPARATOR	= ( COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_RADIO + 1 ) ,
        COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_SUBMENU	= ( COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND_SEPARATOR + 1 ) 
    } 	COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND;

typedef /* [v1_enum] */ 
enum COREWEBVIEW2_PREFERRED_COLOR_SCHEME
    {
        COREWEBVIEW2_PREFERRED_COLOR_SCHEME_AUTO	= 0,
        COREWEBVIEW2_PREFERRED_COLOR_SCHEME_LIGHT	= ( COREWEBVIEW2_PREFERRED_COLOR_SCHEME_AUTO + 1 ) ,
        COREWEBVIEW2_PREFERRED_COLOR_SCHEME_DARK	= ( COREWEBVIEW2_PREFERRED_COLOR_SCHEME_LIGHT + 1 ) 
    } 	COREWEBVIEW2_PREFERRED_COLOR_SCHEME;

typedef /* [v1_enum] */ 
enum COREWEBVIEW2_BROWSING_DATA_KINDS
    {
        COREWEBVIEW2_BROWSING_DATA_KINDS_FILE_SYSTEMS	= ( 1 << 0 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_INDEXED_DB	= ( 1 << 1 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_LOCAL_STORAGE	= ( 1 << 2 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_WEB_SQL	= ( 1 << 3 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_CACHE_STORAGE	= ( 1 << 4 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_ALL_DOM_STORAGE	= ( 1 << 5 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_COOKIES	= ( 1 << 6 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_ALL_SITE	= ( 1 << 7 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_DISK_CACHE	= ( 1 << 8 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_DOWNLOAD_HISTORY	= ( 1 << 9 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_GENERAL_AUTOFILL	= ( 1 << 10 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_PASSWORD_AUTOSAVE	= ( 1 << 11 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_BROWSING_HISTORY	= ( 1 << 12 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_SETTINGS	= ( 1 << 13 ) ,
        COREWEBVIEW2_BROWSING_DATA_KINDS_ALL_PROFILE	= ( 1 << 14 ) 
    } 	COREWEBVIEW2_BROWSING_DATA_KINDS;

DEFINE_ENUM_FLAG_OPERATORS(COREWEBVIEW2_BROWSING_DATA_KINDS);

EXTERN_C const IID LIBID_WebView2Experimental;

#ifndef __ICoreWebView2Experimental5_INTERFACE_DEFINED__
#define __ICoreWebView2Experimental5_INTERFACE_DEFINED__

/* interface ICoreWebView2Experimental5 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2Experimental5 = {0xE05E04CA,0x7924,0x4C04,{0xA8,0x4C,0xA9,0x05,0x72,0xDB,0xA2,0x2A}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E05E04CA-7924-4C04-A84C-A90572DBA22A")
    ICoreWebView2Experimental5 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MemoryUsageTargetLevel( 
            /* [retval][out] */ COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL *level) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_MemoryUsageTargetLevel( 
            /* [in] */ COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL level) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2Experimental5Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2Experimental5 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2Experimental5 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2Experimental5 * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MemoryUsageTargetLevel )( 
            ICoreWebView2Experimental5 * This,
            /* [retval][out] */ COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL *level);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_MemoryUsageTargetLevel )( 
            ICoreWebView2Experimental5 * This,
            /* [in] */ COREWEBVIEW2_MEMORY_USAGE_TARGET_LEVEL level);
        
        END_INTERFACE
    } ICoreWebView2Experimental5Vtbl;

    interface ICoreWebView2Experimental5
    {
        CONST_VTBL struct ICoreWebView2Experimental5Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2Experimental5_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2Experimental5_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2Experimental5_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2Experimental5_get_MemoryUsageTargetLevel(This,level)	\
    ( (This)->lpVtbl -> get_MemoryUsageTargetLevel(This,level) ) 

#define ICoreWebView2Experimental5_put_MemoryUsageTargetLevel(This,level)	\
    ( (This)->lpVtbl -> put_MemoryUsageTargetLevel(This,level) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2Experimental5_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2Experimental6_INTERFACE_DEFINED__
#define __ICoreWebView2Experimental6_INTERFACE_DEFINED__

/* interface ICoreWebView2Experimental6 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2Experimental6 = {0x5da51469,0xdcfb,0x435e,{0x8a,0xbe,0xe7,0x16,0x31,0x2c,0x57,0x7e}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5da51469-dcfb-435e-8abe-e716312c577e")
    ICoreWebView2Experimental6 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE add_ContextMenuRequested( 
            /* [in] */ ICoreWebView2ExperimentalContextMenuRequestedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_ContextMenuRequested( 
            /* [in] */ EventRegistrationToken token) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2Experimental6Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2Experimental6 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2Experimental6 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2Experimental6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *add_ContextMenuRequested )( 
            ICoreWebView2Experimental6 * This,
            /* [in] */ ICoreWebView2ExperimentalContextMenuRequestedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        HRESULT ( STDMETHODCALLTYPE *remove_ContextMenuRequested )( 
            ICoreWebView2Experimental6 * This,
            /* [in] */ EventRegistrationToken token);
        
        END_INTERFACE
    } ICoreWebView2Experimental6Vtbl;

    interface ICoreWebView2Experimental6
    {
        CONST_VTBL struct ICoreWebView2Experimental6Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2Experimental6_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2Experimental6_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2Experimental6_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2Experimental6_add_ContextMenuRequested(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_ContextMenuRequested(This,eventHandler,token) ) 

#define ICoreWebView2Experimental6_remove_ContextMenuRequested(This,token)	\
    ( (This)->lpVtbl -> remove_ContextMenuRequested(This,token) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2Experimental6_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2Experimental8_INTERFACE_DEFINED__
#define __ICoreWebView2Experimental8_INTERFACE_DEFINED__

/* interface ICoreWebView2Experimental8 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2Experimental8 = {0xb753b024,0x05c5,0x40ac,{0xa3,0xf4,0x70,0x1c,0x6f,0x74,0x91,0xf5}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b753b024-05c5-40ac-a3f4-701c6f7491f5")
    ICoreWebView2Experimental8 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Profile( 
            /* [retval][out] */ ICoreWebView2ExperimentalProfile **value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2Experimental8Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2Experimental8 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2Experimental8 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2Experimental8 * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Profile )( 
            ICoreWebView2Experimental8 * This,
            /* [retval][out] */ ICoreWebView2ExperimentalProfile **value);
        
        END_INTERFACE
    } ICoreWebView2Experimental8Vtbl;

    interface ICoreWebView2Experimental8
    {
        CONST_VTBL struct ICoreWebView2Experimental8Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2Experimental8_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2Experimental8_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2Experimental8_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2Experimental8_get_Profile(This,value)	\
    ( (This)->lpVtbl -> get_Profile(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2Experimental8_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler = {0xe9710a06,0x1d1d,0x49b2,{0x82,0x34,0x22,0x6f,0x35,0x84,0x6a,0xe5}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e9710a06-1d1d-49b2-8234-226f35846ae5")
    ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ HRESULT errorCode) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalClearBrowsingDataCompletedHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler * This,
            /* [in] */ HRESULT errorCode);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalClearBrowsingDataCompletedHandlerVtbl;

    interface ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalClearBrowsingDataCompletedHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_Invoke(This,errorCode)	\
    ( (This)->lpVtbl -> Invoke(This,errorCode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionController3_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalCompositionController3_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalCompositionController3 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalCompositionController3 = {0xb134916b,0xa104,0x4d2a,{0x95,0x67,0xc2,0xfd,0x53,0x71,0x43,0x50}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b134916b-a104-4d2a-9567-c2fd53714350")
    ICoreWebView2ExperimentalCompositionController3 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DragEnter( 
            /* [in] */ IDataObject *dataObject,
            /* [in] */ DWORD keyState,
            /* [in] */ POINT point,
            /* [retval][out] */ DWORD *effect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DragLeave( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DragOver( 
            /* [in] */ DWORD keyState,
            /* [in] */ POINT point,
            /* [retval][out] */ DWORD *effect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Drop( 
            /* [in] */ IDataObject *dataObject,
            /* [in] */ DWORD keyState,
            /* [in] */ POINT point,
            /* [retval][out] */ DWORD *effect) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalCompositionController3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalCompositionController3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalCompositionController3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalCompositionController3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DragEnter )( 
            ICoreWebView2ExperimentalCompositionController3 * This,
            /* [in] */ IDataObject *dataObject,
            /* [in] */ DWORD keyState,
            /* [in] */ POINT point,
            /* [retval][out] */ DWORD *effect);
        
        HRESULT ( STDMETHODCALLTYPE *DragLeave )( 
            ICoreWebView2ExperimentalCompositionController3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DragOver )( 
            ICoreWebView2ExperimentalCompositionController3 * This,
            /* [in] */ DWORD keyState,
            /* [in] */ POINT point,
            /* [retval][out] */ DWORD *effect);
        
        HRESULT ( STDMETHODCALLTYPE *Drop )( 
            ICoreWebView2ExperimentalCompositionController3 * This,
            /* [in] */ IDataObject *dataObject,
            /* [in] */ DWORD keyState,
            /* [in] */ POINT point,
            /* [retval][out] */ DWORD *effect);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalCompositionController3Vtbl;

    interface ICoreWebView2ExperimentalCompositionController3
    {
        CONST_VTBL struct ICoreWebView2ExperimentalCompositionController3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalCompositionController3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalCompositionController3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalCompositionController3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalCompositionController3_DragEnter(This,dataObject,keyState,point,effect)	\
    ( (This)->lpVtbl -> DragEnter(This,dataObject,keyState,point,effect) ) 

#define ICoreWebView2ExperimentalCompositionController3_DragLeave(This)	\
    ( (This)->lpVtbl -> DragLeave(This) ) 

#define ICoreWebView2ExperimentalCompositionController3_DragOver(This,keyState,point,effect)	\
    ( (This)->lpVtbl -> DragOver(This,keyState,point,effect) ) 

#define ICoreWebView2ExperimentalCompositionController3_Drop(This,dataObject,keyState,point,effect)	\
    ( (This)->lpVtbl -> Drop(This,dataObject,keyState,point,effect) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalCompositionController3_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCompositionController4_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalCompositionController4_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalCompositionController4 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalCompositionController4 = {0xe6041d7f,0x18ac,0x4654,{0xa0,0x4e,0x8b,0x3f,0x81,0x25,0x1c,0x33}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e6041d7f-18ac-4654-a04e-8b3f81251c33")
    ICoreWebView2ExperimentalCompositionController4 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UIAProvider( 
            /* [retval][out] */ IUnknown **provider) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCoreWebView2PointerInfoFromPointerId( 
            /* [in] */ UINT pointerId,
            /* [in] */ HWND parentWindow,
            /* [in] */ struct COREWEBVIEW2_MATRIX_4X4 transform,
            /* [retval][out] */ ICoreWebView2PointerInfo **pointerInfo) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalCompositionController4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalCompositionController4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalCompositionController4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalCompositionController4 * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_UIAProvider )( 
            ICoreWebView2ExperimentalCompositionController4 * This,
            /* [retval][out] */ IUnknown **provider);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCoreWebView2PointerInfoFromPointerId )( 
            ICoreWebView2ExperimentalCompositionController4 * This,
            /* [in] */ UINT pointerId,
            /* [in] */ HWND parentWindow,
            /* [in] */ struct COREWEBVIEW2_MATRIX_4X4 transform,
            /* [retval][out] */ ICoreWebView2PointerInfo **pointerInfo);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalCompositionController4Vtbl;

    interface ICoreWebView2ExperimentalCompositionController4
    {
        CONST_VTBL struct ICoreWebView2ExperimentalCompositionController4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalCompositionController4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalCompositionController4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalCompositionController4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalCompositionController4_get_UIAProvider(This,provider)	\
    ( (This)->lpVtbl -> get_UIAProvider(This,provider) ) 

#define ICoreWebView2ExperimentalCompositionController4_CreateCoreWebView2PointerInfoFromPointerId(This,pointerId,parentWindow,transform,pointerInfo)	\
    ( (This)->lpVtbl -> CreateCoreWebView2PointerInfoFromPointerId(This,pointerId,parentWindow,transform,pointerInfo) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalCompositionController4_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuItem_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuItem_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalContextMenuItem */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalContextMenuItem = {0x7aed49e3,0xa93f,0x497a,{0x81,0x1c,0x74,0x9c,0x6b,0x6b,0x6c,0x65}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7aed49e3-a93f-497a-811c-749c6b6b6c65")
    ICoreWebView2ExperimentalContextMenuItem : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Label( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CommandId( 
            /* [retval][out] */ INT32 *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ShortcutKeyDescription( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Icon( 
            /* [retval][out] */ IStream **value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Kind( 
            /* [retval][out] */ COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_IsEnabled( 
            /* [in] */ BOOL value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsEnabled( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_IsChecked( 
            /* [in] */ BOOL value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsChecked( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Children( 
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuItemCollection **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE add_CustomItemSelected( 
            /* [in] */ ICoreWebView2ExperimentalCustomItemSelectedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_CustomItemSelected( 
            /* [in] */ EventRegistrationToken token) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalContextMenuItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalContextMenuItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalContextMenuItem * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Label )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_CommandId )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [retval][out] */ INT32 *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ShortcutKeyDescription )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Icon )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [retval][out] */ IStream **value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [retval][out] */ COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsEnabled )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [in] */ BOOL value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsEnabled )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsChecked )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [in] */ BOOL value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsChecked )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Children )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuItemCollection **value);
        
        HRESULT ( STDMETHODCALLTYPE *add_CustomItemSelected )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [in] */ ICoreWebView2ExperimentalCustomItemSelectedEventHandler *eventHandler,
            /* [out] */ EventRegistrationToken *token);
        
        HRESULT ( STDMETHODCALLTYPE *remove_CustomItemSelected )( 
            ICoreWebView2ExperimentalContextMenuItem * This,
            /* [in] */ EventRegistrationToken token);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalContextMenuItemVtbl;

    interface ICoreWebView2ExperimentalContextMenuItem
    {
        CONST_VTBL struct ICoreWebView2ExperimentalContextMenuItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalContextMenuItem_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalContextMenuItem_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalContextMenuItem_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalContextMenuItem_get_Name(This,value)	\
    ( (This)->lpVtbl -> get_Name(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_get_Label(This,value)	\
    ( (This)->lpVtbl -> get_Label(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_get_CommandId(This,value)	\
    ( (This)->lpVtbl -> get_CommandId(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_get_ShortcutKeyDescription(This,value)	\
    ( (This)->lpVtbl -> get_ShortcutKeyDescription(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_get_Icon(This,value)	\
    ( (This)->lpVtbl -> get_Icon(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_get_Kind(This,value)	\
    ( (This)->lpVtbl -> get_Kind(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_put_IsEnabled(This,value)	\
    ( (This)->lpVtbl -> put_IsEnabled(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_get_IsEnabled(This,value)	\
    ( (This)->lpVtbl -> get_IsEnabled(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_put_IsChecked(This,value)	\
    ( (This)->lpVtbl -> put_IsChecked(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_get_IsChecked(This,value)	\
    ( (This)->lpVtbl -> get_IsChecked(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_get_Children(This,value)	\
    ( (This)->lpVtbl -> get_Children(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItem_add_CustomItemSelected(This,eventHandler,token)	\
    ( (This)->lpVtbl -> add_CustomItemSelected(This,eventHandler,token) ) 

#define ICoreWebView2ExperimentalContextMenuItem_remove_CustomItemSelected(This,token)	\
    ( (This)->lpVtbl -> remove_CustomItemSelected(This,token) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalContextMenuItem_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuItemCollection_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuItemCollection_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalContextMenuItemCollection */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalContextMenuItemCollection = {0xf562a2f5,0xc415,0x45cf,{0xb9,0x09,0xd4,0xb7,0xc1,0xe2,0x76,0xd3}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f562a2f5-c415-45cf-b909-d4b7c1e276d3")
    ICoreWebView2ExperimentalContextMenuItemCollection : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ UINT32 *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValueAtIndex( 
            /* [in] */ UINT32 index,
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuItem **value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveValueAtIndex( 
            /* [in] */ UINT32 index) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertValueAtIndex( 
            /* [in] */ UINT32 index,
            /* [in] */ ICoreWebView2ExperimentalContextMenuItem *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalContextMenuItemCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalContextMenuItemCollection * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalContextMenuItemCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalContextMenuItemCollection * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ICoreWebView2ExperimentalContextMenuItemCollection * This,
            /* [retval][out] */ UINT32 *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetValueAtIndex )( 
            ICoreWebView2ExperimentalContextMenuItemCollection * This,
            /* [in] */ UINT32 index,
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuItem **value);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveValueAtIndex )( 
            ICoreWebView2ExperimentalContextMenuItemCollection * This,
            /* [in] */ UINT32 index);
        
        HRESULT ( STDMETHODCALLTYPE *InsertValueAtIndex )( 
            ICoreWebView2ExperimentalContextMenuItemCollection * This,
            /* [in] */ UINT32 index,
            /* [in] */ ICoreWebView2ExperimentalContextMenuItem *value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalContextMenuItemCollectionVtbl;

    interface ICoreWebView2ExperimentalContextMenuItemCollection
    {
        CONST_VTBL struct ICoreWebView2ExperimentalContextMenuItemCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalContextMenuItemCollection_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalContextMenuItemCollection_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalContextMenuItemCollection_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalContextMenuItemCollection_get_Count(This,value)	\
    ( (This)->lpVtbl -> get_Count(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuItemCollection_GetValueAtIndex(This,index,value)	\
    ( (This)->lpVtbl -> GetValueAtIndex(This,index,value) ) 

#define ICoreWebView2ExperimentalContextMenuItemCollection_RemoveValueAtIndex(This,index)	\
    ( (This)->lpVtbl -> RemoveValueAtIndex(This,index) ) 

#define ICoreWebView2ExperimentalContextMenuItemCollection_InsertValueAtIndex(This,index,value)	\
    ( (This)->lpVtbl -> InsertValueAtIndex(This,index,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalContextMenuItemCollection_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuRequestedEventArgs_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuRequestedEventArgs_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalContextMenuRequestedEventArgs */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalContextMenuRequestedEventArgs = {0xa1d309ee,0xc03f,0x11eb,{0x85,0x29,0x02,0x42,0xac,0x13,0x00,0x03}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a1d309ee-c03f-11eb-8529-0242ac130003")
    ICoreWebView2ExperimentalContextMenuRequestedEventArgs : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_MenuItems( 
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuItemCollection **value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ContextMenuTarget( 
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuTarget **value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ POINT *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_SelectedCommandId( 
            /* [in] */ INT32 value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SelectedCommandId( 
            /* [retval][out] */ INT32 *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Handled( 
            /* [in] */ BOOL value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Handled( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeferral( 
            /* [retval][out] */ ICoreWebView2Deferral **deferral) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalContextMenuRequestedEventArgsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_MenuItems )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This,
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuItemCollection **value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ContextMenuTarget )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This,
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuTarget **value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Location )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This,
            /* [retval][out] */ POINT *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_SelectedCommandId )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This,
            /* [in] */ INT32 value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SelectedCommandId )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This,
            /* [retval][out] */ INT32 *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Handled )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This,
            /* [in] */ BOOL value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handled )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This,
            /* [retval][out] */ BOOL *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetDeferral )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventArgs * This,
            /* [retval][out] */ ICoreWebView2Deferral **deferral);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalContextMenuRequestedEventArgsVtbl;

    interface ICoreWebView2ExperimentalContextMenuRequestedEventArgs
    {
        CONST_VTBL struct ICoreWebView2ExperimentalContextMenuRequestedEventArgsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_get_MenuItems(This,value)	\
    ( (This)->lpVtbl -> get_MenuItems(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_get_ContextMenuTarget(This,value)	\
    ( (This)->lpVtbl -> get_ContextMenuTarget(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_get_Location(This,value)	\
    ( (This)->lpVtbl -> get_Location(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_put_SelectedCommandId(This,value)	\
    ( (This)->lpVtbl -> put_SelectedCommandId(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_get_SelectedCommandId(This,value)	\
    ( (This)->lpVtbl -> get_SelectedCommandId(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_put_Handled(This,value)	\
    ( (This)->lpVtbl -> put_Handled(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_get_Handled(This,value)	\
    ( (This)->lpVtbl -> get_Handled(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventArgs_GetDeferral(This,deferral)	\
    ( (This)->lpVtbl -> GetDeferral(This,deferral) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalContextMenuRequestedEventArgs_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuRequestedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuRequestedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalContextMenuRequestedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalContextMenuRequestedEventHandler = {0x04d3fe1d,0xab87,0x42fb,{0xa8,0x98,0xda,0x24,0x1d,0x35,0xb6,0x3c}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("04d3fe1d-ab87-42fb-a898-da241d35b63c")
    ICoreWebView2ExperimentalContextMenuRequestedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2 *sender,
            /* [in] */ ICoreWebView2ExperimentalContextMenuRequestedEventArgs *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalContextMenuRequestedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalContextMenuRequestedEventHandler * This,
            /* [in] */ ICoreWebView2 *sender,
            /* [in] */ ICoreWebView2ExperimentalContextMenuRequestedEventArgs *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalContextMenuRequestedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalContextMenuRequestedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalContextMenuRequestedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalContextMenuRequestedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalContextMenuRequestedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalContextMenuRequestedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalContextMenuRequestedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalContextMenuTarget_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalContextMenuTarget_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalContextMenuTarget */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalContextMenuTarget = {0xb8611d99,0xeed6,0x4f3f,{0x90,0x2c,0xa1,0x98,0x50,0x2a,0xd4,0x72}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b8611d99-eed6-4f3f-902c-a198502ad472")
    ICoreWebView2ExperimentalContextMenuTarget : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Kind( 
            /* [retval][out] */ COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsEditable( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsRequestedForMainFrame( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PageUri( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FrameUri( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HasLinkUri( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LinkUri( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HasLinkText( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_LinkText( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HasSourceUri( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SourceUri( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HasSelection( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_SelectionText( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalContextMenuTargetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalContextMenuTarget * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalContextMenuTarget * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Kind )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsEditable )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsRequestedForMainFrame )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PageUri )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_FrameUri )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasLinkUri )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LinkUri )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasLinkText )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_LinkText )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasSourceUri )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SourceUri )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HasSelection )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_SelectionText )( 
            ICoreWebView2ExperimentalContextMenuTarget * This,
            /* [retval][out] */ LPWSTR *value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalContextMenuTargetVtbl;

    interface ICoreWebView2ExperimentalContextMenuTarget
    {
        CONST_VTBL struct ICoreWebView2ExperimentalContextMenuTargetVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalContextMenuTarget_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalContextMenuTarget_get_Kind(This,value)	\
    ( (This)->lpVtbl -> get_Kind(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_IsEditable(This,value)	\
    ( (This)->lpVtbl -> get_IsEditable(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_IsRequestedForMainFrame(This,value)	\
    ( (This)->lpVtbl -> get_IsRequestedForMainFrame(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_PageUri(This,value)	\
    ( (This)->lpVtbl -> get_PageUri(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_FrameUri(This,value)	\
    ( (This)->lpVtbl -> get_FrameUri(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_HasLinkUri(This,value)	\
    ( (This)->lpVtbl -> get_HasLinkUri(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_LinkUri(This,value)	\
    ( (This)->lpVtbl -> get_LinkUri(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_HasLinkText(This,value)	\
    ( (This)->lpVtbl -> get_HasLinkText(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_LinkText(This,value)	\
    ( (This)->lpVtbl -> get_LinkText(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_HasSourceUri(This,value)	\
    ( (This)->lpVtbl -> get_HasSourceUri(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_SourceUri(This,value)	\
    ( (This)->lpVtbl -> get_SourceUri(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_HasSelection(This,value)	\
    ( (This)->lpVtbl -> get_HasSelection(This,value) ) 

#define ICoreWebView2ExperimentalContextMenuTarget_get_SelectionText(This,value)	\
    ( (This)->lpVtbl -> get_SelectionText(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalContextMenuTarget_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalControllerOptions_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalControllerOptions_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalControllerOptions */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalControllerOptions = {0x12aae616,0x8ccb,0x44ec,{0xbc,0xb3,0xeb,0x18,0x31,0x88,0x16,0x35}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("12aae616-8ccb-44ec-bcb3-eb1831881635")
    ICoreWebView2ExperimentalControllerOptions : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProfileName( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ProfileName( 
            /* [in] */ LPCWSTR value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsInPrivateModeEnabled( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_IsInPrivateModeEnabled( 
            /* [in] */ BOOL value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalControllerOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalControllerOptions * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalControllerOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalControllerOptions * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProfileName )( 
            ICoreWebView2ExperimentalControllerOptions * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProfileName )( 
            ICoreWebView2ExperimentalControllerOptions * This,
            /* [in] */ LPCWSTR value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsInPrivateModeEnabled )( 
            ICoreWebView2ExperimentalControllerOptions * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsInPrivateModeEnabled )( 
            ICoreWebView2ExperimentalControllerOptions * This,
            /* [in] */ BOOL value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalControllerOptionsVtbl;

    interface ICoreWebView2ExperimentalControllerOptions
    {
        CONST_VTBL struct ICoreWebView2ExperimentalControllerOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalControllerOptions_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalControllerOptions_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalControllerOptions_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalControllerOptions_get_ProfileName(This,value)	\
    ( (This)->lpVtbl -> get_ProfileName(This,value) ) 

#define ICoreWebView2ExperimentalControllerOptions_put_ProfileName(This,value)	\
    ( (This)->lpVtbl -> put_ProfileName(This,value) ) 

#define ICoreWebView2ExperimentalControllerOptions_get_IsInPrivateModeEnabled(This,value)	\
    ( (This)->lpVtbl -> get_IsInPrivateModeEnabled(This,value) ) 

#define ICoreWebView2ExperimentalControllerOptions_put_IsInPrivateModeEnabled(This,value)	\
    ( (This)->lpVtbl -> put_IsInPrivateModeEnabled(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalControllerOptions_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalCustomItemSelectedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalCustomItemSelectedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalCustomItemSelectedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalCustomItemSelectedEventHandler = {0x49e1d0bc,0xfe9e,0x4481,{0xb7,0xc2,0x32,0x32,0x4a,0xa2,0x19,0x98}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49e1d0bc-fe9e-4481-b7c2-32324aa21998")
    ICoreWebView2ExperimentalCustomItemSelectedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2ExperimentalContextMenuItem *sender,
            /* [in] */ IUnknown *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalCustomItemSelectedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalCustomItemSelectedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalCustomItemSelectedEventHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalCustomItemSelectedEventHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalCustomItemSelectedEventHandler * This,
            /* [in] */ ICoreWebView2ExperimentalContextMenuItem *sender,
            /* [in] */ IUnknown *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalCustomItemSelectedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalCustomItemSelectedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalCustomItemSelectedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalCustomItemSelectedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalCustomItemSelectedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalCustomItemSelectedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalCustomItemSelectedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalCustomItemSelectedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalController2_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalController2_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalController2 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalController2 = {0x320613e2,0x990f,0x4272,{0xbf,0x90,0xd2,0x43,0xa4,0xff,0x1b,0x8a}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("320613e2-990f-4272-bf90-d243a4ff1b8a")
    ICoreWebView2ExperimentalController2 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_AllowExternalDrop( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_AllowExternalDrop( 
            /* [in] */ BOOL value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalController2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalController2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalController2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalController2 * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowExternalDrop )( 
            ICoreWebView2ExperimentalController2 * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowExternalDrop )( 
            ICoreWebView2ExperimentalController2 * This,
            /* [in] */ BOOL value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalController2Vtbl;

    interface ICoreWebView2ExperimentalController2
    {
        CONST_VTBL struct ICoreWebView2ExperimentalController2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalController2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalController2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalController2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalController2_get_AllowExternalDrop(This,value)	\
    ( (This)->lpVtbl -> get_AllowExternalDrop(This,value) ) 

#define ICoreWebView2ExperimentalController2_put_AllowExternalDrop(This,value)	\
    ( (This)->lpVtbl -> put_AllowExternalDrop(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalController2_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment3_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment3_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalEnvironment3 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalEnvironment3 = {0x9A2BE885,0x7F0B,0x4B26,{0xB6,0xDD,0xC9,0x69,0xBA,0xA0,0x0B,0xF1}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9A2BE885-7F0B-4B26-B6DD-C969BAA00BF1")
    ICoreWebView2ExperimentalEnvironment3 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UpdateRuntime( 
            /* [in] */ ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler *handler) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalEnvironment3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalEnvironment3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalEnvironment3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalEnvironment3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateRuntime )( 
            ICoreWebView2ExperimentalEnvironment3 * This,
            /* [in] */ ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler *handler);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalEnvironment3Vtbl;

    interface ICoreWebView2ExperimentalEnvironment3
    {
        CONST_VTBL struct ICoreWebView2ExperimentalEnvironment3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalEnvironment3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalEnvironment3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalEnvironment3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalEnvironment3_UpdateRuntime(This,handler)	\
    ( (This)->lpVtbl -> UpdateRuntime(This,handler) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalEnvironment3_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment6_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment6_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalEnvironment6 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalEnvironment6 = {0x04d4fe1d,0xab87,0x42fb,{0xa8,0x98,0xda,0x24,0x1d,0x35,0xb6,0x3c}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("04d4fe1d-ab87-42fb-a898-da241d35b63c")
    ICoreWebView2ExperimentalEnvironment6 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateContextMenuItem( 
            /* [in] */ LPCWSTR label,
            /* [in] */ IStream *iconStream,
            /* [in] */ COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND kind,
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuItem **item) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalEnvironment6Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalEnvironment6 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalEnvironment6 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalEnvironment6 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateContextMenuItem )( 
            ICoreWebView2ExperimentalEnvironment6 * This,
            /* [in] */ LPCWSTR label,
            /* [in] */ IStream *iconStream,
            /* [in] */ COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND kind,
            /* [retval][out] */ ICoreWebView2ExperimentalContextMenuItem **item);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalEnvironment6Vtbl;

    interface ICoreWebView2ExperimentalEnvironment6
    {
        CONST_VTBL struct ICoreWebView2ExperimentalEnvironment6Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalEnvironment6_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalEnvironment6_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalEnvironment6_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalEnvironment6_CreateContextMenuItem(This,label,iconStream,kind,item)	\
    ( (This)->lpVtbl -> CreateContextMenuItem(This,label,iconStream,kind,item) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalEnvironment6_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironment8_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalEnvironment8_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalEnvironment8 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalEnvironment8 = {0xae714520,0xd062,0x4d20,{0x9d,0x08,0x50,0x96,0xf5,0x90,0xed,0x1a}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ae714520-d062-4d20-9d08-5096f590ed1a")
    ICoreWebView2ExperimentalEnvironment8 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateCoreWebView2ControllerOptions( 
            /* [in] */ LPCWSTR profileName,
            /* [in] */ BOOL isInPrivateModeEnabled,
            /* [retval][out] */ ICoreWebView2ExperimentalControllerOptions **options) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCoreWebView2ControllerWithOptions( 
            /* [in] */ HWND parentWindow,
            /* [in] */ ICoreWebView2ExperimentalControllerOptions *options,
            /* [in] */ ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCoreWebView2CompositionControllerWithOptions( 
            /* [in] */ HWND parentWindow,
            /* [in] */ ICoreWebView2ExperimentalControllerOptions *options,
            /* [in] */ ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler *handler) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalEnvironment8Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalEnvironment8 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalEnvironment8 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalEnvironment8 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCoreWebView2ControllerOptions )( 
            ICoreWebView2ExperimentalEnvironment8 * This,
            /* [in] */ LPCWSTR profileName,
            /* [in] */ BOOL isInPrivateModeEnabled,
            /* [retval][out] */ ICoreWebView2ExperimentalControllerOptions **options);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCoreWebView2ControllerWithOptions )( 
            ICoreWebView2ExperimentalEnvironment8 * This,
            /* [in] */ HWND parentWindow,
            /* [in] */ ICoreWebView2ExperimentalControllerOptions *options,
            /* [in] */ ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *handler);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCoreWebView2CompositionControllerWithOptions )( 
            ICoreWebView2ExperimentalEnvironment8 * This,
            /* [in] */ HWND parentWindow,
            /* [in] */ ICoreWebView2ExperimentalControllerOptions *options,
            /* [in] */ ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler *handler);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalEnvironment8Vtbl;

    interface ICoreWebView2ExperimentalEnvironment8
    {
        CONST_VTBL struct ICoreWebView2ExperimentalEnvironment8Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalEnvironment8_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalEnvironment8_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalEnvironment8_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalEnvironment8_CreateCoreWebView2ControllerOptions(This,profileName,isInPrivateModeEnabled,options)	\
    ( (This)->lpVtbl -> CreateCoreWebView2ControllerOptions(This,profileName,isInPrivateModeEnabled,options) ) 

#define ICoreWebView2ExperimentalEnvironment8_CreateCoreWebView2ControllerWithOptions(This,parentWindow,options,handler)	\
    ( (This)->lpVtbl -> CreateCoreWebView2ControllerWithOptions(This,parentWindow,options,handler) ) 

#define ICoreWebView2ExperimentalEnvironment8_CreateCoreWebView2CompositionControllerWithOptions(This,parentWindow,options,handler)	\
    ( (This)->lpVtbl -> CreateCoreWebView2CompositionControllerWithOptions(This,parentWindow,options,handler) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalEnvironment8_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFrame3_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalFrame3_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalFrame3 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalFrame3 = {0x6939fef6,0x01ab,0x4b24,{0xb6,0x39,0x7c,0x1b,0x82,0xbc,0x09,0x84}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6939fef6-01ab-4b24-b639-7c1b82bc0984")
    ICoreWebView2ExperimentalFrame3 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE add_PermissionRequested( 
            /* [in] */ ICoreWebView2ExperimentalFramePermissionRequestedEventHandler *handler,
            /* [out] */ EventRegistrationToken *token) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE remove_PermissionRequested( 
            /* [in] */ EventRegistrationToken token) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalFrame3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalFrame3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalFrame3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalFrame3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *add_PermissionRequested )( 
            ICoreWebView2ExperimentalFrame3 * This,
            /* [in] */ ICoreWebView2ExperimentalFramePermissionRequestedEventHandler *handler,
            /* [out] */ EventRegistrationToken *token);
        
        HRESULT ( STDMETHODCALLTYPE *remove_PermissionRequested )( 
            ICoreWebView2ExperimentalFrame3 * This,
            /* [in] */ EventRegistrationToken token);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalFrame3Vtbl;

    interface ICoreWebView2ExperimentalFrame3
    {
        CONST_VTBL struct ICoreWebView2ExperimentalFrame3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalFrame3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalFrame3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalFrame3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalFrame3_add_PermissionRequested(This,handler,token)	\
    ( (This)->lpVtbl -> add_PermissionRequested(This,handler,token) ) 

#define ICoreWebView2ExperimentalFrame3_remove_PermissionRequested(This,token)	\
    ( (This)->lpVtbl -> remove_PermissionRequested(This,token) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalFrame3_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalFramePermissionRequestedEventHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalFramePermissionRequestedEventHandler = {0x845d0edd,0x8bd8,0x429b,{0x99,0x15,0x48,0x21,0x78,0x9f,0x23,0xe9}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("845d0edd-8bd8-429b-9915-4821789f23e9")
    ICoreWebView2ExperimentalFramePermissionRequestedEventHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ ICoreWebView2Frame *sender,
            /* [in] */ ICoreWebView2ExperimentalPermissionRequestedEventArgs *args) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalFramePermissionRequestedEventHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalFramePermissionRequestedEventHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalFramePermissionRequestedEventHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalFramePermissionRequestedEventHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalFramePermissionRequestedEventHandler * This,
            /* [in] */ ICoreWebView2Frame *sender,
            /* [in] */ ICoreWebView2ExperimentalPermissionRequestedEventArgs *args);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalFramePermissionRequestedEventHandlerVtbl;

    interface ICoreWebView2ExperimentalFramePermissionRequestedEventHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalFramePermissionRequestedEventHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_Invoke(This,sender,args)	\
    ( (This)->lpVtbl -> Invoke(This,sender,args) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalFramePermissionRequestedEventHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalPermissionRequestedEventArgs_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalPermissionRequestedEventArgs_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalPermissionRequestedEventArgs */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalPermissionRequestedEventArgs = {0x74d7127f,0x9de6,0x4200,{0x87,0x34,0x42,0xd6,0xfb,0x4f,0xf7,0x41}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("74d7127f-9de6-4200-8734-42d6fb4ff741")
    ICoreWebView2ExperimentalPermissionRequestedEventArgs : public ICoreWebView2PermissionRequestedEventArgs
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Handled( 
            /* [retval][out] */ BOOL *handled) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Handled( 
            /* [in] */ BOOL handled) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalPermissionRequestedEventArgsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Uri )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This,
            /* [retval][out] */ LPWSTR *uri);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PermissionKind )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This,
            /* [retval][out] */ COREWEBVIEW2_PERMISSION_KIND *permissionKind);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsUserInitiated )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This,
            /* [retval][out] */ BOOL *isUserInitiated);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This,
            /* [retval][out] */ COREWEBVIEW2_PERMISSION_STATE *state);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_State )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This,
            /* [in] */ COREWEBVIEW2_PERMISSION_STATE state);
        
        HRESULT ( STDMETHODCALLTYPE *GetDeferral )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This,
            /* [retval][out] */ ICoreWebView2Deferral **deferral);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Handled )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This,
            /* [retval][out] */ BOOL *handled);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_Handled )( 
            ICoreWebView2ExperimentalPermissionRequestedEventArgs * This,
            /* [in] */ BOOL handled);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalPermissionRequestedEventArgsVtbl;

    interface ICoreWebView2ExperimentalPermissionRequestedEventArgs
    {
        CONST_VTBL struct ICoreWebView2ExperimentalPermissionRequestedEventArgsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_get_Uri(This,uri)	\
    ( (This)->lpVtbl -> get_Uri(This,uri) ) 

#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_get_PermissionKind(This,permissionKind)	\
    ( (This)->lpVtbl -> get_PermissionKind(This,permissionKind) ) 

#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_get_IsUserInitiated(This,isUserInitiated)	\
    ( (This)->lpVtbl -> get_IsUserInitiated(This,isUserInitiated) ) 

#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_get_State(This,state)	\
    ( (This)->lpVtbl -> get_State(This,state) ) 

#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_put_State(This,state)	\
    ( (This)->lpVtbl -> put_State(This,state) ) 

#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_GetDeferral(This,deferral)	\
    ( (This)->lpVtbl -> GetDeferral(This,deferral) ) 


#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_get_Handled(This,handled)	\
    ( (This)->lpVtbl -> get_Handled(This,handled) ) 

#define ICoreWebView2ExperimentalPermissionRequestedEventArgs_put_Handled(This,handled)	\
    ( (This)->lpVtbl -> put_Handled(This,handled) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalPermissionRequestedEventArgs_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2PermissionRequestedEventArgs_INTERFACE_DEFINED__
#define __ICoreWebView2PermissionRequestedEventArgs_INTERFACE_DEFINED__

/* interface ICoreWebView2PermissionRequestedEventArgs */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2PermissionRequestedEventArgs = {0x973ae2ef,0xff18,0x4894,{0x8f,0xb2,0x3c,0x75,0x8f,0x04,0x68,0x10}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("973ae2ef-ff18-4894-8fb2-3c758f046810")
    ICoreWebView2PermissionRequestedEventArgs : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Uri( 
            /* [retval][out] */ LPWSTR *uri) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PermissionKind( 
            /* [retval][out] */ COREWEBVIEW2_PERMISSION_KIND *permissionKind) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsUserInitiated( 
            /* [retval][out] */ BOOL *isUserInitiated) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ COREWEBVIEW2_PERMISSION_STATE *state) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_State( 
            /* [in] */ COREWEBVIEW2_PERMISSION_STATE state) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDeferral( 
            /* [retval][out] */ ICoreWebView2Deferral **deferral) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2PermissionRequestedEventArgsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2PermissionRequestedEventArgs * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2PermissionRequestedEventArgs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2PermissionRequestedEventArgs * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Uri )( 
            ICoreWebView2PermissionRequestedEventArgs * This,
            /* [retval][out] */ LPWSTR *uri);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PermissionKind )( 
            ICoreWebView2PermissionRequestedEventArgs * This,
            /* [retval][out] */ COREWEBVIEW2_PERMISSION_KIND *permissionKind);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsUserInitiated )( 
            ICoreWebView2PermissionRequestedEventArgs * This,
            /* [retval][out] */ BOOL *isUserInitiated);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            ICoreWebView2PermissionRequestedEventArgs * This,
            /* [retval][out] */ COREWEBVIEW2_PERMISSION_STATE *state);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_State )( 
            ICoreWebView2PermissionRequestedEventArgs * This,
            /* [in] */ COREWEBVIEW2_PERMISSION_STATE state);
        
        HRESULT ( STDMETHODCALLTYPE *GetDeferral )( 
            ICoreWebView2PermissionRequestedEventArgs * This,
            /* [retval][out] */ ICoreWebView2Deferral **deferral);
        
        END_INTERFACE
    } ICoreWebView2PermissionRequestedEventArgsVtbl;

    interface ICoreWebView2PermissionRequestedEventArgs
    {
        CONST_VTBL struct ICoreWebView2PermissionRequestedEventArgsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2PermissionRequestedEventArgs_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2PermissionRequestedEventArgs_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2PermissionRequestedEventArgs_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2PermissionRequestedEventArgs_get_Uri(This,uri)	\
    ( (This)->lpVtbl -> get_Uri(This,uri) ) 

#define ICoreWebView2PermissionRequestedEventArgs_get_PermissionKind(This,permissionKind)	\
    ( (This)->lpVtbl -> get_PermissionKind(This,permissionKind) ) 

#define ICoreWebView2PermissionRequestedEventArgs_get_IsUserInitiated(This,isUserInitiated)	\
    ( (This)->lpVtbl -> get_IsUserInitiated(This,isUserInitiated) ) 

#define ICoreWebView2PermissionRequestedEventArgs_get_State(This,state)	\
    ( (This)->lpVtbl -> get_State(This,state) ) 

#define ICoreWebView2PermissionRequestedEventArgs_put_State(This,state)	\
    ( (This)->lpVtbl -> put_State(This,state) ) 

#define ICoreWebView2PermissionRequestedEventArgs_GetDeferral(This,deferral)	\
    ( (This)->lpVtbl -> GetDeferral(This,deferral) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2PermissionRequestedEventArgs_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalProfile_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalProfile */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalProfile = {0x05bfce49,0x57e2,0x4963,{0xbb,0xb8,0x38,0x05,0xad,0xe3,0xc4,0x81}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("05bfce49-57e2-4963-bbb8-3805ade3c481")
    ICoreWebView2ExperimentalProfile : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProfileName( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsInPrivateModeEnabled( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ProfilePath( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalProfileVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalProfile * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalProfile * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalProfile * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProfileName )( 
            ICoreWebView2ExperimentalProfile * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsInPrivateModeEnabled )( 
            ICoreWebView2ExperimentalProfile * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProfilePath )( 
            ICoreWebView2ExperimentalProfile * This,
            /* [retval][out] */ LPWSTR *value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalProfileVtbl;

    interface ICoreWebView2ExperimentalProfile
    {
        CONST_VTBL struct ICoreWebView2ExperimentalProfileVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalProfile_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalProfile_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalProfile_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalProfile_get_ProfileName(This,value)	\
    ( (This)->lpVtbl -> get_ProfileName(This,value) ) 

#define ICoreWebView2ExperimentalProfile_get_IsInPrivateModeEnabled(This,value)	\
    ( (This)->lpVtbl -> get_IsInPrivateModeEnabled(This,value) ) 

#define ICoreWebView2ExperimentalProfile_get_ProfilePath(This,value)	\
    ( (This)->lpVtbl -> get_ProfilePath(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalProfile_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile2_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalProfile2_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalProfile2 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalProfile2 = {0xee9e58ed,0x807b,0x4648,{0x91,0x71,0x8c,0x09,0x22,0xef,0xe0,0x71}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ee9e58ed-807b-4648-9171-8c0922efe071")
    ICoreWebView2ExperimentalProfile2 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_PreferredColorScheme( 
            /* [retval][out] */ COREWEBVIEW2_PREFERRED_COLOR_SCHEME *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_PreferredColorScheme( 
            /* [in] */ COREWEBVIEW2_PREFERRED_COLOR_SCHEME value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalProfile2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalProfile2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalProfile2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalProfile2 * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_PreferredColorScheme )( 
            ICoreWebView2ExperimentalProfile2 * This,
            /* [retval][out] */ COREWEBVIEW2_PREFERRED_COLOR_SCHEME *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_PreferredColorScheme )( 
            ICoreWebView2ExperimentalProfile2 * This,
            /* [in] */ COREWEBVIEW2_PREFERRED_COLOR_SCHEME value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalProfile2Vtbl;

    interface ICoreWebView2ExperimentalProfile2
    {
        CONST_VTBL struct ICoreWebView2ExperimentalProfile2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalProfile2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalProfile2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalProfile2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalProfile2_get_PreferredColorScheme(This,value)	\
    ( (This)->lpVtbl -> get_PreferredColorScheme(This,value) ) 

#define ICoreWebView2ExperimentalProfile2_put_PreferredColorScheme(This,value)	\
    ( (This)->lpVtbl -> put_PreferredColorScheme(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalProfile2_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile3_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalProfile3_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalProfile3 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalProfile3 = {0x4e4d8937,0x7fc7,0x4d40,{0xa1,0x01,0xd3,0xc7,0x4b,0xbc,0x37,0x85}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4e4d8937-7fc7-4d40-a101-d3c74bbc3785")
    ICoreWebView2ExperimentalProfile3 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_DefaultDownloadFolderPath( 
            /* [retval][out] */ LPWSTR *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_DefaultDownloadFolderPath( 
            /* [in] */ LPCWSTR value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalProfile3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalProfile3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalProfile3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalProfile3 * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_DefaultDownloadFolderPath )( 
            ICoreWebView2ExperimentalProfile3 * This,
            /* [retval][out] */ LPWSTR *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_DefaultDownloadFolderPath )( 
            ICoreWebView2ExperimentalProfile3 * This,
            /* [in] */ LPCWSTR value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalProfile3Vtbl;

    interface ICoreWebView2ExperimentalProfile3
    {
        CONST_VTBL struct ICoreWebView2ExperimentalProfile3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalProfile3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalProfile3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalProfile3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalProfile3_get_DefaultDownloadFolderPath(This,value)	\
    ( (This)->lpVtbl -> get_DefaultDownloadFolderPath(This,value) ) 

#define ICoreWebView2ExperimentalProfile3_put_DefaultDownloadFolderPath(This,value)	\
    ( (This)->lpVtbl -> put_DefaultDownloadFolderPath(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalProfile3_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalProfile4_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalProfile4_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalProfile4 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalProfile4 = {0xee38b7e1,0x221d,0x465d,{0x86,0x4b,0xea,0x4c,0x22,0x19,0x4d,0xca}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ee38b7e1-221d-465d-864b-ea4c22194dca")
    ICoreWebView2ExperimentalProfile4 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ClearBrowsingData( 
            /* [in] */ COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds,
            /* [in] */ ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler *handler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearBrowsingDataInTimeRange( 
            /* [in] */ COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds,
            /* [in] */ double startTime,
            /* [in] */ double endTime,
            /* [in] */ ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler *handler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearBrowsingDataAll( 
            /* [in] */ ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler *handler) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalProfile4Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalProfile4 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalProfile4 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalProfile4 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ClearBrowsingData )( 
            ICoreWebView2ExperimentalProfile4 * This,
            /* [in] */ COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds,
            /* [in] */ ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler *handler);
        
        HRESULT ( STDMETHODCALLTYPE *ClearBrowsingDataInTimeRange )( 
            ICoreWebView2ExperimentalProfile4 * This,
            /* [in] */ COREWEBVIEW2_BROWSING_DATA_KINDS dataKinds,
            /* [in] */ double startTime,
            /* [in] */ double endTime,
            /* [in] */ ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler *handler);
        
        HRESULT ( STDMETHODCALLTYPE *ClearBrowsingDataAll )( 
            ICoreWebView2ExperimentalProfile4 * This,
            /* [in] */ ICoreWebView2ExperimentalClearBrowsingDataCompletedHandler *handler);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalProfile4Vtbl;

    interface ICoreWebView2ExperimentalProfile4
    {
        CONST_VTBL struct ICoreWebView2ExperimentalProfile4Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalProfile4_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalProfile4_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalProfile4_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalProfile4_ClearBrowsingData(This,dataKinds,handler)	\
    ( (This)->lpVtbl -> ClearBrowsingData(This,dataKinds,handler) ) 

#define ICoreWebView2ExperimentalProfile4_ClearBrowsingDataInTimeRange(This,dataKinds,startTime,endTime,handler)	\
    ( (This)->lpVtbl -> ClearBrowsingDataInTimeRange(This,dataKinds,startTime,endTime,handler) ) 

#define ICoreWebView2ExperimentalProfile4_ClearBrowsingDataAll(This,handler)	\
    ( (This)->lpVtbl -> ClearBrowsingDataAll(This,handler) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalProfile4_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler = {0xF1D2D722,0x3721,0x499C,{0x87,0xF5,0x4C,0x40,0x52,0x60,0x69,0x7A}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F1D2D722-3721-499C-87F5-4C405260697A")
    ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ HRESULT errorCode,
            /* [in] */ ICoreWebView2ExperimentalUpdateRuntimeResult *result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalUpdateRuntimeCompletedHandlerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler * This);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler * This,
            /* [in] */ HRESULT errorCode,
            /* [in] */ ICoreWebView2ExperimentalUpdateRuntimeResult *result);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalUpdateRuntimeCompletedHandlerVtbl;

    interface ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler
    {
        CONST_VTBL struct ICoreWebView2ExperimentalUpdateRuntimeCompletedHandlerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_Invoke(This,errorCode,result)	\
    ( (This)->lpVtbl -> Invoke(This,errorCode,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalUpdateRuntimeCompletedHandler_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalUpdateRuntimeResult_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalUpdateRuntimeResult_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalUpdateRuntimeResult */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalUpdateRuntimeResult = {0xDD503E49,0xAB19,0x47C0,{0xB2,0xAD,0x6D,0xDD,0x09,0xCC,0x3E,0x3A}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DD503E49-AB19-47C0-B2AD-6DDD09CC3E3A")
    ICoreWebView2ExperimentalUpdateRuntimeResult : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ COREWEBVIEW2_UPDATE_RUNTIME_STATUS *status) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ExtendedError( 
            /* [retval][out] */ HRESULT *error) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalUpdateRuntimeResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This,
            /* [retval][out] */ COREWEBVIEW2_UPDATE_RUNTIME_STATUS *status);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedError )( 
            ICoreWebView2ExperimentalUpdateRuntimeResult * This,
            /* [retval][out] */ HRESULT *error);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalUpdateRuntimeResultVtbl;

    interface ICoreWebView2ExperimentalUpdateRuntimeResult
    {
        CONST_VTBL struct ICoreWebView2ExperimentalUpdateRuntimeResultVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalUpdateRuntimeResult_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeResult_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeResult_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalUpdateRuntimeResult_get_Status(This,status)	\
    ( (This)->lpVtbl -> get_Status(This,status) ) 

#define ICoreWebView2ExperimentalUpdateRuntimeResult_get_ExtendedError(This,error)	\
    ( (This)->lpVtbl -> get_ExtendedError(This,error) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalUpdateRuntimeResult_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalSettings6_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalSettings6_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalSettings6 */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalSettings6 = {0xd98de66d,0x46d7,0x4090,{0x91,0xe1,0xed,0xf8,0xe6,0x6f,0xdb,0x7b}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d98de66d-46d7-4090-91e1-edf8e66fdb7b")
    ICoreWebView2ExperimentalSettings6 : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_HiddenPdfToolbarItems( 
            /* [retval][out] */ COREWEBVIEW2_PDF_TOOLBAR_ITEMS *hidden_pdf_toolbar_items) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_HiddenPdfToolbarItems( 
            /* [in] */ COREWEBVIEW2_PDF_TOOLBAR_ITEMS hidden_pdf_toolbar_items) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalSettings6Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalSettings6 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalSettings6 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalSettings6 * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_HiddenPdfToolbarItems )( 
            ICoreWebView2ExperimentalSettings6 * This,
            /* [retval][out] */ COREWEBVIEW2_PDF_TOOLBAR_ITEMS *hidden_pdf_toolbar_items);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_HiddenPdfToolbarItems )( 
            ICoreWebView2ExperimentalSettings6 * This,
            /* [in] */ COREWEBVIEW2_PDF_TOOLBAR_ITEMS hidden_pdf_toolbar_items);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalSettings6Vtbl;

    interface ICoreWebView2ExperimentalSettings6
    {
        CONST_VTBL struct ICoreWebView2ExperimentalSettings6Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalSettings6_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalSettings6_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalSettings6_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalSettings6_get_HiddenPdfToolbarItems(This,hidden_pdf_toolbar_items)	\
    ( (This)->lpVtbl -> get_HiddenPdfToolbarItems(This,hidden_pdf_toolbar_items) ) 

#define ICoreWebView2ExperimentalSettings6_put_HiddenPdfToolbarItems(This,hidden_pdf_toolbar_items)	\
    ( (This)->lpVtbl -> put_HiddenPdfToolbarItems(This,hidden_pdf_toolbar_items) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalSettings6_INTERFACE_DEFINED__ */


#ifndef __ICoreWebView2ExperimentalEnvironmentOptions_INTERFACE_DEFINED__
#define __ICoreWebView2ExperimentalEnvironmentOptions_INTERFACE_DEFINED__

/* interface ICoreWebView2ExperimentalEnvironmentOptions */
/* [unique][object][uuid] */ 


EXTERN_C __declspec(selectany) const IID IID_ICoreWebView2ExperimentalEnvironmentOptions = {0xFF85C98A,0x1BA7,0x4A6B,{0x90,0xC8,0x2B,0x75,0x2C,0x89,0xE9,0xE2}};

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FF85C98A-1BA7-4A6B-90C8-2B752C89E9E2")
    ICoreWebView2ExperimentalEnvironmentOptions : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ExclusiveUserDataFolderAccess( 
            /* [retval][out] */ BOOL *value) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ExclusiveUserDataFolderAccess( 
            /* [in] */ BOOL value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICoreWebView2ExperimentalEnvironmentOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICoreWebView2ExperimentalEnvironmentOptions * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICoreWebView2ExperimentalEnvironmentOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICoreWebView2ExperimentalEnvironmentOptions * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExclusiveUserDataFolderAccess )( 
            ICoreWebView2ExperimentalEnvironmentOptions * This,
            /* [retval][out] */ BOOL *value);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_ExclusiveUserDataFolderAccess )( 
            ICoreWebView2ExperimentalEnvironmentOptions * This,
            /* [in] */ BOOL value);
        
        END_INTERFACE
    } ICoreWebView2ExperimentalEnvironmentOptionsVtbl;

    interface ICoreWebView2ExperimentalEnvironmentOptions
    {
        CONST_VTBL struct ICoreWebView2ExperimentalEnvironmentOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICoreWebView2ExperimentalEnvironmentOptions_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICoreWebView2ExperimentalEnvironmentOptions_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICoreWebView2ExperimentalEnvironmentOptions_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICoreWebView2ExperimentalEnvironmentOptions_get_ExclusiveUserDataFolderAccess(This,value)	\
    ( (This)->lpVtbl -> get_ExclusiveUserDataFolderAccess(This,value) ) 

#define ICoreWebView2ExperimentalEnvironmentOptions_put_ExclusiveUserDataFolderAccess(This,value)	\
    ( (This)->lpVtbl -> put_ExclusiveUserDataFolderAccess(This,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICoreWebView2ExperimentalEnvironmentOptions_INTERFACE_DEFINED__ */

#endif /* __WebView2Experimental_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


