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
/*****************************************************************************
;
;
;    Project : eBoot
;    File    : r6040.c
;    Date    : 2006/08/09
;    Abstract: R6040 driver for KITL
;
;    Modification History:
;    Date      Modify       Description
;    2007/6/22 Daniel       fix the buffer access method for HB301 demo board.
;    2007/7/27 Tim      support KITL with interrupt
;    2007/9/17 Tim      support the phy change detect
;
;*****************************************************************************/
#include <windows.h>
#include <ceddk.h>
#include <ethdbg.h>
#include <oal_log.h>
#include <oal_memory.h>
#include <oal_io.h>
#include <oal_timer.h>
#include <nkexport.h>



//------------------------------------------------------------------------------
#define REG_MCR0                    0x00        /* Reset Value = 0x0000 */ 
#define REG_MCR1                    0x04        /* Reset Value = 0x0010 */
#define REG_MBCR                    0x08        /* Reset Value = 0x1F1A */
#define REG_MTICR                   0x0C        /* Reset Value = 0x0000 */
#define REG_MRICR                   0x10        /* Reset Value = 0x0000 */
#define REG_MTPR                    0x14        /* Reset Value = 0x0000 */
#define REG_MRBSR                   0x18        /* Reset Value = 0x0600 */
#define REG_MRDCR                   0x1A        /* Reset Value = 0x0000 */
#define REG_MLSR                    0x1C        /* Reset Value = 0x0000 */
#define REG_MMDIO                   0x20        /* Reset Value = ------ */
#define REG_MMRD                    0x24        /* Reset Value = 0x0000 */
#define REG_MMWD                    0x28        /* Reset Value = 0x0000 */
#define REG_MTDSA0                  0x2C        /* Reset Value = 0x0000 */
#define REG_MTDSA1                  0x30        /* Reset Value = 0x0000 */
#define REG_MRDSA0                  0x34        /* Reset Value = 0x0600 */
#define REG_MRDSA1                  0x38        /* Reset Value = 0x0000 */
#define REG_MISR                    0x3C        /* Reset Value = 0x0000 */
#define REG_MIER                    0x40        /* Reset Value = 0x0000 */
#define REG_MECISR                  0x44        /* Reset Value = 0x0000 */
#define REG_MECIER                  0x48        /* Reset Value = 0x0000 */
#define REG_MRCNT                   0x50        /* Reset Value = 0x0000 */
#define REG_MECNT0                  0x52        /* Reset Value = 0x0000 */
#define REG_MECNT1                  0x54        /* Reset Value = 0x0000 */
#define REG_MECNT2                  0x56        /* Reset Value = 0x0000 */
#define REG_MECNT3                  0x58        /* Reset Value = 0x0000 */
#define REG_MTCNT                   0x5A        /* Reset Value = 0x0000 */
#define REG_MECNT4                  0x5C        /* Reset Value = 0x0000 */
#define REG_MPCNT                   0x5E        /* Reset Value = 0x0000 */
#define REG_MID01                   0x68        /* Reset Value = ------ */
#define REG_MID02                   0x6A        /* Reset Value = ------ */
#define REG_MID03                   0x6C        /* Reset Value = ------ */
#define REG_MID11                   0x70        /* Reset Value = ------ */
#define REG_MID12                   0x72        /* Reset Value = ------ */
#define REG_MID13                   0x74        /* Reset Value = ------ */
#define REG_MACID                   0xBE        /* Reset Value = 0x6040 */

#define PHY1_ADDR                   1           /* For MAC1 */
#define PHY2_ADDR                   2           /* For MAC2 */
#define PHY_MODE                    0x3100      /* PHY CHIP Register 0 */
#define PHY_CAP                     0x01E1      /* PHY CHIP Register 4 */

#define MBCR_DEFAULT                    0x012A      /* MAC Bus Control Register */

#define MAC_TX_BUFFER_SIZE                      1536
#define MAC_DESC_NUM                    16

#define RX_INT                      0x0001
#define TX_INT                      0x0010
#define PHY_CHG_INT                 0x0200
#define R6040_INT_MASK                  (RX_INT | TX_INT | PHY_CHG_INT)
//------------------------------------------------------------------------------
typedef struct {
    volatile UINT16 status, len;        /* 0-3 */
    
    UINT32  buf;                /* 4-7 */
    
    UINT32  ndesc;              /* 8-B */
    
    UINT32  rev;                /* C-F */
} R6040_DESC;

typedef struct {
    UINT32          ioaddr;
    UINT32          rxPos;
    UINT32          txPos;
    R6040_DESC      *rxDesc;
    R6040_DESC      *txDesc;
    UINT32          pktcnt;
} R6040_BLK;

