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
#include <oal.h>

#include <pc.h>
#include <x86boot.h>
#include <x86kitl.h>
#include <bootarg.h>
#include <nkintr.h>

// Init.c
// The comments in this file will vary from OS version to version.
// Look up OEMGlobal on MSDN for a full reference of OAL functions,
// or see public\common\oak\inc\oemglobal.h
//
// All OAL functions in the template BSP fall into one of three categories:
// REQUIRED - you must implement this function for kernel functionality
// OPTIONAL - you may implement this function to enable specific functionality
// CUSTOM   - this function is a helper function specific to this BSP
//
// OEMGlobal is initialized by the kernel to use default function names
// for all required functions.  See the comments in
// public\common\oak\inc\oemglobal.h for a concise listing of the default
// function names along with their required/optional status.
//
// In this BSP all of the functions are implemented using the default
// function names.  However, with the exception of the OEMInitDebugSerial
// and OEMInit functions, you can change these function names from the
// defaults by pointing the kernel calls to use a different name.  
//

// Declaration of kernel security cookie for linking purposes
extern DWORD_PTR __security_cookie;
extern DWORD_PTR __security_cookie_complement;

// Declaration of serial debug port base address.
extern PUCHAR g_pucIoPortBase;

// Declaration of x86 Boot/KITL/UUID info.
UINT8 g_x86uuid[16];
static X86BootInfo x86Info;
PX86BootInfo g_pX86Info = &x86Info;

//  Global:  dwOEMDrWatsonSize
//
//  Global variable which specify DrWatson buffer size. It can be fixed
//  in config.bib via FIXUPVAR.
//
#define DR_WATSON_SIZE_NOT_FIXEDUP (-1)
DWORD dwOEMDrWatsonSize = DR_WATSON_SIZE_NOT_FIXEDUP;

// Custom x86 Boot Init function.
static void InitBootInfo (BOOT_ARGS *pBootArgs);

// Declaration of x86 common libraries in platform\common
DWORD OEMPowerManagerInit(void);
void InitClock(void);
void x86InitMemory (void);
void x86InitPICs(void);
void PCIInitBusInfo (void);

extern LPCWSTR g_pPlatformManufacturer;
extern LPCWSTR g_pPlatformName;

// ---------------------------------------------------------------------------
// OEMInitDebugSerial: REQUIRED
//
// This function initializes the debug serial port on the target device,
// useful for debugging OAL bringup.
//
// This is the first OAL function that the kernel calls, before the OEMInit
// function and before the kernel data section is fully initialized.
//
// OEMInitDebugSerial can use global variables; however, these variables might
// not be initialized and might subsequently be cleared when the kernel data
// section is initialized.
//
void OEMInitDebugSerial(void)
{

    // Locate bootargs (this is the first opportunity the OAL has to initialize this global).
    //
    InitBootInfo ((BOOT_ARGS *) ((ULONG)(*(PBYTE *)BOOT_ARG_PTR_LOCATION) | 0x80000000));

    switch ( g_pX86Info->ucComPort ) {
    case 1:
        g_pucIoPortBase = (PUCHAR)COM1_BASE;
        break;

    case 2:
        g_pucIoPortBase = (PUCHAR)COM2_BASE;
        break;

    case 3:
        g_pucIoPortBase = (PUCHAR)COM3_BASE;
        break;

    case 4:
        g_pucIoPortBase = (PUCHAR)COM4_BASE;
        break;

    default:
        g_pucIoPortBase = 0;
        break;

    }

    if ( g_pucIoPortBase ) {
        WRITE_PORT_UCHAR(g_pucIoPortBase+comLineControl, 0x80);   // Access Baud Divisor
        WRITE_PORT_UCHAR(g_pucIoPortBase+comDivisorLow, g_pX86Info->ucBaudDivisor); 
        WRITE_PORT_UCHAR(g_pucIoPortBase+comDivisorHigh, 0x00);
        WRITE_PORT_UCHAR(g_pucIoPortBase+comFIFOControl, 0x01);   // Enable FIFO if present
        WRITE_PORT_UCHAR(g_pucIoPortBase+comLineControl, 0x03);   // 8 bit, no parity
        WRITE_PORT_UCHAR(g_pucIoPortBase+comIntEnable, 0x00);     // No interrupts, polled
        WRITE_PORT_UCHAR(g_pucIoPortBase+comModemControl, 0x03);  // Assert DTR, RTS
        OEMWriteDebugString(TEXT("Debug Serial Init\r\n"));
    }
}

