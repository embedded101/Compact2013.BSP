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

    interrup.c

Abstract:

    This is a part of the driver for the National Semiconductor Novell 2000
    Ethernet controller.  It contains the interrupt-handling routines.
    This driver conforms to the NDIS 3.0 interface.

--*/

#include "precomp.h"

//
// On debug builds tell the compiler to keep the symbols for
// internal functions, otw throw them out.
//
#if DBG
#define STATIC
#else
#define STATIC static
#endif



INDICATE_STATUS R6040IndicatePacket(IN PR6040_ADAPTER Adapter, IN INT  Lan );
void R6040DoNextSend(PR6040_ADAPTER Adapter);

//
// This is used to pad short packets.
//
static UCHAR BlankBuffer[60] = "                                                            ";


/***************************************************************************
Routine Description:

    This routine is used to turn on the interrupt mask.

Arguments:

    Context - The adapter for the R6040 to start.

Return Value:

    None.
*****************************************************************************/
void R6040EnableInterrupt(IN NDIS_HANDLE MiniportAdapterContext)
{
    PR6040_ADAPTER Adapter = (PR6040_ADAPTER)(MiniportAdapterContext);

    //RETAILMSG(R6040DBG, (TEXT("R6040EnableInterrupt(): Call CardUnblockInterrupts\r\n")));

    CardUnblockInterrupts(Adapter);
}


/***************************************************************************
Routine Description:

    This routine is used to turn off the interrupt mask.

Arguments:

    Context - The adapter for the R6040 to start.

Return Value:

    None.
*****************************************************************************/
void R6040DisableInterrupt(IN NDIS_HANDLE MiniportAdapterContext)
{
    PR6040_ADAPTER Adapter = (PR6040_ADAPTER)(MiniportAdapterContext);

    //RETAILMSG(R6040DBG, (TEXT("R6040DisableInterrupt(): Call CardBlockInterrupts\r\n")));

    CardBlockInterrupts(Adapter);
}


/***************************************************************************
Routine Description:

    This is the interrupt handler which is registered with the operating
    system. If several are pending (i.e. transmit complete and receive),
    handle them all.  Block new interrupts until all pending interrupts
    are handled.

Arguments:

    InterruptRecognized - Boolean value which returns TRUE if the
        ISR recognizes the interrupt as coming from this adapter.

    QueueDpc - TRUE if a DPC should be queued.

    Context - pointer to the adapter object

Return Value:

    None.
*****************************************************************************/
void R6040Isr(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueDpc,
    IN PVOID Context)
{
    PR6040_ADAPTER Adapter = ((PR6040_ADAPTER)Context);

    //RETAILMSG(R6040DBG, (TEXT("R6040Isr()\r\n")));

    // Force the INT signal from the chip low. When all
    // interrupts are acknowledged interrupts will be unblocked,
    KernelLibIoControl(
        Adapter->hISR,
        IOCTL_GIISR_PORTVALUE,
        NULL,
        0x00,
        &Adapter->InterruptStatus,
        sizeof(USHORT),
        NULL);

    //RETAILMSG(R6040DBG,
    //    (TEXT("R6040Isr(): Adapter->InterruptStatus : 0x%04x\r\n"), Adapter->InterruptStatus));

    CardBlockInterrupts(Adapter);

    *InterruptRecognized = TRUE;
    *QueueDpc = TRUE;

    return;
}


/***************************************************************************
Routine Description:

    This is the defered processing routine for interrupts.  It
    reads from the Interrupt Status Register any outstanding
    interrupts and handles them.

Arguments:

    MiniportAdapterContext - a handle to the adapter block.

Return Value:

    NONE.
*****************************************************************************/
void R6040HandleInterrupt(IN NDIS_HANDLE MiniportAdapterContext)
{
    // The adapter to process
    PR6040_ADAPTER Adapter = ((PR6040_ADAPTER)MiniportAdapterContext);

    NdisDprAcquireSpinLock(&Adapter->Lock);

    // Handle the interrupts
    if (Adapter->InterruptStatus & MISR_TXEND)
    {
        NdisAcquireSpinLock(&Adapter->SendLock);

        // Handle the transmit
        R6040XmitDpc(Adapter);

        NdisReleaseSpinLock(&Adapter->SendLock);
    }

    if (Adapter->InterruptStatus & MISR_RXEND)
    {
        NdisDprAcquireSpinLock(&Adapter->RcvLock);

        // For receives, call this to handle the receive
        R6040RcvDpc(Adapter);

        NdisDprReleaseSpinLock(&Adapter->RcvLock);
    }

    NdisDprReleaseSpinLock(&Adapter->Lock);
}


