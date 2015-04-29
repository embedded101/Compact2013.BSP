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

    card.c

Abstract:

    Card-specific functions for the NDIS 3.0 Novell 2000 driver.

--*/

#include "precomp.h"

UCHAR   defaultMac[6] = {0x12,0x34,0x56,0x78,0x9A,0xBC};

//********************************************************************************************//
//For RDC Setting
//********************************************************************************************//
/*check MAC Identifier Register */
BOOLEAN IsRdcMacID(IN PR6040_ADAPTER Adapter)
{
    USHORT temp;
    BOOLEAN IsRdcId = FALSE;

    NdisRawReadPortUshort(Adapter->IoPAddr + NIC_IDENTIFER, &temp);

    RETAILMSG(R6040DBG, (TEXT("R6040:MAC Id =-> %x\r\n"),temp));

    if( temp == RDC_MAC_ID)
        IsRdcId = TRUE;

    return IsRdcId;
}

/* Read a word data from PHY Chip */
INT PHY_Read(IN PR6040_ADAPTER Adapter, IN INT phy_addr, IN INT reg_idx)
{
    INT i = 0;
    USHORT  data,temp;

    data = MMDIO_READ + (USHORT)reg_idx + ((USHORT)phy_addr << 8);

    NdisRawWritePortUshort(Adapter->IoPAddr + NIC_MDIO_CONTROL, data);

    do{
      NdisRawReadPortUshort(Adapter->IoPAddr+NIC_MDIO_CONTROL, &temp);
    }while( (++i < 2048) && (temp & MMDIO_READ) );

    NdisRawReadPortUshort(Adapter->IoPAddr+NIC_MDIO_READ_DATA, &temp);
    //RETAILMSG(R6040DBG, (TEXT("R6040:PHY_Read ==> PHY_addr: 0x%04x, Data : 0x%04x\r\n"),phy_addr, temp));
    return temp;
}

/* Write a word data from PHY Chip */
VOID PHY_Write(IN PR6040_ADAPTER Adapter, IN USHORT phy_addr,IN USHORT reg_idx,IN USHORT  dat)
{
    int i = 0;
    USHORT  data,temp;

    NdisRawWritePortUshort(Adapter->IoPAddr + NIC_MDIO_WRITE_DATA, dat);
    data = MMDIO_WRITE + (USHORT)reg_idx + ((USHORT)phy_addr << 8);
    NdisRawWritePortUshort(Adapter->IoPAddr + NIC_MDIO_CONTROL, data);

    do{
       NdisRawReadPortUshort(Adapter->IoPAddr+NIC_MDIO_CONTROL, &temp);
    }while( (++i < 2048) && (temp & MMDIO_WRITE) );
}

/* Status of PHY CHIP */
INT PHY_Mode_Chk(IN PR6040_ADAPTER Adapter)
{
    int phy_dat;

    /* PHY Link Status Check */
    phy_dat = PHY_Read(Adapter, Adapter->PhyAddr , 1);
    if (!(phy_dat & 0x0004))
    {
        RETAILMSG(R6040DBG, (TEXT("R6040:PHY_Mode_Chk ==> LINK_FAILED\r\n")));
        return LINK_FAILED; /* Link Failed */
    }

    /* PHY Chip Auto-Negotiation Status */
    phy_dat = PHY_Read(Adapter, Adapter->PhyAddr, 1);
    if (phy_dat & 0x0020)
    {
        /* Auto Negotiation Mode */
        phy_dat = PHY_Read(Adapter, Adapter->PhyAddr, 5);
        phy_dat &= PHY_Read(Adapter, Adapter->PhyAddr, 4);
        if (phy_dat & 0x140)
            phy_dat = FULL_DUPLEX;
        else
            phy_dat = HALF_DUPLEX;
    }
    else {
        /* Force Mode */
        phy_dat = PHY_Read(Adapter, Adapter->PhyAddr, 0);
        if (phy_dat & 0x100)
           phy_dat = FULL_DUPLEX;
        else
           phy_dat = HALF_DUPLEX;
    }

    return phy_dat;
}

