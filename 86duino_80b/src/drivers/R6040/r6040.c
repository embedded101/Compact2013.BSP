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
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:

    R6040.c

Abstract:

    This is the main file for the Novel 2000 Ethernet controller.
    This driver conforms to the NDIS 3.0 miniport interface.

--*/

#include "precomp.h"
#include "devload.h"
//#include <mkfuncs.h>
#include <giisr.h>
#include <ceddk.h>

#ifdef UNDER_CE
#include <resmgr.h>
#endif



//
// On debug builds tell the compiler to keep the symbols for
// internal functions, otw throw them out.
//
#if DBG
#define STATIC
#else
#define STATIC static
#endif

//
// Debugging definitions
//
#if DBG

//
// Default debug mode
//
ULONG R6040DebugFlag =
                       R6040_DEBUG_LOUD
                       | R6040_DEBUG_VERY_LOUD
                       // | R6040_DEBUG_LOG
                       // | R6040_DEBUG_CHECK_DUP_SENDS
                       // | R6040_DEBUG_TRACK_PACKET_LENS
                       // | R6040_DEBUG_WORKAROUND1
                       // | R6040_DEBUG_CARD_BAD
                       // | R6040_DEBUG_CARD_TESTS
                       ;

//
// Debug tracing defintions
//
#define R6040_LOG_SIZE 256
UCHAR R6040LogBuffer[R6040_LOG_SIZE]={0};
UINT R6040LogLoc = 0;

extern
VOID
R6040Log(UCHAR c) {

    R6040LogBuffer[R6040LogLoc++] = c;

    R6040LogBuffer[(R6040LogLoc + 4) % R6040_LOG_SIZE] = '\0';

    if (R6040LogLoc >= R6040_LOG_SIZE)
        R6040LogLoc = 0;
}

#endif



//
// This constant is used for places where NdisAllocateMemory
// needs to be called and the HighestAcceptableAddress does
// not matter.
//
NDIS_PHYSICAL_ADDRESS HighestAcceptableMax =
    NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);

//
// The global Miniport driver block.
//

DRIVER_BLOCK R6040MiniportBlock={0};

//
// List of supported OID for this driver.
//
STATIC UINT R6040SupportedOids[] = {
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_ID,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_GEN_MAXIMUM_SEND_PACKETS
    };

//
// Determines whether failing the initial card test will prevent
// the adapter from being registered.
//
#ifdef CARD_TEST

BOOLEAN InitialCardTest = TRUE;

#else  // CARD_TEST

BOOLEAN InitialCardTest = FALSE;

#endif // CARD_TEST

#ifdef DEBUG
//
// These defines must match the ZONE_* defines in R6040SW.H
//

DBGPARAM dpCurSettings = {
    TEXT("R6040"), {
    TEXT("Errors"),TEXT("Warnings"),TEXT("Functions"),TEXT("Init"),
    TEXT("Interrupts"),TEXT("Receives"),TEXT("Transmits"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),
    TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined"),TEXT("Undefined") },
    0x0009
};
#endif  // DEBUG

  NDIS_PHYSICAL_ADDRESS   PhyAddress;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#pragma NDIS_INIT_FUNCTION(DriverEntry)


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the primary initialization routine for the R6040 driver.
    It is simply responsible for the intializing the wrapper and registering
    the Miniport driver.  It then calls a system and architecture specific
    routine that will initialize and register each adapter.

Arguments:

    DriverObject - Pointer to driver object created by the system.

    RegistryPath - Path to the parameters for this driver in the registry.

Return Value:

    The status of the operation.

--*/

{


    //
    // Receives the status of the NdisMRegisterMiniport operation.
    //
    NDIS_STATUS Status;

    //
    // Characteristics table for this driver.
    //
    NDIS_MINIPORT_CHARACTERISTICS R6040Char;

    //
    // Pointer to the global information for this driver
    //
    PDRIVER_BLOCK NewDriver = &R6040MiniportBlock;

    //
    // Handle for referring to the wrapper about this driver.
    //
    NDIS_HANDLE NdisWrapperHandle;

    RETAILMSG(R6040DBG,
        (TEXT("+R6040:DriverEntry\r\n")));


    //
    // Initialize the wrapper.
    //
    NdisMInitializeWrapper(
                &NdisWrapperHandle,
                DriverObject,
                RegistryPath,
                NULL
                );

    //
    // Save the global information about this driver.
    //
    NewDriver->NdisWrapperHandle = NdisWrapperHandle;
    NewDriver->AdapterQueue = (PR6040_ADAPTER)NULL;

    //
    // Initialize the Miniport characteristics for the call to
    // NdisMRegisterMiniport.
    //
    NdisZeroMemory(&R6040Char,sizeof(R6040Char));
    R6040Char.MajorNdisVersion = R6040_NDIS_MAJOR_VERSION;
    R6040Char.MinorNdisVersion = R6040_NDIS_MINOR_VERSION;
    R6040Char.CheckForHangHandler = R6040CheckForHang;
    R6040Char.DisableInterruptHandler = R6040DisableInterrupt;
    R6040Char.EnableInterruptHandler = R6040EnableInterrupt;
    R6040Char.HaltHandler = R6040Halt;
    R6040Char.HandleInterruptHandler = R6040HandleInterrupt;
    R6040Char.InitializeHandler = R6040Initialize;
    R6040Char.ISRHandler = R6040Isr;
    R6040Char.QueryInformationHandler = R6040QueryInformation;
    R6040Char.ReconfigureHandler = NULL;
    R6040Char.ResetHandler = R6040Reset;
    R6040Char.SendHandler = R6040Send;
    R6040Char.SetInformationHandler = R6040SetInformation;
    R6040Char.TransferDataHandler =   R6040TransferData;

    Status = NdisMRegisterMiniport(
                 NdisWrapperHandle,
                 &R6040Char,
                 sizeof(R6040Char)
                 );

    if (Status == NDIS_STATUS_SUCCESS) {
        RETAILMSG(R6040DBG,
                 (TEXT("-R6040:DriverEntry: Success\r\n")));
        return STATUS_SUCCESS;

    }

    // Terminate the wrapper.
    NdisTerminateWrapper (R6040MiniportBlock.NdisWrapperHandle, NULL);
    R6040MiniportBlock.NdisWrapperHandle = NULL;

    RETAILMSG(R6040DBG,
        (TEXT("-R6040:DriverEntry: Unsuccessful\r\n")));
    return STATUS_UNSUCCESSFUL;

}


#pragma NDIS_PAGEABLE_FUNCTION(R6040Initialize)
extern
NDIS_STATUS
R6040Initialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE WrapperConfigurationContext
    )

/*++

Routine Description:

    R6040Initialize starts an adapter and registers resources with the
    wrapper.

Arguments:

    OpenErrorStatus - Extra status bytes for opening token ring adapters.

    SelectedMediumIndex - Index of the media type chosen by the driver.

    MediumArray - Array of media types for the driver to chose from.

    MediumArraySize - Number of entries in the array.

    MiniportAdapterHandle - Handle for passing to the wrapper when
       referring to this adapter.

    WrapperConfigurationContext - A handle to pass to NdisOpenConfiguration.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING

--*/