static R6040_BLK blk6040;

static UINT8 macAddr[8] = {0x00, 0x00, 0x60, 0xee, 0xaa, 0x11};

UINT32 txcnt = 0;
UINT32 rxcnt = 0;

extern UINT32 OALGetTickCount();

#define TO_REAL(Addr)   (OALVAtoPA((VOID *)(Addr)))
#define TO_VIRT(Addr)   (OALPAtoUA((UINT32)(Addr)))

//------------------------------------------------------------------------------
// Read a word data from PHY Chip
static UINT32 phy_read(UINT16 phy_addr, UINT16 reg_idx)
{
    UINT32 i = 0;

    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MMDIO), 0x2000 + reg_idx + (phy_addr << 8));

    do{}while((i++ < 2048) && (INPORT16((UINT16 *)(blk6040.ioaddr + REG_MMDIO)) & 0x2000));

    return INPORT16((UINT16 *)(blk6040.ioaddr + REG_MMRD));
}


//Write a word data from PHY Chip
static void phy_write(UINT16 phy_addr, UINT16 reg_idx, UINT16 dat)
{
    UINT32 i = 0;

    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MMWD), dat);

    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MMDIO), 0x4000 + reg_idx + (phy_addr << 8));

    do{}while((i++ < 2048) && (INPORT16((UINT16 *)(blk6040.ioaddr + REG_MMDIO)) & 0x4000));
}

static int phy_mode_chk()
{
    UINT16  phy_dat ;
    
    phy_dat = phy_read(PHY1_ADDR, 1);
    if( !(phy_dat & 4))
        return 0x8000 ;                 //Link Fail , full duplex
        
    //PHY Auto-Negotiation 
    phy_dat = phy_read(PHY1_ADDR, 1);
    if(phy_dat & 0x0020)
        {
        //Auto-Negotiation mode
        phy_dat = phy_read(PHY1_ADDR, 5);
        phy_dat &= phy_read(PHY1_ADDR, 4);
        if(phy_dat & 0x140)
            phy_dat = 0x8000;
        else
            phy_dat =0;
        }
    else
        {
        //Force mode
        phy_dat = phy_read(PHY1_ADDR, 0);
        if(phy_dat & 0x100)
            phy_dat = 0x8000;
        else
            phy_dat = 0;
        }

    return phy_dat ;
}
//------------------------------------------------------------------------------
//  Function:  R6040InitDMABuffer
//
//------------------------------------------------------------------------------
BOOL R6040InitDMABuffer(UINT32 address, UINT32 size)
{
        BOOL rc = FALSE;
        UINT32 ph;
        UINT32 i;
        UINT32 next;

        ph = address;
        if ((ph & 0x03) != 0) 
            {
            size -= 4 - (ph & 0x03);
            ph = (ph + 3) & ~0x03;
            }

    /* Set to noncached address */

        OALMSGS(OAL_ERROR, (L"R6040InitDMABuffer: 0x%x, %d\r\n", address, size));
    
        if (size < (sizeof(R6040_DESC) * MAC_DESC_NUM * 2))
            {
            OALMSGS(OAL_ERROR, (L"R6040InitDMABuffer is too samll.\r\n"));
            return rc;
            }
    
        memset((UINT8 *)ph, 0, size);
        memset((UINT8 *)&blk6040, 0, sizeof(blk6040));
    
        blk6040.rxDesc = (R6040_DESC *)ph;
        ph += (sizeof(R6040_DESC) * MAC_DESC_NUM);
        blk6040.txDesc = (R6040_DESC *)ph;
        ph += (sizeof(R6040_DESC) * MAC_DESC_NUM);
    
        // Init rx descriptors and buffer
        for (i = 0, next = 1; i < MAC_DESC_NUM; i++, next++)
            {
        //OALMSGS(OAL_ERROR, (L"rxDesc[%d] = 0x%x\r\n", i, (UINT32)&blk6040.rxDesc[i]));
            next = (next > (MAC_DESC_NUM - 1)) ? 0 : next;
            blk6040.rxDesc[i].status = 0x8000;
            blk6040.rxDesc[i].buf = TO_REAL(ph);
            blk6040.rxDesc[i].ndesc = TO_REAL(&blk6040.rxDesc[next]);
            ph += MAC_TX_BUFFER_SIZE;
            }
 
        // Init tx descriptors and buffer
        for (i = 0, next = 1; i < MAC_DESC_NUM; i++, next++)
            {
        //OALMSGS(OAL_ERROR, (L"txDesc[%d] = 0x%x\r\n", i, (UINT32)&blk6040.txDesc[i]));
            next = (next > (MAC_DESC_NUM - 1)) ? 0 : next;
            blk6040.txDesc[i].buf = TO_REAL(ph);
            blk6040.txDesc[i].ndesc = TO_REAL(&blk6040.txDesc[next]);
            ph += MAC_TX_BUFFER_SIZE;
            }
    
        rc = TRUE;
    
        return rc;
}