/***************************************************************************
Routine Description:

    This is the real interrupt handler for receive/overflow interrupt.

    Called when a receive interrupt is received. It first indicates
    all packets on the card and finally indicates ReceiveComplete().

Arguments:

    Adapter - Pointer to the adapter block.

Return Value:

    TRUE if done with all receives, else FALSE.
*****************************************************************************/
BOOLEAN R6040RcvDpc(IN PR6040_ADAPTER Adapter)
{
    // Status of a received packet.
    INDICATE_STATUS IndicateStatus = INDICATE_OK;

    // Guess at where the packet is located
    PUCHAR PacketLoc;

    // Flag to tell when the receive process is complete
    BOOLEAN Done = TRUE;

    PUCHAR rxd;
    USHORT rxstatus;
    int total_len;

    do
    {
        // Code to fix a loop we got into where the card had been removed,
        // Now, the NDIS unbind routine calls our shutdown
        // handler which sets a flag
        if (Adapter->ShuttingDown)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040RcvDpc(): Shutdown detected\r\n")));
            break;
        }

        // Default to not indicating NdisMEthIndicateReceiveComplete
        Adapter->IndicateReceiveDone = FALSE;

        // A packet was found on the card, indicate it.
        Adapter->ReceivePacketCount++;

        rxd = Adapter->Rx_ring[Adapter->Rx_desc_add].VirtualAddr;

        READMEM16(rxd + DS_STATUS, rxstatus);
        READMEM16(rxd + DS_LEN, total_len);
        total_len -= 4;  /* Skip CRC 4 byte */

        /* OWN bit=1, no packet coming */
        if (rxstatus & 0x8000)
        {
            IndicateStatus = SKIPPED;
            //RETAILMSG(R6040DBG, (TEXT("R6040RcvDpc(): no packet coming \r\n")));
            break;
        }

        //RETAILMSG(R6040DBG, (TEXT("R6040RcvDpc(): total_len = %d\r\n"), total_len));

        Adapter->Rx_desc_add++;
        // Adapter->Rx_free_cnt++;

        // Copy the data to the network stack
        PacketLoc = rxd + BUF_START;
        Adapter->PacketHeaderLoc =  PacketLoc;

        // Indicate the packet to the wrapper
        IndicateStatus = R6040IndicatePacket(Adapter , total_len);
        if (IndicateStatus != CARD_BAD)
        {
            Adapter->FramesRcvGood++;
        }

        // Handle when the card is unable to indicate good packets
        if (IndicateStatus == CARD_BAD)
        {
            RETAILMSG(R6040DBG,
                (TEXT("R6040RcvDpc(): CARD_BAD: R: <%x %x %x %x> \r\n"),
                Adapter->PacketHeader[0],
                Adapter->PacketHeader[1],
                Adapter->PacketHeader[2],
                Adapter->PacketHeader[3]));

            // Start off with receive interrupts disabled.
            Adapter->NicInterruptMask = MIER_RXENDE | MIER_TXENDE;

            // Reset the adapter
            CardReset(Adapter);
            // Since the adapter was just reset, stop indicating packets.
            break;
        }

        /* Rx data already copy to the upper buffer, now can reuse RX descriptor */
        WRITEMEM16(rxd + DS_STATUS, 0x8000); /* Reuse RX descriptor */

        /* Signal RX ready */
        // NdisRawWritePortUshort( Adapter->IoPAddr + NIC_CONTROL0,Adapter->MacControl);

        /* Move to next RX descriptor */
        if (Adapter->Rx_desc_add >= MAX_RX_DESCRIPTORS)
        {
            Adapter->Rx_desc_add = 0;
            Done = FALSE;
            Adapter->ReceivePacketCount = 0;
            // break;
        }

        // Finally, indicate ReceiveComplete to all protocols which received packets
        if (Adapter->IndicateReceiveDone)
        {
            NdisDprReleaseSpinLock(&Adapter->RcvLock);
            NdisMEthIndicateReceiveComplete(Adapter->MiniportAdapterHandle);
            NdisDprAcquireSpinLock(&Adapter->RcvLock);
            Adapter->IndicateReceiveDone = FALSE;
        }
    }while (TRUE);

    return (Done);
}