{
    //
    // Pointer to our newly allocated adapter.
    //
    PR6040_ADAPTER Adapter = NULL;

    //
    // The handle for reading from the registry.
    //
    NDIS_HANDLE ConfigHandle = NULL;

    //
    // The value read from the registry.
    //
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;

    //
    // String names of all the parameters that will be read.
    //
    NDIS_STRING IOAddressStr = IOADDRESS;
    NDIS_STRING InterruptStr = INTERRUPT;
    NDIS_STRING MaxMulticastListStr = MAX_MULTICAST_LIST;
    NDIS_STRING NetworkAddressStr = NETWORK_ADDRESS;
    NDIS_STRING BusTypeStr = NDIS_STRING_CONST("BusType");
    NDIS_STRING BusNumberStr = NDIS_STRING_CONST("BusNumber");

    NDIS_STRING InstanceIndexStr = NDIS_STRING_CONST("InstanceIndex");

    #define NDIS_STRING_CONST_L(x)    {sizeof(x)-2, sizeof(x), x}

    NDIS_STRING SysintrStr      = NDIS_STRING_CONST_L(DEVLOAD_SYSINTR_VALNAME);
    NDIS_STRING PlugAndPlayStr  = NDIS_STRING_CONST("PlugAndPlay");

    NDIS_STRING EightBitIOStr= NDIS_STRING_CONST("EightBitIO");
    //
    // TRUE if there is a configuration error.
    //
    BOOLEAN ConfigError = FALSE;

    //
    // A special value to log concerning the error.
    //
    ULONG ConfigErrorValue = 0;

    //
    // The slot number the adapter is located in, used for
    // Microchannel adapters.
    //
    UINT SlotNumber = 0;

    //
    // The network address the adapter should use instead of the
    // the default burned in address.
    //
    PVOID NetAddress;

    //
    // The number of bytes in the address.  It should be
    // R6040_LENGTH_OF_ADDRESS
    //
    ULONG Length;

    //
    // These are used when calling R6040RegisterAdapter.
    //

    //
    // The physical address of the base I/O port.
    //
    PVOID IoBaseAddr;

    //
    // The interrupt number to use.
    //
    UCHAR InterruptNumber;

    //
    // The number of multicast address to be supported.
    //
    UINT MaxMulticastList;

    ULONG  i;

    //
    // Status of Ndis calls.
    //
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    PNDIS_RESOURCE_LIST             pNdisResourceList;
    DWORD                           cbNdisResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResourceDesc;
    DWORD                           dwResourceCount;
    DWORD                           IoLen;

    RETAILMSG(R6040DBG,(TEXT("\r\nR6040Initialize:\r\n")));


    do
    {
        // Search for the medium type (802.3) in the given array.
        for (i = 0; i < MediumArraySize; i++)
        {
            if (MediumArray[i] == NdisMedium802_3)
            {
                break;
            }

        }

        if (i == MediumArraySize)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040:Initialize: No supported media\r\n")));
            Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
            break;
        }

        *SelectedMediumIndex = i;

        // Set default values.
        IoBaseAddr = DEFAULT_IOBASEADDR;
        InterruptNumber = DEFAULT_INTERRUPTNUMBER;
        MaxMulticastList = DEFAULT_MULTICASTLISTMAX;

        // Allocate memory for the adapter block now.
        Status = NdisAllocateMemory( (PVOID *)&Adapter,
                       sizeof(R6040_ADAPTER),
                       0,
                       HighestAcceptableMax
                       );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040:Initialize: NdisAllocateMemory(R6040_ADAPTER) failed\r\n")));
            break;
        }

        // Clear out the adapter block, which sets all default values to FALSE,
        // or NULL.

     NdisZeroMemory (Adapter, sizeof(R6040_ADAPTER));

        // Open the configuration space.
        NdisOpenConfiguration(
                &Status,
                &ConfigHandle,
                WrapperConfigurationContext
                );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040:Initialize: NdisOpenconfiguration failed 0x%x\r\n"), Status));
            break;
        }

        // Read net address
        NdisReadNetworkAddress(
                        &Status,
                        &NetAddress,
                        &Length,
                        ConfigHandle
                        );

        if ((Status == NDIS_STATUS_SUCCESS) &&
            (Length == R6040_LENGTH_OF_ADDRESS))
        {
            // Save the address that should be used.
            NdisMoveMemory(
                    Adapter->StationAddress,
                    NetAddress,
                    R6040_LENGTH_OF_ADDRESS
                    );
     RETAILMSG(R6040DBG,
        (TEXT("R6040: Kevin NetWork Address [ %02x-%02x-%02x-%02x-%02x-%02x ]\r\n"),
            Adapter->StationAddress[0],
            Adapter->StationAddress[1],
            Adapter->StationAddress[2],
            Adapter->StationAddress[3],
            Adapter->StationAddress[4],
            Adapter->StationAddress[5]));
        }

        //Read InstanceIndex
        NdisReadConfiguration(
                &Status,
                &ReturnedValue,
                ConfigHandle,
                &InstanceIndexStr,
                NdisParameterHexInteger
                );

        if (Status == NDIS_STATUS_SUCCESS)
        {
            Adapter->InstanceIndex = (UCHAR)ReturnedValue->ParameterData.IntegerData;
        }
              InstanceIndex = Adapter->InstanceIndex;

        //Update PHY address from Instance index
        Adapter->PhyAddr = Adapter->InstanceIndex;

        // Read Bus Type (for NE2/AE2 support)
        NdisReadConfiguration(
                &Status,
                &ReturnedValue,
                ConfigHandle,
                &BusTypeStr,
                NdisParameterHexInteger
                );

        if (Status == NDIS_STATUS_SUCCESS)
        {
            Adapter->BusType = (UCHAR)ReturnedValue->ParameterData.IntegerData;
        }

        //  Get the list of resources assigned to the adapter
        pNdisResourceList = NULL;
        cbNdisResourceList = 0;

        //  First call is just to determine buffer size
        NdisMQueryAdapterResources(
                &Status,
                WrapperConfigurationContext,
                pNdisResourceList,
                &cbNdisResourceList);

        Status = NdisAllocateMemory( (PVOID *)&pNdisResourceList,
                       cbNdisResourceList,
                       0,
                       HighestAcceptableMax
                       );
        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040:Initialize: NdisAllocateMemory(NDIS_RESOURCE_LIST) failed\r\n")));
            break;
        }

        NdisMQueryAdapterResources(
                &Status,
                WrapperConfigurationContext,
                pNdisResourceList,
                &cbNdisResourceList);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040:Initialize: NdisMQueryAdapterResources failed\r\n")));
            break;
        }

                NdisAllocateSpinLock(&Adapter->Lock);
                NdisAllocateSpinLock(&Adapter->SendLock);
                NdisAllocateSpinLock(&Adapter->RcvLock);



        //  Search for I/O address and IRQ in assigned resources.
        dwResourceCount = pNdisResourceList->Count;
        pResourceDesc = &(pNdisResourceList->PartialDescriptors[0]);

        while (dwResourceCount--)
        {
            switch(pResourceDesc->Type)
            {
            case CmResourceTypeInterrupt:
                InterruptNumber = (UCHAR)pResourceDesc->u.Interrupt.Vector;
                RETAILMSG(R6040DBG, (TEXT("R6040: Assigned InterruptNumber is %u\r\n"), InterruptNumber));
                break;

            case CmResourceTypePort:
                IoBaseAddr = (PVOID)pResourceDesc->u.Port.Start.LowPart;
                IoLen = pResourceDesc->u.Port.Length;
                Adapter->IoLen = IoLen;
                RETAILMSG(R6040DBG, (TEXT("R6040: Assigned Port is %u Len %u\r\n"), IoBaseAddr, IoLen));
                break;

             case CmResourceTypeMemory:
                  Adapter->MemPhysAddr = pResourceDesc->u.Memory.Start;
                  break;

            default:
                RETAILMSG(R6040DBG, (TEXT("R6040: Unexpected assigned resource type %u\r\n"), pResourceDesc->Type));
                break;
            }
            pResourceDesc++;
        }

        NdisFreeMemory(pNdisResourceList, cbNdisResourceList, 0);

        // Read MaxMulticastList
        NdisReadConfiguration(
                &Status,
                &ReturnedValue,
                ConfigHandle,
                &MaxMulticastListStr,
                NdisParameterInteger
                );

        if (Status == NDIS_STATUS_SUCCESS) {

            MaxMulticastList = ReturnedValue->ParameterData.IntegerData;
            if (ReturnedValue->ParameterData.IntegerData <= DEFAULT_MULTICASTLISTMAX)
                MaxMulticastList = ReturnedValue->ParameterData.IntegerData;
        }

        // Now to use this information and register with the wrapper
        // and initialize the adapter.
        RETAILMSG(R6040DBG, (TEXT("R6040:Registering adapter # buffers %ld\r\n"),
            DEFAULT_NUMBUFFERS));
        RETAILMSG(R6040DBG, (TEXT("R6040:Bus type: %d\n"), Adapter->BusType));
        RETAILMSG(R6040DBG, (TEXT("R6040:I/O base addr 0x%lx\n"), IoBaseAddr));
        RETAILMSG(R6040DBG, (TEXT("R6040:interrupt number %ld\n"),
            InterruptNumber));
        RETAILMSG(R6040DBG, (TEXT("R6040:max multicast %ld\n"),
            DEFAULT_MULTICASTLISTMAX));
        RETAILMSG(R6040DBG, (TEXT("R6040:InstanceIndex %ld\n"),
            Adapter->InstanceIndex));
               RETAILMSG(R6040DBG, (TEXT("R6040:PHY address %ld\n"),
            Adapter->PhyAddr));

        // Set up the parameters.
        Adapter->NumBuffers = DEFAULT_NUMBUFFERS;
        Adapter->IoBaseAddr = IoBaseAddr;

        Adapter->InterruptNumber = InterruptNumber;

        Adapter->MulticastListMax = MaxMulticastList;
        Adapter->MiniportAdapterHandle = MiniportAdapterHandle;

        Adapter->MaxLookAhead = R6040_MAX_LOOKAHEAD;

        //  Read the bus number..

        NdisReadConfiguration(
            &Status,
            &ReturnedValue,
            ConfigHandle,
            &BusNumberStr,
            NdisParameterHexInteger);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            Adapter->BusNumber = (DWORD)ReturnedValue->ParameterData.IntegerData;
        }
        else
        {
            Adapter->BusNumber = 0xffffffff;
        }

        //  Sysintr value will be available for plug and play mode.
        //  i.e. PCI.
        Adapter->IsrInfo.SysIntr = SYSINTR_INVALID;

        NdisReadConfiguration(
            &Status,
            &ReturnedValue,
            ConfigHandle,
            &SysintrStr,
            NdisParameterHexInteger);

       //Kevin Test
      /* Adapter->hISR = LoadIntChainHandler(
                                TEXT("giisr.dll"),
                                TEXT("ISRHandler"),
                                (BYTE)(ReturnedValue->ParameterData.IntegerData));
        if (!Adapter->hISR)
        {
            DEBUGMSG (ZONE_INIT, (TEXT("R6040: Failed to load giisr.dll.\r\n")));
            break;
        }
        //Kevin Test*/

        if (Status == NDIS_STATUS_SUCCESS)
        {
            Adapter->IsrInfo.SysIntr = (DWORD)ReturnedValue->ParameterData.IntegerData;
            RETAILMSG(R6040DBG, (TEXT("R6040:SysIntr %ld\r\n"),Adapter->IsrInfo.SysIntr));
        }

        NdisReadConfiguration(
            &Status,
            &ReturnedValue,
            ConfigHandle,
            &PlugAndPlayStr,
            NdisParameterHexInteger);

        if (Status == NDIS_STATUS_SUCCESS && (DWORD)(ReturnedValue->ParameterData.IntegerData) != 0x00)
        {
            Adapter->bPlugAndPlay = TRUE;
        }
        else
        {
            Adapter->bPlugAndPlay = FALSE;
        }

        // Now do the work.
        Status = R6040RegisterAdapter(Adapter,
                     WrapperConfigurationContext,
                     ConfigError,
                     ConfigErrorValue);

    } while (FALSE);

    if (ConfigHandle)
    {
        NdisCloseConfiguration(ConfigHandle);
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        RETAILMSG(R6040DBG, (TEXT("R6040:Initialize succeeded\n")));
    }
    else // failure
    {
        // Check for a configuration error
        if (ConfigError)
        {
            // Log Error and exit.
            NdisWriteErrorLogEntry(
                Adapter->MiniportAdapterHandle,
                NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
                1,
                ConfigErrorValue
                );
            RETAILMSG(R6040DBG,
                (TEXT("R6040:RegisterAdapter: R6040Initialize had a config error %d\r\n"),
                ConfigErrorValue));
        }

        if (Adapter) {
            NdisFreeMemory(Adapter, sizeof(R6040_ADAPTER), 0);
                        NdisFreeSpinLock(&Adapter->Lock);
                        NdisFreeSpinLock(&Adapter->SendLock);
                        NdisFreeSpinLock(&Adapter->RcvLock);
            }

                for ( i = 0; i < MAX_TX_DESCRIPTORS; i++)
                {
                   if(Adapter->Tx_ring[i].VirtualAddr != NULL)
                   {
                        NdisMFreeSharedMemory(
                          Adapter->MiniportAdapterHandle,
                          DS_SIZE + MAX_BUF_SIZE,
                          FALSE,
                          (PVOID )&Adapter->Tx_ring[i].VirtualAddr,
                          Adapter->Tx_ring[i].PhyAddr);
                   }
                 }

                 for ( i = 0; i < MAX_RX_DESCRIPTORS; i++)
                 {
                   if(Adapter->Rx_ring[i].VirtualAddr != NULL)
                   {
                     NdisMFreeSharedMemory(
                        Adapter->MiniportAdapterHandle,
                        DS_SIZE + MAX_BUF_SIZE,
                        FALSE,
                        (PVOID )&Adapter->Rx_ring[i].VirtualAddr,
                        Adapter->Rx_ring[i].PhyAddr);
                   }
                 }
    }

    return Status;
}


