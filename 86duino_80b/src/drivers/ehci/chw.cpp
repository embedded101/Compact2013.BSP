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
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Module Name:
//     CHW.cpp
// Abstract:
//     This file implements the EHCI specific register routines
//
// Notes:
//
//
#include <windows.h>
#include <nkintr.h>
#define HCD_SUSPEND_RESUME 1 // For test only
#include <Uhcdddsi.h>
#include <globals.hpp>
#include <td.h>
#include <ctd.h>
#include <chw.h>

#ifndef _PREFAST_
#pragma warning(disable: 4068) // Disable pragma warnings
#endif

// ******************************** CDummyPipe **********************************               
const USB_ENDPOINT_DESCRIPTOR dummpDesc = {
    sizeof(USB_ENDPOINT_DESCRIPTOR),USB_ENDPOINT_DESCRIPTOR_TYPE, 0xff,  USB_ENDPOINT_TYPE_INTERRUPT,8,1
};
CDummyPipe::CDummyPipe(IN CPhysMem * const pCPhysMem)
: CPipe( &dummpDesc,FALSE,TRUE,0xff,0xff,0xff,NULL,NULL)
, m_pCPhysMem(pCPhysMem)
{
    ASSERT( m_pCPhysMem!=NULL);
    m_bFrameSMask = 0xff;
    m_bFrameCMask = 0;

};
// ************************************CPeriodicMgr******************************  

CPeriodicMgr::CPeriodicMgr(IN CPhysMem * const pCPhysMem, DWORD dwFrameSize)
//
// Purpose: Contructor :Periodic Transfer or Queue Head Manage module
//
// Parameters:  pCPhysMem - pointer to CPhysMem object
//              dwFrameSize - Isoch Frame Size (Mached with hardware).
//
// Returns: Nothing
//
// Notes: 
// ******************************************************************
    : m_pCPhysMem(pCPhysMem)
    , m_dwFrameSize(dwFrameSize)
    , m_pCDumpPipe(new CDummyPipe(pCPhysMem))
{
    ASSERT(pCPhysMem);
    ASSERT(dwFrameSize == 0x400|| dwFrameSize== 0x200 || dwFrameSize== 0x100);
    m_pFrameList = NULL;
    m_pFramePhysAddr = 0;
    m_dwFrameMask=0xff;
    switch(dwFrameSize) {
        case 0x400: default:
            m_dwFrameMask=0x3ff;
            break;
        case 0x200:
            m_dwFrameMask=0x1ff;
            break;
        case 0x100:
            m_dwFrameMask=0xff;
            break;
    }
    ASSERT(m_pCDumpPipe);
    // Create Dummy Pipe for static    
}
// ******************************************************************               
CPeriodicMgr::~CPeriodicMgr()
//
// Purpose: Decontructor :Periodic Transfer or Queue Head Manage module
//
// Parameters:  
//
// Returns: Nothing
//
// Notes: 
// ******************************************************************
{
    DeInit();
    if (m_pCDumpPipe)
        delete m_pCDumpPipe;
}
// ******************************************************************               
BOOL CPeriodicMgr::Init()
//
// Purpose: Decontructor :Periodic Transfer or Queue Head Manage module Initilization
//
// Parameters:  
//
// Returns: Nothing
//
// Notes: 
// ******************************************************************
{
    
    Lock();
    if ( m_dwFrameSize == 0x400 ||  m_dwFrameSize== 0x200 ||  m_dwFrameSize== 0x100) {
         if (m_pCPhysMem && m_pCPhysMem->AllocateSpecialMemory(m_dwFrameSize*sizeof(DWORD),  ( UCHAR ** )&m_pFrameList))
             m_pFramePhysAddr = m_pCPhysMem->VaToPa((UCHAR *)m_pFrameList);
         else {
            Unlock();
            ASSERT(FALSE);
        }
    }
    ASSERT(m_pFrameList!=NULL);
    for(DWORD dwIndex=0;dwIndex< 2*PERIOD_TABLE_SIZE;dwIndex++) {
        m_pStaticQHArray[dwIndex]= new(m_pCPhysMem) CQH(m_pCDumpPipe);
        if (m_pStaticQHArray[dwIndex] == NULL) {
            Unlock();
            return FALSE;
        }        
    }
    // Actually the 0 never be used. 
    m_pStaticQHArray[0]->QueueQHead(NULL);
    m_pStaticQHArray[1]->QueueQHead(NULL);
    DWORD dwForwardBase=1;
    DWORD dwForwardMask=0;
    for(dwIndex=2;dwIndex< 2*PERIOD_TABLE_SIZE;dwIndex++) {
        if ((dwIndex & (dwIndex-1))==0) { // power of 2.
            dwForwardBase = dwIndex/2;
            dwForwardMask = dwForwardBase -1 ;
        }
        if (m_pStaticQHArray[dwIndex]) {
            m_pStaticQHArray[dwIndex]->QueueQHead(m_pStaticQHArray[dwForwardBase + (dwIndex & dwForwardMask)]);// binary queue head.
        }
        else {
            Unlock();
            return FALSE;
        }
    }
    //Attahed QHead to  FrameList;
    if (m_dwFrameSize && m_pFrameList) {
        for (dwIndex=0;dwIndex<m_dwFrameSize;dwIndex++) {
            CQH * pQH = m_pStaticQHArray[PERIOD_TABLE_SIZE +  dwIndex % PERIOD_TABLE_SIZE];
            if (pQH) {
                CNextLinkPointer staticQueueHead;
                staticQueueHead.SetNextPointer(pQH->GetPhysAddr(),TYPE_SELECT_QH,TRUE);
                *(m_pFrameList+dwIndex) = staticQueueHead.GetDWORD(); //Invalid Physical pointer.
            }
            else {
                Unlock();
                return FALSE;
            }
        }
    }
    else {
        Unlock();
        return FALSE;
    }
    Unlock();
    return TRUE;
}
// ******************************************************************               
void CPeriodicMgr::DeInit()
//
// Purpose: Decontructor :Periodic Transfer or Queue Head Manage module DeInitilization
//
// Parameters:  
//
// Returns: Nothing
//
// Notes: 
// ******************************************************************
{
    Lock();
    for(DWORD dwIndex=0;dwIndex< 2*PERIOD_TABLE_SIZE;dwIndex++) {
        if (m_pStaticQHArray[dwIndex]) {
            //delete( m_pCPhysMem, 0) m_pStaticQHArray[dwIndex];
            m_pStaticQHArray[dwIndex]->~CQH();
            m_pCPhysMem->FreeMemory((PBYTE)m_pStaticQHArray[dwIndex],m_pCPhysMem->VaToPa((PBYTE)m_pStaticQHArray[dwIndex]),CPHYSMEM_FLAG_HIGHPRIORITY | CPHYSMEM_FLAG_NOBLOCK);
            m_pStaticQHArray[dwIndex] = NULL;
        }
    }
    if (m_pFrameList) {
         m_pCPhysMem->FreeSpecialMemory((PBYTE)m_pFrameList);
         m_pFrameList = NULL;
    }
    Unlock();
}
// ******************************************************************               
BOOL CPeriodicMgr::QueueITD(CITD * piTD,DWORD FrameIndex)
//
// Purpose: Decontructor :Queue High Speed Isoch Trasnfer.
//
// Parameters:  
//
// Returns: Nothing
//
// Notes: 
// ******************************************************************
{
    FrameIndex &= m_dwFrameMask;
    Lock();
    if (piTD && m_pFrameList && FrameIndex< m_dwFrameSize) {
        ASSERT(piTD->CNextLinkPointer::GetLinkValid()==FALSE);
        CNextLinkPointer thisITD;
        thisITD.SetNextPointer(piTD->GetPhysAddr(),TYPE_SELECT_ITD,TRUE);
        piTD->CNextLinkPointer::SetDWORD(*(m_pFrameList + FrameIndex));
        *(m_pFrameList+FrameIndex) = thisITD.GetDWORD();
        Unlock();
        return TRUE;
    }
    else {
        ASSERT(FALSE);
    }
    Unlock();
    return FALSE;
}
// ******************************************************************               
BOOL CPeriodicMgr::QueueSITD(CSITD * psiTD,DWORD FrameIndex)
//
// Purpose: Decontructor :Queue High Speed Isoch Trasnfer.
//
// Parameters:  
//
// Returns: Nothing
//
// Notes: 
// ******************************************************************
{
    FrameIndex &= m_dwFrameMask;
    Lock();
    if (psiTD && m_pFrameList && FrameIndex < m_dwFrameSize ) {
        ASSERT(psiTD->CNextLinkPointer::GetLinkValid()==FALSE);
        CNextLinkPointer thisITD;
        thisITD.SetNextPointer( psiTD->GetPhysAddr(),TYPE_SELECT_SITD,TRUE);
        psiTD->CNextLinkPointer::SetDWORD(*(m_pFrameList+ FrameIndex  ));
        *(m_pFrameList+ FrameIndex) = thisITD.GetDWORD();
        Unlock();
        return TRUE;
    }
    else {
        ASSERT(FALSE);
    }
    Unlock();
    return FALSE;
}
BOOL CPeriodicMgr::DeQueueTD(DWORD dwPhysAddr,DWORD FrameIndex)
{
    FrameIndex &= m_dwFrameMask;
    Lock();
    if (m_pFrameList && FrameIndex< m_dwFrameSize) {
        CNextLinkPointer * curPoint = (CNextLinkPointer *)(m_pFrameList+ FrameIndex);
        if (curPoint!=NULL && curPoint->GetLinkValid() && 
                curPoint->GetLinkType()!= TYPE_SELECT_QH &&
                curPoint->GetPointer() != dwPhysAddr ) {
            curPoint=curPoint->GetNextLinkPointer(m_pCPhysMem);
        }
        if (curPoint && curPoint->GetPointer() == dwPhysAddr) { // We find it
            CNextLinkPointer * pNextPoint=curPoint->GetNextLinkPointer(m_pCPhysMem);
            if (pNextPoint ) {
                curPoint->SetDWORD(pNextPoint->GetDWORD());
                Unlock();
                return TRUE;
            }
            else
                ASSERT(FALSE);
        }
        //else 
        //    ASSERT(FALSE);
    }
    else 
        ASSERT(FALSE);
    Unlock();
    return FALSE;
}
PERIOD_TABLE CPeriodicMgr::periodTable[64] =
   {   // period, qh-idx, s-mask
        1,  0, 0xFF,        // Dummy
        1,  0, 0xFF,        // 1111 1111 bits 0..7
        
        2,  0, 0x55,        // 0101 0101 bits 0,2,4,6
        2,  0, 0xAA,        // 1010 1010 bits 1,3,5,7
        
        4,  0, 0x11,        // 0001 0001 bits 0,4 
        4,  0, 0x44,        // 0100 0100 bits 2,6 
        4,  0, 0x22,        // 0010 0010 bits 1,5
        4,  0, 0x88,        // 1000 1000 bits 3,7
        
        8,  0, 0x01,        // 0000 0001 bits 0
        8,  0, 0x10,        // 0001 0000 bits 4
        8,  0, 0x04,        // 0000 0100 bits 2 
        8,  0, 0x40,        // 0100 0000 bits 6
        8,  0, 0x02,        // 0000 0010 bits 1
        8,  0, 0x20,        // 0010 0000 bits 5
        8,  0, 0x08,        // 0000 1000 bits 3
        8,  0, 0x80,        // 1000 0000 bits 7
 
        16,  1, 0x01,       // 0000 0001 bits 0 
        16,  2, 0x01,       // 0000 0001 bits 0 
        16,  1, 0x10,       // 0001 0000 bits 4
        16,  2, 0x10,       // 0001 0000 bits 4 
        16,  1, 0x04,       // 0000 0100 bits 2  
        16,  2, 0x04,       // 0000 0100 bits 2  
        16,  1, 0x40,       // 0100 0000 bits 6  
        16,  2, 0x40,       // 0100 0000 bits 6 
        16,  1, 0x02,       // 0000 0010 bits 1 
        16,  2, 0x02,       // 0000 0010 bits 1 
        16,  1, 0x20,       // 0010 0000 bits 5 
        16,  2, 0x20,       // 0010 0000 bits 5 
        16,  1, 0x08,       // 0000 1000 bits 3 
        16,  2, 0x08,       // 0000 1000 bits 3 
        16,  1, 0x80,       // 1000 0000 bits 7   
        16,  2, 0x80,       // 1000 0000 bits 7 

        32,  3, 0x01,       // 0000 0000 bits 0
        32,  5, 0x01,       // 0000 0000 bits 0
        32,  4, 0x01,       // 0000 0000 bits 0
        32,  6, 0x01,       // 0000 0000 bits 0
        32,  3, 0x10,       // 0000 0000 bits 4
        32,  5, 0x10,       // 0000 0000 bits 4
        32,  4, 0x10,       // 0000 0000 bits 4
        32,  6, 0x10,       // 0000 0000 bits 4
        32,  3, 0x04,       // 0000 0000 bits 2
        32,  5, 0x04,       // 0000 0000 bits 2
        32,  4, 0x04,       // 0000 0000 bits 2
        32,  6, 0x04,       // 0000 0000 bits 2
        32,  3, 0x40,       // 0000 0000 bits 6
        32,  5, 0x40,       // 0000 0000 bits 6
        32,  4, 0x40,       // 0000 0000 bits 6 
        32,  6, 0x40,       // 0000 0000 bits 6
        32,  3, 0x02,       // 0000 0000 bits 1
        32,  5, 0x02,       // 0000 0000 bits 1
        32,  4, 0x02,       // 0000 0000 bits 1
        32,  6, 0x02,       // 0000 0000 bits 1
        32,  3, 0x20,       // 0000 0000 bits 5
        32,  5, 0x20,       // 0000 0000 bits 5
        32,  4, 0x20,       // 0000 0000 bits 5
        32,  6, 0x20,       // 0000 0000 bits 5
        32,  3, 0x04,       // 0000 0000 bits 3
        32,  5, 0x04,       // 0000 0000 bits 3
        32,  4, 0x04,       // 0000 0000 bits 3
        32,  6, 0x04,       // 0000 0000 bits 3
        32,  3, 0x40,       // 0000 0000 bits 7
        32,  5, 0x40,       // 0000 0000 bits 7
        32,  4, 0x40,       // 0000 0000 bits 7
        32,  6, 0x40,       // 0000 0000 bits 7
        
    };