/***************************************************************************
Routine Description:

    This is the real interrupt handler for a transmit complete interrupt.
    R6040Dpc queues a call to it.

    Called after a transmit complete interrupt. It checks the
    status of the transmission, completes the send if needed,
    and sees if any more packets are ready to be sent.

Arguments:

    Adapter  - Pointer to the adapter block.

Return Value:

    None.
*****************************************************************************/
void R6040XmitDpc(IN PR6040_ADAPTER Adapter)
{
    USHORT txstatus;
    UCHAR *txd;

    //RETAILMSG(R6040DBG, (TEXT("R6040XmitDpc()\r\n")));

    while (Adapter->Tx_free_cnt < MAX_TX_DESCRIPTORS)
    {
        txd = Adapter->Tx_ring[ Adapter->Tx_desc_remove].VirtualAddr;
        READMEM16(txd + DS_STATUS, txstatus);

        /* TX Descriptor OWN=1, not complete */
        if (txstatus & 0x8000)
             break;

        if (++Adapter->Tx_desc_remove >= MAX_TX_DESCRIPTORS )
            Adapter->Tx_desc_remove = 0;

        Adapter->Tx_free_cnt++;
    }

    Adapter->FramesXmitGood++;

    //R6040DoNextSend(Adapter);
}

INDICATE_STATUS
R6040IndicatePacket(
    IN PR6040_ADAPTER Adapter,
    IN INT  Len
    )

/*++

Routine Description:

    Indicates the first packet on the card to the protocols.

    NOTE: For MP, non-x86 architectures, this assumes that the packet has been
    read from the card and into Adapter->PacketHeader and Adapter->Lookahead.

    NOTE: For UP x86 systems this assumes that the packet header has been
    read into Adapter->PacketHeader and the minimal lookahead stored in
    Adapter->Lookahead

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    CARD_BAD if the card should be reset;
    INDICATE_OK otherwise.

--*/

{
    // Length of the packet
    UINT PacketLen = Len;

    // Length of the lookahead buffer
    UINT IndicateLen;

    if (PacketLen > 1514) {
        RETAILMSG(R6040DBG,
            (TEXT("R6040:IndicatePacket Third CARD_BAD check failed\r\n")));
        return(SKIPPED);
    }

    // Lookahead amount to indicate
    IndicateLen = (PacketLen > (Adapter->MaxLookAhead + R6040_HEADER_SIZE )) ?
                           (Adapter->MaxLookAhead + R6040_HEADER_SIZE ) :
                           PacketLen;

    // Indicate packet
    Adapter->PacketLen = PacketLen;
    if (IndicateLen < R6040_HEADER_SIZE)
    {
        // Runt Packet
        NdisDprReleaseSpinLock(&Adapter->RcvLock);
        NdisMEthIndicateReceive(
                Adapter->MiniportAdapterHandle,
                (NDIS_HANDLE)Adapter,
                (PCHAR)(Adapter->PacketHeaderLoc),
                IndicateLen,
                NULL,
                0,
                0
                );
        NdisDprAcquireSpinLock(&Adapter->RcvLock);

    } else {
        NdisDprReleaseSpinLock(&Adapter->RcvLock);
        NdisMEthIndicateReceive(
                Adapter->MiniportAdapterHandle,
                (NDIS_HANDLE)Adapter,
                (PCHAR)(Adapter->PacketHeaderLoc),
                R6040_HEADER_SIZE,
                (PCHAR)(Adapter->PacketHeaderLoc) + R6040_HEADER_SIZE,
                IndicateLen - R6040_HEADER_SIZE,
                PacketLen - R6040_HEADER_SIZE
                );
        NdisDprAcquireSpinLock(&Adapter->RcvLock);
    }

    Adapter->IndicateReceiveDone = TRUE;
    return INDICATE_OK;
}