// ---------------------------------------------------------------------------
// OEMInit: REQUIRED
//
// This function intializes device hardware as well as initiates the KITL
// debug transport.
//
// This is the second OAL function that the kernel calls.
//
// When the kernel calls OEMInit, interrupts are disabled and the kernel is
// unable to handle exceptions. The only kernel service available to this
// function is HookInterrupt.
//
void OEMInit(void)
{
    // Fill in hardware initialization code here.
	OEMWriteDebugString(TEXT("++OEMInit\r\n"));
    // initialize interrupts
    OALIntrInit ();
	OEMWriteDebugString(TEXT("Done OEMInrInit\r\n"));
    // initialize PIC
    x86InitPICs();
	OEMWriteDebugString(TEXT("Done x86InitPics\r\n"));
    // Initialize PCI bus information
    PCIInitBusInfo ();
	OEMWriteDebugString(TEXT("Done PCIInitBus\r\n"));
    // Fill in KITL initiation here.

    // starts KITL (will be a no-op if KITLDLL doesn't exist)
    KITLIoctl (IOCTL_KITL_STARTUP, NULL, 0, NULL, 0, NULL);
	OEMWriteDebugString(TEXT("Done Kitl IOCTRL\r\n"));
    // sets the global platform manufacturer name and platform name
    g_oalIoCtlPlatformManufacturer = g_pPlatformManufacturer;
    g_oalIoCtlPlatformName = g_pPlatformName;

    OEMPowerManagerInit();
    OEMWriteDebugString(TEXT("Done OEM PM\r\n"));
    // initialize clock
    InitClock();
    // initialize memory (detect extra ram, MTRR/PAT etc.)
    x86InitMemory();
	OEMWriteDebugString(TEXT("Done x86InitMemory\r\n"));
    // Reserve 128kB memory for Watson Dumps
    dwNKDrWatsonSize = 0;
    if (dwOEMDrWatsonSize != DR_WATSON_SIZE_NOT_FIXEDUP) {
        dwNKDrWatsonSize = dwOEMDrWatsonSize;
    OEMWriteDebugString(TEXT("-- OEMInit\r\n"));
    }
}

