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

    R6040sw.h

Abstract:

    The main header for an Novell 2000 Miniport driver.

Notes:

--*/

#ifndef _R6040SFT_
#define _R6040SFT_

#include "R6040isr.h"

#define DRIVER_MAJOR_VERSION        0x01
#define DRIVER_MINOR_VERSION        0x00

#define R6040_NDIS_MAJOR_VERSION 4
#define R6040_NDIS_MINOR_VERSION 0

/* PCI Register Definitions */
#define PCI_VENDOR_ID_REGISTER      0x00    // PCI Vendor ID Register
#define PCI_DEVICE_ID_REGISTER      0x02    // PCI Device ID Register
#define PCI_CONFIG_ID_REGISTER      0x00    // PCI Configuration ID Register
#define PCI_COMMAND_REGISTER        0x04    // PCI Command Register
#define PCI_STATUS_REGISTER         0x06    // PCI Status Register
#define PCI_REV_ID_REGISTER         0x08    // PCI Revision ID Register
#define PCI_CLASS_CODE_REGISTER     0x09    // PCI Class Code Register
#define PCI_CACHE_LINE_REGISTER     0x0C    // PCI Cache Line Register
#define PCI_LATENCY_TIMER           0x0D    // PCI Latency Timer Register
#define PCI_HEADER_TYPE             0x0E    // PCI Header Type Register
#define PCI_BIST_REGISTER           0x0F    // PCI Built-In SelfTest Register
#define PCI_BAR_0_REGISTER          0x10    // PCI Base Address Register 0
#define PCI_BAR_1_REGISTER          0x14    // PCI Base Address Register 1
#define PCI_BAR_2_REGISTER          0x18    // PCI Base Address Register 2
#define PCI_BAR_3_REGISTER          0x1C    // PCI Base Address Register 3
#define PCI_BAR_4_REGISTER          0x20    // PCI Base Address Register 4
#define PCI_BAR_5_REGISTER          0x24    // PCI Base Address Register 5
#define PCI_SUBVENDOR_ID_REGISTER   0x2C    // PCI SubVendor ID Register
#define PCI_SUBDEVICE_ID_REGISTER   0x2E    // PCI SubDevice ID Register
#define PCI_EXPANSION_ROM           0x30    // PCI Expansion ROM Base Register
#define PCI_INTERRUPT_LINE          0x3C    // PCI Interrupt Line Register
#define PCI_INTERRUPT_PIN           0x3D    // PCI Interrupt Pin Register
#define PCI_MIN_GNT_REGISTER        0x3E    // PCI Min-Gnt Register
#define PCI_MAX_LAT_REGISTER        0x3F    // PCI Max_Lat Register
#define PCI_NODE_ADDR_REGISTER      0x40    // PCI Node Address Register


/* PCI Command Register Bit Definitions */
#define CMD_IO_SPACE            0x0001
#define CMD_MEMORY_SPACE        0x0002
#define CMD_BUS_MASTER          0x0004
#define CMD_SPECIAL_CYCLES      0x0008
#define CMD_MEM_WRT_INVALIDATE  0x0010
#define CMD_VGA_PALLETTE_SNOOP  0x0020
#define CMD_PARITY_RESPONSE     0x0040
#define CMD_WAIT_CYCLE_CONTROL  0x0080
#define CMD_SERR_ENABLE         0x0100
#define CMD_BACK_TO_BACK        0x0200

//
// This macro is used along with the flags to selectively
// turn on debugging.
//

#define DBG_ERROR      1
#define DBG_WARN       2
#define DBG_FUNCTION   4
#define DBG_INIT       8
#define DBG_INTR       16
#define DBG_RCV        32
#define DBG_XMIT       64

#if DBG

#define IF_R6040DEBUG(f) if (R6040DebugFlag & (f))
extern ULONG R6040DebugFlag;

