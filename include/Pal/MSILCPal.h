//===--- include/Pal/MSILCPal.h ---------------------------------*- C++ -*-===//
//
// LLVM-MSILC
//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
// See LICENSE file in the project root for full license information. 
//
//===----------------------------------------------------------------------===//
//
// Minimal PAL for non-Windows platforms.
//
//===----------------------------------------------------------------------===//

#ifndef MSILC_PAL
#define MSILC_PAL

// We still need the PAL EH macros on Windows, so define them here.
#if defined(_MSC_VER)

#if defined(_DEBUG)
#include <windows.h> // For UINT
#endif
#include "staticcontract.h"

// Note: PAL_SEH_RESTORE_GUARD_PAGE is only ever defined in clrex.h, so we only
// restore guard pages automatically when these macros are used from within the
// VM.
#define PAL_SEH_RESTORE_GUARD_PAGE

#define PAL_TRY_NAKED                                                          \
    {                                                                          \
        bool __exHandled; __exHandled = false;                                 \
        DWORD __exCode; __exCode = 0;                                          \
        SCAN_EHMARKER();                                                       \
        __try                                                                  \
        {                                                                      \
            SCAN_EHMARKER_TRY();

#define PAL_EXCEPT_NAKED(Disposition)                                          \
        }                                                                      \
        __except(__exCode = GetExceptionCode(), Disposition)                   \
        {                                                                      \
            __exHandled = true;                                                \
            SCAN_EHMARKER_CATCH();                                             \
            PAL_SEH_RESTORE_GUARD_PAGE

#define PAL_EXCEPT_FILTER_NAKED(pfnFilter, param)                              \
        }                                                                      \
        __except(__exCode = GetExceptionCode(),                                \
                 pfnFilter(GetExceptionInformation(), param))                  \
        {                                                                      \
            __exHandled = true;                                                \
            SCAN_EHMARKER_CATCH();                                             \
            PAL_SEH_RESTORE_GUARD_PAGE

#define PAL_FINALLY_NAKED                                                      \
        }                                                                      \
        __finally                                                              \
        {                                                                      \

#define PAL_ENDTRY_NAKED                                                       \
        }                                                                      \
        PAL_ENDTRY_NAKED_DBG                                                   \
    }                                                                          \


#if defined(_DEBUG) && !defined(DACCESS_COMPILE)
//
// In debug mode, compile the try body as a method of a local class.
// This way, the compiler will check that the body is not directly
// accessing any local variables and arguments.
//
#define PAL_TRY(__ParamType, __paramDef, __paramRef)                           \
{                                                                              \
    __ParamType __param = __paramRef;                                          \
    __ParamType __paramToPassToFilter = __paramRef;                            \
    class __Body                                                               \
    {                                                                          \
    public:                                                                    \
        static void Run(__ParamType __paramDef)                                \
    {                                                                          \
        PAL_TRY_HANDLER_DBG_BEGIN
 
#define PAL_EXCEPT(Disposition)                                                \
            PAL_TRY_HANDLER_DBG_END                                            \
        }                                                                      \
    };                                                                         \
        PAL_TRY_NAKED                                                          \
    __Body::Run(__param);                                                      \
    PAL_EXCEPT_NAKED(Disposition)

#define PAL_EXCEPT_FILTER(pfnFilter)                                           \
            PAL_TRY_HANDLER_DBG_END                                            \
        }                                                                      \
    };                                                                         \
    PAL_TRY_NAKED                                                              \
    __Body::Run(__param);                                                      \
    PAL_EXCEPT_FILTER_NAKED(pfnFilter, __paramToPassToFilter)

#define PAL_FINALLY                                                            \
            PAL_TRY_HANDLER_DBG_END                                            \
        }                                                                      \
    };                                                                         \
    PAL_TRY_NAKED                                                              \
    __Body::Run(__param);                                                      \
    PAL_FINALLY_NAKED

#define PAL_ENDTRY                                                             \
    PAL_ENDTRY_NAKED                                                           \
}

#else // _DEBUG

