//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
#include <windows.h>
#include <pkfuncs.h>
#include <oal.h>
// This is the platform\common ioctl headers.
#include <oal_intr.h>
#include <oal_io.h>
#include <oal_ioctl.h>
#include <x86ioctl.h>

// Cache Information for Vortex86DX (A9120A) rev. A chip
//
const CacheInfo Vortex86DXCacheInfo =
{
    0,               // flags for L1 cache
    (16 * 1024),     // L1 IC total size in bytes
    16,              // L1 IC line size in bytes
    4,               // L1 IC number of ways, 1 for direct mapped
    (16 * 1024),     // L1 DC total size in bytes
    16,              // L1 DC line size in bytes
    4,               // L1 DC number of ways, 1 for direct mapped
    CF_UNIFIED,      // flags for L2 cache
    (128 * 1024),    // L2 IC total size in bytes, 0 means no L2 ICache
    16,              // L2 IC line size in bytes
    1,               // L2 IC number of ways, 1 for direct mapped
    (128 * 1024),    // L2 DC total size in bytes, 0 means no L2 DCache, I/D cache unified, use ICache info fields, just keep as copy of ICache info
    16,              // L2 DC line size in bytes
    1                // L2 DC number of ways, 1 for direct mapped
};

// Cache Information for Vortex86DX (A9121) rev. D and Vortex86MX/DX-II chips
//
const CacheInfo Vortex86MXCacheInfo =
{
    0,               // flags for L1 cache
    (16 * 1024),     // L1 IC total size in bytes
    16,              // L1 IC line size in bytes
    4,               // L1 IC number of ways, 1 for direct mapped
    (16 * 1024),     // L1 DC total size in bytes
    16,              // L1 DC line size in bytes
    4,               // L1 DC number of ways, 1 for direct mapped
    CF_UNIFIED,      // flags for L2 cache, I and D caches unified
    (256 * 1024),    // L2 IC total size in bytes, 0 means no L2 ICache
    16,              // L2 IC line size in bytes
    4,               // L2 IC number of ways, 1 for direct mapped
    (256 * 1024),    // L2 DC total size in bytes, 0 means no L2 DCache, I/D cache unified, use ICache info fields, just keep as copy of ICache info
    16,              // L2 DC line size in bytes
    4                // L2 DC number of ways, 1 for direct mapped
};


// Declaration of x86 common libraries in platform\common
void  RTCPostInit();

// forward declarations of custom IOCTL handler functions
BOOL BSPIoCtlPostInit (
    UINT32 code, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32 *lpBytesReturned
);

BOOL BSPIoCtlHalGetCacheInfo (
    UINT32 code, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32 *lpBytesReturned
);

// g_pPlatformManufacturer: REQUIRED
//
// The platform\common implementation requires that we define this variable
// for device info ioctl handler.
//
LPCWSTR g_pPlatformManufacturer = L"DMP";           // OEM Name

// g_pPlatformName: REQUIRED
//
// The platform\common implementation requires that we define this variable
// for device info ioctl handler.
//
LPCWSTR g_pPlatformName         = L"VDX";           // Platform Name

// g_oalIoCtlPlatformType: REQUIRED
//
// The platform\common implementation requires that we define this variable
// for device info ioctl handler.
//
LPCWSTR g_oalIoCtlPlatformType = L"VDX";            // changeable by IOCTL

// g_oalIoCtlPlatformOEM: REQUIRED
//
// The platform\common implementation requires that we define this variable
// for device info ioctl handler.
//
LPCWSTR g_oalIoCtlPlatformOEM  = L"VDX";            // constant, should've never changed

// g_pszDfltProcessorName: REQUIRED
//
// The platform\common implementation requires that we define this variable
// for processor info ioctl handler.
//
LPCWSTR g_pszDfltProcessorName = L"Vortex86DX";

static BOOL g_fPostInit;

// ---------------------------------------------------------------------------
// OEMIoControl: REQUIRED
//
// This function provides a generic I/O control code (IOCTL) for
// OEM-supplied information.
//
// OEMIoControl is supplied by a platform\common library,
// so we should not implement it here in the BSP.

