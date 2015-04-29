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
#include <oemglobal.h>
#include <nkintr.h>
#include <pc.h>
#include <wdm.h>

// All OAL functions in the BSP, there are three function types:
// REQUIRED - you must implement this function for kernel functionality
// OPTIONAL - you may implement this function
// CUSTOM   - this function is a helper function specific to this BSP

//
// external functions
//
extern VOID OEMWriteDebugByte(UINT8 ch);

//
// local constant.
//

#define LS_TSR_EMPTY        0x40
#define LS_THR_EMPTY        0x20
#define LS_RX_BREAK         0x10
#define LS_RX_FRAMING_ERR   0x08
#define LS_RX_PARITY_ERR    0x04
#define LS_RX_OVERRUN       0x02
#define LS_RX_DATA_READY    0x01

#define LS_RX_ERRORS        ( LS_RX_FRAMING_ERR | LS_RX_PARITY_ERR | LS_RX_OVERRUN )

//
// Global variables.
//

PUCHAR g_pucIoPortBase = 0;

//   14400 = 8
//   16457 = 7 +/-
//   19200 = 6
//   23040 = 5
//   28800 = 4
//   38400 = 3
//   57600 = 2
//  115200 = 1

// ---------------------------------------------------------------------------
// OEMWriteDebugByte: OPTIONAL
//
// This function outputs a byte to the debug monitor port.
//
void OEMWriteDebugByte(BYTE bChar)
{
    if ( g_pucIoPortBase ) {
        while ( !(READ_PORT_UCHAR(g_pucIoPortBase+comLineStatus) & LS_THR_EMPTY) ) {
            ;
        }

        WRITE_PORT_UCHAR(g_pucIoPortBase+comTxBuffer, bChar);
    }
}

// ---------------------------------------------------------------------------
// OEMWriteDebugString: OPTIONAL
//
// This function writes a byte to the debug monitor port.
//
void OEMWriteDebugString(UINT16 *pszStr)
{
    while (*pszStr != L'\0') OEMWriteDebugByte((UINT8)*pszStr++);
}

// ---------------------------------------------------------------------------
// OEMReadDebugByte: OPTIONAL
//
// This function retrieves a byte to the debug monitor port.
//
int OEMReadDebugByte(void)
{
    unsigned char   ucStatus;
    unsigned char   ucChar;

    if ( g_pucIoPortBase ) {
        ucStatus = READ_PORT_UCHAR(g_pucIoPortBase+comLineStatus);

        if ( ucStatus & LS_RX_DATA_READY ) {
            ucChar = READ_PORT_UCHAR(g_pucIoPortBase+comRxBuffer);

            if ( ucStatus & LS_RX_ERRORS ) {
                return (OEM_DEBUG_COM_ERROR);
            } else {
                return (ucChar);
            }

        }
    }

    return (OEM_DEBUG_READ_NODATA);
}

// ---------------------------------------------------------------------------
// OEMWriteDebugLED: OPTIONAL
//
// This function outputs a byte to the target device's specified LED port.
//
void OEMWriteDebugLED(WORD wIndex, DWORD dwPattern)
{
    // no LED
}

// ---------------------------------------------------------------------------
// dpCurSettings: REQUIRED
//
// The variable enables the OAL's internal debug zones so that debug messaging
// macros like DEBUGMSG can be used.  dpCurSettings is a set of names for
// debug zones which are enabled or disabled by the bitfield parameter at
// the end.  There are 16 zones, each with a customizable name.
//
// dpCurSettings is supplied by a platform\common library,
// so we should not implement it here in the BSP.