#define PAL_TRY(__ParamType, __paramDef, __paramRef)                           \
{                                                                              \
    __ParamType __param = __paramRef;                                          \
    __ParamType __paramDef = __param;                                          \
    PAL_TRY_NAKED                                                              \
    PAL_TRY_HANDLER_DBG_BEGIN

#define PAL_EXCEPT(Disposition)                                                \
        PAL_TRY_HANDLER_DBG_END                                                \
        PAL_EXCEPT_NAKED(Disposition)

#define PAL_EXCEPT_FILTER(pfnFilter)                                           \
        PAL_TRY_HANDLER_DBG_END                                                \
        PAL_EXCEPT_FILTER_NAKED(pfnFilter, __param)

#define PAL_FINALLY                                                            \
        PAL_TRY_HANDLER_DBG_END                                                \
        PAL_FINALLY_NAKED

#define PAL_ENDTRY                                                             \
    PAL_ENDTRY_NAKED                                                           \
    }

#endif // _DEBUG

#define PAL_TRY_HANDLER_DBG_BEGIN                   ANNOTATION_TRY_BEGIN;
#define PAL_TRY_HANDLER_DBG_BEGIN_DLLMAIN(_reason)  ANNOTATION_TRY_BEGIN;
#define PAL_TRY_HANDLER_DBG_END                     ANNOTATION_TRY_END;
#define PAL_ENDTRY_NAKED_DBG                                                          

#else // defined(_MSC_VER)

// SAL
#define _In_
#define _Out_opt_
#define _Out_writes_to_opt_(size,count)
#define __out_ecount(count)
#define __inout_ecount(count)
#define __deref_inout_ecount(count)

#if defined(__cplusplus)
extern "C" {
#endif

// Linkage
#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif // __cplusplus

#define _ASSERTE assert
#define __assume(x) (void)0

#define UNALIGNED

#if !defined(_HOST_X86_)
#define __stdcall
#endif

#if defined(__GNUC__)
#define __cdecl __attribute__((cdecl))
#endif

#define PALIMPORT EXTERN_C
#define PALAPI __stdcall

#define _cdecl __cdecl

#define STDMETHODCALLTYPE __stdcall

#define STDAPICALLTYPE __stdcall

#define STDMETHOD(method)       virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method

#define STDAPI        EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(type) EXTERN_C type STDAPICALLTYPE

#define PURE = 0

#if defined(__GNUC__)
#define DECLSPEC_NOVTABLE
#define DECLSPEC_IMPORT
#define DECLSPEC_SELECTANY  __attribute__((weak))
#define SELECTANY extern __attribute__((weak))
#else
#define DECLSPEC_NOVTABLE
#define DECLSPEC_IMPORT
#define DECLSPEC_SELECTANY
#define SELECTANY
#endif

#if defined(__cplusplus)
}
#endif

#define _alloca alloca

// A bunch of source files (e.g. most of the ndp tree) include pal.h
// but are written to be LLP64, not LP64.  (LP64 => long = 64 bits
// LLP64 => longs = 32 bits, long long = 64 bits)
//
// To handle this difference, we #define long to be int (and thus 32 bits) when
// compiling those files.  (See the bottom of this file or search for
// #define long to see where we do this.)
//
// But this fix is more complicated than it seems, because we also use the
// preprocessor to #define __int64 to long for LP64 architectures (__int64
// isn't a builtin in gcc).   We don't want __int64 to be an int (by cascading
// macro rules).  So we play this little trick below where we add
// __cppmungestrip before "long", which is what we're really #defining __int64
// to.  The preprocessor sees __cppmungestriplong as something different than
// long, so it doesn't replace it with int.  The during the cppmunge phase, we
// remove the __cppmungestrip part, leaving long for the compiler to see.
//
// Note that we can't just use a typedef to define __int64 as long before
// #defining long because typedefed types can't be signedness-agnostic (i.e.
// they must be either signed or unsigned) and we want to be able to use
// __int64 as though it were intrinsic