// ---------------------------------------------------------------------------
// g_oalIoCtlTable: REQUIRED
//
// The platform\common implementation requires that we define this variable
// describing which IOCTLs we support.
//
const OAL_IOCTL_HANDLER g_oalIoCtlTable[] = {
    { IOCTL_HAL_REQUEST_SYSINTR,                0,          OALIoCtlHalRequestSysIntr   },
    { IOCTL_HAL_RELEASE_SYSINTR,                0,          OALIoCtlHalReleaseSysIntr   },
    { IOCTL_HAL_REQUEST_IRQ,                    0,          OALIoCtlHalRequestIrq       },

    { IOCTL_HAL_DDK_CALL,                       0,          OALIoCtlHalDdkCall          },

    { IOCTL_HAL_DISABLE_WAKE,                   0,          x86PowerIoctl               },
    { IOCTL_HAL_ENABLE_WAKE,                    0,          x86PowerIoctl               },
    { IOCTL_HAL_GET_WAKE_SOURCE,                0,          x86PowerIoctl               },
    { IOCTL_HAL_PRESUSPEND,                     0,          x86PowerIoctl               },
    { IOCTL_HAL_GET_POWER_DISPOSITION,          0,          x86IoCtlGetPowerDisposition },

    { IOCTL_HAL_GET_CACHE_INFO,                 0,          BSPIoCtlHalGetCacheInfo     },
    { IOCTL_HAL_GET_DEVICEID,                   0,          OALIoCtlHalGetDeviceId      },
    { IOCTL_HAL_GET_DEVICE_INFO,                0,          OALIoCtlHalGetDeviceInfo    },
    { IOCTL_HAL_GET_UUID,                       0,          OALIoCtlHalGetUUID          },
    { IOCTL_HAL_GET_RANDOM_SEED,                0,          OALIoCtlHalGetRandomSeed    },
    
    { IOCTL_PROCESSOR_INFORMATION,              0,          x86IoCtlProcessorInfo       },
    
    { IOCTL_HAL_INIT_RTC,                       0,          x86IoCtlHalInitRTC          },
    { IOCTL_HAL_REBOOT,                         0,          x86IoCtlHalReboot           },

    { IOCTL_HAL_ILTIMING,                       0,          x86IoCtllTiming             },

    { IOCTL_HAL_POSTINIT,                       0,          BSPIoCtlPostInit            },
    
    { IOCTL_HAL_INITREGISTRY,                   0,          x86IoCtlHalInitRegistry     },

    // Required Termination
    { 0,                                        0,          NULL                        }
};

// ---------------------------------------------------------------------------
// BSPIoCtlPostInit: CUSTOM
//
// IOCTL_HAL_POSTINIT is called by the kernel and implemented in the OAL to 
// provide the OEM with a last chance to perform an action before 
// other processes are started.
//
// The handler initialize the critical section for RTC common library here.
//
BOOL BSPIoCtlPostInit (
    UINT32 code, VOID *lpInBuf, UINT32 nInBufSize, VOID *lpOutBuf, 
    UINT32 nOutBufSize, UINT32 *lpBytesReturned
) {
    if (lpBytesReturned) {
        *lpBytesReturned = 0;
    }

    RTCPostInit();

    g_fPostInit = TRUE;

    return TRUE;
}


// ---------------------------------------------------------------------------
// IsAfterPostInit: CUSTOM
//
// TRUE once the kernel has performed a more full featured init. Critical Sections can now be used
//
BOOL IsAfterPostInit() 
{ 
    return g_fPostInit; 
}

//------------------------------------------------------------------------------
//
//  Function:  BSPIoCtlHalGetCacheInfo
//
//  This function returns information about the CPU's instruction and data caches.
//
BOOL BSPIoCtlHalGetCacheInfo(
    UINT32 code, VOID *pInpBuffer, UINT32 inpSize, VOID *pOutBuffer,
    UINT32 outSize, UINT32 *pOutSize)
{    
    PROCESSOR_INFO procInfo;
    DWORD dwBytesReturned;

    // Validate caller's arguments.
    //
    if (!pOutBuffer || (outSize < sizeof(CacheInfo)))
    {
        NKSetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(FALSE);
    }

    // As Vortex86 chips do not support cache size instructions, we are
    // hard coding the cache values as per the inputs provided by ICOP
    // based on the processor name.

    // Calling x86IoCtlProcessorInfo() here to get the processor name. 
    if (x86IoCtlProcessorInfo(IOCTL_PROCESSOR_INFORMATION, NULL, 0, &procInfo, sizeof(PROCESSOR_INFO), &dwBytesReturned))
    {
        if(!wcscmp(procInfo.szProcessorName,L"Vortex86DX"))
        {
            // Copy the cache information into the caller's buffer.
            //
            __try
            {
// If its a Vortex86DX (A9120A) rev. A chip 
#ifdef VORTEX86DX_9120A
                // Vortex86DX (A9120A) rev. A
                memcpy(pOutBuffer, &Vortex86DXCacheInfo, sizeof(CacheInfo));
#else
                // Vortex86DX(A9121)rev.D
                memcpy(pOutBuffer, &Vortex86MXCacheInfo, sizeof(CacheInfo));
#endif
                if (pOutSize) *pOutSize = sizeof(CacheInfo);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                NKSetLastError(ERROR_INVALID_USER_BUFFER);
                OALMSG(OAL_WARN, (
                    L"WARN: BSPIoCtlHalGetCacheInfo: User buffer access violation!\r\n"
                ));
                return(FALSE);
            }
        }
        else if(!wcscmp(procInfo.szProcessorName,L"VortexMX/DX-II"))
        {
            // Copy the cache information into the caller's buffer.
            //
            __try
            {
                //Vortex86MX/DX-II
                memcpy(pOutBuffer, &Vortex86MXCacheInfo, sizeof(CacheInfo));
                if (pOutSize) *pOutSize = sizeof(CacheInfo);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                NKSetLastError(ERROR_INVALID_USER_BUFFER);
                OALMSG(OAL_WARN, (
                    L"WARN: BSPIoCtlHalGetCacheInfo: User buffer access violation!\r\n"
                ));
                return(FALSE);
            }

        }
        else 
        {
            // If the CPUID is not present in Vortex86 processor signature
            // table, go back to generic implementation, and check if the 
            // cache size instructions are supported for the chip or not.  
            if(!x86IoCtlHalGetCacheInfo(code,pInpBuffer,inpSize,pOutBuffer,outSize,pOutSize))
                return(FALSE);
        }
    }

    return(TRUE);
}

