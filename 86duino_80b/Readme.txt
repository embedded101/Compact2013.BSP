Date:		29-April-2015
Release:	86Duino_80B

In this release, the following components are added to the BSP:
  1.  AutoLaunch
      AutoLaunch is an utility to launch application during startup.

  2.  AutoLaunch/CoreCon
      Component needed to establish connectivity between Visual Studio IDE and target device,
      to deploy application to the device to testing and debugging. 

  3.  AutoLaunch/IP Address Broadcast
      When added to image, this application is configured to launch during startup,
      to broadcast the device's IP address via UDP.
      The device's IP address is needed to establish connectivity from the Visual Studio IDE.

      IPAddressDiscovery.exe, a Windows console application to detect UDP messages broadcast.
      from the target device and shown the device's IP address, in the BSP's \Misc directory.  
      Launch this application on the development machine prior to power on the target device.

**  Eboot.bin (Ethernet bootloader) is now in the \Misc directory.


--------------------------------------------------------------
Date:  		21-January-2015
Release:	86Duino_80A

This is the initial release of the Compact 2013 BSP for 86Duino.

Ethernet bootloader (Eboot) is included with the BSP, in the \Eboot folder.

The Diskprep utility is used to format and configure SD, Micro-SD and USB flash storage 
with BIOSLoader to launch Eboot and Compact-2013 OS runtime image.

For more information about about Diskprep, refer to the following URL:
    http://bit.ly/1CkfB0X
    Note:  Diskprep usage for Compact 7 is identical to Compact 2013
 