#define R6040_DEBUG_LOUD               0x00000001  // debugging info
#define R6040_DEBUG_VERY_LOUD          0x00000002  // excessive debugging info
#define R6040_DEBUG_LOG                0x00000004  // enable R6040Log
#define R6040_DEBUG_CHECK_DUP_SENDS    0x00000008  // check for duplicate sends
#define R6040_DEBUG_TRACK_PACKET_LENS  0x00000010  // track directed packet lens
#define R6040_DEBUG_WORKAROUND1        0x00000020  // drop DFR/DIS packets
#define R6040_DEBUG_CARD_BAD           0x00000040  // dump data if CARD_BAD
#define R6040_DEBUG_CARD_TESTS         0x00000080  // print reason for failing

//
// Macro for deciding whether to print a lot of debugging information.
//

#define IF_LOUD(A) IF_R6040DEBUG( R6040_DEBUG_LOUD ) { A }
#define IF_VERY_LOUD(A) IF_R6040DEBUG( R6040_DEBUG_VERY_LOUD ) { A }

//
// Whether to use the R6040Log buffer to record a trace of the driver.
//
#define IF_LOG(A) IF_R6040DEBUG( R6040_DEBUG_LOG ) { A }
extern VOID R6040Log(UCHAR);

//
// Whether to do loud init failure
//
#define IF_INIT(A) A

//
// Whether to do loud card test failures
//
#define IF_TEST(A) IF_R6040DEBUG( R6040_DEBUG_CARD_TESTS ) { A }

//
// Windows CE debug zones
//
#define ZONE_ERROR      DEBUGZONE(0)
#define ZONE_WARN       DEBUGZONE(1)
#define ZONE_FUNCTION   DEBUGZONE(2)
#define ZONE_INIT       DEBUGZONE(3)
#define ZONE_INTR       DEBUGZONE(4)
#define ZONE_RCV        DEBUGZONE(5)
#define ZONE_XMIT       DEBUGZONE(6)

#else

//
// This is not a debug build, so make everything quiet.
//
#define IF_LOUD(A)
#define IF_VERY_LOUD(A)
#define IF_LOG(A)
#define IF_INIT(A)
#define IF_TEST(A)

#endif

//
// Adapter->NumBuffers
//
// controls the number of transmit buffers on the packet.
// Choices are 1 through 12.
//

#define DEFAULT_NUMBUFFERS 12


//
// Create a macro for moving memory from place to place.  Makes
// the code more readable and portable in case we ever support
// a shared memory R6040 adapter.
//
#define R6040_MOVE_MEM(dest,src,size) NdisMoveMemory(dest,src,size)


//
// The status of link.
//

typedef enum {
    LINK_FAILED = 0xFFFF,
    HALF_DUPLEX = 0x0000,
    FULL_DUPLEX = 0x8000
} LINK_STATU;


//
// The status of transmit buffers.
//

typedef enum {
    EMPTY = 0x00,
    FULL = 0x02
} BUFFER_STATUS;

//
// Type of an interrupt.
//

typedef enum {
    RECEIVE     = 0x0001,
    TRANSMIT    = 0x0010,
    UNKNOWN     = 0xFFFF
} INTERRUPT_TYPE;

//
// Result of R6040IndicatePacket().
//
typedef enum {
    INDICATE_OK,
    SKIPPED,
    ABORT,
    CARD_BAD
} INDICATE_STATUS;

//************************************************************************//
//For RDC Setting
//************************************************************************//
// ------------------------------------------------------------------------
// Macros for writing shared memory structures - no need for byte flipping

#define READMEM8(   _reg_, _val_ ) ((_val_) = *((volatile BYTE *)(_reg_)))
#define WRITEMEM8(  _reg_, _val_ ) (*((volatile BYTE *)(_reg_)) = (_val_))
#define READMEM16(  _reg_, _val_ ) ((_val_) = *((volatile short *)(_reg_)))
#define WRITEMEM16( _reg_, _val_ ) (*((volatile SHORT *)(_reg_)) = (_val_))
#define READMEM32(  _reg_, _val_ ) ((_val_) = *((volatile LONG *)(_reg_)))
#define WRITEMEM32( _reg_, _val_ ) (*((volatile LONG *)(_reg_)) = (_val_))

/* PHY CHIP Address */
#define PHY1_ADDR   1   /* For MAC1 */
#define PHY2_ADDR   2   /* For MAC2 */
#define PHY_MODE    0x3100  /* PHY CHIP Register 0 */
#define PHY_CAP     0x01E1  /* PHY CHIP Register 4 */