VOID CardSetMulticast(IN PR6040_ADAPTER Adapter,int num_addrs,char *address_list)
{
    ULONG IoAddr;
    const USHORT *Addr;
    int i;

    UCHAR  Address[8];
    UCHAR  *ptr;
    USHORT Dummy;

      RETAILMSG(R6040DBG,
        (TEXT("R6040: CardSetMulticast\r\n")));

    IoAddr = Adapter->IoPAddr + NIC_MUITICAST_ADDRESS1_LOW;
    Addr = (USHORT*) address_list;

    /* Set MultiCast Address */
    for (i = 0; (i < num_addrs) && (i < 4); i++)
    {
        NdisRawWritePortUshort(IoAddr,Addr[0]);
        IoAddr += 2;
        NdisRawWritePortUshort(IoAddr,Addr[1]);
        IoAddr += 2;
        NdisRawWritePortUshort(IoAddr,Addr[2]);
        IoAddr += 4;
        Addr += 3;  /* Point to next multicast address */
    }

    /* Set non-use multicast address register */
    for (i = num_addrs; i < 4; i++)
    {
        NdisRawWritePortUshort(IoAddr,0xFFFF);
        IoAddr += 2;
        NdisRawWritePortUshort(IoAddr,0xFFFF);
        IoAddr += 2;
        NdisRawWritePortUshort(IoAddr,0xFFFF);
        IoAddr += 4;
    }

     IoAddr = (Adapter->IoPAddr + NIC_MUITICAST_ADDRESS1_LOW);
     ptr = Address;
     for (i = 0; i < R6040_LENGTH_OF_ADDRESS/2; i++)
     {
        NdisRawReadPortUshort(IoAddr, &Dummy);
        WRITEMEM32(ptr, Dummy);
        ptr+=2;
        IoAddr+=2;
     }

     RETAILMSG(R6040DBG,
        (TEXT("R6040: MUITICAST_ADDRESS1 [ %02x-%02x-%02x-%02x-%02x-%02x ]\r\n"),
            Address[0],
            Address[1],
            Address[2],
            Address[3],
            Address[4],
            Address[5]));

     IoAddr = (Adapter->IoPAddr + NIC_MUITICAST_ADDRESS2_LOW);
     ptr = Address;
     for (i = 0; i < R6040_LENGTH_OF_ADDRESS/2; i++)
     {
        NdisRawReadPortUshort(IoAddr, &Dummy);
        WRITEMEM32(ptr, Dummy);
        ptr+=2;
        IoAddr+=2;
     }

     RETAILMSG(R6040DBG,
        (TEXT("R6040: MUITICAST_ADDRESS2 [ %02x-%02x-%02x-%02x-%02x-%02x ]\r\n"),
            Address[0],
            Address[1],
            Address[2],
            Address[3],
            Address[4],
            Address[5]));

     IoAddr = (Adapter->IoPAddr + NIC_MUITICAST_ADDRESS3_LOW);
     ptr = Address;
     for (i = 0; i < R6040_LENGTH_OF_ADDRESS/2; i++)
     {
        NdisRawReadPortUshort(IoAddr, &Dummy);
        WRITEMEM32(ptr, Dummy);
        ptr+=2;
        IoAddr+=2;
     }

     RETAILMSG(R6040DBG,
        (TEXT("R6040: MUITICAST_ADDRESS3 [ %02x-%02x-%02x-%02x-%02x-%02x ]\r\n"),
            Address[0],
            Address[1],
            Address[2],
            Address[3],
            Address[4],
            Address[5]));

}

// Daniel+20070103
VOID CardSetMACAddress()
{
    const USHORT *addr;
    //UCHAR   defaultMac[6] = {0x12,0x34,0x56,0x78,0x9A,0xBC};
    UCHAR   *mac;

    RETAILMSG(R6040DBG, (TEXT("R6040: CardSetMACAddress\r\n")));

    mac = (UCHAR *)defaultMac;

    RETAILMSG(R6040DBG, (TEXT("R6040: MAC_Debug:: %x-%x-%x-%x-%x-%x\r\n"),
                        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));

    addr = (USHORT *)mac;

    /* Set MAC Address */
    NdisRawWritePortUshort(R6040IoPAddr + NIC_MUITICAST_ADDRESS0_LOW, addr[0]);
    NdisRawWritePortUshort(R6040IoPAddr + NIC_MUITICAST_ADDRESS0_MIDDEL, addr[1]);
    NdisRawWritePortUshort(R6040IoPAddr + NIC_MUITICAST_ADDRESS0_HIGH, addr[2]);
}


