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
#include <oal_log.h>

// ---------------------------------------------------------------------------
// OEMInterruptEnable: REQUIRED
//
// This function performs all hardware operations necessary to enable the
// specified hardware interrupt.
//
// OEMInterruptEnable is supplied by a platform\common library,
// so we should not implement it here in the BSP.

// ---------------------------------------------------------------------------
// OEMInterruptDisable: REQUIRED
//
// This function performs all hardware operations necessary to disable the
// specified hardware interrupt.
//
// OEMInterruptDisable is supplied by a platform\common library,
// so we should not implement it here in the BSP.

// ---------------------------------------------------------------------------
// OEMInterruptDone: REQUIRED
//
// This function signals completion of interrupt processing.  This function
// should re-enable the interrupt if the interrupt was previously masked.
//
// OEMInterruptDone is supplied by a platform\common library,
// so we should not implement it here in the BSP.

// ---------------------------------------------------------------------------
// OEMInterruptMask: REQUIRED
//
// This function masks or unmasks the interrupt according to its SysIntr
// value.
//
// OEMInterruptMask is supplied by a platform\common library,
// so we should not implement it here in the BSP.

// ---------------------------------------------------------------------------
// OEMNotifyIntrOccurs: OPTIONAL
//
// This function is called by the kernel when an interrupt occurs.  It is
// not an ISR intended to handle interrupts.
//
DWORD OEMNotifyIntrOccurs(DWORD dwSysIntr)
{
  // Fill in code here.

  return 0;
}