#define BUF_START   32  // Buffer start offset
#define DS_SIZE     32  // Descriptor Size 32 byte
#define DS_STATUS   0   // Descriptor strcture :Status
#define DS_LEN      2   // Descriptor strcture :Length
#define DS_BUFP     4   // Descriptor strcture :Buffer pointer
#define DS_NDSP     8   // Descriptor strcture :Next Descriptor pointer

typedef struct _R6040_DESCRIPTOR_INFO{
   UCHAR*   VirtualAddr;
   NDIS_PHYSICAL_ADDRESS   PhyAddr;
}R6040_DESCRIPTOR_INFO, *PR6040_DESCRIPTOR_INFO;


typedef struct _R6040_DESCRIPTOR {
    USHORT  status;     /* 0-1 */
    USHORT  len;        /* 2-3 */
    ULONG   buf;            /* 4-7 */
    ULONG   ndesc;          /* 8-B */
    USHORT  hidx;           /* C-D */
    USHORT  rev1;           /* E-F */
    USHORT  rev2;           /* 10-11 */
    USHORT  rev3;
}R6040_DESCRIPTOR, * PR6040_DESCRIPTOR;

/* MAC setting */
#define TX_DCNT     32  /* TX descriptor count */
#define RX_DCNT     32  /* RX descriptor count */
#define MAX_BUF_SIZE    0x600
#define ALLOC_DESC_SIZE ((TX_DCNT+RX_DCNT)* (MAX_BUF_SIZE + DS_SIZE))
#define MBCR_DEFAULT    0x012A  /* MAC Bus Control Register */


typedef struct _R6040_PRIVATE {
    //struct net_device_stats stats;
    //spinlock_t lock;
    //struct timer_list timer;
    //struct pci_dev *pdev;

    PR6040_DESCRIPTOR  Rx_Insert_ptr;
    PR6040_DESCRIPTOR  Rx_Remove_ptr;
    PR6040_DESCRIPTOR  Tx_Insert_ptr;
    PR6040_DESCRIPTOR  Tx_Remove_ptr;
    USHORT  Tx_Free_Desc, Rx_Free_Desc, Phy_Addr, Phy_Mode;
    USHORT  MCR0, MCR1;
    //dma_addr_t desc_dma;
    char    *Desc_Pool;
}R6040_PRIVATE, * PR6040_PRIVATE;

ULONG R6040IoPAddr;
int InstanceIndex;

//*************************************************************************//

#define NIC_POLLING_DELAY        1

//
// Size of the ethernet header
//
#define R6040_HEADER_SIZE 14

//
// Size of the ethernet address
//
#define R6040_LENGTH_OF_ADDRESS 6

//
// Maximum number of bytes in a packet
//
#define MAX_PACKET 1514

//
// Number of bytes allowed in a lookahead (max)
//
#define R6040_MAX_LOOKAHEAD (MAX_PACKET - R6040_HEADER_SIZE)

//
// Maximum number of transmit buffers on the card.
//
#define MAX_XMIT_BUFS   12

//
// Definition of a transmit buffer.
//
typedef UINT XMIT_BUF;

//
// Number of 256-byte buffers in a transmit buffer.
//
#define BUFS_PER_TX 1

//
// Size of a single transmit buffer.
//
#define TX_BUF_SIZE (BUFS_PER_TX*256)

#define MAX_RX_DESCRIPTORS  32     // number of Rx descriptors
#define MAX_TX_DESCRIPTORS  32     // number of Tx descriptors

//
// This structure contains information about the driver
// itself.  There is only have one of these structures.
//
typedef struct _DRIVER_BLOCK {

    //
    // NDIS wrapper information.
    //
    NDIS_HANDLE NdisMacHandle;          // returned from NdisRegisterMac
    NDIS_HANDLE NdisWrapperHandle;      // returned from NdisInitializeWrapper

    //
    // Adapters registered for this Miniport driver.
    //
    struct _R6040_ADAPTER * AdapterQueue;

} DRIVER_BLOCK, * PDRIVER_BLOCK;