BOOLEAN CardAllocateTxRxDescriptor(IN PR6040_ADAPTER Adapter)
{
    UCHAR    *tdp = 0;
    UCHAR    *p_tdp = 0;
    ULONG    phyaddr = 0;
    int                i;
    BOOL status = TRUE;

    // Init TX descriptor
    for ( i = 0; i < MAX_TX_DESCRIPTORS; i++)
    {
        NdisMAllocateSharedMemory(
            Adapter->MiniportAdapterHandle,
             DS_SIZE + MAX_BUF_SIZE,
             FALSE,
            (PVOID )&Adapter->Tx_ring[i].VirtualAddr,
            &Adapter->Tx_ring[i].PhyAddr);
        if(!(Adapter->Tx_ring[i].VirtualAddr))
        {
            RETAILMSG(R6040DBG,(TEXT("R6040:Failed to allocate TX descriptor memory\r\n")));
            status = FALSE;
            goto cleanup;
        }

     tdp = Adapter->Tx_ring[i].VirtualAddr;

    if(i == 0)
      phyaddr = NdisGetPhysicalAddressLow(Adapter->Tx_ring[i].PhyAddr); /* Save the firse the Allocate memory */
        else
    {
           WRITEMEM32((p_tdp + DS_NDSP),
        NdisGetPhysicalAddressLow(Adapter->Tx_ring[i].PhyAddr));
    }
        p_tdp = (char*)tdp;     /* Previous descr pointer */
    }
    // link last Descriptor to first:
    WRITEMEM32((p_tdp + DS_NDSP),phyaddr);

    *tdp = 0;
    *p_tdp = 0;
    phyaddr = 0;

    // Init RX descriptor
    //tdp = Adapter->Desc_Pool + (MAX_TX_DESCRIPTORS * (sizeof(R6040_DESCRIPTOR) + MAX_BUF_SIZE));
    for ( i = 0; i < MAX_RX_DESCRIPTORS; i++)
    {
        NdisMAllocateSharedMemory(
            Adapter->MiniportAdapterHandle,
             DS_SIZE + MAX_BUF_SIZE,
             FALSE,
            (PVOID )&Adapter->Rx_ring[i].VirtualAddr,
            &Adapter->Rx_ring[i].PhyAddr);

        if(!Adapter->Rx_ring[i].VirtualAddr)
        {
            RETAILMSG(R6040DBG,(TEXT("R6040:Failed to allocate Rx descriptor memory\r\n")));
            status = FALSE;
            goto cleanup;
        }

        tdp = Adapter->Rx_ring[i].VirtualAddr;

        if(i == 0)
      phyaddr = NdisGetPhysicalAddressLow(Adapter->Rx_ring[i].PhyAddr); /* Save the firse the Allocate memory */
        else
        {
            WRITEMEM32((p_tdp + DS_NDSP),
        NdisGetPhysicalAddressLow(Adapter->Rx_ring[i].PhyAddr));
        }
        p_tdp = (char*)tdp;     /* Previous descr pointer */
    }
    // link last Descriptor to first:
    WRITEMEM32((p_tdp + DS_NDSP),phyaddr);
    
    cleanup:
        
    return status;
}
//********************************************************************************************//
#pragma NDIS_PAGEABLE_FUNCTION(CardInitialize)

BOOLEAN
CardInitialize(
    IN PR6040_ADAPTER Adapter
    )

/*++

Routine Description:

    Initializes the card into a running state.

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    TRUE, if all goes well, else FALSE.

--*/

{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG    Addr;

    USHORT temp;
    //UCHAR   defaultMac[6];
    USHORT *addr = (USHORT *)defaultMac;
    RETAILMSG(R6040DBG,
        (TEXT("R6040: CardInitialize\r\n")));


    _asm
    {
      mov dx, 0xcf8
      mov eax, 0x80004010
      out dx, eax
      mov dx, 0xcfc
      in  ax, dx
      mov dx, ax
      and dx, 0xfffe
      add dx, 0x68

      in  ax, dx
      mov defaultMac[0], al
      mov defaultMac[1], ah
      add dx, 2

      in  ax, dx
      mov defaultMac[2], al
      mov defaultMac[3], ah
      add dx, 2

      in  ax, dx
      mov defaultMac[4], al
      mov defaultMac[5], ah
    }



    NdisDprAcquireSpinLock(&Adapter->Lock);

    if(!IsRdcMacID(Adapter))
       return (FALSE);

    // Reset the card.
    SyncCardStop(Adapter);

    /* Set MAC Address */
    NdisRawWritePortUshort(R6040IoPAddr + NIC_MUITICAST_ADDRESS0_LOW, addr[0]);
    NdisRawWritePortUshort(R6040IoPAddr + NIC_MUITICAST_ADDRESS0_MIDDEL, addr[1]);
    NdisRawWritePortUshort(R6040IoPAddr + NIC_MUITICAST_ADDRESS0_HIGH, addr[2]);

    // Pause
    NdisMSleep(1000);

    /* Set MAC & Multicast Address Register */
    CardSetMACAddress( );
    CardSetMulticast( Adapter , 0 , 0);

    // Init descriptor
    Adapter->Tx_free_cnt = MAX_RX_DESCRIPTORS;
    Adapter->Rx_free_cnt = MAX_RX_DESCRIPTORS;
    Adapter->Tx_desc_add = 0;
    Adapter->Tx_desc_remove = 0;
    Adapter->Rx_desc_add = 0;
    Adapter->Rx_desc_remove = 0;
    Adapter->MediaState = NdisMediaStateDisconnected;

    ResetTxRing(Adapter);
    ResetRxRing(Adapter);

    Addr = NdisGetPhysicalAddressLow(Adapter->Tx_ring[0].PhyAddr);
    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_TX_DESCRIPTOR_START_ADDRESS0, LSB(Addr));
    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_TX_DESCRIPTOR_START_ADDRESS1, MSB(Addr));
    Addr = 0;
    Addr = NdisGetPhysicalAddressLow(Adapter->Rx_ring[0].PhyAddr);
    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_RX_DESCRIPTOR_START_ADDRESS0, LSB(Addr));
    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_RX_DESCRIPTOR_START_ADDRESS1, MSB(Addr));

 NdisRawReadPortUshort(Adapter->IoPAddr + NIC_TX_DESCRIPTOR_START_ADDRESS0, &temp);
 RETAILMSG(R6040DBG, (TEXT("R6040:CardInitialize == NIC_TX_DESCRIPTOR_START_ADDRESS0 : 0x%04x\r\n"),temp));
 NdisRawReadPortUshort(Adapter->IoPAddr + NIC_TX_DESCRIPTOR_START_ADDRESS1, &temp);
 RETAILMSG(R6040DBG, (TEXT("R6040:CardInitialize == NIC_TX_DESCRIPTOR_START_ADDRESS1 : 0x%04x\r\n"),temp));
 NdisRawReadPortUshort(Adapter->IoPAddr + NIC_RX_DESCRIPTOR_START_ADDRESS0, &temp);
 RETAILMSG(R6040DBG, (TEXT("R6040:CardInitialize == NIC_RX_DESCRIPTOR_START_ADDRESS0 : 0x%04x\r\n"),temp));
 NdisRawReadPortUshort(Adapter->IoPAddr + NIC_RX_DESCRIPTOR_START_ADDRESS1, &temp);
 RETAILMSG(R6040DBG, (TEXT("R6040:CardInitialize == NIC_RX_DESCRIPTOR_START_ADDRESS1 : 0x%04x\r\n"),temp));

    // Buffer Size Register
    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_RX_BUFFER_SIZE, MAX_BUF_SIZE);

    //Tx Interrupt Control Register
    //NdisRawWritePortUshort( Adapter->IoPAddr + NIC_TX_INTR_CONTROL, 0x0100);

    // Rx Interrupt Control Register
    //NdisRawWritePortUshort( Adapter->IoPAddr + NIC_RX_INTR_CONTROL, 0x0100);

    //RX Descriptor count
    NdisRawWritePortUshort(  Adapter->IoPAddr + NIC_RX_DESCRIPTOR_CONTROL, ((MAX_RX_DESCRIPTORS - 1) << 8) + MAX_RX_DESCRIPTORS);

