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

    R6040hw.h

Abstract:

    The main program for an R6040 miniport driver.

Notes:

--*/

#ifndef _R6040HARDWARE_
#define _R6040HARDWARE_



//
// Default value for Adapter->IoBaseAddr
//
#define DEFAULT_IOBASEADDR (PVOID)0x300

#define CIS_NET_ADDR_OFFSET 0xff0

//
// Default value for Adapter->InterruptNumber
//
#define DEFAULT_INTERRUPTNUMBER 3


//
// Default value for Adapter->MulticastListMax
//
#define DEFAULT_MULTICASTLISTMAX 3


//
// Offsets from Adapter->IoPAddr of the ports used to access
// the 8390 NIC registers.
//
// The names in parenthesis are the abbreviations by which
// the registers are referred to in the 8390 data sheet.
//
// Some of the offsets appear more than once
// because they have have relevant page 0 and page 1 values,
// or they are different registers when read than they are
// when written. The notation MSB indicates that only the
// MSB can be set for this register, the LSB is assumed 0.
//

#if 0

#define NIC_COMMAND         0x0     // (CR)
#define NIC_PAGE_START      0x1     // (PSTART)   MSB, write-only
#define NIC_PHYS_ADDR       0x1     // (PAR0)     page 1
#define NIC_PAGE_STOP       0x2     // (PSTOP)    MSB, write-only
#define NIC_BOUNDARY        0x3     // (BNRY)     MSB
#define NIC_XMIT_START      0x4     // (TPSR)     MSB, write-only
#define NIC_XMIT_STATUS     0x4     // (TSR)      read-only
#define NIC_XMIT_COUNT_LSB  0x5     // (TBCR0)    write-only
#define NIC_XMIT_COUNT_MSB  0x6     // (TBCR1)    write-only
#define NIC_FIFO            0x6     // (FIFO)     read-only
//#define NIC_INTR_STATUS     0x7     // (ISR)
#define NIC_CURRENT         0x7     // (CURR)     page 1
#define NIC_MC_ADDR         0x8     // (MAR0)     page 1
#define NIC_CRDA_LSB        0x8     // (CRDA0)
#define NIC_RMT_ADDR_LSB    0x8     // (RSAR0)
#define NIC_CRDA_MSB        0x9     // (CRDA1)
#define NIC_RMT_ADDR_MSB    0x9     // (RSAR1)
#define NIC_RMT_COUNT_LSB   0xa     // (RBCR0)    write-only
#define NIC_RMT_COUNT_MSB   0xb     // (RBCR1)    write-only
#define NIC_RCV_CONFIG      0xc     // (RCR)      write-only
#define NIC_RCV_STATUS      0xc     // (RSR)      read-only
#define NIC_XMIT_CONFIG     0xd     // (TCR)      write-only
#define NIC_FAE_ERR_CNTR    0xd     // (CNTR0)    read-only
#define NIC_DATA_CONFIG     0xe     // (DCR)      write-only
#define NIC_CRC_ERR_CNTR    0xe     // (CNTR1)    read-only
#define NIC_INTR_MASK       0xf     // (IMR)      write-only
#define NIC_MISSED_CNTR     0xf     // (CNTR2)    read-only
#define NIC_RACK_NIC        0xBE     // Byte to read or write
#define NIC_RESET           0x1f    // (RESET)

#else
//***********************************************************************************/
//For RDC Setting
//***********************************************************************************/

#define PCI_CONFIG_ADDRESS_REG           0x0CF8
#define PCI_CONFIG_DATA_REG              0x0CFC

#define PCI_CONFIG_COMMAND_REG           0x04
#define PCI_CONFIG_IO_MAPPED_REG         0x10
#define PCI_CONFIG_MEMORY_MAPPED_REG     0x14
#define PCI_CONFIG_INT_REG               0x3C

#define PCI_CONFIG_REG_ENABLE            0x80000000


