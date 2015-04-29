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
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//------------------------------------------------------------------------------

#include <windows.h>
#include <bootarg.h>
#include "fs.h"
#include "debug.h"
#include "bldr.h"

#define READ_BUFFER_START 0x0C900

#pragma pack(1)
typedef struct
{
    BYTE nSize;
    BYTE Reserved;
    USHORT Sectors;
    PUCHAR Buffer;
    DWORD BlockL;
    DWORD BlockH;
} READ_PACKET, *PREAD_PACKET;
#pragma pack()

//
// BIOS functions interface. See bios.asm for implementation.
//
//BYTE BIOS_ReadSectors(UCHAR Drive, USHORT c, UCHAR h, UCHAR s, USHORT nSectors, PUCHAR pBuffer);
BYTE BIOS_ReadSectorsEx(UCHAR Drive, READ_PACKET* pPacket);
BYTE BIOS_ResetDisk(UCHAR Drive);

//
// BIOS INT13H error number-to-text mapping.
//
#ifdef FULLINT13MESSAGES
struct
{
    BYTE   nErrVal;
    LPCSTR pErrText;
} DiskErrs[] =
{
  { 0x01, "Invalid function in AH or invalid parameter" },
  { 0x02, "Address mark not found" },
  { 0x03, "Disk write-protected" },
  { 0x04, "Sector not found/read error" },
  { 0x05, "Reset failed (hard disk)" },
  { 0x06, "Disk changed (floppy)" },
  { 0x07, "Drive parameter activity failed (hard disk)" },
  { 0x08, "DMA overrun" },
  { 0x09, "Data boundary error (attempted DMA across a 64K boundary)" },
  { 0x0A, "Bad sector detected (hard disk)" },
  { 0x0B, "Bad track detected (hard disk)" },
  { 0x0C, "Unsupported track or invalid media" },
  { 0x0D, "Invalid number of sectors on format (PS/2 hard disk)" },
  { 0x0E, "Control data address mark detected (hard disk)" },
  { 0x0F, "DMA arbitration level out of range (hard disk)" },
  { 0x10, "Uncorrectable CRC or ECC error on read" },
  { 0x11, "Data ECC corrected (hard disk)" },
  { 0x20, "Controller failure" },
  { 0x31, "No media in drive (IBM/MS INT 13 extensions)" },
  { 0x32, "Incorrect drive type stored in CMOS" },
  { 0x40, "Seek failed" },
  { 0x80, "Timeout (not ready)" },
  { 0xAA, "Drive not ready (hard disk)" },
  { 0xB0, "Volume not locked in drive (INT 13 extensions)" },
  { 0xB1, "Volume locked in drive (INT 13 extensions)" },
  { 0xB2, "Volume not removable (INT 13 extensions)" },
  { 0xB3, "Volume in use (INT 13 extensions)" },
  { 0xB4, "Lock count exceeded (INT 13 extensions)" },
  { 0xB5, "Valid eject request failed (INT 13 extensions)" },
  { 0xB6, "Volume present but read protected (INT 13 extensions)" },
  { 0xBB, "Undefined error (hard disk)" },
  { 0xCC, "Write fault (hard disk)" },
  { 0xE0, "Status register error (hard disk)" },
  { 0xFF, "Sense operation failed (hard disk)" },
  {    0, "<unknown>" }
};
#endif

//
// Convert the error value returned by the BIOS INT13h call into a text message.
//
#ifdef FULLINT13MESSAGES

LPCSTR GetErrorText(BYTE nErrVal)
{
    BYTE nCount;

    for (nCount = 0 ; DiskErrs[nCount].nErrVal ; nCount++)
    {
        if (DiskErrs[nCount].nErrVal == nErrVal)
            break;
    }

    return(DiskErrs[nCount].pErrText);
}
#endif

//
// Convert an LBA value to a physical CHS value (used by the BIOS).
//
ULONG LBA2PCHS(ULONG LBA,
               USHORT NumHeads,
               USHORT SectorsPerTrack,
               PUSHORT pC,
               PUCHAR pH,
               PUCHAR pS)
{
    USHORT Temp = 0;

    if (pC == NULL || pH == NULL || pS == NULL)
        return(-1);

    // Do the math...
    *pC = (USHORT)(LBA / (NumHeads * SectorsPerTrack));
    Temp = (USHORT)(LBA % (NumHeads * SectorsPerTrack));
    *pH = (UCHAR)(Temp / SectorsPerTrack);
    *pS = (UCHAR)(Temp % SectorsPerTrack) + 1;

    return(0);
}


//
// Read the specified number of sectors from the Drive/LBA combination specified.
//
BOOLEAN ReadSectors(UCHAR DriveID,
                    ULONG LBA,
                    USHORT nSectors,
                    PUCHAR pBuffer)
{
//    USHORT c = 0;
//    UCHAR  h = 0;
//    UCHAR  s = 0;
    UCHAR  Status = 0;
//    UCHAR Retries = 0;
    READ_PACKET Packet = { 0 };


    // Check arguments.
    if (!pBuffer)
        return(FALSE);

    Packet.nSize = sizeof(Packet);
    Packet.BlockL = LBA;
    Packet.Buffer = pBuffer;
    Packet.Sectors = nSectors;
//    WARNMSG(MSG_CANNOT_RESET_DISK_CTRL, ("Reading %d sectors.\n", nSectors));

    // Convert LBA address to P-CHS for IDE controller.
    //if (LBA2PCHS(LBA, pDriveInfo->NumHeads, pDriveInfo->SectorsPerTrack, &c, &h, &s))
    //    return(FALSE);

    // If we're reading from a floppy and it fails, retry a couple of times
    // to allow for the floppy being spun up.
    //if (pDriveInfo->DriveId < 0x80)
    //    Retries = FLOPPY_IO_RETRIES;
    //else
    //    Retries = FIXEDDISK_IO_RETRIES;

    // Call the BIOS to read a sector.
//  WARNMSG(MSG_CANNOT_RESET_DISK_CTRL, ("DriveId: %d\n", pDriveInfo->DriveId));
    if ((Status = BIOS_ReadSectorsEx(DriveID, &Packet)) != 0)
    {
        // If the read failed, retry the operation and reset the controller each time...
        //if (BIOS_ResetDisk(pDriveInfo->DriveId))
        {
//          WARNMSG(MSG_CANNOT_RESET_DISK_CTRL, ("Unable to reset disk controller (drive=0x%x)\n", pDriveInfo->DriveId));
//          break;
        }
    }
//  WARNMSG(MSG_CANNOT_RESET_DISK_CTRL, ("Packet.Buffer: 0x%x, pBuffer: 0x%x\n", Packet.Buffer, pBuffer));

    if (Status)
    {
#ifdef FULLINT13MESSAGES
        ERRMSG(MSG_CANNOT_READ_SECTOR || (Status << 24),
               ("Unable to read %d sector(s) at LBA=0x%x (status=0x%x: %s)\n", nSectors, LBA, Status, GetErrorText(Status)));
//               ("Unable to read %d sector(s) at CHS=0x%x:0x%x:0x%x (status=0x%x: %s)\n", nSectors, c, h, s, Status, GetErrorText(Status)));
#else
        ERRMSG(MSG_CANNOT_READ_SECTOR || (Status << 24),
               ("Unable to read %d sector(s) at LBA=0x%x (status=0x%x)\n", nSectors, LBA, Status));
//               ("Unable to read %d sector(s) at CHS=0x%x:0x%x:0x%x (status=0x%x)\n", nSectors, c, h, s, Status));
#endif
        return(FALSE);
    }

    return(TRUE);
}
