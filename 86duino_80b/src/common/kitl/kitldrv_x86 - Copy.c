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
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
#include <windows.h>
#include <x86kitl.h>
#include <bsp_ethdrv.h>

static OAL_KITL_ETH_DRIVER DrvRTL   = OAL_ETHDRV_RTL8139;    // RTL8139
static OAL_KITL_ETH_DRIVER DrvRDC   = OAL_ETHDRV_R6040;      // R6040   /* RDC MAC driver */


const SUPPORTED_NIC g_NicSupported []=
{
//   VenId   DevId   UpperMAC      Type              Name   Drivers
//  ---------------------------------------------------------------------------------
    {0x10EC, 0x8129, 0x000000, EDBG_ADAPTER_RTL8139, "RT", &DrvRTL   }, /* RealTek */
    {0x10EC, 0x8139, 0x00900B, EDBG_ADAPTER_RTL8139, "RT", &DrvRTL   }, /* RealTek  */
    {0x10EC, 0x8139, 0x00D0C9, EDBG_ADAPTER_RTL8139, "RT", &DrvRTL   }, /* RealTek */
    {0x10EC, 0x8139, 0x00E04C, EDBG_ADAPTER_RTL8139, "RT", &DrvRTL   }, /* RealTek */
    {0x1186, 0x1300, 0x0050BA, EDBG_ADAPTER_RTL8139, "DL", &DrvRTL   }, /* D-Link */
    {0x17F3, 0x6040, 0x001BEB, EDBG_ADAPTER_OEM,     "RD", &DrvRDC   } /* RDC */
};

const int g_nNumNicSupported = sizeof (g_NicSupported) / sizeof (g_NicSupported[0]);