#define NIC_CONTROL0                             0x0     // (MCR0)
#define NIC_CONTROL1                             0x4     // (MCR1)
#define NIC_BUS_CONTROL                          0x8     // (MBCR)
#define NIC_TX_INTR_CONTROL                      0xC     // (MTICR)
#define NIC_RX_INTR_CONTROL                      0x10    // (MRICR)
#define NIC_TX_POLL_COMMAND                      0x14    // (MTPR)
#define NIC_RX_BUFFER_SIZE                       0x18    // (MRBSR)
#define NIC_RX_DESCRIPTOR_CONTROL                0x1A     // (MRDCR)
#define NIC_LAST_STATUS                          0x1C     // (MLSR)
#define NIC_MDIO_CONTROL                         0x20     // (MMDIO)
#define NIC_MDIO_READ_DATA                       0x24     // (MMRD)
#define NIC_MDIO_WRITE_DATA                      0x28     // (MMWD)
#define NIC_TX_DESCRIPTOR_START_ADDRESS0         0x2C     // (MTDSA0)
#define NIC_TX_DESCRIPTOR_START_ADDRESS1         0x30     // (MTDSA1)
#define NIC_RX_DESCRIPTOR_START_ADDRESS0         0x34     // (MRDSA0)
#define NIC_RX_DESCRIPTOR_START_ADDRESS1         0x38     // (MRDSA1)
#define NIC_INTR_STATUS                          0x3C     // (MISR)
#define NIC_INTR_ENABLE                          0x40     // (MIER)
#define NIC_EVENT_COUNT_INT_STATUS               0x44     // (MECISR)
#define NIC_EVENT_COUNT_INT_ENABLE               0x48     // (MECIER)
#define NIC_SUCCESSFULLY_RX_PACKET_COUNTER       0x50     // (MRCNT)
#define NIC_EVENT_COUNTER0                       0x52     // (MECNT0)
#define NIC_EVENT_COUNTER1                       0x54     // (MECNT1)
#define NIC_EVENT_COUNTER2                       0x56     // (MECNT2)
#define NIC_EVENT_COUNTER3                       0x58     // (MECNT3)
#define NIC_SUCCESSFULLY_TX_PACKET_COUNTER       0x5A     // (MTCNT)
#define NIC_EVENT_COUNTER4                       0x5C    // (MECNT4)
#define NIC_PAUSE_FRAME_COUNTER                  0x5E    // (MPCNT)
#define NIC_HASH_TABLE_WORD0                     0x60    // (MAR0)
#define NIC_HASH_TABLE_WORD1                     0x62    // (MAR1)
#define NIC_HASH_TABLE_WORD2                     0x64    // (MAR2)
#define NIC_HASH_TABLE_WORD3                     0x66    // (MAR3)
#define NIC_MUITICAST_ADDRESS0_LOW               0x68    // (MID0L)
#define NIC_MUITICAST_ADDRESS0_MIDDEL            0x6A    // (MID0M)
#define NIC_MUITICAST_ADDRESS0_HIGH              0x6C    // (MID0H)
#define NIC_MUITICAST_ADDRESS1_LOW               0x70    // (MID1L)
#define NIC_MUITICAST_ADDRESS1_MIDDEL            0x72    // (MID1M)
#define NIC_MUITICAST_ADDRESS1_HIGH              0x74    // (MID1H)
#define NIC_MUITICAST_ADDRESS2_LOW               0x78    // (MID2L)
#define NIC_MUITICAST_ADDRESS2_MIDDEL            0x7A    // (MID2M)
#define NIC_MUITICAST_ADDRESS2_HIGH              0x7C    // (MID2H)
#define NIC_MUITICAST_ADDRESS3_LOW               0x80    // (MID3L)
#define NIC_MUITICAST_ADDRESS3_MIDDEL            0x82    // (MID3M)
#define NIC_MUITICAST_ADDRESS3_HIGH              0x84    // (MID3H)
#define NIC_PHY_STATUS_CHANGE_CONFIG             0x88    // (MPSCCR)
#define NIC_PHY_STATUS                           0x8A    // (MPSR)
#define NIC_IDENTIFER                            0xBE    // (MACID)

/* RDC MAC ID */
#define RDC_MAC_ID  0x6040
#define R6040_IO_PORT_COUNT 256