#pragma NDIS_PAGEABLE_FUNCTION(R6040RegisterAdapter)
NDIS_STATUS
R6040RegisterAdapter(
    IN PR6040_ADAPTER Adapter,
    IN NDIS_HANDLE WrapperConfigurationContext,
    IN BOOLEAN ConfigError,
    IN ULONG ConfigErrorValue
    )

/*++

Routine Description:

    Called when a new adapter should be registered. It allocates space for
    the adapter, initializes the adapter's block, registers resources
    with the wrapper and initializes the physical adapter.

Arguments:

    Adapter - The adapter structure.

    WrapperConfigurationContext - Handle passed to R6040Initialize.

    ConfigError - Was there an error during configuration reading.

    ConfigErrorValue - Value to log if there is an error.

Return Value:

    Indicates the success or failure of the registration.

--*/

{
    // General purpose return from NDIS calls
    NDIS_STATUS status;

    // check that NumBuffers <= MAX_XMIT_BUFS

    if (Adapter->NumBuffers > MAX_XMIT_BUFS)
        return(NDIS_STATUS_RESOURCES);

    // Inform the wrapper of the physical attributes of this adapter.
        NdisMSetAttributesEx(
            Adapter->MiniportAdapterHandle,
            (NDIS_HANDLE) Adapter,
            0,
            NDIS_ATTRIBUTE_DESERIALIZE | NDIS_ATTRIBUTE_BUS_MASTER | NDIS_ATTRIBUTE_ALWAYS_GIVES_RX_PACKET_OWNERSHIP ,
            NdisInterfacePci);

    /*NdisMSetAttributes(
        Adapter->MiniportAdapterHandle,
        (NDIS_HANDLE)Adapter,
        TRUE,
        Adapter->BusType
    );*/

    // Register the port addresses.
    status = NdisMRegisterIoPortRange(
                 (PVOID *)(&(Adapter->IoPAddr)),
                 Adapter->MiniportAdapterHandle,
                 (ULONG)Adapter->IoBaseAddr,
                 Adapter->IoLen
             );

    if (status != NDIS_STATUS_SUCCESS)
            return(status);

    if (Adapter->IoPAddr == 0)
        return NDIS_STATUS_FAILURE;

    R6040IoPAddr = Adapter->IoPAddr;

    // Initialize the card.
    RETAILMSG(R6040DBG,
        (TEXT("R6040:RegisterAdapter calling CardInitialize\r\n")));

    //Allocate Descriptor
    if(!CardAllocateTxRxDescriptor(Adapter))
    {
        status = NDIS_STATUS_ADAPTER_NOT_FOUND;
        goto fail2;
    }
    

    if (!CardInitialize(Adapter))
    {
        // Card seems to have failed.
        RETAILMSG(R6040DBG,
            (TEXT("R6040:RegisterAdapter CardInitialize -- Failed\r\n")));

        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_ADAPTER_NOT_FOUND,
            0
        );

        status = NDIS_STATUS_ADAPTER_NOT_FOUND;

        goto fail2;
    }

    RETAILMSG(R6040DBG,
        (TEXT("R6040:RegisterAdapter CardInitialize  -- Success\r\n")));

    // Initialize the receive variables.
    Adapter->NicReceiveConfig = RCR_REJECT_ERR;

    // Initialize the transmit buffer control.
    Adapter->CurBufXmitting = (XMIT_BUF)-1;

    // Read the Ethernet address off of the PROM.
    if (!CardReadEthernetAddress(Adapter))
    {
        RETAILMSG(R6040DBG,
            (TEXT("R6040:RegisterAdapter Could not read the ethernet address\r\n")));

        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_ADAPTER_NOT_FOUND,
            0
            );

        status = NDIS_STATUS_ADAPTER_NOT_FOUND;

        goto fail2;
    }

    // Now initialize the NIC and Gate Array registers.
    Adapter->NicInterruptMask = MIER_TXENDE | MIER_RXENDE;

    // Link us on to the chain of adapters for this driver.
    Adapter->NextAdapter = R6040MiniportBlock.AdapterQueue;
    R6040MiniportBlock.AdapterQueue = Adapter;


    RETAILMSG(R6040DBG, (TEXT("R6040:LoadIntChainHandler interrupt number %ld\r\n"),Adapter->InterruptNumber));

    //  If this is loaded by plug and play bus driver, then
    //  we need to load ISR.
    if (Adapter->bPlugAndPlay)
    {
        status = LoadISR(Adapter);

        if (status != NDIS_STATUS_SUCCESS)
        {
            RETAILMSG(R6040DBG,
                (TEXT("NE2000:: Failed LoadISR().\r\n")));

            goto fail3;
        }
    }

    // Disable the Interrupt
    CardBlockInterrupts(Adapter);

    // Initialize the interrupt.
    status = NdisMRegisterInterrupt(
                 &Adapter->Interrupt,
                 Adapter->MiniportAdapterHandle,
                 Adapter->InterruptNumber,
                 Adapter->InterruptNumber,
                 TRUE,
                 TRUE,
                 NdisInterruptLatched
             );

    if (status != NDIS_STATUS_SUCCESS)
    {
        RETAILMSG(R6040DBG,
            (TEXT("R6040:RegisterAdapter NdisMRegisterInterrupt failed 0x%x\r\n"), status));
        NdisWriteErrorLogEntry(
            Adapter->MiniportAdapterHandle,
            NDIS_ERROR_CODE_INTERRUPT_CONNECT,
            0
        );

        goto fail3;
    }

    RETAILMSG(R6040DBG,
        (TEXT("R6040:RegisterAdapter Interrupt Connected\r\n")));

    // register a shutdown handler for this card
    NdisMRegisterAdapterShutdownHandler(
        Adapter->MiniportAdapterHandle,     // miniport handle.
        Adapter,                            // shutdown context.
        R6040Shutdown                       // shutdown handler.
        );

    // Enable the Interrupt
    CardUnblockInterrupts(Adapter);


    // Initialization completed successfully.
    RETAILMSG(R6040DBG, (TEXT("R6040:RegisterAdapter OK\r\n")));

    return(NDIS_STATUS_SUCCESS);

    // Code to unwind what has already been set up when a part of
    // initialization fails, which is jumped into at various
    // points based on where the failure occured. Jumping to
    // a higher-numbered failure point will execute the code
    // for that block and all lower-numbered ones.