/***************************************************************************
Routine Description:

    A protocol calls the R6040TransferData request (indirectly via
    NdisTransferData) from within its Receive event handler
    to instruct the driver to copy the contents of the received packet
    a specified packet buffer.

Arguments:

    MiniportAdapterContext - Context registered with the wrapper, really
        a pointer to the adapter.

    MiniportReceiveContext - The context value passed by the driver on its call
    to NdisMEthIndicateReceive.  The driver can use this value to determine
    which packet, on which adapter, is being received.

    ByteOffset - An unsigned integer specifying the offset within the
    received packet at which the copy is to begin.  If the entire packet
    is to be copied, ByteOffset must be zero.

    BytesToTransfer - An unsigned integer specifying the number of bytes
    to copy.  It is legal to transfer zero bytes; this has no effect.  If
    the sum of ByteOffset and BytesToTransfer is greater than the size
    of the received packet, then the remainder of the packet (starting from
    ByteOffset) is transferred, and the trailing portion of the receive
    buffer is not modified.

    Packet - A pointer to a descriptor for the packet storage into which
    the MAC is to copy the received packet.

    BytesTransfered - A pointer to an unsigned integer.  The MAC writes
    the actual number of bytes transferred into this location.  This value
    is not valid if the return status is STATUS_PENDING.

Notes:

  - The MacReceiveContext will be a pointer to the open block for
    the packet.
*****************************************************************************/
NDIS_STATUS R6040TransferData(
    OUT PNDIS_PACKET InPacket,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer)
{
    // Variables for the number of bytes to copy, how much can be
    // copied at this moment, and the total number of bytes to copy.
    UINT BytesLeft, BytesNow, BytesWanted;

    // Current NDIS_BUFFER to copy into
    PNDIS_BUFFER CurBuffer;

    PUCHAR BufStart;

    // Address on the adapter to copy from
    const UCHAR *CurCardLoc;

    // Length and offset into the buffer.
    UINT BufLen, BufOff;

    // The adapter to transfer from.
    PR6040_ADAPTER Adapter = ((PR6040_ADAPTER)MiniportReceiveContext);


    //RETAILMSG(R6040DBG, (TEXT("R6040TransferData()\r\n")));

    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    // Add the packet header onto the offset.
    ByteOffset+= R6040_HEADER_SIZE;

    // See how much data there is to transfer.
    if (ByteOffset+BytesToTransfer > Adapter->PacketLen)
    {
        if (Adapter->PacketLen < ByteOffset)
        {
            *BytesTransferred = 0;
            return(NDIS_STATUS_FAILURE);
        }

        BytesWanted = Adapter->PacketLen - ByteOffset;
    }
    else
    {
        BytesWanted = BytesToTransfer;
    }

    // Set the number of bytes left to transfer
    BytesLeft = BytesWanted;
    {
        // Copy data from the card -- it is not completely stored in the
        // adapter structure.
        // Determine where the copying should start.
        CurCardLoc = Adapter->PacketHeaderLoc + ByteOffset;

        // Get location to copy into
        NdisQueryPacket(InPacket, NULL, NULL, &CurBuffer, NULL);
        NdisQueryBuffer(CurBuffer, (PVOID *)&BufStart, &BufLen);

        BufOff = 0;

        // Loop, filling each buffer in the packet until there
        // are no more buffers or the data has all been copied.
        while (CurBuffer && BytesLeft > 0)
        {
            // See how much data to read into this buffer.
            if ((BufLen-BufOff) > BytesLeft)
            {
                BytesNow = BytesLeft;
            }
            else
            {
                BytesNow = (BufLen - BufOff);
            }

            // Copy up the data.
            NdisMoveMemory(BufStart+BufOff,CurCardLoc,BytesNow);

            // Update offsets and counts
            CurCardLoc += BytesNow;
            BytesLeft -= BytesNow;

            // Is the transfer done now?
            if (BytesLeft == 0)
            {
                break;
            }

            // Was the end of this packet buffer reached?
            BufOff += BytesNow;

            if (BufOff == BufLen)
            {
                NdisGetNextBuffer(CurBuffer, &CurBuffer);
                if (CurBuffer == (PNDIS_BUFFER)NULL)
                {
                    break;
                }

                NdisQueryBuffer(CurBuffer, (PVOID *)&BufStart, &BufLen);
                BufOff = 0;
            }
        }

        *BytesTransferred = BytesWanted - BytesLeft;

        NdisDprReleaseSpinLock(&Adapter->RcvLock);
        return(NDIS_STATUS_SUCCESS);
    }

    NdisDprReleaseSpinLock(&Adapter->RcvLock);

    return(NDIS_STATUS_SUCCESS);
}