// Constants for the NIC MAC CONTROL Register0.
#define MCR0_FULLD     0x8000
#define MCR0_TXEIE     0x4000
#define MCR0_XMTEN     0x1000
#define MCR0_FCEN      0x0200
#define MCR0_AMCP      0x0100
#define MCR0_RXEIE     0x0080
#define MCR0_FBCP      0x0040
#define MCR0_PROM      0x0020
#define MCR0_ADRB      0x0010
#define MCR0_ALONG     0x0008
#define MCR0_ARUNT     0x0004
#define MCR0_RCVEN     0x0002
#define MCR0_ACRCER    0x0001

// Constants for the NIC MAC CONTROL Register1.
#define MCR1_AUCP    0x8000
#define MCR1_WIDX    0x4000
#define MCR1_MRST      0x0001

// Constants for the NIC MAC MDIO CONTROL Register.
#define MMDIO_WRITE   0x4000
#define MMDIO_READ    0x2000


// Constants for the NIC INTR MASK
// Configure which ISR settings actually cause interrupts.
#define MIER_CLEAN_MASK    0x0000        // Clean all interrupt
#define MIER_MCHGE         0x0200        // PHY link changed interrupt enable
#define MIER_ECNTOE        0x0100        // Event counter overflow interrupt enable
#define MIER_TXEIEN        0x0080        // Tx early interrupt enable
#define MIER_TXENDE        0x0010        // TX packet finish interrupt enable
#define MIER_RXEIE         0x0008        // RX early interrupt enable
#define MIER_RXFFE         0x0004        // RX FIFO full interrupt enable
#define MIER_RXDNAE        0x0002        // RX descriptor Unavailable interrupt enable
#define MIER_RXENDE        0x0001        // RX packet finish interrupt enable


// Constants for the NIC_INTR_STATUS register.
// Indicate the cause of an interrupt.
#define MISR_EMPTY       0x0000        // No bits set in ISR
#define MISR_RXEND       0x0001        // Receive packet finish interrupt status
#define MISR_RXDUA       0x0002        // RX descriptor Unavailable interrupt status
#define MISR_RXFF        0x0004        // RX FIFO full interrupt status
#define MISR_RXEI        0x0008        // RX early interrupt status
#define MISR_TXEND       0x0010        // Tx packet finish interrupt status
#define MISR_TXEI        0x0080        // TX early interrupt status
#define MISR_ECNTO       0x0100        // Event counter overflow interrupt status
#define MISR_PCHG        0x0200        // PHY media changed interrupt status

#endif
//***********************************************************************************/


//
// Constants for the NIC_COMMAND register.
//
// Start/stop the card, start transmissions, and select
// which page of registers was seen through the ports.
//

#define CR_STOP         (UCHAR)0x01        // reset the card
#define CR_START        (UCHAR)0x02        // start the card
#define CR_XMIT         (UCHAR)0x04        // begin transmission
#define CR_NO_DMA       (UCHAR)0x20        // stop remote DMA

#define CR_PS0          (UCHAR)0x40        // low bit of page number
#define CR_PS1          (UCHAR)0x80        // high bit of page number
#define CR_PAGE0        (UCHAR)0x00        // select page 0
#define CR_PAGE1        CR_PS0             // select page 1
#define CR_PAGE2        CR_PS1             // select page 2

#define CR_DMA_WRITE    (UCHAR)0x10        // Write
#define CR_DMA_READ     (UCHAR)0x08        // Read
#define CR_SEND         (UCHAR)0x18        // send


//
// Constants for the NIC_XMIT_STATUS register.
//
// Indicate the result of a packet transmission.
//

#define TSR_XMIT_OK     (UCHAR)0x01        // transmit with no errors
#define TSR_COLLISION   (UCHAR)0x04        // collided at least once
#define TSR_ABORTED     (UCHAR)0x08        // too many collisions
#define TSR_NO_CARRIER  (UCHAR)0x10        // carrier lost
#define TSR_NO_CDH      (UCHAR)0x40        // no collision detect heartbeat


//
// Constants for the NIC_INTR_STATUS register.
//
// Indicate the cause of an interrupt.
//