#if (PHY_MODE == 0x3100)
    {
        // PHY Mode Check
        int i;

        //Daniel+20070208
        for (i = 0; i < 3; i++)
        {
            Adapter->PhyMode = PHY_Mode_Chk(Adapter);
            if (Adapter->PhyMode != LINK_FAILED)
                break;
        }

        if (i == 3)
            Adapter->PhyMode = HALF_DUPLEX;
        //Daniel-

        RETAILMSG(R6040DBG, (TEXT("R6040:CardInitialize == PHY_mode : 0x%04x\n"),Adapter->PhyMode));
    }
#endif  //  (PHY_MODE==0x3100)

    // MAC Bus Control Register
    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_BUS_CONTROL, MBCR_DEFAULT);

    //MAC TX/RX Enable
    Adapter->MacControl = 0;       /* Clear promiscous & duplex */
    Adapter->MacControl = (MCR0_XMTEN | MCR0_RCVEN);
    if(Adapter->PhyMode == FULL_DUPLEX)
       Adapter->MacControl |= Adapter->PhyMode;

    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_CONTROL0 , Adapter->MacControl);


    NdisRawReadPortUshort(Adapter->IoPAddr + NIC_CONTROL0, &temp);
    RETAILMSG(R6040DBG, (TEXT("R6040:CardInitialize == NIC_CONTROL0 : 0x%04x\r\n"),temp));

    //Interrupt Mask Register
    //NdisRawWritePortUshort( Adapter->IoPAddr + NIC_INTR_ENABLE, MIER_RXENDE | MIER_TXENDE);

    //Daniel+20080208
    //NdisMSleep(2000);

    //Adapter->RamBase = (PUCHAR)0x4000;
    //Adapter->RamSize = 0x4000;
    Adapter->EightBitSlot = FALSE;
    NdisDprReleaseSpinLock(&Adapter->Lock);

    return(TRUE);
}


#pragma NDIS_PAGEABLE_FUNCTION(CardReadEthernetAddress)

BOOLEAN CardReadEthernetAddress(
    IN PR6040_ADAPTER Adapter
)

/*++

Routine Description:

    Reads in the Ethernet address from the Novell 2000.

Arguments:

    Adapter - pointer to the adapter block.

Return Value:

    The address is stored in Adapter->PermanentAddress, and StationAddress if it
    is currently zero.

--*/

{
    UINT    c;
    ULONG IoAddr;
    char *Addr;

    RETAILMSG(R6040DBG,
        (TEXT("R6040: CardReadEthernetAddress\r\n")));

    // Read in the station address. (We have to read words -- 2 * 6 -- bytes)
     IoAddr = (Adapter->IoPAddr + NIC_MUITICAST_ADDRESS0_LOW);
     Addr = Adapter->PermanentAddress;
     for (c = 0; c < R6040_LENGTH_OF_ADDRESS/2; c++)
     {
        USHORT Dummy;

        NdisRawReadPortUshort(IoAddr, &Dummy);
        WRITEMEM16(Addr, Dummy);
        Addr+=2;
        IoAddr+=2;
     }

      RETAILMSG(R6040DBG,
        (TEXT("R6040: PermanentAddress [ %02x-%02x-%02x-%02x-%02x-%02x ]\r\n"),
            Adapter->PermanentAddress[0],
            Adapter->PermanentAddress[1],
            Adapter->PermanentAddress[2],
            Adapter->PermanentAddress[3],
            Adapter->PermanentAddress[4],
            Adapter->PermanentAddress[5]));

    // Use the burned in address as the station address, unless the
    // registry specified an override value.
    if ((Adapter->StationAddress[0] == 0x00) &&
        (Adapter->StationAddress[1] == 0x00) &&
        (Adapter->StationAddress[2] == 0x00) &&
        (Adapter->StationAddress[3] == 0x00) &&
        (Adapter->StationAddress[4] == 0x00) &&
        (Adapter->StationAddress[5] == 0x00)
    )
    {
        Adapter->StationAddress[0] = Adapter->PermanentAddress[0];
        Adapter->StationAddress[1] = Adapter->PermanentAddress[1];
        Adapter->StationAddress[2] = Adapter->PermanentAddress[2];
        Adapter->StationAddress[3] = Adapter->PermanentAddress[3];
        Adapter->StationAddress[4] = Adapter->PermanentAddress[4];
        Adapter->StationAddress[5] = Adapter->PermanentAddress[5];
    }

    return(TRUE);
}