CQH * CPeriodicMgr::QueueQHead(CQH * pQh,UCHAR uInterval,UCHAR offset,BOOL bHighSpeed)
{   
    if (pQh) {
        if (uInterval> PERIOD_TABLE_SIZE)
            uInterval= PERIOD_TABLE_SIZE;
        Lock();
        for (UCHAR bBit=PERIOD_TABLE_SIZE;bBit!=0;bBit>>=1) {
            if ((bBit & uInterval)!=0) { // THis is correct interval
                // Normalize the parameter.
                uInterval = bBit;
                if (offset>=uInterval)
                    offset = uInterval -1;
                CQH * pStaticQH=NULL ;
                if (bHighSpeed) {
                    pStaticQH=m_pStaticQHArray[ periodTable[uInterval+offset].qhIdx +1] ;
                    pQh->SetSMask(periodTable[uInterval+offset].InterruptScheduleMask);
                }
                else 
                    pStaticQH =  m_pStaticQHArray[uInterval+offset];
                if (pStaticQH!=NULL) {
                    pQh->QueueQHead( pStaticQH->GetNextQueueQHead(m_pCPhysMem));
                    pStaticQH->QueueQHead( pQh );
                    Unlock();
                    return pStaticQH;
                }
                else
                    ASSERT(FALSE);                    
            }
        }
        ASSERT(FALSE);
        CQH * pStaticQH = m_pStaticQHArray[1];
        if (pStaticQH!=NULL) {
            pQh->QueueQHead( pStaticQH->GetNextQueueQHead(m_pCPhysMem));
            if (bHighSpeed)
                pQh->SetSMask(0xff);
            pStaticQH->QueueQHead( pQh );
            Unlock();
            return pStaticQH;
        }
        else
            ASSERT(FALSE);                    

        Unlock();
    }
    ASSERT(FALSE);
    return NULL;
    
}

BOOL CPeriodicMgr::DequeueQHead( CQH * pQh)
{
    if (pQh==NULL) {
        ASSERT(FALSE);
        return FALSE;
    }
    Lock();
    for (DWORD dwIndex=PERIOD_TABLE_SIZE;dwIndex<2*PERIOD_TABLE_SIZE;dwIndex ++) {
        CQH *pCurPrev= m_pStaticQHArray[dwIndex];
        if (pCurPrev!=NULL) {
            while (pCurPrev!=NULL) {
                CQH *pCur=pCurPrev->GetNextQueueQHead(m_pCPhysMem);
                if (pCur == pQh)
                    break;
                else
                    pCurPrev = pCur;
            }
            if (pCurPrev!=NULL) { // Found
                ASSERT(pCurPrev->GetNextQueueQHead(m_pCPhysMem) == pQh);
                pCurPrev->QueueQHead(pQh->GetNextQueueQHead(m_pCPhysMem));
                pQh->QueueQHead(NULL);
                Unlock();
                Sleep(2); // Make Sure it outof EHCI Scheduler.
                return TRUE;
            }
                    
        }
        else
            ASSERT(FALSE);
    }
    Unlock();
    ASSERT(FALSE);
    return FALSE;
}


// ************************************CAsyncMgr******************************  


CAsyncMgr::CAsyncMgr(IN CPhysMem * const pCPhysMem)
    :m_pCPhysMem(pCPhysMem)
    , m_pCDumpPipe(new CDummyPipe(pCPhysMem))
{
     m_pStaticQHead =NULL;
}
CAsyncMgr::~CAsyncMgr()
{
    DeInit();
    if (m_pCDumpPipe)
        delete m_pCDumpPipe;
}
BOOL CAsyncMgr::Init()
{
    Lock();
    m_pStaticQHead = new (m_pCPhysMem) CQH(m_pCDumpPipe);
    if (m_pStaticQHead) {
        m_pStaticQHead->SetReclamationFlag(TRUE);
        m_pStaticQHead ->QueueQHead(m_pStaticQHead); // Point to itself.
        Unlock();
        return TRUE;
    }
    Unlock();
    return FALSE;
}
void CAsyncMgr::DeInit()
{
    Lock();
    if (m_pStaticQHead){
        //delete (m_pCPhysMem) m_pStaticQHead;
        m_pStaticQHead->~CQH();
        m_pCPhysMem->FreeMemory((PBYTE)m_pStaticQHead,m_pCPhysMem->VaToPa((PBYTE)m_pStaticQHead),CPHYSMEM_FLAG_HIGHPRIORITY | CPHYSMEM_FLAG_NOBLOCK);
    }
    m_pStaticQHead= NULL;
    Unlock();
}
CQH *  CAsyncMgr::QueueQH(CQH * pQHead)
{
    if (m_pStaticQHead && pQHead){
        Lock();
        pQHead->QueueQHead( m_pStaticQHead ->GetNextQueueQHead(m_pCPhysMem));
        m_pStaticQHead ->QueueQHead(pQHead);
        Unlock();
        return m_pStaticQHead;
    };
    return NULL;        
}
BOOL CAsyncMgr::DequeueQHead( CQH * pQh)
{
    CQH * pPrevQH = m_pStaticQHead;
    CQH * pCurQH = NULL;
    Lock();
    for (DWORD dwIndex=0;dwIndex<0x1000;dwIndex++)
        if (pPrevQH) {
            pCurQH= pPrevQH->GetNextQueueQHead(m_pCPhysMem);
            if (pCurQH == m_pStaticQHead || pCurQH == pQh)
                break;
            else
                pPrevQH = pCurQH;
        };
    if ( pCurQH && pPrevQH &&  pCurQH == pQh) {
        pPrevQH->QueueQHead(pCurQH ->GetNextQueueQHead(m_pCPhysMem));
        Unlock();
        return TRUE;
    }
    else
        ASSERT(FALSE);
    Unlock();
    return FALSE;
        
};

// ******************************BusyPipeList****************************
BOOL  CBusyPipeList::Init()
{
    m_fCheckTransferThreadClosing=FALSE;
    m_pBusyPipeList = NULL;
#ifdef DEBUG
    m_debug_numItemsOnBusyPipeList=0;
#endif

    return TRUE;

}
void CBusyPipeList::DeInit()
{
    m_fCheckTransferThreadClosing=TRUE;    
}
// Scope: private static
ULONG CBusyPipeList::CheckForDoneTransfersThread( )
//
// Purpose: Thread for checking whether busy pipes are done their
//          transfers. This thread should be activated whenever we
//          get a USB transfer complete interrupt (this can be
//          requested by the InterruptOnComplete field of the TD)
//
// Parameters: 32 bit pointer passed when instantiating thread (ignored)
//                       
// Returns: 0 on thread exit
//
// Notes: 
// ******************************************************************
{
    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE, (TEXT("EHCI: +CPipe::CheckForDoneTransfersThread\n")) );

    PPIPE_LIST_ELEMENT pPrev = NULL;
    PPIPE_LIST_ELEMENT pCurrent = NULL;

    Lock();
#ifdef DEBUG // make sure m_debug_numItemsOnBusyPipeList is accurate
    {
        int debugCount = 0;
        PPIPE_LIST_ELEMENT pDebugElement = m_pBusyPipeList;
        while ( pDebugElement != NULL ) {
            pDebugElement = pDebugElement->pNext;
            debugCount++;
        }
        DEBUGCHK( debugCount == m_debug_numItemsOnBusyPipeList );
    }
    BOOL fDebugNeedProcessing = m_debug_numItemsOnBusyPipeList > 0;
    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE && fDebugNeedProcessing, (TEXT("EHCI: CPipe::CheckForDoneTransfersThread - #pipes to check = %d\n"), m_debug_numItemsOnBusyPipeList) );
    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE && !fDebugNeedProcessing, (TEXT("EHCI: CPipe::CheckForDoneTransfersThread - warning! Called when no pipes were busy\n")) );
#endif // DEBUG
    pPrev = NULL;
    pCurrent = m_pBusyPipeList;
    while ( pCurrent != NULL ) {
        pCurrent->pPipe->CheckForDoneTransfers();
            // this pipe is still busy. Move to next item
        pPrev = pCurrent;
        pCurrent = pPrev->pNext;
    }
    DEBUGMSG( ZONE_TRANSFER && ZONE_VERBOSE && fDebugNeedProcessing, (TEXT("EHCI: CPipe::CheckForDoneTransfersThread - #pipes still busy = %d\n"), m_debug_numItemsOnBusyPipeList) );
    Unlock();
    return 0;
}
// ******************************************************************
// Scope: protected static 
BOOL CBusyPipeList::AddToBusyPipeList( IN CPipe * const pPipe,
                               IN const BOOL fHighPriority )