#if 0
#define ISR_EMPTY       (UCHAR)0x0000        // no bits set in ISR
#define ISR_RCV         (UCHAR)0x0001        // packet received with no errors
#define ISR_XMIT        (UCHAR)0x0002        // packet transmitted with no errors
#define ISR_RCV_ERR     (UCHAR)0x0004        // error on packet reception
#define ISR_XMIT_ERR    (UCHAR)0x0008        // error on packet transmission
#define ISR_OVERFLOW    (UCHAR)0x0010        // receive buffer overflow
#define ISR_COUNTER     (UCHAR)0x0020        // MSB set on tally counter
#define ISR_DMA_DONE    (UCHAR)0x0040        // RDC
#define ISR_RESET       (UCHAR)0x0080        // (not an interrupt) card is reset
#endif

//
// Constants for the NIC_RCV_CONFIG register.
//
// Configure what type of packets are received.
//

#define RCR_REJECT_ERR  (UCHAR)0x00        // reject error packets
#define RCR_BROADCAST   (UCHAR)0x04        // receive broadcast packets
#define RCR_MULTICAST   (UCHAR)0x08        // receive multicast packets
#define RCR_ALL_PHYS    (UCHAR)0x10        // receive ALL directed packets
#define RCR_MONITOR     (UCHAR)0x20        // don't collect packets


//
// Constants for the NIC_RCV_STATUS register.
//
// Indicate the status of a received packet.
//
// These are also used to interpret the status byte in the
// packet header of a received packet.
//

#define RSR_PACKET_OK   (UCHAR)0x01        // packet received with no errors
#define RSR_CRC_ERROR   (UCHAR)0x02        // packet received with CRC error
#define RSR_MULTICAST   (UCHAR)0x20        // packet received was multicast
#define RSR_DISABLED    (UCHAR)0x40        // received is disabled
#define RSR_DEFERRING   (UCHAR)0x80        // receiver is deferring


//
// Constants for the NIC_XMIT_CONFIG register.
//
// Configures how packets are transmitted.
//

#define TCR_NO_LOOPBACK (UCHAR)0x00        // normal operation
#define TCR_LOOPBACK    (UCHAR)0x02        // loopback (set when NIC is stopped)

#define TCR_INHIBIT_CRC (UCHAR)0x01        // inhibit appending of CRC

#define TCR_NIC_LBK     (UCHAR)0x02        // loopback through the NIC
#define TCR_SNI_LBK     (UCHAR)0x04        // loopback through the SNI
#define TCR_COAX_LBK    (UCHAR)0x06        // loopback to the coax


//
// Constants for the NIC_DATA_CONFIG register.
//
// Set data transfer sizes.
//

#define DCR_BYTE_WIDE   (UCHAR)0x00        // byte-wide DMA transfers
#define DCR_WORD_WIDE   (UCHAR)0x01        // word-wide DMA transfers

#define DCR_LOOPBACK    (UCHAR)0x00        // loopback mode (TCR must be set)
#define DCR_NORMAL      (UCHAR)0x08        // normal operation

#define DCR_FIFO_2_BYTE (UCHAR)0x00        // 2-byte FIFO threshhold
#define DCR_FIFO_4_BYTE (UCHAR)0x20        // 4-byte FIFO threshhold
#define DCR_FIFO_8_BYTE (UCHAR)0x40        // 8-byte FIFO threshhold
#define DCR_FIFO_12_BYTE (UCHAR)0x60       // 12-byte FIFO threshhold
#define DCR_AUTO_INIT   (UCHAR)0x10        // Auto-init to remove packets from ring


//
// Constants for the NIC_INTR_MASK register.
//
// Configure which ISR settings actually cause interrupts.
//

#define IMR_RCV         (UCHAR)0x01        // packet received with no errors
#define IMR_XMIT        (UCHAR)0x02        // packet transmitted with no errors
#define IMR_RCV_ERR     (UCHAR)0x04        // error on packet reception
#define IMR_XMIT_ERR    (UCHAR)0x08        // error on packet transmission
#define IMR_OVERFLOW    (UCHAR)0x10        // receive buffer overflow
#define IMR_COUNTER     (UCHAR)0x20        // MSB set on tally counter