fail3:
   RETAILMSG(R6040DBG,
        (TEXT("R6040 : R6040RegisterAdapter  : fail13\r\n")));
    // Take us out of the AdapterQueue.

    if (R6040MiniportBlock.AdapterQueue == Adapter)
    {
        R6040MiniportBlock.AdapterQueue = Adapter->NextAdapter;
    }
    else
    {
        PR6040_ADAPTER TmpAdapter = R6040MiniportBlock.AdapterQueue;

        while (TmpAdapter->NextAdapter != Adapter)
        {
            TmpAdapter = TmpAdapter->NextAdapter;
        }

        TmpAdapter->NextAdapter = TmpAdapter->NextAdapter->NextAdapter;
    }

    //  If the ISR has been loaded, unload it.
    if (Adapter->hISR)
        UnloadISR(Adapter);

fail2:
   RETAILMSG(R6040DBG,
        (TEXT("R6040 : R6040RegisterAdapter : fail12\r\n")));

    NdisMDeregisterIoPortRange(
        Adapter->MiniportAdapterHandle,
        (ULONG)Adapter->IoBaseAddr,
        Adapter->IoLen,
        (PVOID)Adapter->IoPAddr
    );

    return(status);
}



extern
VOID
R6040Shutdown(
    IN NDIS_HANDLE MiniportAdapterContext
    )
/*++

Routine Description:

    R6040Shutdown is called to shut down the adapter. We need to unblock any
    threads which may be called in, and terminate any loops.  This function is
    called by NDIS.DLL when a card removal is detected.  At that
    point, any access to the R6040 registers may return random data, as the bus
    is floating.

Arguments:

    MiniportAdapterContext - The context value that the Miniport returned
        from R6040Initialize; actually as pointer to an R6040_ADAPTER.

Return Value:

    None.

--*/
{
    PR6040_ADAPTER Adapter;

    Adapter = PR6040_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    Adapter->ShuttingDown = TRUE;
    // Shut down the chip.  Note that the card may not be present, so this
    // routine should not do anything that might hang.
RETAILMSG(R6040DBG,
        (TEXT("R6040:R6040Shutdown\n")));

    CardStop(Adapter);
}