//
// Purpose: Add the pipe indicated by pPipe to our list of busy pipes.
//          This allows us to check for completed transfers after 
//          getting an interrupt, and being signaled via 
//          SignalCheckForDoneTransfers
//
// Parameters: pPipe - pipe to add to busy list
//
//             fHighPriority - if TRUE, add pipe to start of busy list,
//                             else add pipe to end of list.
//
// Returns: TRUE if pPipe successfully added to list, else FALSE
//
// Notes: 
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE, (TEXT("EHCI: +CPipe::AddToBusyPipeList - new pipe(%s) 0x%x, pri %d\n"), pPipe->GetPipeType(), pPipe, fHighPriority ));

    PREFAST_DEBUGCHK( pPipe != NULL );
    BOOL fSuccess = FALSE;

    // make sure there nothing on the pipe already (it only gets officially added after this function succeeds).
    Lock();
#ifdef DEBUG
{
    // make sure this pipe isn't already in the list. That should never happen.
    // also check that our m_debug_numItemsOnBusyPipeList is correct
    PPIPE_LIST_ELEMENT pBusy = m_pBusyPipeList;
    int count = 0;
    while ( pBusy != NULL ) {
        DEBUGCHK( pBusy->pPipe != NULL &&
                  pBusy->pPipe != pPipe );
        pBusy = pBusy->pNext;
        count++;
    }
    DEBUGCHK( m_debug_numItemsOnBusyPipeList == count );
}
#endif // DEBUG
    
    PPIPE_LIST_ELEMENT pNewBusyElement = new PIPE_LIST_ELEMENT;
    if ( pNewBusyElement != NULL ) {
        pNewBusyElement->pPipe = pPipe;
        if ( fHighPriority || m_pBusyPipeList == NULL ) {
            // add pipe to start of list
            pNewBusyElement->pNext = m_pBusyPipeList;
            m_pBusyPipeList = pNewBusyElement;
        } else {
            // add pipe to end of list
            PPIPE_LIST_ELEMENT pLastElement = m_pBusyPipeList;
            while ( pLastElement->pNext != NULL ) {
                pLastElement = pLastElement->pNext;
            }
            pNewBusyElement->pNext = NULL;
            pLastElement->pNext = pNewBusyElement;
        }
        fSuccess = TRUE;
    #ifdef DEBUG
        m_debug_numItemsOnBusyPipeList++;
    #endif // DEBUG
    }
    Unlock();
    DEBUGMSG( ZONE_PIPE, (TEXT("EHCI: -CPipe::AddToBusyPipeList - new pipe(%s) 0x%x, pri %d, returning BOOL %d\n"), pPipe->GetPipeType(), pPipe, fHighPriority, fSuccess) );
    return fSuccess;
}

// ******************************************************************
// Scope: protected static
void CBusyPipeList::RemoveFromBusyPipeList( IN CPipe * const pPipe )
//
// Purpose: Remove this pipe from our busy pipe list. This happens if
//          the pipe is suddenly aborted or closed while a transfer
//          is in progress
//
// Parameters: pPipe - pipe to remove from busy list
//
// Returns: Nothing
//
// Notes: 
// ******************************************************************
{
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE, (TEXT("EHCI: +CPipe::RemoveFromBusyPipeList - pipe(%s) 0x%x\n"), pPipe->GetPipeType(), pPipe ) );
    Lock();
#ifdef DEBUG
    BOOL debug_fRemovedPipe = FALSE;
{
    // check m_debug_numItemsOnBusyPipeList
    PPIPE_LIST_ELEMENT pBusy = m_pBusyPipeList;
    int count = 0;
    while ( pBusy != NULL ) {
        DEBUGCHK( pBusy->pPipe != NULL );
        pBusy = pBusy->pNext;
        count++;
    }
    DEBUGCHK( m_debug_numItemsOnBusyPipeList == count );
}
#endif // DEBUG
    PPIPE_LIST_ELEMENT pPrev = NULL;
    PPIPE_LIST_ELEMENT pCurrent = m_pBusyPipeList;
    while ( pCurrent != NULL ) {
        if ( pCurrent->pPipe == pPipe ) {
            // Remove item from the linked list
            if ( pCurrent == m_pBusyPipeList ) {
                DEBUGCHK( pPrev == NULL );
                m_pBusyPipeList = m_pBusyPipeList->pNext;
            } else {
                DEBUGCHK( pPrev != NULL &&
                          pPrev->pNext == pCurrent );
                pPrev->pNext = pCurrent->pNext;
            }
            delete pCurrent;
            pCurrent = NULL;
        #ifdef DEBUG
            debug_fRemovedPipe = TRUE;
            DEBUGCHK( --m_debug_numItemsOnBusyPipeList >= 0 );
        #endif // DEBUG
            break;
        } else {
            // Check next item
            pPrev = pCurrent;
            pCurrent = pPrev->pNext;
        }
    }
    Unlock();
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE && debug_fRemovedPipe, (TEXT("EHCI: -CPipe::RemoveFromBusyPipeList, removed pipe(%s) 0x%x\n"), pPipe->GetPipeType(), pPipe));
    DEBUGMSG( ZONE_PIPE && ZONE_VERBOSE && !debug_fRemovedPipe, (TEXT("EHCI: -CPipe::RemoveFromBusyPipeList, pipe(%s) 0x%x was not on busy list\n"), pPipe->GetPipeType(), pPipe ));
}

#define ASYNC_PARK_MODE 1
#define FRAME_LIST_SIZE 0x400

// ************************************CHW ******************************  

const TCHAR CHW::m_s_cpszName[5] = L"EHCI";
CHW::CHW( IN const REGISTER portBase,
                              IN const DWORD dwSysIntr,
                              IN CPhysMem * const pCPhysMem,
                              //IN CUhcd * const pHcd,
                              IN LPVOID pvUhcdPddObject,
                              IN LPCTSTR lpDeviceRegistry)
: m_cBusyPipeList(FRAME_LIST_SIZE)
, m_cPeriodicMgr (pCPhysMem,FRAME_LIST_SIZE)
, m_cAsyncMgr(pCPhysMem)
, m_deviceReg(HKEY_LOCAL_MACHINE,lpDeviceRegistry)
{
// definitions for static variables
    DEBUGMSG( ZONE_INIT, (TEXT("EHCI: +CHW::CHW base=0x%x, intr=0x%x\n"), portBase, dwSysIntr));
    g_fPowerUpFlag = FALSE;
    g_fPowerResuming = FALSE;
    m_capBase = portBase;
    m_portBase = portBase+Read_CapLength();//EHCI 2.2.1   
    m_NumOfPort=Read_HCSParams().bit.N_PORTS;
    m_NumOfCompanionControllers = Read_HCSParams().bit.N_CC;
    
    //m_pHcd = pHcd;
    m_pMem = pCPhysMem;
    m_pPddContext = pvUhcdPddObject;
    m_frameCounterHighPart = 0;
    m_frameCounterLowPart = 0;
    m_FrameListMask = FRAME_LIST_SIZE-1;  
    m_pFrameList = 0;

    m_dwSysIntr = dwSysIntr;
    m_hUsbInterruptEvent = NULL;
    m_hUsbHubChangeEvent = NULL;
    m_hUsbInterruptThread = NULL;
    m_fUsbInterruptThreadClosing = FALSE;

    m_fFrameLengthIsBeingAdjusted = FALSE;
    m_fStopAdjustingFrameLength = FALSE;
    m_hAdjustDoneCallbackEvent = NULL;
    m_uNewFrameLength = 0;
    m_dwCapability = 0;
    m_bDoResume=FALSE;
#ifdef USB_IF_ELECTRICAL_TEST_MODE
    m_currTestMode = USB_EHCI_TEST_MODE_DISABLED;
#endif
    m_dwQueuedAsyncQH = 0;

    m_dwEHCIHwID = USB_HW_ID_GENERIC_EHCI;
    m_dwEHCIHwRev = 0;

    m_hAsyncDoorBell=CreateEvent(NULL, FALSE,FALSE,NULL);
    InitializeCriticalSection( &m_csFrameCounter );
}
CHW::~CHW()
{
    if (m_dwSysIntr)
        KernelIoControl(IOCTL_HAL_DISABLE_WAKE, &m_dwSysIntr, sizeof(m_dwSysIntr), NULL, 0, NULL);
    DeInitialize();
    if (m_hAsyncDoorBell)
        CloseHandle(m_hAsyncDoorBell);
    DeleteCriticalSection( &m_csFrameCounter );
}