//
// This structure contains all the information about a single
// adapter that this driver is controlling.
//
typedef struct _R6040_ADAPTER {

    char *Desc_Pool;
    UINT Tx_free_cnt;       // Number of free TX descriptors
    UINT Rx_free_cnt;       // Number of free RX descriptors

    R6040_DESCRIPTOR_INFO  Rx_ring[MAX_RX_DESCRIPTORS];   // location of Rx descriptors

    int Tx_desc_add;              // descriptor index for additions
    int Tx_desc_remove;           // descriptor index for remove

    int Rx_desc_add;              // descriptor index for additions
    int Rx_desc_remove;           // descriptor index for remove

    R6040_DESCRIPTOR_INFO  Tx_ring[MAX_TX_DESCRIPTORS];  // location of Tx descriptors

    NDIS_PHYSICAL_ADDRESS   MemPhysAddr;
    USHORT   MacControl;
    NDIS_MEDIA_STATE    MediaState;

    // spin locks
    NDIS_SPIN_LOCK          Lock;
    NDIS_SPIN_LOCK          SendLock;
    NDIS_SPIN_LOCK          RcvLock;


    //
    // This is the handle given by the wrapper for calling ndis
    // functions.
    //
    NDIS_HANDLE MiniportAdapterHandle;

    //
    // Interrupt object.
    //
    NDIS_MINIPORT_INTERRUPT Interrupt;

    //
    // used by DriverBlock->AdapterQueue
    //
    struct _R6040_ADAPTER * NextAdapter;

    //
    // This is a count of the number of receives that have been
    // indicated in a row.  This is used to limit the number
    // of sequential receives so that one can periodically check
    // for transmit complete interrupts.
    //
    ULONG ReceivePacketCount;

    //
    // Configuration information
    //

    //
    // Number of buffer in this adapter.
    //
    UINT NumBuffers;

    //
    // Physical address of the IoBaseAddress
    //
    PVOID IoBaseAddr;

    //
    // Interrupt number this adapter is using.
    //
    UCHAR InterruptNumber;


    //
    // Number of multicast addresses that this adapter is to support.
    //
    UINT MulticastListMax;

    //
    // The type of bus that this adapter is running on.  Either ISA or PCI
    //
    UCHAR BusType;
    DWORD BusNumber;
    DWORD DevNumber;
    DWORD FunNumber;
    ULONG MemBaseAddr;

    //
    // InterruptType is the next interrupt that should be serviced.
    //
    UCHAR InterruptType;

    //
    // Transmit information.
    //

    //
    // The next available empty transmit buffer.
    //
    XMIT_BUF NextBufToFill;

    //
    // The next full transmit buffer waiting to transmitted.  This
    // is valid only if CurBufXmitting is -1
    //
    XMIT_BUF NextBufToXmit;

    //
    // This transmit buffer that is currently transmitting.  If none,
    // then the value is -1.
    //
    XMIT_BUF CurBufXmitting;

    //
    // TRUE if a transmit has been started, and have not received the
    // corresponding transmit complete interrupt.
    //
    BOOLEAN TransmitInterruptPending;

    //
    // TRUE if a receive buffer overflow occurs while a
    // transmit complete interrupt was pending.
    //
    BOOLEAN OverflowRestartXmitDpc;

    //
    // The length of each packet in the Packets list.
    //
    UINT PacketLens[MAX_TX_DESCRIPTORS];

    //
    // The first packet we have pending.
    //
    PNDIS_PACKET FirstPacket;

    //
    // The tail of the pending queue.
    //
    PNDIS_PACKET LastPacket;

    //
    // The address of the start of the transmit buffer space.
    //
    PUCHAR XmitStart;

    //
    // The address of the start of the receive buffer space.
    PUCHAR PageStart;

    //
    // The address of the end of the receive buffer space.
    //
    PUCHAR PageStop;

    //
    // Status of the last transmit.
    //
    UCHAR XmitStatus;

    //
    // The value to write to the adapter for the start of
    // the transmit buffer space.
    //
    UCHAR NicXmitStart;

    //
    // The value to write to the adapter for the start of
    // the receive buffer space.
    //
    UCHAR NicPageStart;

    //
    // The value to write to the adapter for the end of
    // the receive buffer space.
    //
    UCHAR NicPageStop;


    //
    // Receive information
    //

    //
    // The value to write to the adapter for the next receive
    // buffer that is free.
    //
    UCHAR NicNextPacket;

    //
    // The next receive buffer that will be filled.
    //
    UCHAR Current;

    //
    // Total length of a received packet.
    //
    UINT PacketLen;


    //
    // Operational information.
    //

    //
    // Mapped address of the base io port.
    //
    ULONG IoPAddr;
    ULONG IoLen;
    ULONG VirtualIoAddress;

    //
    // InterruptStatus tracks interrupt sources that still need to be serviced,
    // it is the logical OR of all card interrupts that have been received and not
    // processed and cleared. (see also INTERRUPT_TYPE definition in R6040.h)
    //
    USHORT InterruptStatus;

    //
    // The ethernet address currently in use.
    //
    UCHAR StationAddress[R6040_LENGTH_OF_ADDRESS];

    //
    // The ethernet address that is burned into the adapter.
    //
    UCHAR PermanentAddress[R6040_LENGTH_OF_ADDRESS];

    //
    // The adapter space address of the start of on board memory.
    //
    PUCHAR RamBase;

    //
    // The number of K on the adapter.
    //
    ULONG RamSize;

    //
    // The current packet filter in use.
    //
    ULONG PacketFilter;

    //
    // TRUE if a receive buffer overflow occured.
    //
    BOOLEAN BufferOverflow;

    //
    // TRUE if the driver needs to call NdisMEthIndicateReceiveComplete
    //
    BOOLEAN IndicateReceiveDone;

    //
    // TRUE if this is an R6040 in an eight bit slot.
    //
    BOOLEAN EightBitSlot;

    //
    // Flag that is set when we are shutting down the interface
    //
    BOOLEAN ShuttingDown;

    //
    // Statistics used by Set/QueryInformation.
    //

    ULONG FramesXmitGood;               // Good Frames Transmitted
    ULONG FramesRcvGood;                // Good Frames Received
    ULONG FramesXmitBad;                // Bad Frames Transmitted
    ULONG FramesXmitOneCollision;       // Frames Transmitted with one collision
    ULONG FramesXmitManyCollisions;     // Frames Transmitted with > 1 collision
    ULONG FrameAlignmentErrors;         // FAE errors counted
    ULONG CrcErrors;                    // CRC errors counted
    ULONG MissedPackets;                // missed packet counted

    //
    // Reset information.
    //

    UCHAR NicMulticastRegs[8];          // contents of card multicast registers
    UCHAR NicReceiveConfig;             // contents of NIC RCR
    UCHAR NicInterruptMask;             // contents of NIC IMR when ints enabled

    //
    // The lookahead buffer size in use.
    //
    ULONG MaxLookAhead;

    //
    // These are for the current packet being indicated.
    //

    //
    // The NIC appended header.  Used to find corrupted receive packets.
    //
    UCHAR PacketHeader[4];

    //
    // R6040 address of the beginning of the packet.
    //
    PUCHAR PacketHeaderLoc;

    //
    // Lookahead buffer
    //
    UCHAR Lookahead[R6040_MAX_LOOKAHEAD + R6040_HEADER_SIZE];

    //
    // List of multicast addresses in use.
    //
    CHAR Addresses[DEFAULT_MULTICASTLISTMAX][R6040_LENGTH_OF_ADDRESS];
    ULONG NumMulticastAddressesInUse;


    //
    //  Plug and play adapter.
    //  Bus driver does all the necessary configurations.
    //

    BOOL    bPlugAndPlay;

    //
    //  ISR handle from LoadIntChainHandler()
    //

    HANDLE  hISR;

   R6040_ISR_INFO IsrInfo;  // Shared with R6040isr.dll

   INT     InstanceIndex;
   INT     PhyAddr;
   INT     PhyMode;

} R6040_ADAPTER, * PR6040_ADAPTER;



