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
//------------------------------------------------------------------------------
//
//  Header:  bsp_ethdrv.h
//
//  This header file contains prototype for ethernet KITL devices driver
//
#ifndef __BSP_ETHDRV_H
#define __BSP_ETHDRV_H

#if __cplusplus
extern "C" {
#endif

#include <halether.h>

//------------------------------------------------------------------------------
// Prototypes for R6040

BOOL   R6040InitDMABuffer(UINT32 address, UINT32 size);
BOOL   R6040Init(UINT8 *pAddress, UINT32 offset, UINT16 mac[3]);
UINT16 R6040SendFrame(UINT8 *pData, UINT32 length);
UINT16 R6040GetFrame(UINT8 *pData, UINT16 *pLength);
VOID   R6040EnableInts();
VOID   R6040DisableInts();
VOID   R6040CurrentPacketFilter(UINT32 filter);
BOOL   R6040MulticastList(UINT8 *pAddresses, UINT32 count);

#define OAL_ETHDRV_R6040      { \
    R6040Init, R6040InitDMABuffer, NULL, R6040SendFrame, R6040GetFrame, \
    R6040EnableInts, R6040DisableInts, \
    NULL, NULL, R6040CurrentPacketFilter, R6040MulticastList \
}

//------------------------------------------------------------------------------

#if __cplusplus
}
#endif

#endif