// ******************************************************************
BOOL CHW::Initialize( )
// Purpose: Reset and Configure the Host Controller with the schedule.
//
// Parameters: portBase - base address for host controller registers
//
//             dwSysIntr - system interrupt number to use for USB
//                         interrupts from host controller
//
//             frameListPhysAddr - physical address of frame list index
//                                 maintained by CPipe class
//
//             pvUhcdPddObject - PDD specific structure used during suspend/resume
//
// Returns: TRUE if initialization succeeded, else FALSE
//
// Notes: This function is only called from the CUhcd::Initialize routine.
//
//        This function is static
// ******************************************************************
{
    DEBUGMSG( ZONE_INIT, (TEXT("%s: +CHW::Initialize\n"),GetControllerName()));

    DEBUGCHK( m_frameCounterLowPart == 0 &&
              m_frameCounterHighPart == 0 );

    // set up the frame list area.
    if ( m_portBase == 0 || 
            m_cPeriodicMgr.Init()==FALSE ||
            m_cAsyncMgr.Init() == FALSE ||
            m_cBusyPipeList.Init()==FALSE) {
        DEBUGMSG( ZONE_ERROR, (TEXT("%s: -CHW::Initialize - zero Register Base or CeriodicMgr or CAsyncMgr fails\n"),GetControllerName()));
        ASSERT(FALSE);
        return FALSE;
    }
    // read registry setting for ECHI core ID
    if (!ReadUSBHwInfo()) {
        return FALSE;
    }
    DEBUGMSG(ZONE_INIT && ZONE_REGISTERS, (TEXT("%s: CHW::Initialize - signalling global reset\n"),GetControllerName()));
    {
        USBCMD usbcmd;
        // check flag whether or not to reset the USB core here, CI13611 core is reset in PDD and does not need to be reset again here
        if (m_dwEHCIHwID != USB_HW_ID_CI13611A) {
            usbcmd=Read_USBCMD();
            usbcmd.bit.HCReset=1;
            Write_USBCMD(usbcmd);
        }
        for (DWORD dwCount=0;dwCount<50 && (Read_USBCMD().bit.HCReset!=0);dwCount++)
            Sleep( 20 );
        usbcmd=Read_USBCMD();
        if (usbcmd.bit.HCReset!=0) // If can not reset within 1 second, we assume this is bad device.
            return FALSE;
        usbcmd.bit.FrameListSize=0;// We use 1k for Periodic List.
        m_FrameListMask = 0x3ff;  // Available Bit in 
        Write_USBCMD(usbcmd);
    }
    DEBUGMSG(ZONE_INIT && ZONE_REGISTERS, (TEXT("%s: CHW::Initialize - end signalling global reset\n"),GetControllerName()));
    DEBUGMSG(ZONE_INIT && ZONE_REGISTERS, (TEXT("%s: CHW::Initialize - setting USBINTR to all interrupts on\n"),GetControllerName()));
    {
        USBINTR usbint;
        // initialize interrupt register - set all interrupts to enabled
        usbint.ul=(DWORD)-1;
        usbint.bit.Reserved=0;
        Write_USBINTR(usbint );
    }
    DEBUGMSG(ZONE_INIT && ZONE_REGISTERS && ZONE_VERBOSE, (TEXT("%s: CHW::Initialize - setting FRNUM = 0\n"),GetControllerName()));
    // initialize FRNUM register with index 0 of frame list
    {
        FRINDEX frindex;
        frindex.ul=0;
        Write_FRINDEX(frindex);
    }
    Write_EHCIRegister(CTLDSSEGMENT,0);//We only support 32-bit address space now.
    // initialize FLBASEADD with address of frame list
    {
        ULONG frameListPhysAddr = m_cPeriodicMgr.GetFrameListPhysAddr();
        DEBUGMSG(ZONE_INIT && ZONE_REGISTERS && ZONE_VERBOSE, (TEXT("%s: CHW::Initialize - setting FLBASEADD = 0x%X\n"),GetControllerName(), frameListPhysAddr));
        DEBUGCHK( frameListPhysAddr != 0 );
        // frame list should be aligned on a 4Kb boundary
        DEBUGCHK( (frameListPhysAddr & EHCD_FLBASEADD_MASK) == frameListPhysAddr );
        Write_EHCIRegister(PERIODICLISTBASE,frameListPhysAddr);
        // Follow the rule in 4.8 EHCI
        USBCMD usbcmd=Read_USBCMD();
        while (usbcmd.bit.PSchedEnable!= Read_USBSTS().bit.PSStatus)
            Sleep(1);
        if (usbcmd.bit.PSchedEnable!=1) {
            usbcmd.bit.PSchedEnable=1;
            Write_USBCMD(usbcmd);
        }
    }
    // Initial Async Shedule to Enable.
    DEBUGMSG(ZONE_INIT && ZONE_REGISTERS && ZONE_VERBOSE, (TEXT("%s: CHW::Initialize - Enable Async Sched \n"),GetControllerName()));
    {
        Write_EHCIRegister(ASYNCLISTADDR,m_cAsyncMgr.GetPhysAddr());
#ifdef ASYNC_PARK_MODE
        if (Read_HHCCP_CAP().bit.Async_Park) {
            USBCMD usbcmd=Read_USBCMD();
            usbcmd.bit.ASchedPMEnable=1;
            usbcmd.bit.ASchedPMCount =3;
            Write_USBCMD(usbcmd);
        }
#endif
    }    
    // m_hUsbInterrupt - Auto Reset, and Initial State = non-signaled
    DEBUGCHK( m_hUsbInterruptEvent == NULL );
    TCHAR *cpszHsUsbFnIntrEvent = NULL;
    if (m_dwEHCIHwID == USB_HW_ID_CI13611A) {
        if (!ReadUsbInterruptEventName(&cpszHsUsbFnIntrEvent)) {
            if (cpszHsUsbFnIntrEvent != NULL) delete [] cpszHsUsbFnIntrEvent;
            DEBUGMSG(ZONE_ERROR, (TEXT("%s: -CHW::Initialize. Error reading USB OTG Interrupt event name from registry\n"),GetControllerName()));
            return FALSE;
        }
    }
    m_hUsbInterruptEvent = CreateEvent( NULL, FALSE, FALSE, cpszHsUsbFnIntrEvent );
    if (cpszHsUsbFnIntrEvent != NULL) delete [] cpszHsUsbFnIntrEvent; 
    m_hUsbHubChangeEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( m_hUsbInterruptEvent == NULL || m_hUsbHubChangeEvent==NULL ) {
        DEBUGMSG(ZONE_ERROR, (TEXT("%s: -CHW::Initialize. Error creating USBInterrupt or USBHubInterrupt event\n"),GetControllerName()));
        return FALSE;
    }

    InterruptDisable( m_dwSysIntr ); // Just to make sure this is really ours.
    // Initialize Interrupt. When interrupt id # m_sysIntr is triggered,
    // m_hUsbInterruptEvent will be signaled. Last 2 params must be NULL
    if ( !InterruptInitialize( m_dwSysIntr, m_hUsbInterruptEvent, NULL, NULL) ) {
        DEBUGMSG(ZONE_ERROR, (TEXT("%s: -CHW::Initialize. Error on InterruptInitialize\r\n"),GetControllerName()));
        return FALSE;
    }
    // Start up our IST - the parameter passed to the thread
    // is unused for now
    DEBUGCHK( m_hUsbInterruptThread == NULL &&
              m_fUsbInterruptThreadClosing == FALSE );
    if (m_hUsbInterruptThread==NULL)
        m_hUsbInterruptThread = CreateThread( 0, 0, UsbInterruptThreadStub, this, 0, NULL );
    if ( m_hUsbInterruptThread == NULL ) {
        DEBUGMSG(ZONE_ERROR, (TEXT("%s: -CHW::Initialize. Error creating IST\n"),GetControllerName()));
        return FALSE;
    }
    CeSetThreadPriority( m_hUsbInterruptThread, g_IstThreadPriority );
    // Initial All port route to this host.
    DEBUGMSG(ZONE_INIT && ZONE_REGISTERS && ZONE_VERBOSE, (TEXT("%s: CHW::Initialize - Initial All port route \n"),GetControllerName()));
    {
        Write_EHCIRegister(CONFIGFLAG,1);
        // Power On all the port.
        for (DWORD dwPort=1; dwPort <= m_NumOfPort ; dwPort ++ ) {
            PORTSC portSc = Read_PORTSC( dwPort);
            portSc.bit.Power=1;
            portSc.bit.Owner=0;
            // Do not touch write to clean register
            portSc.bit.ConnectStatusChange=0;
            portSc.bit.EnableChange=0;
            portSc.bit.OverCurrentChange=0;
            
            Write_PORTSC(dwPort,portSc);                
        }
        Sleep(50); // Port need to 50 ms to reset.
    }    
    
    DEBUGMSG( ZONE_INIT, (TEXT("%s: -CHW::Initialize, success!\n"),GetControllerName()));
    return TRUE;
}
// ******************************************************************
void CHW::DeInitialize( void )
//
// Purpose: Delete any resources associated with static members
//
// Parameters: none
//
// Returns: nothing
//
// Notes: This function is only called from the ~CUhcd() routine.
//
//        This function is static
// ******************************************************************
{
    m_fUsbInterruptThreadClosing = TRUE; // tell USBInterruptThread that we are closing
    // m_hAdjustDoneCallbackEvent <- don't need to do anything to this
    // m_uNewFrameLength <- don't need to do anything to this

    // Wake up the interrupt thread and give it time to die.
    if ( m_hUsbInterruptEvent ) {
        SetEvent(m_hUsbInterruptEvent);
        if ( m_hUsbInterruptThread ) {
            DWORD dwWaitReturn = WaitForSingleObject(m_hUsbInterruptThread, 1000);
            if ( dwWaitReturn != WAIT_OBJECT_0 ) {
                DEBUGCHK( 0 );
            }
            CloseHandle(m_hUsbInterruptThread);
            m_hUsbInterruptThread = NULL;
        }
        // we have to close our interrupt before closing the event!
        InterruptDisable( m_dwSysIntr );

        CloseHandle(m_hUsbInterruptEvent);
        m_hUsbInterruptEvent = NULL;
    } else {
        InterruptDisable( m_dwSysIntr );
    }

    if ( m_hUsbHubChangeEvent ) {
        CloseHandle(m_hUsbHubChangeEvent);
        m_hUsbHubChangeEvent = NULL;
    }

    // Stop The Controller.
    {
        USBCMD usbcmd=Read_USBCMD();
        usbcmd.bit.RunStop=0;
        Write_USBCMD(usbcmd);
        while( Read_USBSTS().bit.HCHalted == 0 ) //Wait until it stop.
            Sleep(1);
    }
    m_cPeriodicMgr.DeInit();
    m_cAsyncMgr.DeInit();
    m_cBusyPipeList.DeInit();
    // no need to free the frame list; the entire pool will be freed as a unit.
    m_pFrameList = 0;
    m_fUsbInterruptThreadClosing = FALSE;
    m_frameCounterLowPart = 0;
    m_frameCounterHighPart = 0;
}

// ******************************************************************
void CHW::EnterOperationalState( void )
//
// Purpose: Signal the host controller to start processing the schedule
//
// Parameters: None
//
// Returns: Nothing.
//
// Notes: This function is only called from the CUhcd::Initialize routine.
//        It assumes that CPipe::Initialize and CHW::Initialize
//        have already been called.
//
//        This function is static
// ******************************************************************
{
    DEBUGMSG( ZONE_INIT, (TEXT("%s: +CHW::EnterOperationalState\n"),GetControllerName()));
    DWORD dwIntThreshCtrl = EHCI_REG_IntThreshCtrl_DEFAULT;
    if (!(m_deviceReg.IsKeyOpened() && m_deviceReg.GetRegValue(EHCI_REG_IntThreshCtrl, (LPBYTE)&dwIntThreshCtrl, sizeof(dwIntThreshCtrl)))) {
        dwIntThreshCtrl = EHCI_REG_IntThreshCtrl_DEFAULT;
    }
    DEBUGMSG(ZONE_INIT && ZONE_REGISTERS && ZONE_VERBOSE, (TEXT("%s: CHW::EnterOperationalState - clearing status reg\n"),GetControllerName()));
    Clear_USBSTS( );
    USBCMD usbcmd=Read_USBCMD();

    DEBUGMSG(ZONE_INIT && ZONE_REGISTERS && ZONE_VERBOSE, (TEXT("%s: CHW::EnterOperationalState - setting USBCMD run bit\n"),GetControllerName()));
    usbcmd.bit.FrameListSize = 0; // 1k Flame Entry. Sync with Initialization.
    usbcmd.bit.IntThreshCtrl = dwIntThreshCtrl; // Setup by registry.
    usbcmd.bit.RunStop = 1;
    Write_USBCMD( usbcmd );

    DEBUGMSG( ZONE_INIT, (TEXT("%s: -CHW::EnterOperationalState\n"),GetControllerName()));
}

// ******************************************************************
void CHW::StopHostController( void )
//
// Purpose: Signal the host controller to stop processing the schedule
//
// Parameters: None
//
// Returns: Nothing.
//
// Notes:
//
//        This function is static
// ******************************************************************
{
    Lock();
    USBCMD usbcmd=Read_USBCMD();
    // Check run bit. Despite what the UHCI spec says, Intel's controller
    // does not always set the HCHALTED bit when the controller is stopped.
    if(usbcmd.bit.RunStop) {
        // clear run bit
        usbcmd.bit.RunStop= 0;
        Write_USBCMD( usbcmd );  
        USBINTR usbIntr;
        usbIntr.ul=0;
        // clear all interrupts
        Write_USBINTR(usbIntr);
        // spin until the controller really is stopped
        while( Read_USBSTS().bit.HCHalted == 0 ) //Wait until it stop.
            Sleep(0);
    }
    Unlock();
}
BOOL CHW::EnableDisableAsyncSch(BOOL fEnable)
{    
    Lock();
    USBCMD usbcmd=Read_USBCMD();
    if ( (usbcmd.bit.ASchedEnable ==1)!= (fEnable==TRUE)) {
        // Follow the rule in 4.8 EHCI
        while (usbcmd.bit.ASchedEnable!= Read_USBSTS().bit.ASStatus)
            Sleep(1);
        usbcmd.bit.ASchedEnable = (fEnable==TRUE?1:0);
        Write_USBCMD(usbcmd);
    }
    Unlock();
    return TRUE;
}

