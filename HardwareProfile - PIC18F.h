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
********************************************************************/

#ifndef HARDWARE_PROFILE_PIC18_H
#define HARDWARE_PROFILE_PIC18_H

//#define DEMO_BOARD USER_DEFINED_BOARD

#include "usb_config.h"

//#if defined(USA_USB)			servono cmq se compili la robba usb in progetto...

    //#define USE_SELF_POWER_SENSE_IO
    #define tris_self_power       // Input
    #define self_power          1

    //#define USE_USB_BUS_SENSE_IO
    #define tris_usb_bus_sense      // Input
    #define USB_BUS_SENSE       1 

   
    //Uncomment this to make the output HEX of this project 
    //   to be able to be bootloaded using the HID bootloader
//    #define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER	

    //If the application is going to be used with the HID bootloader
    //  then this will provide a function for the application to 
    //  enter the bootloader from the application (optional)



    /** Board definition ***********************************************/
    //These definitions will tell the main() function which board is
    //  currently selected.  This will allow the application to add
    //  the correct configuration bits as wells use the correct
    //  initialization functions for the board.  These defitions are only
    //  required in the stack provided demos.  They are not required in
    //  final application design.

#if defined(USA_USB) // v. sopra

    #define CLOCK_FREQ 48000000L
    #define GetSystemClock() CLOCK_FREQ   

#else

    #define CLOCK_FREQ 8000000L
    #define GetSystemClock() CLOCK_FREQ   

#endif

    /** LED ************************************************************/

		#define mInitAllLEDs()      LATA &= 0b11011111; TRISA &= 0b11011111;
    
		// 
    #define mLED_1              LATAbits.LATA5
    #define mGetLED_1()         mLED_1
    #define mLED_1_On()         mLED_1 = 1;
    #define mLED_1_Off()        mLED_1 = 0;
    #define mLED_1_Toggle()     mLED_1 ^= 1;

    
    /** SWITCH *********************************************************/
    #define mInitSwitch1()      // fisso TRISAbits.TRISA3=1;
    #define mInitSwitch2()      // fisso TRISAbits.TRISA1=1;
    #define mInitSwitch3()      // fisso TRISAbits.TRISA0=1;
    #define mInitAllSwitches()  mInitSwitch1();mInitSwitch2();mInitSwitch3();
    #define sw1                 PORTAbits.RA1
    #define sw2                 PORTAbits.RA0
		#define sw3                 PORTAbits.RA3

    #define m_I2CClkBit           LATCbits.LATC0
    #define m_I2CDataBit          LATCbits.LATC1
    #define m_I2CDataBitI         PORTCbits.RC1
    #define m_I2CClkBitI          PORTCbits.RC0
    #define I2CDataTris      TRISCbits.TRISC1
    #define I2CClkTris       TRISCbits.TRISC0
		#define SPI_232_I 			PORTCbits.RC1
		#define SPI_232_IO 			LATCbits.LATC1
		#define SPI_232_I_TRIS 			TRISCbits.TRISC1
		#define SPI_232_O 			LATCbits.LATC0
		#define SPI_232_OI 			PORTCbits.RC0
		#define SPI_232_O_TRIS 			TRISCbits.TRISC0
  
/* 
RC7, pin 9 = seg. DP
RC6, pin 8 = seg. G
RC5, pin 5 = seg. F
RC4, pin 6 = seg. E
RC3, pin 7 = seg. D
RC2, pin 14 = seg. C
RC1, pin 15 = seg. B
RC0, pin 16 = seg. A

RB4, pin = cat 3
RB5, pin = cat 2
RB6, pin = cat 1
*/

    #define m_Rele			          LATBbits.LATB7			// 

    /** POT ************************************************************/
//    #define mInitPOT()  {AD1PCFGLbits.PCFG5 = 0;    AD1CON2bits.VCFG = 0x0;    AD1CON3bits.ADCS = 0xFF;    AD1CON1bits.SSRC = 0x0;    AD1CON3bits.SAMC = 0b10000;    AD1CON1bits.FORM = 0b00;    AD1CON2bits.SMPI = 0x0;    AD1CON1bits.ADON = 1;}
// 		RB13



    /** I/O pin definitions ********************************************/
    #define INPUT_PIN 1
    #define OUTPUT_PIN 0


#endif  //HARDWARE_PROFILE_H