VOID ResetTxRing(IN PR6040_ADAPTER Adapter)
{
    int i;
    UCHAR* tdp;

    for ( i = 0; i < MAX_TX_DESCRIPTORS; i++) {
        tdp = Adapter->Tx_ring[i].VirtualAddr;

        WRITEMEM16(tdp + DS_STATUS, 0);
        WRITEMEM16(tdp + DS_LEN, 0);
        WRITEMEM32(tdp + DS_BUFP,
          (LONG)(NdisGetPhysicalAddressLow(Adapter->Tx_ring[i].PhyAddr) + DS_SIZE));
    }
}

VOID ResetRxRing(IN PR6040_ADAPTER Adapter)
{
    UCHAR *rfd;
    int i;

    for ( i = 0; i < MAX_RX_DESCRIPTORS; i++ ) {
        rfd = Adapter->Rx_ring[i].VirtualAddr;
        WRITEMEM16(rfd + DS_LEN , 0);  /* Length field = 0 */
        WRITEMEM16(rfd + DS_STATUS , 0x8000);    /* Own=1 */
        WRITEMEM32(rfd + DS_BUFP,
           (LONG)(NdisGetPhysicalAddressLow(Adapter->Rx_ring[i].PhyAddr) + DS_SIZE));
    }
    Adapter->Rx_desc_add = 0;
    Adapter->Rx_desc_remove = 0;
}

VOID CardStop(
    IN PR6040_ADAPTER Adapter
)

/*++

Routine Description:

    Stops the card.

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    None.

--*/

{
    BOOLEAN bTimer = FALSE;

   RETAILMSG(R6040DBG,
        (TEXT("R6040: CardStop\r\n")));

    NdisDprAcquireSpinLock(&Adapter->Lock);
    NdisDprAcquireSpinLock(&Adapter->SendLock);
    NdisDprAcquireSpinLock(&Adapter->RcvLock);

    // Turn on the STOP bit in the Command register.
    SyncCardStop(Adapter);

   CardInitialize(Adapter);

    //Interrupt Mask Register
    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_INTR_ENABLE, Adapter->NicInterruptMask);

    NdisDprReleaseSpinLock(&Adapter->RcvLock);
    NdisDprReleaseSpinLock(&Adapter->SendLock);
    NdisDprReleaseSpinLock(&Adapter->Lock);

}

BOOLEAN CardReset(
    IN PR6040_ADAPTER Adapter
)

/*++

Routine Description:

    Resets the card.

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    TRUE if everything is OK.

--*/

{
   RETAILMSG(R6040DBG,
        (TEXT("@@@@@@@@@@R6040 : CardReset@@@@@@@@@@@@@@\r\n")));

    // Stop the chip
    CardStop(Adapter);

    // Wait for the card to finish any receives or transmits
    Sleep(2);

    return TRUE;
}


#if 0
BOOLEAN CardCopyDownPacket(
    IN PR6040_ADAPTER  Adapter,
    IN PNDIS_PACKET     Packet,
    OUT PUINT           Length
)

/*++

Routine Description:

    Copies the packet Packet down starting at the beginning of
    transmit buffer , fills in Length to be the
    length of the packet.

Arguments:

    Adapter - pointer to the adapter block
    Packet - the packet to copy down

Return Value:

    Length - the length of the data in the packet in bytes.
    TRUE if the transfer completed with no problems.

--*/