BOOL CHW::AsyncBell()
{
    m_DoorBellLock.Lock();
    ResetEvent(m_hAsyncDoorBell);
    Lock();
    USBCMD usbcmd=Read_USBCMD();
    usbcmd.bit.IntOnAADoorbell=1;
    Write_USBCMD( usbcmd );  
    Unlock();
    DWORD dwReturn=WaitForSingleObject( m_hAsyncDoorBell,10);
    m_DoorBellLock.Unlock();
    return (dwReturn == WAIT_OBJECT_0);
}
CQH *  CHW::AsyncQueueQH(CQH * pQHead)
{ 
    m_cAsyncMgr.Lock();
    m_dwQueuedAsyncQH++;
    EnableDisableAsyncSch(TRUE);
    m_cAsyncMgr.Unlock();
    return m_cAsyncMgr.QueueQH(pQHead);
};
BOOL  CHW::AsyncDequeueQH( CQH * pQh) 
{   
    BOOL bReturn= m_cAsyncMgr.DequeueQHead( pQh);
    if (bReturn) {
        AsyncBell();
        m_cAsyncMgr.Lock();
        if (m_dwQueuedAsyncQH)
            m_dwQueuedAsyncQH--;
        else
            ASSERT(FALSE);
        if (m_dwQueuedAsyncQH==0) { // We can turn async schedual off.
            EnableDisableAsyncSch(FALSE);
        }
        m_cAsyncMgr.Unlock();
    }
    return bReturn;
};
BOOL CHW::PeriodQueueITD(CITD * piTD,DWORD FrameIndex) 
{ 
    FRINDEX frameIndex= Read_FRINDEX();
    if (((FrameIndex  - frameIndex.bit.FrameIndex) & m_FrameListMask) > 1)        
        return  m_cPeriodicMgr.QueueITD(piTD,FrameIndex); 
    else 
        return FALSE;// To Close EHCI 4.7.2.1
};
BOOL CHW::PeriodQueueSITD(CSITD * psiTD,DWORD FrameIndex)
{ 
    FRINDEX frameIndex= Read_FRINDEX();
    if (((FrameIndex  - frameIndex.bit.FrameIndex) & m_FrameListMask) > 1)        
        return  m_cPeriodicMgr.QueueSITD(psiTD,FrameIndex);
    else
        return FALSE;
};
BOOL CHW::PeriodDeQueueTD(DWORD dwPhysAddr,DWORD FrameIndex) 
{ 
    FRINDEX frameIndex= Read_FRINDEX();
    
    while (((FrameIndex  - frameIndex.bit.FrameIndex) & m_FrameListMask) <=1)  {
        Sleep(1);
        frameIndex= Read_FRINDEX();
    }
    return  m_cPeriodicMgr.DeQueueTD(dwPhysAddr, FrameIndex); 
};

DWORD CALLBACK CHW::CeResumeThreadStub ( IN PVOID context )
{
    return ((CHW *)context)->CeResumeThread ( );
}
// ******************************************************************
DWORD CHW::CeResumeThread ( )
//
// Purpose: Force the HCD to reset and regenerate itself after power loss.
//
// Parameters: None
//
// Returns: Nothing.
//
// Notes: Because the PDD is probably maintaining pointers to the Hcd and Memory
//   objects, we cannot free/delete them and then reallocate. Instead, we destruct
//   them explicitly and use the placement form of the new operator to reconstruct
//   them in situ. The two flags synchronize access to the objects so that they
//   cannot be accessed before being reconstructed while also guaranteeing that
//   we don't miss power-on events that occur during the reconstruction.
//
//        This function is static
// ******************************************************************
{
    // reconstruct the objects at the same addresses where they were before;
    // this allows us not to have to alert the PDD that the addresses have changed.

    DEBUGCHK( g_fPowerResuming == FALSE );

    // order is important! resuming indicates that the hcd object is temporarily invalid
    // while powerup simply signals that a powerup event has occurred. once the powerup
    // flag is cleared, we will repeat this whole sequence should it get resignalled.
    g_fPowerUpFlag = FALSE;
    g_fPowerResuming = TRUE;

    DeviceDeInitialize();
    for(;;) {  // breaks out upon successful reinit of the object

        if (DeviceInitialize())
            break;
        // getting here means we couldn't reinit the HCD object!
        ASSERT(FALSE);
        DEBUGMSG(ZONE_ERROR, (TEXT("%s: USB cannot reinit the HCD at CE resume; retrying...\n"),GetControllerName()));
        DeviceDeInitialize();
        Sleep(15000);
    }

    // the hcd object is valid again. if a power event occurred between the two flag
    // assignments above then the IST will reinitiate this sequence.
    g_fPowerResuming = FALSE;
    if (g_fPowerUpFlag)
        PowerMgmtCallback(TRUE);
    
    return 0;
}
DWORD CHW::UsbInterruptThreadStub( IN PVOID context )
{
    return ((CHW *)context)->UsbInterruptThread();
}

// ******************************************************************
DWORD CHW::UsbInterruptThread( )
//
// Purpose: Main IST to handle interrupts from the USB host controller
//
// Parameters: context - parameter passed in when starting thread,
//                       (currently unused)
//
// Returns: 0 on thread exit.
//
// Notes:
//
//        This function is private
// ******************************************************************
{
    DEBUGMSG(ZONE_INIT && ZONE_VERBOSE, (TEXT("%s: +CHW::Entered USBInterruptThread\n"),GetControllerName()));

    while ( !m_fUsbInterruptThreadClosing ) {
        WaitForSingleObject(m_hUsbInterruptEvent, INFINITE);
        if ( m_fUsbInterruptThreadClosing ) {
            break;
        }

        USBSTS usbsts = Read_USBSTS();
    #ifdef DEBUG
        DWORD dwFrame;
        GetFrameNumber(&dwFrame); // calls UpdateFrameCounter
        DEBUGMSG( ZONE_REGISTERS, (TEXT("%s: !!!interrupt!!!! on frame index + 1 = 0x%08x, USBSTS = 0x%04x\n"),GetControllerName(), dwFrame, usbsts.ul ) );
        if (usbsts.bit.HSError) { // Error Happens.
            DumpAllRegisters( );
            ASSERT(FALSE);
        }
    #else
        UpdateFrameCounter();
    #endif // DEBUG
        if (usbsts.bit.PortChanged) {
            SetEvent(m_hUsbHubChangeEvent);
        }
        Write_USBSTS(usbsts);//        Clear_USBSTS( );

        if (usbsts.bit.ASAdvance) {
            SetEvent(m_hAsyncDoorBell);
        }
        // We need to differentiate between USB interrupts, which are
        // for transfers, and host interrupts (EHCI spec 2.3.2).
        // For the former, we need to call CPipe::SignalCheckForDoneTransfers.
        // For the latter, we need to call whoever will handle
        // resume/error processing.

        // For now, we just notify CPipe so that transfers
        // can be checked for completion

        // This flag gets cleared in the resume thread.
        if(g_fPowerUpFlag)
        {
            if (m_bDoResume) {
                g_fPowerUpFlag=FALSE;
                Lock();
                USBCMD USBCmd = Read_USBCMD();
                USBCmd.bit.RunStop=1;
                Sleep(20); 
                Write_USBCMD(USBCmd);
                Unlock();
            }
            else {
                if (g_fPowerResuming) {
                    // this means we've restarted an IST and it's taken an early interrupt;
                    // just pretend it didn't happen for now because we're about to be told to exit again.
                    continue;
                }
                HcdPdd_InitiatePowerUp((DWORD) m_pPddContext);
                HANDLE ht;
                while ((ht = CreateThread(NULL, 0, CeResumeThreadStub, this, 0, NULL)) == NULL) {
                    RETAILMSG(1, (TEXT("HCD IST: cannot spin a new thread to handle CE resume of USB host controller; sleeping.\n")));
                    Sleep(15000);  // 15 seconds later, maybe it'll work.
                }
                CeSetThreadPriority( ht, g_IstThreadPriority );
                CloseHandle(ht);
                
                // The CE resume thread will force this IST to exit so we'll be cooperative proactively.
                break;
            }
        }
        else if (usbsts.bit.USBINT || usbsts.bit.USBERRINT)
            m_cBusyPipeList.CheckForDoneTransfersThread();

        InterruptDone(m_dwSysIntr);
    }

    DEBUGMSG(ZONE_INIT && ZONE_VERBOSE, (TEXT("%s: -CHW::Leaving USBInterruptThread\n"),GetControllerName()));

    return (0);
}
// ******************************************************************
void CHW::UpdateFrameCounter( void )
//
// Purpose: Updates our internal frame counter
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: The UHCI frame number register is only 11 bits, or 2047
//        long. Thus, the counter will wrap approx. every 2 seconds.
//        That is insufficient for Isoch Transfers, which
//        may need to be scheduled out into the future by more
//        than 2 seconds. So, we maintain an internal 32 bit counter
//        for the frame number, which will wrap in 50 days.
//
//        This function should be called at least once every two seconds,
//        otherwise we will miss frames.
//
// ******************************************************************
{
#ifdef DEBUG
    DWORD dwTickCountLastTime = GetTickCount();
#endif

    EnterCriticalSection( &m_csFrameCounter );

#ifdef DEBUG
    // If this fails, we haven't been called in a long time,
    // so the frame number is no longer accurate
    if (GetTickCount() - dwTickCountLastTime >= 800 )
        DEBUGMSG(ZONE_WARNING, (TEXT("!UHCI - CHW::UpdateFrameCounter missed frame count;")
                     TEXT(" isoch packets may have been dropped.\n")));
    dwTickCountLastTime = GetTickCount();
#endif // DEBUG

    DWORD currentFRNUM = Read_FRINDEX().bit.FrameIndex;
    DWORD dwCarryBit = m_FrameListMask + 1;
    if ((currentFRNUM & dwCarryBit ) != (m_frameCounterHighPart & dwCarryBit ) ) { // Overflow
        m_frameCounterHighPart += dwCarryBit;
    }
    m_frameCounterLowPart = currentFRNUM;

    LeaveCriticalSection( &m_csFrameCounter );
}

// ******************************************************************
BOOL CHW::GetFrameNumber( OUT LPDWORD lpdwFrameNumber )
//
// Purpose: Return the current frame number
//
// Parameters: None
//
// Returns: 32 bit current frame number
//
// Notes: See also comment in UpdateFrameCounter
// ******************************************************************
{
    EnterCriticalSection( &m_csFrameCounter );

    // This algorithm is right out of the Win98 uhcd.c code
    UpdateFrameCounter();
    DWORD frame = m_frameCounterHighPart + (m_frameCounterLowPart & m_FrameListMask);
        
    LeaveCriticalSection( &m_csFrameCounter );

    *lpdwFrameNumber=frame;
    return TRUE;
}
// ******************************************************************
BOOL CHW::GetFrameLength( OUT LPUSHORT lpuFrameLength )
//
// Purpose: Return the current frame length in 12 MHz clocks
//          (i.e. 12000 = 1ms)
//
// Parameters: None
//
// Returns: frame length
//
// Notes: Only part of the frame length is stored in the hardware
//        register, so an offset needs to be added.
// ******************************************************************
{
    *lpuFrameLength=60000;
    return TRUE;
}
// ******************************************************************
BOOL CHW::SetFrameLength( IN HANDLE , IN USHORT  )
//
// Purpose: Set the Frame Length in 12 Mhz clocks. i.e. 12000 = 1ms
//
// Parameters:  hEvent - event to set when frame has reached required
//                       length
//
//              uFrameLength - new frame length
//
// Returns: TRUE if frame length changed, else FALSE
//
// Notes:
// ******************************************************************
{
    return FALSE;
}
// ******************************************************************
BOOL CHW::StopAdjustingFrame( void )
//
// Purpose: Stop modifying the host controller frame length
//
// Parameters: None
//
// Returns: TRUE
//
// Notes:
// ******************************************************************
{
    return FALSE;
}
// ******************************************************************
BOOL CHW::DidPortStatusChange( IN const UCHAR port )
//
// Purpose: Determine whether the status of root hub port # "port" changed
//
// Parameters: port - 0 for the hub itself, otherwise the hub port number
//
// Returns: TRUE if status changed, else FALSE
//
// Notes:
// ******************************************************************
{
    USB_HUB_AND_PORT_STATUS s;
    CHW::GetPortStatus(port, s);
    return s.change.word ? TRUE : FALSE;
}