//
// Given a MiniportContextHandle return the PR6040_ADAPTER
// it represents.
//
#define PR6040_ADAPTER_FROM_CONTEXT_HANDLE(Handle) \
    ((PR6040_ADAPTER)(Handle))

//
// Given a pointer to a R6040_ADAPTER return the
// proper MiniportContextHandle.
//
#define CONTEXT_HANDLE_FROM_PR6040_ADAPTER(Ptr) \
    ((NDIS_HANDLE)(Ptr))

//
// Macros to extract high and low bytes of a word.
//
#define MSB(Value) ((USHORT)((((ULONG)Value) >> 16) & 0xffff))
#define LSB(Value) ((USHORT)(((ULONG)Value) & 0xffff))

//
// What we map into the reserved section of a packet.
// Cannot be more than 8 bytes (see ASSERT in R6040.c).
//
typedef struct _MINIPORT_RESERVED {
    PNDIS_PACKET Next;    // used to link in the queues (4 bytes)
} MINIPORT_RESERVED, * PMINIPORT_RESERVED;


//
// Retrieve the MINIPORT_RESERVED structure from a packet.
//
#define RESERVED(Packet) ((PMINIPORT_RESERVED)((Packet)->MiniportReserved))

//
// Procedures which log errors.
//

typedef enum _R6040_PROC_ID {
    cardReset,
    cardCopyDownPacket,
    cardCopyDownBuffer,
    cardCopyUp
} R6040_PROC_ID;