{
    // Addresses of the Buffers to copy from and to.
    PUCHAR CurBufAddress;
    PUCHAR OddBufAddress;

    // Length of each of the above buffers
    UINT CurBufLen;
    UINT PacketLength;

    // Was the last transfer of an odd length?
    BOOLEAN OddBufLen = FALSE;

    // Current NDIS_BUFFER that is being copied from
    PNDIS_BUFFER CurBuffer;

    UCHAR     *to_p;
    UCHAR     *txd;


    // Programmed I/O, have to transfer the data.
    NdisQueryPacket(Packet, NULL, NULL, &CurBuffer, &PacketLength);

    //RETAILMSG(R6040DBG, (TEXT("CardCopyDownPacket(): packet length = %d\r\n"), PacketLength));

    *Length = 0;

    // Skip 0 length copies
    if (PacketLength == 0)
    {
        return(TRUE);
    }

    // Get the starting buffer address
    txd = Adapter->Tx_ring[Adapter->Tx_desc_add].VirtualAddr;

    Adapter->Tx_free_cnt--;

    // Get address and length of the first buffer in the packet
    NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);

    while (CurBuffer && (CurBufLen == 0 || CurBufAddress == NULL))
    {
        NdisGetNextBuffer(CurBuffer, &CurBuffer);
        NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);
    }

    // Set TX descriptor length field,
    // if length less than 60 byte, directly padding to 60byte
    if (PacketLength < 60)
      WRITEMEM16(txd + DS_LEN, 60);
    else
      WRITEMEM16(txd + DS_LEN, PacketLength);

    // Copy from the sglist into the txcb
    to_p = txd + BUF_START;

    if (CurBufAddress)
    {
        // Copy the data now
        do {
            // Write the previous byte with this one
            if (OddBufLen)
            {
                // It seems that the card stores words in LOW:HIGH order
                WRITEMEM16( to_p, (USHORT)(*OddBufAddress | ((*CurBufAddress) << 8)) );

                OddBufLen = FALSE;
                CurBufAddress++;
                CurBufLen--;
            }

            NdisMoveMemory(to_p, CurBufAddress , CurBufLen);

            to_p +=  CurBufLen;

                // Save trailing byte (if an odd lengthed transfer)
                if (CurBufLen & 0x1)
                {
                    OddBufAddress = CurBufAddress + (CurBufLen - 1);
                    OddBufLen = TRUE;
                }

            // Move to the next buffer
            NdisGetNextBuffer(CurBuffer, &CurBuffer);

            if (CurBuffer){
                NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);
            }

            // Get address and length of the next buffer
            while (CurBuffer && (CurBufLen == 0 || CurBufAddress == NULL)) {
                NdisGetNextBuffer(CurBuffer, &CurBuffer);
                if (CurBuffer){
                    NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);
                }
            }

         } while (CurBuffer);

        // Write trailing byte (if necessary)
        if (OddBufLen)
        {
            USHORT  usTmp;
            to_p--;
            usTmp = (USHORT)*OddBufAddress;
            WRITEMEM16(to_p, usTmp);
        }

        // Set TX Descriptor Status field
        WRITEMEM16(txd + DS_STATUS, 0x8000);
        CardStartXmit(Adapter);

        // Next descriptor
        if ( ++Adapter->Tx_desc_add >= MAX_TX_DESCRIPTORS)
            Adapter->Tx_desc_add = 0;

    }

    // Return length written
    *Length += PacketLength;

    return TRUE;
}
#endif

//Daniel+20061226
/***************************************************************************
Routine Description:

    Copies the packet Packet down starting at the beginning of
    transmit buffer , fills in Length to be the
    length of the packet.

Arguments:

    Adapter - pointer to the adapter block
    Packet - the packet to copy down

Return Value:

    Length - the length of the data in the packet in bytes.
    TRUE if the transfer completed with no problems.

*****************************************************************************/
BOOLEAN CardCopyDownPacket(
    IN PR6040_ADAPTER   Adapter,
    IN PNDIS_PACKET     Packet,
    OUT PUINT           Length)
{
    // Addresses of the Buffers to copy from and to.
    PUCHAR CurBufAddress;
    const UCHAR *OddBufAddress;

    // Length of each of the above buffers
    UINT CurBufLen;
    UINT PacketLength;

    // Was the last transfer of an odd length?
    BOOLEAN OddBufLen = FALSE;

    // Current NDIS_BUFFER that is being copied from
    PNDIS_BUFFER CurBuffer;

    UCHAR     *to_p;
    UCHAR     *txd;

    // Programmed I/O, have to transfer the data.
    NdisQueryPacket(Packet, NULL, NULL, &CurBuffer, &PacketLength);

    *Length = 0;

    // Skip 0 length copies
    if (PacketLength == 0)
    {
        return(TRUE);
    }

    // Get the starting buffer address
    txd = Adapter->Tx_ring[Adapter->Tx_desc_add].VirtualAddr;

    Adapter->Tx_free_cnt--;

    // Get address and length of the first buffer in the packet
    NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);

    while (CurBuffer && (CurBufLen == 0 || CurBufAddress == NULL))
    {
        NdisGetNextBuffer(CurBuffer, &CurBuffer);
        NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);
    }

    // Set TX descriptor length field,
    // if length less than 60 byte, directly padding to 60byte
    if (PacketLength < 60)
      WRITEMEM16(txd + DS_LEN, 60);
    else
      WRITEMEM16(txd + DS_LEN, PacketLength);

    // Copy from the sglist into the txcb
    to_p = txd + BUF_START;

    if (CurBufAddress)
    {
        // Copy the data now
        do
        {
            NdisMoveMemory(to_p, CurBufAddress , CurBufLen);

            to_p +=  CurBufLen;

            // Save trailing byte (if an odd lengthed transfer)
            if (CurBufLen & 0x1)
            {
                USHORT  usTmp;

                OddBufAddress = CurBufAddress + (CurBufLen - 1);
                usTmp = (USHORT)*OddBufAddress;
                WRITEMEM16(to_p-1, usTmp);
            }

            // Move to the next buffer
            NdisGetNextBuffer(CurBuffer, &CurBuffer);

            if (CurBuffer)
            {
                NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);
            }

            // Get address and length of the next buffer
            while (CurBuffer && (CurBufLen == 0 || CurBufAddress == NULL))
            {
                NdisGetNextBuffer(CurBuffer, &CurBuffer);
                if (CurBuffer)
                {
                    NdisQueryBuffer(CurBuffer, (PVOID *)&CurBufAddress, &CurBufLen);
                }
            }

         } while (CurBuffer);

        // Set TX Descriptor Status field
        WRITEMEM16(txd + DS_STATUS, 0x8000);
        CardStartXmit(Adapter);

        // Next descriptor
        if ( ++Adapter->Tx_desc_add >= MAX_TX_DESCRIPTORS)
        {
            Adapter->Tx_desc_add = 0;
        }
    }

    // Return length written
    *Length += PacketLength;

    return TRUE;
}