#if defined(BIT64)
#define __int64     long
#else // _WIN64
#define __int64     long long
#endif // _WIN64

#define __int32     int
#define __int16     short int
#define __int8      char        // assumes char is signed

typedef void VOID, *PVOID, *LPVOID, *LPCVOID;

#if !defined(PLATFORM_UNIX)
typedef long LONG
typedef unsigned long ULONG;
typedef unsigned long DWORD
#else
typedef int LONG;           // NOTE: diff from windows.h, for LP64 compat
typedef unsigned int ULONG; // NOTE: diff from windows.h, for LP64 compat
typedef unsigned int DWORD; // NOTE: diff from windows.h, for LP64 compat
#endif

#if defined (PLATFORM_UNIX)
#if defined(__cplusplus)
typedef char16_t WCHAR;
#else
typedef unsigned short WCHAR;
#endif
#endif

typedef int BOOL;
typedef unsigned char BYTE, *PBYTE;
typedef char CHAR, *LPCSTR;
typedef unsigned char UCHAR;
typedef unsigned short WORD;
typedef short SHORT;
typedef unsigned short USHORT;
typedef WCHAR *LPWSTR, *LPCWSTR;
typedef int INT;
typedef unsigned int UINT;

typedef float FLOAT;
typedef double DOUBLE;

typedef BYTE BOOLEAN;

typedef signed __int32 INT32;
typedef unsigned __int32 UINT32;
typedef unsigned __int32 ULONG32;
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;
typedef unsigned __int64 ULONG64;
typedef ULONGLONG DWORD64, *PDWORD64;

#define _W64

#if _WIN64
typedef unsigned __int64 UINT_PTR;
typedef unsigned __int64 ULONG_PTR;
#else
typedef _W64 unsigned __int32 UINT_PTR;
typedef _W64 unsigned __int32 ULONG_PTR;
#endif

typedef ULONG_PTR SIZE_T;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// Handles
typedef VOID *HANDLE;
typedef HANDLE HINSTANCE;

// HRESULTs
typedef LONG HRESULT;

// diff from Win32
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((ULONG)(sev)<<31) | ((ULONG)(fac)<<16) | ((ULONG)(code))) )

#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)

#define S_OK _HRESULT_TYPEDEF_(0x00000000L)

#define FACILITY_NULL 0

#define NO_ERROR 0L

#define SEVERITY_ERROR 1

#define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
#define FAILED(Status) ((HRESULT)(Status)<0)


// EH

#define EXCEPTION_NONCONTINUABLE 0x1
#define EXCEPTION_UNWINDING 0x2

#define EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct _EXCEPTION_RECORD {
    DWORD ExceptionCode;
    DWORD ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    DWORD NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PVOID ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS, *LPEXCEPTION_POINTERS;

#define EXCEPTION_CONTINUE_SEARCH   0
#define EXCEPTION_EXECUTE_HANDLER   1
#define EXCEPTION_CONTINUE_EXECUTION -1

#define GetExceptionCode (DWORD)__exception_code
#define GetExceptionInformation (PEXCEPTION_POINTERS)__exception_info

#define METHOD_CANNOT_BE_FOLDED_DEBUG

#define ANNOTATION_TRY_BEGIN                { }
#define ANNOTATION_TRY_END                  { }
#define ANNOTATION_FN_THROWS                { }
#define ANNOTATION_FN_GC_NOTRIGGER          { }
#define ANNOTATION_FN_SO_TOLERANT           { }

#define STATIC_CONTRACT_THROWS              ANNOTATION_FN_THROWS
#define STATIC_CONTRACT_GC_NOTRIGGER        ANNOTATION_FN_GC_NOTRIGGER
#define STATIC_CONTRACT_SO_TOLERANT         ANNOTATION_FN_SO_TOLERANT

namespace StaticContract
{
    struct ScanThrowMarkerStandard
    {
        __attribute__((noinline)) ScanThrowMarkerStandard()
        {
            METHOD_CANNOT_BE_FOLDED_DEBUG;
            STATIC_CONTRACT_THROWS;
            STATIC_CONTRACT_GC_NOTRIGGER;
            STATIC_CONTRACT_SO_TOLERANT;
        }
    };