extern
VOID
R6040Halt(
    IN NDIS_HANDLE MiniportAdapterContext
    )

/*++

Routine Description:

    R6040Halt removes an adapter that was previously initialized.

Arguments:

    MiniportAdapterContext - The context value that the Miniport returned
        from R6040Initialize; actually as pointer to an R6040_ADAPTER.

Return Value:

    None.

--*/

{
    PR6040_ADAPTER Adapter;
    int   i;
    BOOLEAN bTimer = FALSE;
    Adapter = PR6040_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

RETAILMSG(R6040DBG,
        (TEXT("R6040:R6040Halt\r\n")));

    // Shut down the chip.
    CardStop(Adapter);

    //  Unload the ISR if necessary.
    if (Adapter->hISR)
        UnloadISR(Adapter);

    // Disconnect the interrupt line.
    NdisMDeregisterInterrupt(&Adapter->Interrupt);

    // Pause, waiting for any DPC stuff to clear.
    //NdisStallExecution(250000);
    Sleep(250);

    NdisMDeregisterIoPortRange(Adapter->MiniportAdapterHandle,
                               (ULONG)Adapter->IoBaseAddr,
                               Adapter->IoLen,
                               (PVOID)Adapter->IoPAddr
                               );

    // Remove the adapter from the global queue of adapters.
    if (R6040MiniportBlock.AdapterQueue == Adapter) {
        R6040MiniportBlock.AdapterQueue = Adapter->NextAdapter;
    } else {
        PR6040_ADAPTER TmpAdapter = R6040MiniportBlock.AdapterQueue;
        while (TmpAdapter->NextAdapter != Adapter) {
            TmpAdapter = TmpAdapter->NextAdapter;
        }
        TmpAdapter->NextAdapter = TmpAdapter->NextAdapter->NextAdapter;
    }

    // was this the last NIC?
    if ((R6040MiniportBlock.AdapterQueue == NULL) &&
        (R6040MiniportBlock.NdisWrapperHandle)) {
        NdisTerminateWrapper (R6040MiniportBlock.NdisWrapperHandle, NULL);
        R6040MiniportBlock.NdisWrapperHandle = NULL;
    }

    NdisFreeMemory(Adapter, sizeof(R6040_ADAPTER), 0);

    NdisFreeSpinLock(&Adapter->Lock);
    NdisFreeSpinLock(&Adapter->SendLock);
    NdisFreeSpinLock(&Adapter->RcvLock);

    for ( i = 0; i < MAX_TX_DESCRIPTORS; i++)
    {
        if(Adapter->Tx_ring[i].VirtualAddr != NULL)
        {
           NdisMFreeSharedMemory(
            Adapter->MiniportAdapterHandle,
             DS_SIZE + MAX_BUF_SIZE,
             FALSE,
            (PVOID )&Adapter->Tx_ring[i].VirtualAddr,
            Adapter->Tx_ring[i].PhyAddr);
        }
    }
    for ( i = 0; i < MAX_RX_DESCRIPTORS; i++)
    {
        if(Adapter->Rx_ring[i].VirtualAddr != NULL)
        {
           NdisMFreeSharedMemory(
            Adapter->MiniportAdapterHandle,
             DS_SIZE + MAX_BUF_SIZE,
             FALSE,
            (PVOID )&Adapter->Rx_ring[i].VirtualAddr,
            Adapter->Rx_ring[i].PhyAddr);
        }
    }
}

NDIS_STATUS
R6040Reset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
    )
/*++

Routine Description:

    The R6040Reset request instructs the Miniport to issue a hardware reset
    to the network adapter.  The driver also resets its software state.  See
    the description of NdisMReset for a detailed description of this request.

Arguments:

    AddressingReset - Does the adapter need the addressing information reloaded.

    MiniportAdapterContext - Pointer to the adapter structure.

Return Value:

    The function value is the status of the operation.

--*/

{
    // Pointer to the adapter structure.
    PR6040_ADAPTER Adapter = (PR6040_ADAPTER)MiniportAdapterContext;

    // Temporary looping variable

   RETAILMSG(R6040DBG,
        (TEXT("R6040 : R6040Reset\r\n")));

    // Clear the values for transmits, they will be reset these for after
    // the reset is completed.

    Adapter->FirstPacket = NULL;
    Adapter->LastPacket = NULL;

    // Physically reset the card.
    Adapter->NicInterruptMask = MIER_RXENDE | MIER_TXENDE;

    return (CardReset(Adapter) ? NDIS_STATUS_SUCCESS : NDIS_STATUS_FAILURE);
}

NDIS_STATUS
R6040QueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
)