BOOLEAN SyncCardStop(
    IN PVOID SynchronizeContext
)

/*++

Routine Description:

    Sets the NIC_COMMAND register to stop the card.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    TRUE if the power has failed.

--*/

{
    PR6040_ADAPTER Adapter = ((PR6040_ADAPTER)SynchronizeContext);
    USHORT  temp;
    int i=0;

    RETAILMSG(R6040DBG,
        (TEXT("@@@@@@@@R6040:SyncCardStop@@@@@@@@@@@@@\r\n")));

    /* Stop MAC */
    NdisRawWritePortUshort(Adapter->IoPAddr + NIC_INTR_ENABLE , MIER_CLEAN_MASK); /* Mask Off Interrupt */
    NdisRawWritePortUshort(Adapter->IoPAddr + NIC_CONTROL1 , MCR1_MRST);  /* Reset RDC MAC */

    //Daniel+
    // Reset internal state machine
    NdisRawWritePortUshort(Adapter->IoPAddr + 0xAC , 0x2);
    NdisRawWritePortUshort(Adapter->IoPAddr + 0xAC , 0x0);
    //Daniel-

    do{
      NdisRawReadPortUshort( Adapter->IoPAddr + NIC_CONTROL1,&temp);
    }while((++i < 2048) && (temp & MCR1_MRST));

    return(FALSE);
}

VOID
CardStartXmit(
    IN PR6040_ADAPTER Adapter
    )

/*++

Routine Description:

    Sets the NIC_COMMAND register to start a transmission.
    The transmit buffer number is taken from Adapter->CurBufXmitting
    and the length from Adapter->PacketLens[Adapter->CurBufXmitting].

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    TRUE if the power has failed.

--*/

{
    // trigger to send
    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_TX_POLL_COMMAND , 0x01);  //Start transmitting
}

BOOLEAN
SyncCardSetReceiveConfig(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Sets the value of the "receive configuration" NIC register to
    the value of Adapter->NicReceiveConfig.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PR6040_ADAPTER Adapter = ((PR6040_ADAPTER)SynchronizeContext);
    USHORT temp;

    RETAILMSG(R6040DBG,
        (TEXT("R6040: SyncCardSetReceiveConfig\r\n")));

    Adapter->MacControl = 0;       /* Clear promiscous & duplex */
    Adapter->MacControl = MCR0_XMTEN | MCR0_RCVEN;

    //Daniel+20070208
    if(Adapter->PhyMode == FULL_DUPLEX)
    {
        RETAILMSG(R6040DBG, (TEXT("R6040: Full Duplex bit\r\n")));
        Adapter->MacControl |= Adapter->PhyMode;
    }
#if 0
    if(Adapter->PhyMode & FULL_DUPLEX)
    {
        RETAILMSG(R6040DBG, (TEXT("R6040: Full Duplex bit\r\n")));
        Adapter->MacControl |= Adapter->PhyMode;
    }
#endif
    //Daniel-

    // The broadcast bit
    if(Adapter->NicReceiveConfig & RCR_BROADCAST)
    {
        RETAILMSG(R6040DBG,
            (TEXT("R6040: Filter The broadcast bit\r\n")));
        //Adapter->MacControl |= MCR0_FBCP;
    }

    // The multicast bit
    if(Adapter->NicReceiveConfig & RCR_MULTICAST)
    {
        RETAILMSG(R6040DBG,
            (TEXT("R6040: Filter The multicast bit\r\n")));
        //Adapter->MacControl |= MCR0_AMCP;
    }

    // The promiscuous physical bit
    if(Adapter->NicReceiveConfig & RCR_ALL_PHYS)
    {
        RETAILMSG(R6040DBG,
            (TEXT("R6040: Filter The promiscuous bit\r\n")));
        Adapter->MacControl |= MCR0_PROM;
    }

    NdisRawWritePortUshort( Adapter->IoPAddr + NIC_CONTROL0 , Adapter->MacControl);

    NdisRawReadPortUshort(Adapter->IoPAddr + NIC_CONTROL0, &temp);
    RETAILMSG(R6040DBG, (TEXT("R6040:SyncCardSetReceiveConfig == NIC_CONTROL0 : 0x%04x\r\n"),temp));

    return FALSE;
}