//------------------------------------------------------------------------------
//  Function:  R6040Init
//
// OUTPORT8 ((UINT8  *)(blk6040.ioaddr + reg), 0x00);
// OUTPORT16((UINT16 *)(blk6040.ioaddr + reg), 0x00);
// OUTPORT32((UINT32 *)(blk6040.ioaddr + reg), 0x00);
//
// value8 = INPORT8 ((UINT8  *)(blk6040.ioaddr  + reg));
// value16 = INPORT16((UINT16 *)(blk6040.ioaddr + reg));
// value32 = INPORT32((UINT32 *)(blk6040.ioaddr + reg));
//
//------------------------------------------------------------------------------
BOOL R6040Init(UINT8 *pAddress, UINT32 offset, UINT16 mac[3])
{
        BOOL rc = FALSE;
        UINT32  val_32 = 0;
        UINT16  val_16 = 0, tmp;
        UINT8   val_8 = 0;
        UINT16  *ma;
        UINT32  i, tmp32;
    UINT8   privMac[8] = {0};
 
        OALMSGS(OAL_ERROR, (L"DRV.R6040Init \r\n"));
    
    OALMSGS(OAL_ERROR, (L"R6040 Edbg driver for eboot and KITL debug mode.\r\n"));
    
        // Get virtual uncached address
        blk6040.ioaddr = (UINT32)pAddress;

    // Check MAC identifier
    val_16 = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MACID));
    if (val_16 != 0x6040)
        {
        OALMSGS(OAL_ERROR, (L"REG_MACID error!\r\n"));
        return 0;
        }

    // Get the MAC address from MAC
    ma = (UINT16 *)privMac;
    ma[0] = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MID01));
    ma[1] = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MID02));
    ma[2] = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MID03));
    if (ma[0] == 0 && ma[1] == 0 && ma[2] == 0)
        {
        OALMSGS(OAL_ERROR, (L"no mac address found!\r\n"));
        ma = (UINT16 *)macAddr;
        }

    OALMSGS(OAL_ERROR, (L"MAC Address:%x:%x:%x:%x:%x:%x\r\n",
                           ma[0] & 0x00FF, ma[0] >> 8,
                           ma[1] & 0x00FF, ma[1] >> 8,
                           ma[2] & 0x00FF, ma[2] >> 8));

    // MAC Reset
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MCR1), 0x1);
    
    // Reset internal state machine
    OUTPORT16((UINT16 *)(blk6040.ioaddr + 0xAC), 0x2);
    OUTPORT16((UINT16 *)(blk6040.ioaddr + 0xAC), 0x0);

    // Waiting for MAC ready 
    for (i = 0; i < 1000000; i++);

    // Set mac address
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MID01), ma[0]);
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MID02), ma[1]);
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MID03), ma[2]);
    memcpy((UINT8 *)mac, (UINT8 *)ma, 6);

    // Set Tx descriptor
    tmp32 = TO_REAL(blk6040.txDesc);
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MTDSA0), (UINT16)tmp32);
    tmp =  (UINT16) (tmp32 >> 16);
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MTDSA1), tmp);
    
#if 0
    // Debug messages for Tx descriptor
    tmp = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MTDSA0));
    OALMSGS(OAL_ERROR, (L"REG_MTDSA0 = 0x%x\r\n", tmp));
    tmp = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MTDSA1));
    OALMSGS(OAL_ERROR, (L"REG_MTDSA1 = 0x%x\r\n", tmp));
#endif

    // Set Rx descriptor
    tmp32 = TO_REAL(blk6040.rxDesc);
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MRDSA0), (UINT16)tmp32);
    tmp =  (UINT16) (tmp32 >> 16);
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MRDSA1), tmp);

#if 0
    // Debug messages for Rx descriptor
    tmp = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MRDSA0));
    OALMSGS(OAL_ERROR, (L"REG_MRDSA0 = 0x%x\r\n", tmp));
    tmp = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MRDSA1));
    OALMSGS(OAL_ERROR, (L"REG_MRDSA1 = 0x%x\r\n", tmp));