/*++

Routine Description:

    The R6040QueryInformation process a Query request for
    NDIS_OIDs that are specific about the Driver.

Arguments:

    MiniportAdapterContext - a pointer to the adapter.

    Oid - the NDIS_OID to process.

    InformationBuffer -  a pointer into the
    NdisRequest->InformationBuffer into which store the result of the query.

    InformationBufferLength - a pointer to the number of bytes left in the
    InformationBuffer.

    BytesWritten - a pointer to the number of bytes written into the
    InformationBuffer.

    BytesNeeded - If there is not enough room in the information buffer
    then this will contain the number of bytes needed to complete the
    request.

Return Value:

    The function value is the status of the operation.

--*/
{
    // Pointer to the adapter structure.
    PR6040_ADAPTER Adapter = (PR6040_ADAPTER)MiniportAdapterContext;

    //   General Algorithm:
    //
    //      Switch(Request)
    //         Get requested information
    //         Store results in a common variable.
    //      default:
    //         Try protocol query information
    //         If that fails, fail query.
    //
    //      Copy result in common variable to result buffer.
    //   Finish processing

    UINT BytesLeft = InformationBufferLength;
    PUCHAR InfoBuffer = (PUCHAR)(InformationBuffer);
    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;
    NDIS_HARDWARE_STATUS HardwareStatus = NdisHardwareStatusReady;
    NDIS_MEDIUM Medium = NdisMedium802_3;

    // This variable holds result of query
    USHORT Phy_Mode;
    ULONG GenericULong;
    USHORT GenericUShort;
    UCHAR GenericArray[6];
    UINT MoveBytes = sizeof(ULONG);
    PVOID MoveSource = (PVOID)(&GenericULong);


    // Make sure that int is 4 bytes.  Else GenericULong must change
    // to something of size 4.
    ASSERT(sizeof(ULONG) == 4);

    // Switch on request type
    switch (Oid) {

    case OID_GEN_VENDOR_DRIVER_VERSION:
        GenericULong = (DRIVER_MAJOR_VERSION << 16 | DRIVER_MINOR_VERSION);
        break;

    case OID_GEN_MAC_OPTIONS:
        GenericULong = (ULONG)(NDIS_MAC_OPTION_TRANSFERS_NOT_PEND  |
                               NDIS_MAC_OPTION_RECEIVE_SERIALIZED  |
                               NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                               NDIS_MAC_OPTION_NO_LOOPBACK
                               );

        break;

    case OID_GEN_SUPPORTED_LIST:
        MoveSource = (PVOID)(R6040SupportedOids);
        MoveBytes = sizeof(R6040SupportedOids);
        break;

    case OID_GEN_HARDWARE_STATUS:
        HardwareStatus = NdisHardwareStatusReady;
        MoveSource = (PVOID)(&HardwareStatus);
        MoveBytes = sizeof(NDIS_HARDWARE_STATUS);

        break;

    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:
        MoveSource = (PVOID) (&Medium);
        MoveBytes = sizeof(NDIS_MEDIUM);
        break;

    case OID_GEN_MEDIA_CONNECT_STATUS:
RETAILMSG(R6040DBG,
        (TEXT("R6040: OID_GEN_MEDIA_CONNECT_STATUS\r\n")));

       // GenericULong = Adapter->MediaState;

         NdisDprAcquireSpinLock(&Adapter->Lock);
         Phy_Mode = PHY_Mode_Chk(Adapter);
         NdisDprReleaseSpinLock(&Adapter->Lock);

         if(Phy_Mode == LINK_FAILED)
         {
            GenericULong = NdisMediaStateDisconnected ;
            Adapter->MediaState = NdisMediaStateDisconnected;
            }
         else
         {
            GenericULong = NdisMediaStateConnected;
            Adapter->MediaState = NdisMediaStateConnected;
            }
        break;

    case OID_GEN_MAXIMUM_LOOKAHEAD:
        GenericULong = R6040_MAX_LOOKAHEAD;
        break;


    case OID_GEN_MAXIMUM_FRAME_SIZE:
        GenericULong = (ULONG)(1514 - R6040_HEADER_SIZE);
        break;

    case OID_GEN_MAXIMUM_TOTAL_SIZE:
        GenericULong = (ULONG)(1514);
        break;

    case OID_GEN_LINK_SPEED:
        GenericULong = (ULONG)(1000000);
        break;

    case OID_GEN_TRANSMIT_BUFFER_SPACE:
        GenericULong = (ULONG)2048;
        break;

    case OID_GEN_RECEIVE_BUFFER_SPACE:
        GenericULong = (ULONG)2048;
        break;

    case OID_GEN_TRANSMIT_BLOCK_SIZE:
        GenericULong = (ULONG)1024;
        break;

    case OID_GEN_RECEIVE_BLOCK_SIZE:
        GenericULong = (ULONG)1024;
        break;

    case OID_GEN_VENDOR_ID:
        NdisMoveMemory(
            (PVOID)&GenericULong,
            Adapter->PermanentAddress,
            3
            );
        GenericULong &= 0xFFFFFF00;
        GenericULong |= 0x01;
        MoveSource = (PVOID)(&GenericULong);
        MoveBytes = sizeof(GenericULong);
        break;

    case OID_GEN_VENDOR_DESCRIPTION:
        MoveSource = (PVOID)"R6040 Ethernet Adapter.";
        MoveBytes = 21;
        break;

    case OID_GEN_DRIVER_VERSION:
        GenericUShort = ((USHORT)R6040_NDIS_MAJOR_VERSION << 8) |
                R6040_NDIS_MINOR_VERSION;

        MoveSource = (PVOID)(&GenericUShort);
        MoveBytes = sizeof(GenericUShort);
        break;

    case OID_GEN_CURRENT_PACKET_FILTER:
        GenericULong = (ULONG)(Adapter->PacketFilter);
        break;

    case OID_GEN_CURRENT_LOOKAHEAD:
        GenericULong = (ULONG)(Adapter->MaxLookAhead);
        break;

    case OID_802_3_PERMANENT_ADDRESS:
        R6040_MOVE_MEM((PCHAR)GenericArray,
                    Adapter->PermanentAddress,
                    R6040_LENGTH_OF_ADDRESS);

        MoveSource = (PVOID)(GenericArray);
        MoveBytes = sizeof(Adapter->PermanentAddress);

        break;

    case OID_802_3_CURRENT_ADDRESS:
        R6040_MOVE_MEM((PCHAR)GenericArray,
                    Adapter->StationAddress,
                    R6040_LENGTH_OF_ADDRESS);

        MoveSource = (PVOID)(GenericArray);
        MoveBytes = sizeof(Adapter->StationAddress);

        break;

    case OID_802_3_MULTICAST_LIST:
        MoveSource = (PVOID)(Adapter->Addresses);
        MoveBytes = Adapter->NumMulticastAddressesInUse * R6040_LENGTH_OF_ADDRESS;
        break;

    case OID_802_3_MAXIMUM_LIST_SIZE:
        GenericULong = (ULONG) (Adapter->MulticastListMax);
        break;

    case OID_GEN_XMIT_OK:
        GenericULong = (UINT)(Adapter->FramesXmitGood);
        break;

    case OID_GEN_RCV_OK:
        GenericULong = (UINT)(Adapter->FramesRcvGood);
        break;

    case OID_GEN_XMIT_ERROR:
        GenericULong = (UINT)(Adapter->FramesXmitBad);
        break;

    case OID_GEN_RCV_ERROR:
        GenericULong = (UINT)(Adapter->CrcErrors);
        break;

    case OID_GEN_RCV_NO_BUFFER:
        GenericULong = (UINT)(Adapter->MissedPackets);
        break;

    case OID_802_3_RCV_ERROR_ALIGNMENT:
        GenericULong = (UINT)(Adapter->FrameAlignmentErrors);
        break;

    case OID_802_3_XMIT_ONE_COLLISION:
        GenericULong = (UINT)(Adapter->FramesXmitOneCollision);
        break;

    case OID_802_3_XMIT_MORE_COLLISIONS:
        GenericULong = (UINT)(Adapter->FramesXmitManyCollisions);
        break;

    case OID_GEN_MAXIMUM_SEND_PACKETS:
        GenericULong = 1;
        break;

    default:
        StatusToReturn = NDIS_STATUS_INVALID_OID;
        break;

    }

    if (StatusToReturn == NDIS_STATUS_SUCCESS) {
        if (MoveBytes > BytesLeft) {
            // Not enough room in InformationBuffer.
            *BytesNeeded = MoveBytes;
            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        } else {
            // Store result.
            R6040_MOVE_MEM(InfoBuffer, MoveSource, MoveBytes);
            *BytesWritten = MoveBytes;
        }
    }

    return StatusToReturn;
}

extern
NDIS_STATUS
R6040SetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    )