// ******************************************************************
DWORD CHW::GetNumOfPorts() 
//
// Purpose: Get the number of ports on the root hub
//
// Parameters: 
//
// Returns: The number of ports on the root hub
//
// Notes:
// ******************************************************************
{ 
    return m_NumOfPort;
}

// ******************************************************************
BOOL CHW::GetPortStatus( IN const UCHAR port,
                         OUT USB_HUB_AND_PORT_STATUS& rStatus )
//
// Purpose: This function will return the current root hub port
//          status in a non-hardware specific format
//
// Parameters: port - 0 for the hub itself, otherwise the hub port number
//
//             rStatus - reference to USB_HUB_AND_PORT_STATUS to get the
//                       status
//
// Returns: TRUE
//
// Notes:
// ******************************************************************
{
    memset( &rStatus, 0, sizeof( USB_HUB_AND_PORT_STATUS ) );
    if ( port > 0 ) {
        // request refers to a root hub port

        // read the port status register
        PORTSC portSC = Read_PORTSC( port );
        if (portSC.bit.Power && portSC.bit.Owner==0) {
            // Now fill in the USB_HUB_AND_PORT_STATUS structure
            rStatus.change.port.ConnectStatusChange = portSC.bit.ConnectStatusChange;
            rStatus.change.port.PortEnableChange = portSC.bit.EnableChange;
            rStatus.change.port.OverCurrentChange = portSC.bit.OverCurrentChange;
            // for root hub, we don't set any of these change bits:
            DEBUGCHK( rStatus.change.port.SuspendChange == 0 );
            DEBUGCHK( rStatus.change.port.ResetChange == 0 );
            if (m_dwEHCIHwID == USB_HW_ID_CI13611A) {
                // map from generic EHCI port bits to CI13611 specific port bits
                PORTSC_CI13611 *pPortSC = (PORTSC_CI13611 *)&portSC;
                //ChipIdea Core needs to check these bits for speed.
                //portSC.bit.HighSpeedPort == 1 for HighSpeed and
                //portSC.bit.PortSpeed     == 00 for FullSpeed
                //                            01 for Low Speed
                //                            10 for HighSpeed
                //                            11 for Undefined
                if (pPortSC->bit.HighSpeedPort && pPortSC->bit.PortSpeed == 0x2) {
                    rStatus.status.port.DeviceIsHighSpeed = 1;
                    rStatus.status.port.DeviceIsLowSpeed = 0;                    
                }
                else if (!pPortSC->bit.HighSpeedPort && pPortSC->bit.PortSpeed == 0x1) {
                    rStatus.status.port.DeviceIsLowSpeed = 1;
                    rStatus.status.port.DeviceIsHighSpeed = 0 ;
                }
                else if (!pPortSC->bit.HighSpeedPort && pPortSC->bit.PortSpeed == 0x0) {
                    rStatus.status.port.DeviceIsLowSpeed = 0;
                    rStatus.status.port.DeviceIsHighSpeed = 0 ;                
                }
            } else {
                //From EHCI Spec 2.3.9.
                if(portSC.bit.LineStatus != 0x1 ) {
                    rStatus.status.port.DeviceIsLowSpeed = 0;
                    rStatus.status.port.DeviceIsHighSpeed = 1;
                }
                else {
                    rStatus.status.port.DeviceIsLowSpeed = 1;
                    rStatus.status.port.DeviceIsHighSpeed = 0;
                }
            }
            rStatus.status.port.PortConnected = portSC.bit.ConnectStatus;
            rStatus.status.port.PortEnabled =  portSC.bit.Enabled;
            rStatus.status.port.PortOverCurrent = portSC.bit.OverCurrentActive ;
            // root hub ports are always powered
            rStatus.status.port.PortPower = 1;
            rStatus.status.port.PortReset = portSC.bit.Reset;
            rStatus.status.port.PortSuspended =  portSC.bit.Suspend;
            if (portSC.bit.ForcePortResume) { // Auto Resume Status special code.
                
                rStatus.change.port.SuspendChange=1;
                rStatus.status.port.PortSuspended=0;
                
                portSC.bit.ConnectStatusChange=0;
                portSC.bit.EnableChange=0;
                portSC.bit.OverCurrentChange=0;        

                portSC.bit.ForcePortResume =0;
                portSC.bit.Suspend=0;
                Write_PORTSC(port,portSC);
            }
        }
        else if (portSC.bit.Power) {
            Write_PORTSC( port, portSC );
        }
    }
#ifdef DEBUG
    else {
        // request is to Hub. rStatus was already memset to 0 above.
        DEBUGCHK( port == 0 );
        // local power supply good
        DEBUGCHK( rStatus.status.hub.LocalPowerStatus == 0 );
        // no over current condition
        DEBUGCHK( rStatus.status.hub.OverCurrentIndicator == 0 );
        // no change in power supply status
        DEBUGCHK( rStatus.change.hub.LocalPowerChange == 0 );
        // no change in over current status
        DEBUGCHK( rStatus.change.hub.OverCurrentIndicatorChange == 0 );
    }
#endif // DEBUG

    return TRUE;
}

// ******************************************************************
BOOL CHW::RootHubFeature( IN const UCHAR port,
                          IN const UCHAR setOrClearFeature,
                          IN const USHORT feature )
//
// Purpose: This function clears all the status change bits associated with
//          the specified root hub port.
//
// Parameters: port - 0 for the hub itself, otherwise the hub port number
//
// Returns: TRUE iff the requested operation is valid, FALSE otherwise.
//
// Notes: Assume that caller has already verified the parameters from a USB
//        perspective. The HC hardware may only support a subset of that
//        (which is indeed the case for UHCI).
// ******************************************************************
{
    if (port == 0) {
        // request is to Hub but...
        // uhci has no way to tweak features for the root hub.
        return FALSE;
    }

    // mask the change bits because we write 1 to them to clear them //
    PORTSC portSC= Read_PORTSC( port );
    if ( portSC.bit.Power && portSC.bit.Owner==0) {
        portSC.bit.ConnectStatusChange=0;
        portSC.bit.EnableChange=0;
        portSC.bit.OverCurrentChange=0;
        if (setOrClearFeature == USB_REQUEST_SET_FEATURE)
            switch (feature) {
              case USB_HUB_FEATURE_PORT_RESET:              portSC.bit.Reset=1;break;
              case USB_HUB_FEATURE_PORT_SUSPEND:            portSC.bit.Suspend=1; break;
              case USB_HUB_FEATURE_PORT_POWER:              portSC.bit.Power=1;break;
              default: return FALSE;
            }
        else
            switch (feature) {
              case USB_HUB_FEATURE_PORT_ENABLE:             portSC.bit.Enabled=0; break;
              case USB_HUB_FEATURE_PORT_SUSPEND:            // EHCI 2.3.9
                if (portSC.bit.Suspend !=0 ) {
                    portSC.bit.ForcePortResume=1; 
                    Write_PORTSC( port, portSC );
                    Sleep(20);
                    portSC.bit.ForcePortResume=0;
                }
                break;
              case USB_HUB_FEATURE_C_PORT_CONNECTION:       portSC.bit.ConnectStatusChange=1;break;
              case USB_HUB_FEATURE_C_PORT_ENABLE:           portSC.bit.EnableChange=1; break;
              case USB_HUB_FEATURE_C_PORT_OVER_CURRENT:     portSC.bit.OverCurrentChange=1; break;
              case USB_HUB_FEATURE_C_PORT_RESET:            
              case USB_HUB_FEATURE_C_PORT_SUSPEND:
              case USB_HUB_FEATURE_PORT_POWER:
              default: return FALSE;
            }

        Write_PORTSC( port, portSC );
        return TRUE;
    }
    else
        return FALSE;
}


// ******************************************************************
BOOL CHW::ResetAndEnablePort( IN const UCHAR port )
//
// Purpose: reset/enable device on the given port so that when this
//          function completes, the device is listening on address 0
//
// Parameters: port - root hub port # to reset/enable
//
// Returns: TRUE if port reset and enabled, else FALSE
//
// Notes: This function takes approx 60 ms to complete, and assumes
//        that the caller is handling any critical section issues
//        so that two different ports (i.e. root hub or otherwise)
//        are not reset at the same time. please refer 4.2.2 for detail
// ******************************************************************
{
    BOOL fSuccess = FALSE;

    PORTSC portSC=Read_PORTSC(port);
    // no point reset/enabling the port unless something is attached
    if ( portSC.bit.Power && portSC.bit.Owner==0 && portSC.bit.ConnectStatus ) {
        // Do not touch Write to Clear Bit.
        portSC.bit.ConnectStatusChange=0;
        portSC.bit.EnableChange=0;
        portSC.bit.OverCurrentChange=0;

        //Note: If the EHCI hardware does not have a companion controllers(CC),  
        //and instead has transaction translators, then the EHCI controller will 
        //handle the Low- and Full-Speed devices also.
        //If USB1.1 Host Controllers are available, then, let them handle the 
        //Low- and Full-Speed devices.
        if( m_NumOfCompanionControllers == 0  || //There are no CC, handle everything
            portSC.bit.LineStatus != 1 ) { //not K-State,means not Low-Speed
            // turn on reset bit
            portSC.bit.Reset =1 ;
            portSC.bit.Enabled=0;
            Write_PORTSC( port, portSC );
            // Note - Win98 waited 10 ms here. But, the new USB 1.1 spec
            // section 7.1.7.3 recommends 50ms for root hub ports
            Sleep( 50 );

            // Clear the reset bit
            portSC.bit.Reset =0 ;
            Write_PORTSC( port, portSC );
            for (DWORD dwIndex=0; dwIndex<10 && Read_PORTSC(port).bit.Reset!=0 ; dwIndex++)
                Sleep(10);
            
            portSC = Read_PORTSC( port );
            if ( portSC.bit.Enabled && portSC.bit.Reset == 0 ) {
                // port is enabled
                fSuccess = TRUE;
            }
        }

        //
        // clear port connect & enable change bits
        //
        if (fSuccess) {
            portSC.bit.ConnectStatusChange=0; // Do not clean ConnectStatusChange.
            portSC.bit.EnableChange=1;
        }
        else  // Turn Off the OwnerShip. EHCI 4.2.2 
            portSC.bit.Owner=1;
        Write_PORTSC( port, portSC );
            
        // USB 1.1 spec, 7.1.7.3 - device may take up to 10 ms
        // to recover after reset is removed
        Sleep( 10 );
    }

    DEBUGMSG( ZONE_REGISTERS, (TEXT("%s: Root hub, after reset & enable, port %d portsc = 0x%04x\n"),GetControllerName(), port, Read_PORTSC( port ) ) );
    return fSuccess;
}
// ******************************************************************
void CHW::DisablePort( IN const UCHAR port )
//
// Purpose: disable the given root hub port
//
// Parameters: port - port # to disable
//
// Returns: nothing
//
// Notes: This function will take about 10ms to complete
// ******************************************************************
{
    PORTSC portSC=Read_PORTSC(port);;
    // no point doing any work unless the port is enabled
    if ( portSC.bit.Power && portSC.bit.Owner==0 && portSC.bit.Enabled ) {
        // clear port enabled bit and enabled change bit,
        // but don't alter the connect status change bit,
        // which is write-clear.
        portSC.bit.Enabled=0;
        portSC.bit.ConnectStatusChange=0;
        portSC.bit.EnableChange=1;
        portSC.bit.OverCurrentChange=0;        
        Write_PORTSC( port, portSC );

        // disable port can take some time to act, because
        // a USB request may have been in progress on the port.
        Sleep( 10 );
    }
}

#ifdef USB_IF_ELECTRICAL_TEST_MODE