//++
//
// VOID
// CardStart(
//    IN PR6040_ADAPTER Adapter
//    )
//
//
// Routine Description:
//
//    Starts the card.
//
// Arguments:
//
//    Adapter - pointer to the adapter block
//
// Return Value:
//
//    None.
//
//--
    //
    // Assume that the card has been stopped as in CardStop.
    //

//#define CardStart(Adapter) \
//    NdisRawWritePortUchar(((Adapter->IoPAddr)+NIC_XMIT_CONFIG), TCR_NO_LOOPBACK)



//++
//
// VOID
// CardSetAllMulticast(
//     IN PR6040_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Enables every bit in the card multicast bit mask.
//  Calls SyncCardSetAllMulticast.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardSetAllMulticast(Adapter) \
    NdisMSynchronizeWithInterrupt(&(Adapter)->Interrupt, \
                SyncCardSetAllMulticast, (PVOID)(Adapter))


//++
//
// VOID
// CardCopyMulticastRegs(
//     IN PR6040_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Writes out the entire multicast bit mask to the card from
//  Adapter->NicMulticastRegs.  Calls SyncCardCopyMulticastRegs.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

 #define CardCopyMulticastRegs(Adapter) \
    NdisMSynchronizeWithInterrupt(&(Adapter)->Interrupt, \
                SyncCardCopyMulticastRegs, (PVOID)(Adapter))

//++
//
// VOID
// CardGetInterruptStatus(
//     IN PR6040_ADAPTER Adapter,
//     OUT PUCHAR InterrupStatus
//     )
//
// Routine Description:
//
//  Reads the interrupt status (ISR) register from the card. Only
//  called at IRQL INTERRUPT_LEVEL.
//
// Arguments:
//
//  Adapter - The adapter block.
//
//  InterruptStatus - Returns the value of ISR.
//
// Return Value:
//
//--

#define CardGetInterruptStatus(_Adapter,_InterruptStatus) \
    NdisRawReadPortUshort(((_Adapter)->IoPAddr+NIC_INTR_STATUS), (_InterruptStatus))


//++
//
// VOID
// CardSetReceiveConfig(
//     IN PR6040_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Sets the receive configuration (RCR) register on the card.
//  The value used is Adapter->NicReceiveConfig. Calls
//  SyncCardSetReceiveConfig.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

#define CardSetReceiveConfig(Adapter) \
     NdisMSynchronizeWithInterrupt(&(Adapter)->Interrupt, \
               SyncCardSetReceiveConfig, (PVOID)(Adapter))


//++
//
// VOID
// CardAcknowledgeOverflowInterrupt(
//     IN PR6040_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Acknowledges an overflow interrupt by setting the bit in
//  the interrupt status (ISR) register. Calls
//  SyncCardAcknowledgeOverflow.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

//Kevin  #define CardAcknowledgeOverflowInterrupt(Adapter) \
//Kevin     SyncCardAcknowledgeOverflow(Adapter)


//++
//
// VOID
// CardAcknowledgeCounterInterrupt(
//     IN PR6040_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Acknowledges a counter interrupt by setting the bit in
//  the interrupt status (ISR) register.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

//#define CardAcknowledgeCounterInterrupt(Adapter) \
//    NdisRawWritePortUchar(((Adapter)->IoPAddr+NIC_INTR_STATUS), ISR_COUNTER)

//++
//
// VOID
// CardUpdateCounters(
//     IN PR6040_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Updates the values of the three counters (frame alignment
//  errors, CRC errors, and missed packets) by reading in their
//  current values from the card and adding them to the ones
//  stored in the Adapter structure. Calls SyncCardUpdateCounters.
//
// Arguments:
//
//  Adapter - The adapter block.
//
// Return Value:
//
//  None.
//
//--

//Kevin  #define CardUpdateCounters(Adapter) \
//Kevin    NdisMSynchronizeWithInterrupt(&(Adapter)->Interrupt, \
//Kevin                SyncCardUpdateCounters, (PVOID)(Adapter))


#endif // _R6040HARDWARE_
