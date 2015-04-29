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
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    keywords.h

Abstract:

    Contains all Ndis2 and Ndis3 mac-specific keywords.

Notes:

--*/
#ifndef NDIS2
#define NDIS2 0
#endif

#if NDIS2

#define IOADDRESS  NDIS_STRING_CONST("IOBASE")
#define INTERRUPT  NDIS_STRING_CONST("INTERRUPT")
#define MAX_MULTICAST_LIST  NDIS_STRING_CONST("MAXMULTICAST")
#define NETWORK_ADDRESS  NDIS_STRING_CONST("NETADDRESS")
#define BUS_TYPE  NDIS_STRING_CONST("BusType")

#else // NDIS3

#define IOADDRESS  NDIS_STRING_CONST("IoBaseAddress")
#define INTERRUPT  NDIS_STRING_CONST("InterruptNumber")
#define MAX_MULTICAST_LIST  NDIS_STRING_CONST("MaximumMulticastList")
#define NETWORK_ADDRESS  NDIS_STRING_CONST("NetworkAddress")
#define BUS_TYPE  NDIS_STRING_CONST("BusType")

#endif