    struct ScanThrowMarkerTerminal
    {
        __attribute__((noinline)) ScanThrowMarkerTerminal()
        {
            METHOD_CANNOT_BE_FOLDED_DEBUG;
        }
    };

    struct ScanThrowMarkerIgnore
    {
        __attribute__((noinline)) ScanThrowMarkerIgnore()
        {
            METHOD_CANNOT_BE_FOLDED_DEBUG;
        }
    };
}
typedef StaticContract::ScanThrowMarkerStandard ScanThrowMarker;

// This is used to annotate code as throwing a terminal exception, and should
// be used immediately before the throw so that infer that it can be inferred
// that the block in which this annotation appears throws unconditionally.
#define SCAN_THROW_MARKER do { ScanThrowMarker __throw_marker; } while (0)

#define SCAN_EHMARKER()
#define SCAN_EHMARKER_TRY()
#define SCAN_EHMARKER_END_TRY()
#define SCAN_EHMARKER_CATCH()
#define SCAN_EHMARKER_END_CATCH()

#if defined(__cplusplus)
extern "C"
{
#endif // __cplusplus

    //
    // Exception Handling ABI Level I: Base ABI
    //

    typedef enum
    {
        _URC_NO_REASON = 0,
        _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
        _URC_FATAL_PHASE2_ERROR = 2,
        _URC_FATAL_PHASE1_ERROR = 3,
        _URC_NORMAL_STOP = 4,
        _URC_END_OF_STACK = 5,
        _URC_HANDLER_FOUND = 6,
        _URC_INSTALL_CONTEXT = 7,
        _URC_CONTINUE_UNWIND = 8,
    } _Unwind_Reason_Code;

    typedef enum
    {
        _UA_SEARCH_PHASE = 1,
        _UA_CLEANUP_PHASE = 2,
        _UA_HANDLER_FRAME = 4,
        _UA_FORCE_UNWIND = 8,
    } _Unwind_Action;
    #define _UA_PHASE_MASK (_UA_SEARCH_PHASE|_UA_CLEANUP_PHASE)

    struct _Unwind_Context;

    void *_Unwind_GetIP(struct _Unwind_Context *context);
    void _Unwind_SetIP(struct _Unwind_Context *context, void *new_value);
    void *_Unwind_GetCFA(struct _Unwind_Context *context);
    void *_Unwind_GetGR(struct _Unwind_Context *context, int index);
    void _Unwind_SetGR(struct _Unwind_Context *context, int index,
                       void *new_value);

    struct _Unwind_Exception;

    typedef void (*_Unwind_Exception_Cleanup_Fn)(
        _Unwind_Reason_Code urc,
        struct _Unwind_Exception *exception_object);

    struct _Unwind_Exception
    {
        ULONG64 exception_class;
        _Unwind_Exception_Cleanup_Fn exception_cleanup;
        UINT_PTR private_1;
        UINT_PTR private_2;
    } __attribute__((aligned));

    void _Unwind_DeleteException(struct _Unwind_Exception *exception_object);

    typedef _Unwind_Reason_Code
    (*_Unwind_Trace_Fn)(struct _Unwind_Context *context, void *pvParam);
    _Unwind_Reason_Code _Unwind_Backtrace(_Unwind_Trace_Fn pfnTrace,
                                          void *pvParam);

    _Unwind_Reason_Code
    _Unwind_RaiseException(struct _Unwind_Exception *exception_object);
    __attribute__((noreturn)) void
    _Unwind_Resume(struct _Unwind_Exception *exception_object);

    //
    // Exception Handling ABI Level II: C++ ABI
    //

    void *__cxa_begin_catch(void *exceptionObject) throw();
    void __cxa_end_catch();

#if defined(__cplusplus)
};
#endif // __cplusplus

typedef LONG EXCEPTION_DISPOSITION;

enum {
    ExceptionContinueExecution,
    ExceptionContinueSearch,
    ExceptionNestedException,
    ExceptionCollidedUnwind,
};