/*++

Routine Description:

    R6040SetInformation handles a set operation for a
    single OID.

Arguments:

    MiniportAdapterContext - Context registered with the wrapper, really
        a pointer to the adapter.

    Oid - The OID of the set.

    InformationBuffer - Holds the data to be set.

    InformationBufferLength - The length of InformationBuffer.

    BytesRead - If the call is successful, returns the number
        of bytes read from InformationBuffer.

    BytesNeeded - If there is not enough data in InformationBuffer
        to satisfy the OID, returns the amount of storage needed.

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_PENDING
    NDIS_STATUS_INVALID_LENGTH
    NDIS_STATUS_INVALID_OID

--*/
{
    // Pointer to the adapter structure.
    PR6040_ADAPTER Adapter = (PR6040_ADAPTER)MiniportAdapterContext;

    // General Algorithm:
    //
    //     Verify length
    //     Switch(Request)
    //        Process Request


    UINT BytesLeft = InformationBufferLength;
    const UCHAR *InfoBuffer = (const UCHAR *)(InformationBuffer);

    // Variables for a particular request
    UINT OidLength;

    // Variables for holding the new values to be used.
    ULONG LookAhead;
    ULONG Filter;

    // Status of the operation.
    NDIS_STATUS StatusToReturn = NDIS_STATUS_SUCCESS;
    int i;

    // Get Oid and Length of request
    OidLength = BytesLeft;

    switch (Oid) {

    case OID_802_3_MULTICAST_LIST:
RETAILMSG(R6040DBG,
        (TEXT("R6040: OID_802_3_MULTICAST_LIST\r\n")));
        // Verify length
        if ((OidLength % R6040_LENGTH_OF_ADDRESS) != 0){

            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;

            *BytesRead = 0;
            *BytesNeeded = 0;
            break;
        }
        OidLength = min(sizeof(Adapter->Addresses), OidLength);

/*
         * Clear old multicast entries, if any, before accepting a
         * set of new ones.
         */

        NdisZeroMemory(Adapter->Addresses,sizeof(Adapter->Addresses));

        // Set the new list on the adapter.
        NdisMoveMemory(Adapter->Addresses, InfoBuffer, OidLength);
        Adapter->NumMulticastAddressesInUse = OidLength / R6040_LENGTH_OF_ADDRESS;

        RETAILMSG(R6040DBG, (TEXT("R6040:MulticastAddressesInUse =-> %d\r\n"),Adapter->NumMulticastAddressesInUse));

        for(i=0;i<(int)Adapter->NumMulticastAddressesInUse;i++)
        {
          RETAILMSG(R6040DBG, (TEXT("R6040:MulticastAddresses %d = %02x:%02x:%02x:%02x:%02x:%02x\r\n"),
                       i, Adapter->Addresses[i][0],Adapter->Addresses[i][1],Adapter->Addresses[i][2],
                       Adapter->Addresses[i][3],Adapter->Addresses[i][4],Adapter->Addresses[i][5]));
        }


        //  If we are currently receiving all multicast or
        //  we are promiscuous then we DO NOT call this, or
        //  it will reset those settings.
        if
        (
            !(Adapter->PacketFilter & (NDIS_PACKET_TYPE_ALL_MULTICAST |
                                       NDIS_PACKET_TYPE_PROMISCUOUS))
        )
        {
            NdisDprAcquireSpinLock(&Adapter->Lock);
            NdisDprAcquireSpinLock(&Adapter->RcvLock);
            StatusToReturn = DispatchSetMulticastAddressList(Adapter);
            NdisDprReleaseSpinLock(&Adapter->RcvLock);
            NdisDprReleaseSpinLock(&Adapter->Lock);
        }
        else
        {
            //  Our list of multicast addresses is kept by the
            //  wrapper.
            StatusToReturn = NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_CURRENT_PACKET_FILTER:
RETAILMSG(R6040DBG,
        (TEXT("R6040: OID_GEN_CURRENT_PACKET_FILTER\r\n")));

        // Verify length
        if (OidLength != 4 ) {
            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
            *BytesRead = 0;
            *BytesNeeded = 0;
            break;
        }

        R6040_MOVE_MEM(&Filter, InfoBuffer, 4);
        // Verify bits
        if (Filter & (NDIS_PACKET_TYPE_SOURCE_ROUTING |
              NDIS_PACKET_TYPE_SMT |
              NDIS_PACKET_TYPE_MAC_FRAME |
              NDIS_PACKET_TYPE_FUNCTIONAL |
              NDIS_PACKET_TYPE_ALL_FUNCTIONAL |
              NDIS_PACKET_TYPE_GROUP
             )) {
            StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
            *BytesRead = 4;
            *BytesNeeded = 0;
            break;
        }
        // Set the new value on the adapter.
        Adapter->PacketFilter = Filter;
        NdisDprAcquireSpinLock(&Adapter->Lock);
        NdisDprAcquireSpinLock(&Adapter->RcvLock);
        StatusToReturn = DispatchSetPacketFilter(Adapter);
        NdisDprReleaseSpinLock(&Adapter->RcvLock);
        NdisDprReleaseSpinLock(&Adapter->Lock);
        StatusToReturn = NDIS_STATUS_SUCCESS;  //Kevin
        break;

    case OID_GEN_CURRENT_LOOKAHEAD:

        // Verify length
        if (OidLength != 4) {
            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
            *BytesRead = 0;
            *BytesNeeded = 0;
            break;
        }

        // Store the new value.
        R6040_MOVE_MEM(&LookAhead, InfoBuffer, 4);

        if (LookAhead <= R6040_MAX_LOOKAHEAD) {
            Adapter->MaxLookAhead = LookAhead;
        } else {
            StatusToReturn = NDIS_STATUS_INVALID_LENGTH;
        }

        break;

    default:
        StatusToReturn = NDIS_STATUS_INVALID_OID;
        *BytesRead = 0;
        *BytesNeeded = 0;
        break;
    }

    if (StatusToReturn == NDIS_STATUS_SUCCESS) {
        *BytesRead = BytesLeft;
        *BytesNeeded = 0;
    }
    return(StatusToReturn);
}

NDIS_STATUS
DispatchSetPacketFilter(
    IN PR6040_ADAPTER Adapter
    )

/*++

Routine Description:

    Sets the appropriate bits in the adapter filters
    and modifies the card Receive Configuration Register if needed.

Arguments:

    Adapter - Pointer to the adapter block

Return Value:

    The final status (always NDIS_STATUS_SUCCESS).

Notes:

  - Note that to receive all multicast packets the multicast
    registers on the card must be filled with 1's. To be
    promiscuous that must be done as well as setting the
    promiscuous physical flag in the RCR. This must be done
    as long as ANY protocol bound to this adapter has their
    filter set accordingly.

--*/


{
    // See what has to be put on the card.
    if
    (
        Adapter->PacketFilter & (NDIS_PACKET_TYPE_ALL_MULTICAST |
                                 NDIS_PACKET_TYPE_PROMISCUOUS)
    )
    {
        // need "all multicast" now.
        CardSetAllMulticast(Adapter);    // fills it with 1's
    }
    else
    {
        // No longer need "all multicast".
        DispatchSetMulticastAddressList(Adapter);
    }
    // The multicast bit in the RCR should be on if ANY protocol wants
    // multicast/all multicast packets (or is promiscuous).
    if
    (
        Adapter->PacketFilter & (NDIS_PACKET_TYPE_ALL_MULTICAST |
                                 NDIS_PACKET_TYPE_MULTICAST |
                                 NDIS_PACKET_TYPE_PROMISCUOUS)
    )
    {
RETAILMSG(R6040DBG,
        (TEXT("R6040: Filter The multicast bit\r\n")));
        Adapter->NicReceiveConfig |= RCR_MULTICAST;
    }
    else
    {
        Adapter->NicReceiveConfig &= ~RCR_MULTICAST;
    }

    // The promiscuous physical bit in the RCR should be on if ANY
    // protocol wants to be promiscuous.
    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
    {
RETAILMSG(R6040DBG,
        (TEXT("R6040: Filter The promiscuous bit\r\n")));
        Adapter->NicReceiveConfig |= RCR_ALL_PHYS;
    }
    else
    {
        Adapter->NicReceiveConfig &= ~RCR_ALL_PHYS;
    }

    // The broadcast bit in the RCR should be on if ANY protocol wants
    // broadcast packets (or is promiscuous).
    if
    (
        Adapter->PacketFilter & (NDIS_PACKET_TYPE_BROADCAST |
                                 NDIS_PACKET_TYPE_PROMISCUOUS)
    )
    {
RETAILMSG(R6040DBG,
        (TEXT("R6040: Filter The broadcast bit\r\n")));
        Adapter->NicReceiveConfig |= RCR_BROADCAST;
    }
    else
    {
        Adapter->NicReceiveConfig &= ~RCR_BROADCAST;
    }

    CardSetReceiveConfig(Adapter);

    return(NDIS_STATUS_SUCCESS);
}



NDIS_STATUS
DispatchSetMulticastAddressList(
    IN PR6040_ADAPTER Adapter
    )

/*++

Routine Description:

    Sets the multicast list for this open

Arguments:

    Adapter - Pointer to the adapter block

Return Value:

    NDIS_STATUS_SUCESS

Implementation Note:

    When invoked, we are to make it so that the multicast list in the filter
    package becomes the multicast list for the adapter. To do this, we
    determine the required contents of the NIC multicast registers and
    update them.


--*/
{
    // Update the local copy of the NIC multicast regs and copy them to the NIC
    //CardFillMulticastRegs(Adapter);
    //CardCopyMulticastRegs(Adapter);
    CardSetMulticast(Adapter,Adapter->NumMulticastAddressesInUse,&Adapter->Addresses[0][0]);

    return NDIS_STATUS_SUCCESS;
}


BOOLEAN R6040CheckForHang(
    IN  NDIS_HANDLE     MiniportAdapterContext
    )
/*++

Routine Description:

    MiniportCheckForHang handler

Arguments:

    MiniportAdapterContext  Pointer to our adapter

Return Value:

    TRUE    This NIC needs a reset
    FALSE   Everything is fine

--*/
{
    PR6040_ADAPTER         Adapter = (PR6040_ADAPTER) MiniportAdapterContext;
    NDIS_MEDIA_STATE    CurrMediaState;
    USHORT              Phy_Mode;
    NDIS_STATUS         Status;

    NdisDprAcquireSpinLock(&Adapter->Lock);

   Phy_Mode = PHY_Mode_Chk(Adapter);

   if(Phy_Mode == LINK_FAILED)
       CurrMediaState = NdisMediaStateDisconnected ;
   else
       CurrMediaState = NdisMediaStateConnected;

    if (CurrMediaState != Adapter->MediaState)
    {
        Adapter->MediaState = CurrMediaState;
        //RETAILMSG(R6040DBG, (TEXT("Media state changed to %s\r\n"),
          //  ((CurrMediaState == NdisMediaStateConnected)?
            //TEXT("Connected"): TEXT("Disconnected"))));

        Status = (CurrMediaState == NdisMediaStateConnected) ?
                 NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT;

        NdisDprReleaseSpinLock(&Adapter->Lock);

        // Indicate the media event
        NdisMIndicateStatus(Adapter->MiniportAdapterHandle, Status, (PVOID)0, 0);

        NdisMIndicateStatusComplete(Adapter->MiniportAdapterHandle);
    }
    else
    {
        NdisDprReleaseSpinLock(&Adapter->Lock);
    }
    return(FALSE);

}

//
// Standard Windows DLL entrypoint.
// Since Windows CE NDIS miniports are implemented as DLLs, a DLL entrypoint is
// needed.
//
BOOL __stdcall
DllEntry(
  HANDLE hDLL,
  DWORD dwReason,
  LPVOID lpReserved
)
{

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DEBUGREGISTER(hDLL);
        RETAILMSG(R6040DBG, (TEXT("R6040: DLL_PROCESS_ATTACH\r\n")));
    DisableThreadLibraryCalls((HMODULE) hDLL);
        break;
    case DLL_PROCESS_DETACH:
        RETAILMSG(R6040DBG, (TEXT("R6040: DLL_PROCESS_DETACH\r\n")));
        break;
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//  LoadISR()
//
//  Routine Description:
//
//      This routine is going to load Interrupt Service Handler into nk.exe
//      space.
//      The ISR (R6040ISR.DLL) decides if given interrupt is our interrupt.
//      This applies only to PCI bus adapter.
//
//  Arguments:
//
//      Adapter :: R6040_ADAPTER strucuture..
//
//
//  Return Value:
//
//      NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_FAILURE otherwise
//

NDIS_STATUS
LoadISR(PR6040_ADAPTER Adapter)
{
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;
    BOOL                dwAddressSpace;
    GIISR_INFO Info;
    DWORD inIoSpace = 1;    // io space
    PHYSICAL_ADDRESS PortAddress = {(DWORD)Adapter->IoBaseAddr, 0};

    RETAILMSG(R6040DBG,
        (TEXT("R6040:LoadISR\r\n")));

   do
    {
        if (Adapter->InterruptNumber >= 0xFF)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040:: Platform does not support installable ISR.\r\n")));
            Adapter->hISR = NULL;
            Status = NDIS_STATUS_SUCCESS;
            break;
        }


        Adapter->hISR = LoadIntChainHandler(
                            TEXT("giisr.dll"),
                            TEXT("ISRHandler"),
                            Adapter->InterruptNumber);

        if (Adapter->hISR == NULL)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

            if (!TransBusAddrToStatic(Adapter->BusType,
                                      Adapter->BusNumber,
                                      PortAddress,
                                      Adapter->IoLen,
                                      &dwAddressSpace,
                                      (PPVOID)&(Adapter->IsrInfo.PortAddr)))
            {
                RETAILMSG(R6040DBG, (L"R6040:LoadISR - Failed TransBusAddrToStatic\r\n"));
                return FALSE;
            }

            RETAILMSG(R6040DBG, (L"Installed ISR handler, Irq = %d, PhysAddr = 0x%x\r\n",Adapter->InterruptNumber, Adapter->IsrInfo.PortAddr));

            // Set up ISR handler
            Info.SysIntr = Adapter->IsrInfo.SysIntr;
            Info.CheckPort = TRUE;
            Info.PortIsIO = TRUE;
            Info.UseMaskReg = FALSE;
            Info.PortAddr = (DWORD)Adapter->IsrInfo.PortAddr + NIC_INTR_STATUS;
            Info.PortSize = sizeof(SHORT);
            Info.Mask = Adapter->NicInterruptMask;

            if (!KernelLibIoControl(Adapter->hISR,
                                   IOCTL_GIISR_INFO,
                                   &Info,
                                   sizeof(Info),
                                   NULL,
                                   0,
                                   NULL))
            {
                RETAILMSG(R6040DBG, (L"R6040:LoadISR - Failed KernelLibIoControl\r\n"));
                break;
            }

        Status = NDIS_STATUS_SUCCESS;
    }while(FALSE);


    return Status;

}   //  LoadISR();



////////////////////////////////////////////////////////////////////////////////
//  UnloadISR()
//
//  Routine Description:
//
//      Unload the ISR loaded via LoadISR()
//
//  Arguments:
//
//      Adapter :: R6040_ADAPTER strucuture..
//
//
//  Return Value:
//
//      NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_FAILURE otherwise
//

NDIS_STATUS
UnloadISR(PR6040_ADAPTER Adapter)
{
    //
    //  If the ISR has been loaded, unload it.
    //
    RETAILMSG(R6040DBG,
        (TEXT("R6040:UnloadISR\r\n")));

   if (Adapter->hISR)
    {
        FreeIntChainHandler(Adapter->hISR);
        Adapter->hISR = NULL;
    }

    return NDIS_STATUS_SUCCESS;

}   //  UnloadISR()