//
// Special error log codes.
//
#define R6040_ERRMSG_CARD_SETUP          (ULONG)0x01
#define R6040_ERRMSG_DATA_PORT_READY     (ULONG)0x02
#define R6040_ERRMSG_HANDLE_XMIT_COMPLETE (ULONG)0x04

//
// Declarations for functions in R6040.c.
//
NDIS_STATUS
R6040SetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    );

VOID
R6040Shutdown(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
R6040Halt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
R6040RegisterAdapter(
    IN PR6040_ADAPTER Adapter,
    IN NDIS_HANDLE ConfigurationHandle,
    IN BOOLEAN ConfigError,
    IN ULONG ConfigErrorValue
    );

NDIS_STATUS
R6040Initialize(
    OUT PNDIS_STATUS OpenErrorStatus,
    OUT PUINT SelectedMediumIndex,
    IN PNDIS_MEDIUM MediumArray,
    IN UINT MediumArraySize,
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_HANDLE ConfigurationHandle
    );

NDIS_STATUS
R6040TransferData(
    OUT PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred,
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_HANDLE MiniportReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer
    );

NDIS_STATUS
R6040Send(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags
    );

NDIS_STATUS
R6040Reset(
    OUT PBOOLEAN AddressingReset,
    IN NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
R6040QueryInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    );

VOID
R6040Halt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

NDIS_STATUS
DispatchSetPacketFilter(
    IN PR6040_ADAPTER Adapter
    );

NDIS_STATUS
DispatchSetMulticastAddressList(
    IN PR6040_ADAPTER Adapter
    );


//
// Interrup.c
//
BOOLEAN R6040CheckForHang(
    IN  NDIS_HANDLE     MiniportAdapterContext
    );

VOID
R6040EnableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
R6040DisableInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
R6040Isr(
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueDpc,
    IN PVOID Context
    );

VOID
R6040HandleInterrupt(
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
R6040XmitDpc(
    IN PR6040_ADAPTER Adapter
    );

BOOLEAN
R6040RcvDpc(
    IN PR6040_ADAPTER Adapter
    );


//
// Declarations of functions in card.c.
//

//********************************************************************//
//For RDC Setting
//********************************************************************//
INT PHY_Read(IN PR6040_ADAPTER Adapter,IN INT phy_addr, IN INT reg_idx);
VOID Phy_Write(IN PR6040_ADAPTER Adapter,IN USHORT phy_addr, IN USHORT reg_idx, IN USHORT dat);
INT PHY_Mode_Chk(IN PR6040_ADAPTER Adapter);
BOOLEAN IsRdcMacID(IN PR6040_ADAPTER Adapter);
VOID ResetTxRing(IN PR6040_ADAPTER Adapter);
VOID ResetRxRing(IN PR6040_ADAPTER Adapter);
NDIS_STATUS TransmitMemAllocate(IN PR6040_ADAPTER Adapter);
NDIS_STATUS ReceiveMemAllocate(IN PR6040_ADAPTER Adapter);
//********************************************************************//

VOID CardSetMulticast(IN PR6040_ADAPTER Adapter,int num_addrs,char *address_list);
VOID CardSetMACAddress();

BOOLEAN CardAllocateTxRxDescriptor(IN PR6040_ADAPTER Adapter
    );

BOOLEAN
CardInitialize(
    IN PR6040_ADAPTER Adapter
    );

BOOLEAN
CardReadEthernetAddress(
    IN PR6040_ADAPTER Adapter
    );

VOID
CardStop(
    IN PR6040_ADAPTER Adapter
    );

BOOLEAN
CardTest(
    IN PR6040_ADAPTER Adapter
    );

BOOLEAN
CardReset(
    IN PR6040_ADAPTER Adapter
    );

BOOLEAN
CardCopyDownPacket(
    IN PR6040_ADAPTER Adapter,
    IN PNDIS_PACKET Packet,
    OUT UINT * Length
    );

VOID
CardStartXmit(
    IN PR6040_ADAPTER Adapter
    );

BOOLEAN
SyncCardStop(
    IN PVOID SynchronizeContext
    );


BOOLEAN
SyncCardSetReceiveConfig(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardSetAllMulticast(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardSetInterruptMask(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardAcknowledgeOverflow(
    IN PVOID SynchronizeContext
    );

BOOLEAN
SyncCardUpdateCounters(
    IN PVOID SynchronizeContext
    );

VOID
CardFillMulticastRegs(
    IN PR6040_ADAPTER Adapter
    );



VOID
CardGetMulticastBit(
    IN UCHAR Address[R6040_LENGTH_OF_ADDRESS],
    OUT UCHAR * Byte,
    OUT UCHAR * Value
    );


    
    
/*++

Routine Description:

    Determines the type of the interrupt on the card. The order of
    importance is overflow, then transmit complete, then receive.
    Counter MSB is handled first since it is simple.

Arguments:

    Adapter - pointer to the adapter block

    InterruptStatus - Current Interrupt Status.

Return Value:

    The type of the interrupt

--*/
#define CARD_GET_INTERRUPT_TYPE(_A, _I)            \
        (_I & (MIER_TXENDE)) ?           \
          TRANSMIT :                               \
        (_I & (MIER_RXENDE)) ?                           \
          RECEIVE :                                \
              UNKNOWN


NDIS_STATUS
LoadISR(PR6040_ADAPTER Adapter);

NDIS_STATUS
UnloadISR(PR6040_ADAPTER Adapter);

#define SYSINTR_INVALID         0xFFFFFFFF

//++
//
// void
// CardBlockInterrupts(
//     IN PR6040_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Blocks all interrupts from the card by clearing the
//  interrupt mask (IMR) register. Only called from
//  IRQL INTERRUPT_LEVEL.
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
__inline void CardBlockInterrupts(PR6040_ADAPTER Adapter)
{
   //Kevin   if (Adapter->IsrInfo.pIntMaskVirt) {
   //     InterlockedExchange(Adapter->IsrInfo.pIntMaskVirt, MIER_CLEAN_MASK);
   // }
    NdisRawWritePortUshort(Adapter->IoPAddr+NIC_INTR_ENABLE, MIER_CLEAN_MASK);
}


//++
//
// VOID
// CardUnblockInterrupts(
//     IN PR6040_ADAPTER Adapter
//     )
//
// Routine Description:
//
//  Unblocks all interrupts from the card by setting the
//  interrupt mask (IMR) register. Only called from IRQL
//  INTERRUPT_LEVEL.
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

__inline void CardUnblockInterrupts(PR6040_ADAPTER Adapter)
{
  //Kevin   if (Adapter->IsrInfo.pIntMaskVirt) {
  //      InterlockedExchange(Adapter->IsrInfo.pIntMaskVirt, Adapter->NicInterruptMask);
  //  }
    NdisRawWritePortUshort(Adapter->IoPAddr+NIC_INTR_ENABLE, Adapter->NicInterruptMask);
}

#endif // R6040SFT