// A pretend exception code that we use to stand for external exceptions,
// such as a C++ exception leaking across a P/Invoke boundary into
// COMPlusFrameHandler.
#define EXCEPTION_FOREIGN 0xe0455874    // 0xe0000000 | 'EXT'

// Test whether the argument exceptionObject is an SEH exception.  If it is,
// return the associated exception pointers.  If it is not, return NULL.
typedef void (*PFN_PAL_BODY)(void *pvParam);

typedef struct _PAL_DISPATCHER_CONTEXT {
    _Unwind_Action actions;
    struct _Unwind_Exception *exception_object;
    struct _Unwind_Context *context;
} PAL_DISPATCHER_CONTEXT;

typedef EXCEPTION_DISPOSITION (*PFN_PAL_EXCEPTION_FILTER)(
    EXCEPTION_POINTERS *ExceptionPointers,
    PAL_DISPATCHER_CONTEXT *DispatcherContext,
    void *pvParam);

PALIMPORT
PALAPI
struct _Unwind_Exception *PAL_TryExcept(
    PFN_PAL_BODY pfnBody,
    PFN_PAL_EXCEPTION_FILTER pfnFilter,
    void *pvParam,
    BOOL *pfExecuteHandler);

PALIMPORT
VOID
PALAPI
RaiseException(
           DWORD dwExceptionCode,
           DWORD dwExceptionFlags,
           DWORD nNumberOfArguments,
           const ULONG_PTR *lpArguments);


//
// Possible results from PAL_TryExcept:
//
//   returned exception  pfExecuteHandler  means
//   ------------------  ----------------  -----------------------------------
//   NULL                any               No exception escaped from the try
//                                           block.
//   non-NULL            FALSE             An exception escaped from the try
//                                           block, but the filter did not want
//                                           to handle it.
//   non-NULL            TRUE              An exception escaped from the try
//                                           block, and the filter wanted to
//                                           handle it.
//

#define DEBUG_OK_TO_RETURN_BEGIN(arg)
#define DEBUG_OK_TO_RETURN_END(arg)

#define PAL_DUMMY_CALL

#if defined(__cplusplus)
class PAL_CatchHolder
{
public:
    PAL_CatchHolder(_Unwind_Exception *exceptionObject)
    {
        __cxa_begin_catch(exceptionObject);
    }

    ~PAL_CatchHolder()
    {
        __cxa_end_catch();
    }
};

class PAL_ExceptionHolder
{
private:
    _Unwind_Exception *m_exceptionObject;
public:
    PAL_ExceptionHolder(_Unwind_Exception *exceptionObject)
    {
        m_exceptionObject = exceptionObject;
    }

    ~PAL_ExceptionHolder()
    {
        if (m_exceptionObject)
        {
            _Unwind_DeleteException(m_exceptionObject);
        }
    }

    void SuppressRelease()
    {
        m_exceptionObject = 0;
    }
};

class PAL_NoHolder
{
public:
    void SuppressRelease() {}
};
#endif // __cplusplus