// initial the Boot Info.
//
static void InitBootInfo (BOOT_ARGS *pBootArgs)
{
    static LPCSTR s_oalDeviceNameRoot = "VDX";     // ASCII, initial value
    if (BOOT_ARG_VERSION_SIG != pBootArgs->dwVersionSig) {
        // nothing passed from bootloader, set default
        //memset (&x86Info, 0, sizeof(x86Info));
        
    } else {
        x86Info.dwKitlIP            = pBootArgs->EdbgAddr.dwIP;
        x86Info.dwKitlBaseAddr      = pBootArgs->dwEdbgBaseAddr;
        x86Info.dwKitlDebugZone     = pBootArgs->dwEdbgDebugZone;
        x86Info.KitlTransport       = pBootArgs->KitlTransport;
        x86Info.ucKitlAdapterType   = pBootArgs->ucEdbgAdapterType;
        x86Info.ucComPort           = pBootArgs->ucComPort;
        x86Info.ucBaudDivisor       = pBootArgs->ucBaudDivisor;
        x86Info.ucKitlIrq           = pBootArgs->ucEdbgIRQ;
        x86Info.dwRebootAddr        = (BOOTARG_SIG == pBootArgs->dwEBootFlag)? pBootArgs->dwEBootAddr : g_pNKGlobal->dwStartupAddr;
        x86Info.cxDisplayScreen     = pBootArgs->cxDisplayScreen;
        x86Info.cyDisplayScreen     = pBootArgs->cyDisplayScreen;
        x86Info.bppScreen           = pBootArgs->bppScreen;
        x86Info.ucPCIConfigType     = pBootArgs->ucPCIConfigType;
        x86Info.NANDBootFlags       = pBootArgs->NANDBootFlags;     // Boot flags related to NAND support.
        x86Info.NANDBusNumber       = pBootArgs->NANDBusNumber;     // NAND controller PCI bus number.
        x86Info.NANDSlotNumber      = pBootArgs->NANDSlotNumber;    // NAND controller PCI slot number.
        x86Info.fStaticIP           = pBootArgs->EdbgFlags & EDBG_FLAGS_STATIC_IP;
        memcpy (&x86Info.wMac[0], &pBootArgs->EdbgAddr.wMAC[0], sizeof (x86Info.wMac));
        memcpy (&x86Info.szDeviceName[0], &pBootArgs->szDeviceNameRoot[0], sizeof (x86Info.szDeviceName));
        if (pBootArgs->ucLoaderFlags & LDRFL_KITL_DISABLE_VMINI)
        {
            x86Info.fKitlVMINI = FALSE;
        }
        else
        {
            x86Info.fKitlVMINI = TRUE;
        }

        if (pBootArgs->ucLoaderFlags & LDRFL_FORMAT_STORE)
        {
            x86Info.fFormatUserStore = TRUE;
        }
        else
        {
            x86Info.fFormatUserStore = FALSE;
        }
    }

    g_pOemGlobal->pKitlInfo = &x86Info;

    // if name root not specified, use BSP specific name root.
    // NOTE: device name can change when KITL started.
    if (!x86Info.szDeviceName[0]) {
        memcpy (&x86Info.szDeviceName[0], &s_oalDeviceNameRoot[0], sizeof (x86Info.szDeviceName));
    } else {
        s_oalDeviceNameRoot = (LPCSTR) pBootArgs->szDeviceNameRoot;
    }

    // initialize fields that are required to be non-zero
    if (!x86Info.ucBaudDivisor) {
        x86Info.ucBaudDivisor = 3;  // default to 38400
    }
    if (!x86Info.dwRebootAddr) {
        x86Info.dwRebootAddr = g_pNKGlobal->dwStartupAddr;      // no bootloader, set to startup
    }
    if (!x86Info.ucPCIConfigType) {
        x86Info.ucPCIConfigType = 1;                // default PCI config type 1
    }

    // set up our UUID based upon the MAC address of the ethernet card
    // note that this is not really universally unique, but we return it
    // for test purposes

    // Microsoft test manufacturer ID
    g_x86uuid[0] = (UCHAR)0x00;
    g_x86uuid[1] = (UCHAR)0x30;
    g_x86uuid[2] = (UCHAR)0xBD;
    g_x86uuid[3] = (UCHAR)0x2D;
    g_x86uuid[4] = (UCHAR)0x73;
    g_x86uuid[5] = (UCHAR)0x32;

    // Next 16-bits: Version/variant - Version 1.8 format (48/16/64) from Windows Mobile docs
    g_x86uuid[6] = (UCHAR)1;
    g_x86uuid[7] = (UCHAR)8;

    // Last 64-bits: Unique ID
    g_x86uuid[8]  = (UCHAR) ((x86Info.wMac[0] >> 0) & 0xFF);
    g_x86uuid[9]  = (UCHAR) ((x86Info.wMac[0] >> 8) & 0xFF);
    g_x86uuid[10] = (UCHAR) ((x86Info.wMac[1] >> 0) & 0xFF);
    g_x86uuid[11] = (UCHAR) ((x86Info.wMac[1] >> 8) & 0xFF);
    g_x86uuid[12] = (UCHAR) ((x86Info.wMac[2] >> 0) & 0xFF);
    g_x86uuid[13] = (UCHAR) ((x86Info.wMac[2] >> 8) & 0xFF);
    // MAC addr. only has 6 bytes.
    g_x86uuid[14] = 0x00;
    g_x86uuid[15] = 0x00;
}

//BYTE IsRunningOnVirtualMachine()
//{
//                return 0;
//}

