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
#include <x86ioctl.h>
#include <oal.h>
#include <pc.h>

BOOL x86IoCtlHalQueryFormatPartition(
                               UINT32 code, 
                               __in_bcount(nInBufSize) void *lpInBuf, 
                               UINT32 nInBufSize, 
                               __out_bcount(nOutBufSize) void *lpOutBuf, 
                               UINT32 nOutBufSize, 
                               __out UINT32 *lpBytesReturned
                               ) ;

const OAL_IOCTL_HANDLER g_oalIoCtlTable [] = {

{ IOCTL_HAL_QUERY_FORMAT_PARTITION,         0,      x86IoCtlHalQueryFormatPartition },

#include <ioctl_tab.h>

};

LPCWSTR g_pPlatformManufacturer = L"Microsoft Corporation";     // OEM Name
LPCWSTR g_pPlatformName         = L"CEPC";          // Platform Name
LPCWSTR g_oalIoCtlPlatformType  = L"CEPC";          // changeable by IOCTL
LPCWSTR g_oalIoCtlPlatformOEM   = L"CEPC";          // constant, should've never changed
LPCSTR  g_oalDeviceNameRoot     = "CEPC";           // ASCII, initial value

LPCWSTR g_pszDfltProcessorName = L"i486";

const DWORD g_dwBSPMsPerIntr = 1;                   // interrupt every ms