#define PAL_TRY(__ParamType, __paramDef, __paramRef)                           \
{                                                                              \
    struct __HandlerData                                                       \
    {                                                                          \
        __ParamType __param;                                                   \
        EXCEPTION_DISPOSITION __handlerDisposition;                            \
        __HandlerData(__ParamType param) : __param(param) {}                   \
    };                                                                         \
    __HandlerData __handlerData(__paramRef);                                   \
    class __Body                                                               \
    {                                                                          \
    public:                                                                    \
        static void Run(void *__pvHandlerData)                                 \
        {                                                                      \
            __ParamType __paramDef =                                           \
                ((__HandlerData *)__pvHandlerData)->__param;                   \
            PAL_DUMMY_CALL;

// On Windows 32bit, we dont invoke filters on the second pass. To ensure the
// same happens on other platforms, we check if we are in the first phase or
// not. If we are, we invoke the filter and save the disposition in a local
// static. 
//
// However, if we are not in the first phase but in the second, and thus
// unwinding, then we return the disposition saved from the first pass back
// (similar to how CRT does it on x86).
#define PAL_EXCEPT(dispositionExpression)                                      \
        }                                                                      \
        static EXCEPTION_DISPOSITION Handler(                                  \
            EXCEPTION_POINTERS *ExceptionPointers,                             \
            PAL_DISPATCHER_CONTEXT *DispatcherContext,                         \
            void *pvHandlerData)                                               \
        {                                                                      \
            DEBUG_OK_TO_RETURN_BEGIN(PAL_EXCEPT)                               \
            __HandlerData *pHandlerData = (__HandlerData *)pvHandlerData;      \
            void *pvParam = NULL;                                              \
            pvParam = pHandlerData->__param;                                   \
            if (!(ExceptionPointers->ExceptionRecord->ExceptionFlags &         \
                    EXCEPTION_UNWINDING))                                      \
                pHandlerData->__handlerDisposition =                           \
                    (EXCEPTION_DISPOSITION) (dispositionExpression);           \
            return pHandlerData->__handlerDisposition;                         \
            DEBUG_OK_TO_RETURN_END(PAL_EXCEPT)                                 \
        }                                                                      \
    };                                                                         \
    BOOL __fExecuteHandler;                                                    \
    _Unwind_Exception *__exception =                                           \
        PAL_TryExcept(__Body::Run, __Body::Handler, &__handlerData,            \
                      &__fExecuteHandler);                                     \
    PAL_NoHolder __exceptionHolder;                                            \
    if (__exception && __fExecuteHandler)                                      \
    {                                                                          \
        PAL_CatchHolder __catchHolder(__exception);                            \
        __exception = NULL;

#define PAL_EXCEPT_FILTER(filter) PAL_EXCEPT(filter(ExceptionPointers, pvParam))

// Executes the handler if the specified exception code matches
// the one in the exception. Otherwise, returns EXCEPTION_CONTINUE_SEARCH.
#define PAL_EXCEPT_IF_EXCEPTION_CODE(dwExceptionCode)                          \
        PAL_EXCEPT(((ExceptionPointers->ExceptionRecord->ExceptionCode ==      \
            dwExceptionCode) ?                                                 \
                EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH))

#define PAL_FINALLY                                                            \
        }                                                                      \
        static EXCEPTION_DISPOSITION Filter(                                   \
            EXCEPTION_POINTERS *ExceptionPointers,                             \
            PAL_DISPATCHER_CONTEXT *DispatcherContext,                         \
            void *pvHandlerData)                                               \
        {                                                                      \
            DEBUG_OK_TO_RETURN_BEGIN(PAL_FINALLY)                              \
            return EXCEPTION_CONTINUE_SEARCH;                                  \
            DEBUG_OK_TO_RETURN_END(PAL_FINALLY)                                \
        }                                                                      \
    };                                                                         \
    BOOL __fExecuteHandler;                                                    \
    _Unwind_Exception *__exception =                                           \
        PAL_TryExcept(__Body::Run, __Body::Filter, &__handlerData,             \
                      &__fExecuteHandler);                                     \
    PAL_ExceptionHolder __exceptionHolder(__exception);                        \
    {

#define PAL_ENDTRY                                                             \
    }                                                                          \
    if (__exception)                                                           \
    {                                                                          \
        __exceptionHolder.SuppressRelease();                                   \
        _Unwind_Resume(__exception);                                           \
    }                                                                          \
}


// COM
typedef struct _GUID {
    ULONG   Data1;    // NOTE: diff from Win32, for LP64
    USHORT  Data2;
    USHORT  Data3;
    UCHAR   Data4[ 8 ];
} GUID;

#if defined(__cplusplus)
#define REFGUID const GUID &
#else
#define REFGUID const GUID *
#endif

typedef GUID IID;
#if defined(__cplusplus)
#define REFIID const IID &
#else
#define REFIID const IID *
#endif