#endif

    // PHY mode setting
    phy_write(PHY1_ADDR, 4, PHY_CAP);
    phy_write(PHY1_ADDR, 0, PHY_MODE);
    
    
    if( (phy_read(PHY1_ADDR, 2)==0x2E) && (phy_read(PHY1_ADDR, 3)==0xD000))
        {
        i = phy_read(PHY1_ADDR , 0x1B) | 1 ;
        phy_write(PHY1_ADDR, 0x1B , i );
        }

    // Config MAC bus
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MBCR), MBCR_DEFAULT);
    // Read Clear the status
    INPORT16((UINT16 *)(blk6040.ioaddr + REG_MISR));
    

    // Enable Tx and Rx
    //OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MCR0), 0x1022);
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MCR0), 0x9002);

    rc = TRUE;
    return rc;
}


//------------------------------------------------------------------------------
//  Function:  R6040SendFrame
//
//------------------------------------------------------------------------------
UINT16 R6040SendFrame(UINT8 *pData, UINT32 length)
{
    R6040_DESC  *desc;
    UINT8 *buf;

    if (blk6040.txPos > (MAC_DESC_NUM - 1))
        blk6040.txPos = 0;
    
    desc = &blk6040.txDesc[blk6040.txPos];

    //OALMSGS(OAL_ERROR, (L"R6040SendFrame: desc=0x%x \r\n", desc));

        if (desc->status & 0x8000)
        {
        OALMSGS(OAL_ERROR, (L"R6040SendFrame: no descriptor for CPU! \r\n"));
        return -1;
        }


    // Translate the buf address for eboot or KITL debug mode.
    if ((UINT32)desc & 0x80000000)
        buf = (UINT8 *)TO_VIRT(desc->buf);
    else
        buf = (UINT8 *)desc->buf;
    

    if (length < 60)
        length = 60;    
    memcpy(buf, pData, length);
    
    // Trigger MAC to send packet.
    desc->status = 0x8000;
    desc->len = length;
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MTPR), 0x01);
    
    //OALMSGS(OAL_ERROR, (L"<TxCnt=%d, buf=0x%x, TxPktLen=%d>\r\n", txcnt++, buf, length));
        
    blk6040.txPos++ ;
    
        return 0;
}


//------------------------------------------------------------------------------
//  Function:  R6040GetFrame
//
//------------------------------------------------------------------------------
UINT16 R6040GetFrame(UINT8 *pData, UINT16 *pLength)
{
    R6040_DESC  *desc;
    const UINT8 *buf;
    UINT16      status;
    UINT16      phy_mode;
    UINT16      mcr0;

    // Read Clear the status
    status = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MISR));

        if( status & PHY_CHG_INT )
            {
            phy_mode = phy_mode_chk();
        mcr0 = INPORT16((UINT16 *)(blk6040.ioaddr + REG_MCR0));
        mcr0 &= 0x7FFFF;
        mcr0 |= phy_mode;
        OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MCR0), mcr0);
            }

    if (blk6040.rxPos > (MAC_DESC_NUM - 1))
        blk6040.rxPos = 0;
    
    desc = &blk6040.rxDesc[blk6040.rxPos];
    
    if (desc->status & 0x8000)
        *pLength = 0;
    else
        {
        if ((UINT32)desc & 0x80000000)
            buf = (UINT8 *)TO_VIRT(desc->buf);
        else
        buf = (UINT8 *)desc->buf;

        memcpy(pData, buf, desc->len);
        *pLength = desc->len;
        desc->status = 0x8000;
        //OALMSGS(OAL_ERROR, (L"<RxCnt=%d, buf = 0x%x, RxPktLen=%d>\r\n", rxcnt++, buf, desc->len));
        blk6040.rxPos++;
    }
        
    
    
    return (*pLength);
}


//------------------------------------------------------------------------------
//  Function:  R6040EnableInts
//
//------------------------------------------------------------------------------
VOID R6040EnableInts()
{
    OALMSGS(OAL_ERROR, (L"+DRV.R6040EnableInts\r\n"));  
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MIER), 1);    //KILT only care RX     
}


//------------------------------------------------------------------------------
//  Function:  R6040DisableInts
//
//------------------------------------------------------------------------------
VOID R6040DisableInts()
{
    OALMSGS(OAL_ERROR, (L"+DRV.R6040DisableInts\r\n")); 
    OUTPORT16((UINT16 *)(blk6040.ioaddr + REG_MIER), 0x0000);
}


//------------------------------------------------------------------------------
//  Function:  R6040CurrentPacketFilter
//
//------------------------------------------------------------------------------
void R6040CurrentPacketFilter(UINT32 filter)
{
    OALMSGS(OAL_ERROR, (L"+R6040CurrentPacketFilter\r\n"));
}


//------------------------------------------------------------------------------
//  Function:  R6040MulticastList
//
//------------------------------------------------------------------------------
BOOL R6040MulticastList(UINT8 *pAddresses, UINT32 count)
{
    OALMSGS(OAL_ERROR, (L"+R6040MulticastList\r\n"));
    return 0;
}

