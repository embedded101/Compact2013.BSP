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

// ---------------------------------------------------------------------------
// OEMIdle: REQUIRED
//
// This function is called by the kernel to place the CPU in the idle state
// when there are no threads ready to run.
//
// If you implement OEMIdleEx, you can leave this function in a stub form
// as the kernel will prefer OEMIdleEx (OEMIdleEx has better performance).
//
// OEMIdle is supplied by a platform\common library,
// so we should not implement it here in the BSP.

// ---------------------------------------------------------------------------
// OEMPowerOff: REQUIRED
//
// The function is responsible for any final power-down state and for
// placing the CPU into a suspend state.
//
// OEMPowerOff is supplied by a platform\common library,
// so we should not implement it here in the BSP.

// ---------------------------------------------------------------------------
// OEMHaltSystem: OPTIONAL
//
// The function is called when the kernel is about to halt the system.
//
void OEMHaltSystem(void)
{
  // Fill in halt code here.

  return;
}