/***************************************************************************
Routine Description:


    The R6040Send request instructs a driver to transmit a packet through
    the adapter onto the medium.

Arguments:

    MiniportAdapterContext - Context registered with the wrapper, really
        a pointer to the adapter.

    Packet - A pointer to a descriptor for the packet that is to be
    transmitted.

    SendFlags - Optional send flags

Notes:

    This miniport driver will always accept a send.  This is because
    the R6040 has limited send resources and the driver needs packets
    to copy to the adapter immediately after a transmit completes in
    order to keep the adapter as busy as possible.

    This is not required for other adapters, as they have enough
    resources to keep the transmitter busy until the wrapper submits
    the next packet.
*****************************************************************************/
NDIS_STATUS R6040Send(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags)
{
    PR6040_ADAPTER Adapter = (PR6040_ADAPTER)(MiniportAdapterContext);

    NdisAcquireSpinLock(&Adapter->SendLock);

    //RETAILMSG(R6040DBG, (TEXT("<")));

    // Put the packet on the send queue.
    if (Adapter->FirstPacket == NULL)
    {
        Adapter->FirstPacket = Packet;
    }
    else
    {
        RESERVED(Adapter->LastPacket)->Next = Packet;
    }

    RESERVED(Packet)->Next = NULL;
    Adapter->LastPacket = Packet;

    // Process the next send
    R6040DoNextSend(Adapter);

    //RETAILMSG(R6040DBG, (TEXT(">\r\n")));

    NdisReleaseSpinLock(&Adapter->SendLock);

    return(NDIS_STATUS_PENDING);
}


/***************************************************************************
Routine Description:

    This routine examines if the packet at the head of the packet
    list can be copied to the adapter, and does so.

Arguments:

    Adapter - Pointer to the adapter block.

Return Value:

    None
*****************************************************************************/
void R6040DoNextSend(PR6040_ADAPTER Adapter)
{
    // The packet to process.
    PNDIS_PACKET Packet;

    // The current destination transmit buffer.
    // XMIT_BUF TmpBuf1;

    // Length of the packet
    ULONG Len;

    //RETAILMSG(R6040DBG, (TEXT("+")));

    // Check if we have enough resources and a packet to process
    while ((Adapter->FirstPacket != NULL) /*&&!Adapter->Tx_free_cnt*/)
    {
        // If we're shutting down, just get out of here (card may not be present)
        if (Adapter->ShuttingDown)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040DoNextSend(): Shutdown detected\r\n")));
            break;
        }

        //If Queue full
        if (Adapter->Tx_free_cnt <= 0)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040DoNextSend(): Queue full\r\n")));
            break;
        }

        // Get the length of the packet.
        NdisQueryPacket(
            Adapter->FirstPacket,
            NULL,
            NULL,
            NULL,
            &Len
            );

        //RETAILMSG(R6040DBG, (TEXT("R6040DoNextSend(): packet length = %d\r\n"), Len));

        // Remove the packet from the queue.
        Packet = Adapter->FirstPacket;
        Adapter->FirstPacket = RESERVED(Packet)->Next;

        if (Packet == Adapter->LastPacket)
        {
            Adapter->LastPacket = NULL;
        }

        // Copy down the packet.
        if (CardCopyDownPacket(Adapter, Packet,
                                &Adapter->PacketLens[Adapter->Tx_desc_add]) == FALSE)
        {
            RETAILMSG(R6040DBG, (TEXT("R6040DoNextSend(): Copy Down packet error\r\n")));

            NdisReleaseSpinLock(&Adapter->SendLock);

            NdisMSendComplete(
                Adapter->MiniportAdapterHandle,
                Packet,
                NDIS_STATUS_FAILURE
                );

            NdisAcquireSpinLock(&Adapter->SendLock);
            continue;
        }


        // Ack the send immediately.  If for some reason it
        // should fail, the protocol should be able to handle
        // the retransmit.
        NdisReleaseSpinLock(&Adapter->SendLock);

        //RETAILMSG(R6040DBG, (TEXT("R6040DoNextSend(): NdisMSendComplete \r\n")));

        NdisMSendComplete(
                Adapter->MiniportAdapterHandle,
                Packet,
                NDIS_STATUS_SUCCESS
                );

        NdisAcquireSpinLock(&Adapter->SendLock);
    }
    //RETAILMSG(R6040DBG, (TEXT("-")));
}