typedef GUID CLSID;
#if defined(__cplusplus)
#define REFCLSID const CLSID &
#else
#define REFCLSID const CLSID *
#endif

#define MIDL_INTERFACE(x)   struct DECLSPEC_NOVTABLE

#define EXTERN_GUID(itf,l1,s1,s2,c1,c2,c3,c4,c5,c6,c7,c8)                      \
    EXTERN_C const IID DECLSPEC_SELECTANY itf =                                \
        {l1,s1,s2,{c1,c2,c3,c4,c5,c6,c7,c8}}

#define interface struct

#define DECLARE_INTERFACE(iface) interface DECLSPEC_NOVTABLE iface
#define DECLARE_INTERFACE_(iface, baseiface)                                   \
    interface DECLSPEC_NOVTABLE iface : public baseiface

typedef interface IUnknown IUnknown;

typedef /* [unique] */ IUnknown *LPUNKNOWN;

// 00000000-0000-0000-C000-000000000046
EXTERN_C const IID IID_IUnknown;

MIDL_INTERFACE("00000000-0000-0000-C000-000000000046")
IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        void **ppvObject) = 0;

    virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;

    virtual ULONG STDMETHODCALLTYPE Release( void) = 0;
};

interface IStream;
interface IRecordInfo;
interface ITypeInfo;

typedef SHORT VARIANT_BOOL;
typedef LONG SCODE;

typedef union tagCY {
    struct {
#if BIGENDIAN
        LONG    Hi;
        ULONG   Lo;
#else
        ULONG   Lo;
        LONG    Hi;
#endif
    } u;
    LONGLONG int64;
} CY;

typedef WCHAR *BSTR;
typedef double DATE;

typedef struct tagDEC {
#if BIGENDIAN
    union {
        struct {
            BYTE sign;
            BYTE scale;
        } u;
        USHORT signscale;
    } u;
    USHORT wReserved;
#else
    USHORT wReserved;
    union {
        struct {
            BYTE scale;
            BYTE sign;
        } u;
        USHORT signscale;
    } u;
#endif
    ULONG Hi32;
    union {
        struct {
            ULONG Lo32;
            ULONG Mid32;
        } v;
        ULONGLONG Lo64;
    } v;
} DECIMAL;

typedef unsigned short VARTYPE;

typedef struct tagVARIANT VARIANT;

struct tagVARIANT
{
    union
    {
        struct
        {
    #if BIGENDIAN
            WORD wReserved1;
            VARTYPE vt;
    #else
            VARTYPE vt;
            WORD wReserved1;
    #endif
            WORD wReserved2;
            WORD wReserved3;
            union
            {
                LONGLONG llVal;
                LONG lVal;
                BYTE bVal;
                SHORT iVal;
                FLOAT fltVal;
                DOUBLE dblVal;
                VARIANT_BOOL boolVal;
                SCODE scode;
                CY cyVal;
                DATE date;
                BSTR bstrVal;
                interface IUnknown *punkVal;
                BYTE *pbVal;
                SHORT *piVal;
                LONG *plVal;
                LONGLONG *pllVal;
                FLOAT *pfltVal;
                DOUBLE *pdblVal;
                VARIANT_BOOL *pboolVal;
                SCODE *pscode;
                CY *pcyVal;
                DATE *pdate;
                BSTR *pbstrVal;
                interface IUnknown **ppunkVal;
                VARIANT *pvarVal;
                PVOID byref;
                CHAR cVal;
                USHORT uiVal;
                ULONG ulVal;
                ULONGLONG ullVal;
                INT intVal;
                UINT uintVal;
                DECIMAL *pdecVal;
                CHAR *pcVal;
                USHORT *puiVal;
                ULONG *pulVal;
                ULONGLONG *pullVal;
                INT *pintVal;
                UINT *puintVal;
                struct __tagBRECORD
                {
                    PVOID pvRecord;
                    interface IRecordInfo *pRecInfo;
                } brecVal;
            } n3;
        } n2;
    DECIMAL decVal;
    } n1;
};

#endif // _MSC_VER

#endif // MSILC_PAL