void CHW::SuspendPort(IN const UCHAR port )
{
 
    PORTSC portSC=Read_PORTSC(port);
    // no point doing any work unless the port is enabled
    if ( portSC.bit.Power && portSC.bit.Owner==0 && portSC.bit.Enabled ) {
        // set port suspend bit. but don't alter the enabled change bit,
        // connect status change bit, over current change bit
        // which is write-clear.
        portSC.bit.Suspend=1;
        portSC.bit.ConnectStatusChange=0;
        portSC.bit.EnableChange=0;
        portSC.bit.OverCurrentChange=0;        
        Write_PORTSC( port, portSC );;

        // suspend port can take some time to act, because
        // a USB request may have been in progress on the port.
        Sleep( 10 );    
    }
}

void CHW::ResumePort(IN const UCHAR port )
{
    PORTSC portSC=Read_PORTSC(port);
    // no point doing any work unless the port is enabled
    if ( portSC.bit.Power && portSC.bit.Owner==0 && portSC.bit.Enabled ) {
        // clr port suspend bit. but don't alter the enabled change bit,
        // connect status change bit, over current change bit
        // which is write-clear.
        portSC.bit.ForcePortResume=1;
        portSC.bit.ConnectStatusChange=0;
        portSC.bit.EnableChange=0;
        portSC.bit.OverCurrentChange=0;        
        Write_PORTSC( port, portSC );
        Sleep(20);
        portSC.bit.ForcePortResume=0;
        Write_PORTSC( port, portSC );

        // resume port can take upto 2 ms to act.
        // Give it ample time.
        Sleep(10); 
    }    
}

#endif //#ifdef USB_IF_ELECTRICAL_TEST_MODE

BOOL CHW::WaitForPortStatusChange (HANDLE m_hHubChanged)
{
    if (m_hUsbHubChangeEvent) {
        if (m_hHubChanged!=NULL) {
            HANDLE hArray[2];
            hArray[0]=m_hHubChanged;
            hArray[1]=m_hUsbHubChangeEvent;
            WaitForMultipleObjects(2,hArray,FALSE,INFINITE);
        }
        else
            WaitForSingleObject(m_hUsbHubChangeEvent,INFINITE);
        return TRUE;
    }
    return FALSE;
}

// ******************************************************************
VOID CHW::PowerMgmtCallback( IN BOOL fOff )
//
// Purpose: System power handler - called when device goes into/out of
//          suspend.
//
// Parameters:  fOff - if TRUE indicates that we're entering suspend,
//                     else signifies resume
//
// Returns: Nothing
//
// Notes: This needs to be implemented for HCDI
// ******************************************************************
{
    if ( fOff )
    {
        if ((GetCapability() & HCD_SUSPEND_RESUME)!= 0) {
            m_bDoResume=TRUE;
            SuspendHostController();
        }
        else {
            m_bDoResume=FALSE;;
            CHW::StopHostController();
        }
    }
    else
    {   // resuming...
        g_fPowerUpFlag = TRUE;
        if (m_bDoResume)
            ResumeHostController();
        if (!g_fPowerResuming)
            // can't use member data while `this' is invalid
            SetInterruptEvent(m_dwSysIntr);
    }
    return;
}
VOID CHW::SuspendHostController()
{
    if ( m_portBase != 0 ) {
        // initialize interrupt register - set only RESUME interrupts to enabled
        Lock();
        USBCMD usbcmd=Read_USBCMD();
        usbcmd.bit.RunStop=0;
        Write_USBCMD(usbcmd);
        Unlock();
        // EHCI do not have group suspend. But. We can suspend each port.
        for (UINT port =1; port <= m_NumOfPort; port ++) {
            PORTSC portSC=Read_PORTSC(port);;
            // no point doing any work unless the port is enabled
            if ( portSC.bit.Power && portSC.bit.Owner==0 && portSC.bit.Enabled ) {
                portSC.bit.ConnectStatusChange=0;
                portSC.bit.EnableChange=0;
                portSC.bit.OverCurrentChange=0;        
                //
                portSC.bit.ForcePortResume =0;
                portSC.bit.Suspend=1;
                portSC.bit.WakeOnConnect = 1;
                portSC.bit.WakeOnDisconnect =1;
                portSC.bit.WakeOnOverCurrent =1;
                Write_PORTSC( port, portSC );
            }
        }

    }
}
VOID CHW::ResumeHostController()
{
    if ( m_portBase != 0 ) {
        // I need 20 ms delay here 30(30ns)*1000*20
        for (UINT port =1; port <= m_NumOfPort; port ++) {
            PORTSC portSC=Read_PORTSC(port);;
            // no point doing any work unless the port is enabled
            if ( portSC.bit.Power && portSC.bit.Owner==0 && portSC.bit.Enabled ) {
                portSC.bit.ConnectStatusChange=0;
                portSC.bit.EnableChange=0;
                portSC.bit.OverCurrentChange=0;        
                //
                portSC.bit.ForcePortResume =1;
                portSC.bit.Suspend=0;
                Write_PORTSC( port, portSC );
            }
        }
        for (DWORD dwIndex =0; dwIndex<30*1000*20; dwIndex++)
            Read_USBCMD();
        for (port =1; port <= m_NumOfPort; port ++) {
            PORTSC portSC=Read_PORTSC(port);;
            // no point doing any work unless the port is enabled
            if ( portSC.bit.Power && portSC.bit.Owner==0 && portSC.bit.Enabled ) {
                portSC.bit.ConnectStatusChange=0;
                portSC.bit.EnableChange=0;
                portSC.bit.OverCurrentChange=0;        
                //
                portSC.bit.ForcePortResume =0;
                portSC.bit.Suspend=0;
                Write_PORTSC( port, portSC );
            }
        }
        Lock();
        USBCMD usbcmd=Read_USBCMD();
        usbcmd.bit.RunStop=1;
        Write_USBCMD(usbcmd);
        Unlock();
    }
    ResumeNotification();

}
DWORD CHW::SetCapability(DWORD dwCap)
{
    m_dwCapability |= dwCap; 
    if ( (m_dwCapability & HCD_SUSPEND_RESUME)!=0) {
        KernelIoControl(IOCTL_HAL_ENABLE_WAKE, &m_dwSysIntr, sizeof(m_dwSysIntr), NULL, 0, NULL);
    }
    return m_dwCapability;
};

