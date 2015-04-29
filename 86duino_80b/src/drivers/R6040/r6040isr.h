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
// File: R6040ISR.H
//
// Purpose: Header file shared between R6040.dll and R6040isr.dll
//
//

#ifndef _R6040ISR_H_
#define _R6040ISR_H_

#include <pkfuncs.h>

#define USER_IOCTL(X)           (IOCTL_KLIB_USER + (X))

#define IOCTL_R6040ISR_INFO    USER_IOCTL(0)

typedef struct _R6040_ISR_INFO
{
    DWORD   SysIntr;            //  SYSINTR for ISR handler to return (if associated device is asserting IRQ)
    DWORD   PortAddr;           //  Port Address
    //
    // Switching to page 2 to read the interrupt mask register will abort any in-progress "DMA" activity
    // and can adversely impact other IST processing. Instead, R6040.dll will write the current interrupt
    // mask register state here.
    //
    LPDWORD pIntMaskVirt;  // Virtual address containing the current interrupt mask register state
    LPDWORD pIntMaskPhys;  // Physical address containing the current interrupt mask register state

} R6040_ISR_INFO, *PR6040_ISR_INFO;

#endif // _R6040ISR_H_