BOOLEAN
SyncCardSetAllMulticast(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Turns on all the bits in the multicast register. Used when
    the card must receive all multicast packets.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PR6040_ADAPTER Adapter = ((PR6040_ADAPTER)SynchronizeContext);
    ULONG IoAddr;
    int i;

RETAILMSG(R6040DBG,
        (TEXT("R6040: SyncCardSetAllMulticast\r\n")));

    IoAddr = Adapter->IoPAddr + NIC_MUITICAST_ADDRESS1_LOW;
    /* Set non-use multicast address register */
    for (i = 0; i < 4; i++)
    {
        NdisRawWritePortUshort(IoAddr,0xFFFF);
        IoAddr += 2;
        NdisRawWritePortUshort(IoAddr,0xFFFF);
        IoAddr += 2;
        NdisRawWritePortUshort(IoAddr,0xFFFF);
        IoAddr += 4;
    }

    return FALSE;

}

BOOLEAN
SyncCardUpdateCounters(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Updates the values of the three counters (frame alignment errors,
    CRC errors, and missed packets).

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
RETAILMSG(R6040DBG,
        (TEXT("R6040: SyncCardUpdateCounters\r\n")));

    return FALSE;
}

VOID
CardFillMulticastRegs(
    IN PR6040_ADAPTER Adapter
    )

/*++

Routine Description:

    Erases and refills the card multicast registers. Used when
    an address has been deleted and all bits must be recomputed.

Arguments:

    Adapter - pointer to the adapter block

Return Value:

    None.

--*/

{
    UINT i;
    UCHAR Byte, Bit;

    //
    // First turn all bits off.
    //

    for (i=0; i<8; i++) {

        Adapter->NicMulticastRegs[i] = 0;

    }

    //
    // Now turn on the bit for each address in the multicast list.
    //

    for ( ; i > 0; ) {

        i--;

        CardGetMulticastBit(Adapter->Addresses[i], &Byte, &Bit);

        Adapter->NicMulticastRegs[Byte] |= Bit;

    }

}

ULONG
CardComputeCrc(
    IN PUCHAR Buffer,
    IN UINT Length
    )

/*++

Routine Description:

    Runs the AUTODIN II CRC algorithm on buffer Buffer of
    length Length.

Arguments:

    Buffer - the input buffer

    Length - the length of Buffer

Return Value:

    The 32-bit CRC value.

Note:

    This is adapted from the comments in the assembly language
    version in _GENREQ.ASM of the DWB NE1000/2000 driver.

--*/

{
    ULONG Crc, Carry;
    UINT i, j;
    UCHAR CurByte;

    Crc = 0xffffffff;

    for (i = 0; i < Length; i++) {
        CurByte = Buffer[i];
        for (j = 0; j < 8; j++) {
            Carry = ((Crc & 0x80000000) ? 1 : 0) ^ (CurByte & 0x01);
            Crc <<= 1;
            CurByte >>= 1;
            if (Carry) {
                Crc = (Crc ^ 0x04c11db6) | Carry;
            }
        }
    }
    return Crc;
}

VOID
CardGetMulticastBit(
    IN UCHAR Address[R6040_LENGTH_OF_ADDRESS],
    OUT UCHAR * Byte,
    OUT UCHAR * Value
    )

/*++

Routine Description:

    For a given multicast address, returns the byte and bit in
    the card multicast registers that it hashes to. Calls
    CardComputeCrc() to determine the CRC value.

Arguments:

    Address - the address

    Byte - the byte that it hashes to

    Value - will have a 1 in the relevant bit

Return Value:

    None.

--*/

{
    ULONG Crc;
    UINT BitNumber;

    //
    // First compute the CRC.
    //

    Crc = CardComputeCrc(Address, R6040_LENGTH_OF_ADDRESS);


    //
    // The bit number is now in the 6 most significant bits of CRC.
    //

    BitNumber = (UINT)((Crc >> 26) & 0x3f);

    *Byte = (UCHAR)(BitNumber / 8);
    *Value = (UCHAR)((UCHAR)1 << (BitNumber % 8));
}

BOOLEAN
SyncCardCopyMulticastRegs(
    IN PVOID SynchronizeContext
    )

/*++

Routine Description:

    Sets the eight bytes in the card multicast registers.

Arguments:

    SynchronizeContext - pointer to the adapter block

Return Value:

    None.

--*/

{
    PR6040_ADAPTER Adapter = ((PR6040_ADAPTER)SynchronizeContext);
    UINT i;

    for (i=0; i<8; i++)
    {
        NdisRawWritePortUshort( Adapter->IoPAddr + NIC_MUITICAST_ADDRESS0_LOW ,
                        Adapter->NicMulticastRegs[i]);
    }

    return FALSE;

}