// utility functions
BOOL CHW::ReadUSBHwInfo()
{
    m_dwEHCIHwID = USB_HW_ID_GENERIC_EHCI;

    if (m_deviceReg.IsKeyOpened() && m_deviceReg.GetRegValue(EHCI_REG_USBHwID, (LPBYTE)&m_dwEHCIHwID, sizeof(m_dwEHCIHwID))) {       
        if (m_dwEHCIHwID == USB_HW_ID_CI13611A) { // verify that it is a ChipIdea CI13611A indeed
            if (!m_deviceReg.GetRegValue(EHCI_REG_USBHwRev, (LPBYTE)&m_dwEHCIHwRev, sizeof(m_dwEHCIHwRev))) {
                DEBUGMSG(ZONE_ERROR, (TEXT("%s: CHW::Initialize - CI13611 core requires a revision value in registry\n"),GetControllerName()));
                ASSERT(FALSE);
                return FALSE;
            }

            DEBUGCHK( m_portBase != 0 );
            DWORD dwID;
            __try {
                dwID = READ_REGISTER_ULONG( (PULONG)(m_portBase - Read_CapLength() - EHCI_HW_CI13611_OFFSET_CAPLENGTH) );
            } __except( EXCEPTION_EXECUTE_HANDLER ) {
                DEBUGMSG(ZONE_ERROR, (TEXT("%s: CHW::Initialize - CI13611 ID register not found\n"),GetControllerName()));
                ASSERT(FALSE);
                return FALSE;
            }            
            
            DWORD dwRev = (dwID & 0x00ff0000) >> 16;
            dwID &= 0x000000ff;

            if (dwID != EHCI_HW_CI13611_ID || dwRev < m_dwEHCIHwRev) {
                DEBUGMSG(ZONE_ERROR, (TEXT("%s: CHW::Initialize - unsupported ChipIdea USB core\n"),GetControllerName()));
                ASSERT(FALSE);
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL CHW::ReadUsbInterruptEventName(TCHAR **ppCpszHsUsbFnIntrEvent)
{
    if (ppCpszHsUsbFnIntrEvent == NULL) {
        return FALSE;
    }

    *ppCpszHsUsbFnIntrEvent = NULL;

    if (m_dwEHCIHwID == USB_HW_ID_CI13611A) {
        DWORD  dwDataLen = 0;
        // read registry setting for optional named event identifier
        if (m_deviceReg.IsKeyOpened() && m_deviceReg.GetRegSize( EHCI_REG_HSUSBFN_INTERRUPT_EVENT, dwDataLen )) {
            *ppCpszHsUsbFnIntrEvent = new TCHAR[dwDataLen / sizeof(TCHAR)];

            if (*ppCpszHsUsbFnIntrEvent == NULL) {
                return FALSE;
            }

            if (!m_deviceReg.GetRegValue(EHCI_REG_HSUSBFN_INTERRUPT_EVENT, (LPBYTE)*ppCpszHsUsbFnIntrEvent, dwDataLen)) {                
                return FALSE;
            }
        } else {
            DEBUGMSG(ZONE_ERROR, (TEXT("%s: -CHW::Initialize. Error reading HS USB FN Interrupt event name from registry\n"),GetControllerName()));
            return FALSE;
        }  
    }

    return TRUE;
}

#ifdef USB_IF_ELECTRICAL_TEST_MODE

// ******************************************************************
BOOL CHW::SetTestMode(IN UINT portNum, IN UINT mode)
//
// Purpose: Set or reset the specified port in the specified test mode
//
// Parameters:  portNum - port number of the port
//
//              mode - test mode to put the port in.
//
//              fClear  - disable test mode, otherwise enable..
//
// Returns: TRUE if Success.
//
// Notes: This needs to be implemented for HCDI. information about electrical
//        test modes is found in EHCI Specification-section 4.14, USB2.0 
//        specification-sections 7.1.20, 9.4.9.
{
    /*
    What needs to be done here?
    1.  Lock
    2.  Set testmode flag inside this hcd object
    3.  Disable periodic and Asynchronous schedule
    4.  Suspend all the enabled root ports by setting the syspend bit in PORTSC reg
    5.  Set RUN/Stop bit in USBCMD to 0
    6.  Wait for HCHalted in USBSTS to transition to 1
    7.  Set Port Test Control field in PORTSC reg to the desired test mode. 
    8.  If Test_Force_Enable is desired, then set Rn/stop bit to 1.
    9.  When test is complete, halt the controller, reset the controller.
    10. Unlock
    */

    DWORD dwRet = FALSE;
    //Validate inputs
    if(0 == portNum || portNum > m_NumOfPort) {
        return FALSE;
    }
    
    PORTSC portsc = Read_PORTSC(portNum);
    switch(mode) {
        case USB_EHCI_TEST_MODE_DISABLED:
            if(m_currTestMode != mode) {
                m_currTestMode = mode;
                Lock();
                portsc.bit.TestControl = mode;
                portsc.bit.OverCurrentChange=0;
                portsc.bit.EnableChange=0;
                portsc.bit.ConnectStatusChange=0;
                Write_PORTSC(portNum, portsc);
                Unlock();                
                ReturnFromTestMode();
                return TRUE;
            }
            else {
                return FALSE;
            }
                
            break;
            
        case USB_EHCI_TEST_MODE_J_STATE:
        case USB_EHCI_TEST_MODE_K_STATE:
        case USB_EHCI_TEST_MODE_SE0_NAK:
        case USB_EHCI_TEST_MODE_TEST_PACKET:
        case USB_EHCI_TEST_MODE_FORCE_ENABLE:
            if((m_currTestMode != USB_EHCI_TEST_MODE_DISABLED) ||
                (FALSE == PrepareForTestMode()))
                return FALSE;
            Lock();
            portsc.bit.TestControl = mode;
            portsc.bit.OverCurrentChange=0;
            portsc.bit.EnableChange=0;
            portsc.bit.ConnectStatusChange=0;
            Write_PORTSC(portNum, portsc);

            if(USB_EHCI_TEST_MODE_FORCE_ENABLE == mode) {
                USBCMD usbcmd=Read_USBCMD();
                // Check run bit. 
                if(!usbcmd.bit.RunStop) {
                    // set run bit
                    usbcmd.bit.RunStop= 1;
                    Write_USBCMD( usbcmd );  
                    // spin until the controller really is stopped
                    while( Read_USBSTS().bit.HCHalted == 1 ) //Wait until it runs.
                        Sleep(0);
                }
            }
            m_currTestMode = mode;
            Unlock();
            return TRUE;
            break;
            
        default:
            return FALSE;                
    }

    
}

BOOL CHW::PrepareForTestMode()
{
    Lock();
    //Disable Periodic and Asynchronous schedule
    USBCMD usbcmd = Read_USBCMD();
    usbcmd.bit.PSchedEnable = 0;
    Write_USBCMD(usbcmd);
    while(usbcmd.bit.PSchedEnable != Read_USBSTS().bit.PSStatus)
        Sleep(0);
    
    EnableDisableAsyncSch(FALSE);

    //Suspend all the enabled root ports by setting the syspend bit in PORTSC reg

    for(UCHAR i = 1; i <= m_NumOfPort; i++){
        SuspendPort(i);
    }

    //Set RUN/Stop bit in USBCMD to 0
    //Wait for HCHalted in USBSTS to transition to 1
    usbcmd=Read_USBCMD();
    // Check run bit. 
    if(usbcmd.bit.RunStop) {
        // clear run bit
        usbcmd.bit.RunStop= 0;
        Write_USBCMD( usbcmd );  
        // spin until the controller really is stopped
        while( Read_USBSTS().bit.HCHalted == 0 ) //Wait until it stop.
            Sleep(0);
    }
    Unlock();
    return TRUE;    
}

BOOL CHW::ReturnFromTestMode()
{
   //When test is complete, halt the controller, reset the controller.
   //EHCI Spec 4.14 says that the controller is to be reset using the HCReset
   //bit in USBCMD. All the internal pipelines, timers, counters, state machines
   //etc will return to its initial calue. Hence the HCD will have to be 
   //reinitialized.
   Lock();
   CHW::StopHostController();
   CHW::DeInitialize();

   //Reset the HostController Hardware
   USBCMD usbcmd=Read_USBCMD();
    usbcmd.bit.HCReset=1;
    Write_USBCMD(usbcmd);
    for (DWORD dwCount=0;dwCount<50 && (Read_USBCMD().bit.HCReset!=0);dwCount++)
        Sleep( 20 );

    if (Read_USBCMD().bit.HCReset!=0) // If can not reset within 1 second, we assume this is bad device.
        return FALSE;
    
    for(UCHAR i = 1; i <= m_NumOfPort; i++){
        ResumePort(i);
    }

    //Initialize the host controller
    CHW::Initialize();
    CHW::EnterOperationalState();
    Unlock();
    return TRUE;
}

#endif //#ifdef USB_IF_ELECTRICAL_TEST_MODE

#ifdef DEBUG
// ******************************************************************
void CHW::DumpUSBCMD( void )
//
// Purpose: Queries Host Controller for contents of USBCMD, and prints
//          them to DEBUG output. Bit definitions are in UHCI spec 2.1.1
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: used in DEBUG mode only
//
//        This function is static
// ******************************************************************
{
    __try {
         USBCMD usbcmd=Read_USBCMD();

        DEBUGMSG(ZONE_REGISTERS, (TEXT("%s: \tCHW - USB COMMAND REGISTER (USBCMD) = 0x%X. Dump:\n"),GetControllerName(), usbcmd.ul));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tHost Controller Reset = %s\n"), (usbcmd.bit.HCReset ? TEXT("Set") : TEXT("Not Set"))));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tRun/Stop = %s\n"), ( usbcmd.bit.RunStop ? TEXT("Run") : TEXT("Stop"))));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_REGISTERS, (TEXT("%s: \t\tCHW - FAILED WHILE DUMPING USBCMD!!!\n"),GetControllerName()));
    }
}
// ******************************************************************
void CHW::DumpUSBSTS( void )
//
// Purpose: Queries Host Controller for contents of USBSTS, and prints
//          them to DEBUG output. Bit definitions are in UHCI spec 2.1.2
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: used in DEBUG mode only
//
//        This function is static
// ******************************************************************
{
    __try {
        USBSTS usbsts = Read_USBSTS();
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\tCHW - USB STATUS REGISTER (USBSTS) = 0x%X. Dump:\n"), usbsts.ul));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tHCHalted = %s\n"), ( usbsts.bit.HCHalted ? TEXT("Halted") : TEXT("Not Halted"))));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tHost System Error = %s\n"), (usbsts.bit.HSError ? TEXT("Set") : TEXT("Not Set"))));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tUSB Error Interrupt = %s\n"), (usbsts.bit.USBERRINT ? TEXT("Set") : TEXT("Not Set"))));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tUSB Interrupt = %s\n"), (usbsts.bit.USBINT ? TEXT("Set") : TEXT("Not Set"))));

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tCHW - FAILED WHILE DUMPING USBSTS!!!\n")));
    }
}
// ******************************************************************
void CHW::DumpUSBINTR( void )
//
// Purpose: Queries Host Controller for contents of USBINTR, and prints
//          them to DEBUG output. Bit definitions are in UHCI spec 2.1.3
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: used in DEBUG mode only
//
//        This function is static
// ******************************************************************
{
    __try {
        USBINTR usbintr = Read_USBINTR();
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\tCHW - USB INTERRUPT REGISTER (USBINTR) = 0x%X. Dump:\n"), usbintr.ul));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tCHW - FAILED WHILE DUMPING USBINTR!!!\n")));
    }
}
// ******************************************************************
void CHW::DumpFRNUM( void )
//
// Purpose: Queries Host Controller for contents of FRNUM, and prints
//          them to DEBUG output. Bit definitions are in UHCI spec 2.1.4
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: used in DEBUG mode only
//
//        This function is static
// ******************************************************************
{
    __try {
        FRINDEX frindex = Read_FRINDEX();
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\tCHW - FRAME NUMBER REGISTER (FRNUM) = 0x%X. Dump:\n"), frindex.ul));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tMicroFrame number (bits 2:0) = %d\n"), frindex.bit.microFlame));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tFrame index (bits 11:3) = %d\n"), frindex.bit.FrameIndex));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tCHW - FAILED WHILE DUMPING FRNUM!!!\n")));
    }
}

// ******************************************************************
void CHW::DumpFLBASEADD( void )
//
// Purpose: Queries Host Controller for contents of FLBASEADD, and prints
//          them to DEBUG output. Bit definitions are in UHCI spec 2.1.5
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: used in DEBUG mode only
//
//        This function is static
// ******************************************************************
{
    DWORD    dwData = 0;

    __try {
        dwData = Read_EHCIRegister( PERIODICLISTBASE );
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\tCHW - FRAME LIST BASE ADDRESS REGISTER (FLBASEADD) = 0x%X. Dump:\n"), dwData));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tFLBASEADD address base (bits 11:0 masked) = 0x%X\n"), (dwData & EHCD_FLBASEADD_MASK)));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tCHW - FAILED WHILE DUMPING FLBASEADD!!!\n")));
    }
}
// ******************************************************************
void CHW::DumpSOFMOD( void )
//
// Purpose: Queries Host Controller for contents of SOFMOD, and prints
//          them to DEBUG output. Bit definitions are in UHCI spec 2.1.6
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: used in DEBUG mode only
//
//        This function is static
// ******************************************************************
{
    __try {
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\tCHW - ASYNCLISTADDR = 0x%X. Dump:\n"),Read_EHCIRegister( ASYNCLISTADDR)));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tCHW CONFIGFLAG = %x\n"), Read_EHCIRegister(CONFIGFLAG)));
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tCHW CTLDSSEGMENT = %x\n"), Read_EHCIRegister(CTLDSSEGMENT)));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tCHW - FAILED WHILE DUMPING SOFMOD!!!\n")));
    }
}

// ******************************************************************
void CHW::DumpPORTSC(IN const USHORT port)
//
// Purpose: Queries Host Controller for contents of PORTSC #port, and prints
//          them to DEBUG output. Bit definitions are in UHCI spec 2.1.7
//
// Parameters: port - the port number to read. It must be such that
//                    1 <= port <= UHCD_NUM_ROOT_HUB_PORTS
//
// Returns: Nothing
//
// Notes: used in DEBUG mode only
//
//        This function is static
// ******************************************************************
{
    DWORD    dwData = 0;

    __try {
        DEBUGCHK( port >=  1 && port <=  m_NumOfPort );
        if (port >=  1 && port <=  m_NumOfPort ) {
            PORTSC portSC = Read_PORTSC( port );
            DEBUGMSG(ZONE_REGISTERS, (TEXT("\tCHW - PORT STATUS AND CONTROL REGISTER (PORTSC%d) = 0x%X. Dump:\n"), port, dwData));
            if ( portSC.bit.Power && portSC.bit.Owner==0 && portSC.bit.Enabled ) {
                DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tHub State = %s\n"), (portSC.bit.Suspend ? TEXT("Suspend") : TEXT("Enable"))));
                DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tPort Reset = %s\n"), ( portSC.bit.Reset ? TEXT("Reset") : TEXT("Not Reset"))));
                DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tResume Detect = %s\n"), (portSC.bit.ForcePortResume ? TEXT("Set") : TEXT("Not Set"))));
                DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tLine Status = %d\n"), ( portSC.bit.LineStatus )));
                DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tPort Enable/Disable Change  = %s\n"), ( portSC.bit.EnableChange ? TEXT("Set") : TEXT("Not Set"))));
                DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tConnect Status Change = %s\n"), (portSC.bit.ConnectStatusChange? TEXT("Set") : TEXT("Not Set"))));
                DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tConnect Status = %s\n"), (portSC.bit.ConnectStatus ? TEXT("Device Present") : TEXT("No Device Present"))));
            } else {
                DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tHub State this port Disabled\n")));
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DEBUGMSG(ZONE_REGISTERS, (TEXT("\t\tCHW - FAILED WHILE DUMPING PORTSC%d!!!\n"), port));
    }
}

// ******************************************************************
void CHW::DumpAllRegisters( void )
//
// Purpose: Queries Host Controller for all registers, and prints
//          them to DEBUG output. Register definitions are in UHCI spec 2.1
//
// Parameters: None
//
// Returns: Nothing
//
// Notes: used in DEBUG mode only
//
//        This function is static
// ******************************************************************
{
    DEBUGMSG(ZONE_REGISTERS, (TEXT("CHW - DUMP REGISTERS BEGIN\n")));
    DumpUSBCMD();
    DumpUSBSTS();
    DumpUSBINTR();
    DumpFRNUM();
    DumpFLBASEADD();
    DumpSOFMOD();
    for ( USHORT port = 1; port <=  m_NumOfPort; port++ ) {
        DumpPORTSC( port );
    }
    DEBUGMSG(ZONE_REGISTERS, (TEXT("CHW - DUMP REGISTERS DONE\n")));
}
#endif


