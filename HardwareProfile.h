/********************************************************************
 FileName:      HardwareProfile.h
 Dependencies:  See INCLUDES section
 Processor:     PIC18, PIC24, or PIC32 USB Microcontrollers
 Hardware:      The code is natively intended to be used on the 
                  following hardware platforms: 
                    PICDEM™ FS USB Demo Board
                    PIC18F46J50 FS USB Plug-In Module
                    PIC18F87J50 FS USB Plug-In Module
                    Explorer 16 + PIC24 or PIC32 USB PIMs
                    PIC24F Starter Kit
                    Low Pin Count USB Development Kit
                  The firmware may be modified for use on other USB 
                    platforms by editing this file (HardwareProfile.h)
 Compiler:  	Microchip C18 (for PIC18), C30 (for PIC24), 
                  or C32 (for PIC32)
 Company:       Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the “Company”) for its PIC® Microcontroller is intended and
 supplied to you, the Company’s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

********************************************************************
 File Description:

 Change History:
  Rev   Date         Description
  1.0   11/19/2004   Initial release
  2.1   02/26/2007   Updated for simplicity and to use common coding style
  2.3   09/15/2008   Broke out each hardware platform into its own
                     "HardwareProfile - xxx.h" file
	2023 sparatevi :)
********************************************************************/

#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

//#define DEMO_BOARD USER_DEFINED_BOARD

#include "usb_config.h"


#if defined(__18CXX)
  #include "HardwareProfile - PIC18F.h"
#elif defined(__XC)
  #include "HardwareProfile - PIC16F.h"
#elif defined(__C30__)
  #include "HardwareProfile - PIC24FJ64gb004.h"
#else
	#error bad pic
#endif

#endif  //HARDWARE_PROFILE_H
