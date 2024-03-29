/********************************************************************
 FileName:		main.c
 Dependencies:	See INCLUDES section
 Processor:		PIC16, PIC18, PIC24, and PIC32 USB Microcontrollers
 Hardware:		This demo is natively intended to be used on Microchip USB demo
 				boards supported by the MCHPFSUSB stack.  See release notes for
 				support matrix.  This demo can be modified for use on other hardware
 				platforms.
 Complier:  	Microchip C18 (for PIC18), C30 (for PIC24), C32 (for PIC32)
 Company:		Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the Company) for its PIC� Microcontroller is intended and
 supplied to you, the Company?s customer, for use solely and
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
  Rev   Description
  ----  -----------------------------------------
  2.6a  Added support for PIC24FJ256GB210

  2.7   No change

  2.8   Improvements to USBCBSendResume(), to make it easier to use.
  2.9   Added event transfer terminated handler code, for improved MSD
        error case handling.  Increased RAM reserved for the C software 
        stack on PIC18 devices.  The previous version did not allocate
        enough RAM for the worst case scenario, when USB_INTERRUPTS mode
        was used, causing potentially unexpected operation.

W1209 voltmetro
(basato su pcb W1209)
 GD 29/11/2023

********************************************************************/

/** INCLUDES *******************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#if defined(__18CXX)
#include <delays.h>
#include <reset.h>
#include <timers.h>
#include <portb.h>
#include <adc.h>
//#include <outcompare.h>
#else
//#include "Flash.h"
#include "HEFlash.h"
#endif

#include "w1209_volt.h"

//#include "swi2c.h"

#ifdef USA_USB
#include "USB.h"
#endif
#include "HardwareProfile.h"


#ifdef USA_USB
#include "usb_function_hid.h"
#endif


/** CONFIGURATION **************************************************/

#if defined(__18CXX)
// https://forum.microchip.com/s/topic/a5C3l000000MUIBEA4/t346503 per pin speciali

        #pragma config CPUDIV = NOCLKDIV
        #pragma config USBDIV = OFF
//        #pragma config FOSC   = HS  per USB SERVE quarzo! 6 o 24MHz...
//        #pragma config PLLEN  = ON
        #pragma config FOSC   = IRC		// HS  per USB SERVE quarzo! 6 o 24MHz...
        #pragma config PLLEN  = OFF		// vado a 8 MHz... v. sotto
        #pragma config FCMEN  = OFF
        #pragma config IESO   = ON
        #pragma config PWRTEN = ON
        #pragma config BOREN  = OFF
        #pragma config BORV   = 30
        #pragma config WDTEN  = OFF
        #pragma config WDTPS  = 32768
        #pragma config MCLRE  = OFF
        #pragma config HFOFST = OFF
        #pragma config STVREN = ON
        #pragma config LVP    = OFF
#ifdef __EXTENDED18__
        #pragma config XINST    = ON       // Extended Instruction Set
#else
        #pragma config XINST    = OFF
#endif
        #pragma config BBSIZ  = OFF
        #pragma config CP0    = OFF
        #pragma config CP1    = OFF
        #pragma config CPB    = OFF
        #pragma config WRT0   = OFF
        #pragma config WRT1   = OFF
        #pragma config WRTB   = OFF
        #pragma config WRTC   = OFF
        #pragma config EBTR0  = OFF
        #pragma config EBTR1  = OFF
        #pragma config EBTRB  = OFF       


#pragma romdata myidlocs=0x200000
const rom char data0=0x04u;
const rom char data1=0x04u;
const rom char data2=0x04u;
const rom char data3=0x07u;
const rom char data4=0x00u;
const rom char data5=0x00u;
const rom char data6=0x00u;
const rom char data7=0x00u;
#pragma romdata
#endif


#if defined(__XC8)
// PIC16F1459 Configuration Bit Settings
// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = ON        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = ON      // Power-up Timer Enable (PWRT enabled)
#pragma config MCLRE = OFF       // MCLR Pin Function Select (MCLR/VPP pin function is I/O)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover Mode (Internal/External Switchover Mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config CPUDIV = CLKDIV6 // CPU System Clock Selection Bit (CPU system clock divided by 6)
#pragma config USBLSCLK = 48MHz // USB Low SPeed Clock Selection bit (System clock expects 48 MHz, FS/LS USB CLKENs divide-by is set to 8.)
#pragma config PLLMULT = 3x     // PLL Multipler Selection Bit (3x Output Frequency Selected)
#pragma config PLLEN = ENABLED  // PLL Enable Bit (3x or 4x PLL Enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = OFF         // Low-Voltage Programming Enable (Low-voltage programming disabled (dice che altrimenti si perde MCLR I/O..)
// ma i pin usb/ICSP funziano solo in LVP!!

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

#endif


/** VARIABLES ******************************************************/
#if defined(__18CXX)
#pragma udata
#endif

#define CLOCK_TUNE_DEFAULT 960		//perde 1 ora su 12, 25/6/12, ecc
WORD clockTune=CLOCK_TUNE_DEFAULT;		//deve andare in Flash...

#ifdef __DEBUG
BYTE SW[3]={1,1,1};
#endif

enum EDITS {
	MENU_NOEDIT=0,
	MENU_VOLT,
	MENU_VOLTRANGE,
	MENU_SECONDI,
	MENU_MINUTI,
	MENU_ORA,
	MENU_GIORNO,
	MENU_MESE,
	MENU_ANNO,
	MENU_MAX
	};


static const rom char CopyrString[]= {'V','o','l','t','m','e','t','r','o',' ','W','1','2','0','9',' ',
#ifdef USA_USB
'c','o','n',' ','U','S','B',' ',
#endif
	'v',VERNUMH+'0','.',VERNUML/10+'0',(VERNUML % 10)+'0', ' ','3','0','/','1','1','/','2','3', 0 };

const rom BYTE table_7seg[]={ 0, 	//PGFEDCBA
													0b00111111,0b00000110,0b01011011,0b01001111,0b01100110,
										 			0b01101101,0b01111101,0b00000111,0b01111111,0b01101111,
													0b01000000,
//													0b00000001,0b00000010,0b00000100,0b00001000,0b00010000,	//indicatori...
//													0b00100000,0b01000000,0b10000000
//													0b01110100,0b01010100,0b01101101,0b01100111,0b00110111,	//indicatori...
//													0b01110111,0b01111000,0b10000000
													0b01110111,0b01111100,0b00111001,0b01011110,0b01111001,0b01110001,	//A..F
													0b00111101,0b01110110,0b00110000,0b00001110,0b01110110,0b00111000,	//G..L
													0b00110111,0b01010100,0b01011100,0b01110011,0b01100111,0b01010000,	//M..R (o piccola per distinguere)
													0b01101101,0b01111000,0b00111110,0b00011100,0b00011100,0b00110110,	//S..X
													0b01101110,0b01011011		//Y..Z
	};

#if defined(__18CXX)
#pragma udata

#if defined(__18F14K50) || defined(__18F13K50) || defined(__18LF14K50) || defined(__18LF13K50) 
    #pragma udata usbram2
#elif defined(__18F2455) || defined(__18F2550) || defined(__18F4455) || defined(__18F4550)\
    || defined(__18F2458) || defined(__18F2453) || defined(__18F4558) || defined(__18F4553)
    #pragma udata USB_VARIABLES=0x500
#elif defined(__18F4450) || defined(__18F2450)
    #pragma udata USB_VARIABLES=0x480
#else
    #pragma udata
#endif
#endif
#if defined(__XC8)
    #if defined(_18F14K50) || defined(_18F13K50) || defined(_18LF14K50) || defined(_18LF13K50)
        #define IN_DATA_BUFFER_ADDRESS 0x260
        #define OUT_DATA_BUFFER_ADDRESS (IN_DATA_BUFFER_ADDRESS + HID_INT_IN_EP_SIZE)
        #define IN_DATA_BUFFER_ADDRESS_TAG @IN_DATA_BUFFER_ADDRESS
        #define OUT_DATA_BUFFER_ADDRESS_TAG @OUT_DATA_BUFFER_ADDRESS
    #elif  defined(_18F2455)   || defined(_18F2550)   || defined(_18F4455)  || defined(_18F4550)\
        || defined(_18F2458)   || defined(_18F2453)   || defined(_18F4558)  || defined(_18F4553)\
        || defined(_18LF24K50) || defined(_18F24K50)  || defined(_18LF25K50)\
        || defined(_18F25K50)  || defined(_18LF45K50) || defined(_18F45K50)
        #define IN_DATA_BUFFER_ADDRESS 0x500
        #define OUT_DATA_BUFFER_ADDRESS (IN_DATA_BUFFER_ADDRESS + HID_INT_IN_EP_SIZE)
        #define IN_DATA_BUFFER_ADDRESS_TAG @IN_DATA_BUFFER_ADDRESS
        #define OUT_DATA_BUFFER_ADDRESS_TAG @OUT_DATA_BUFFER_ADDRESS
    #elif defined(_18F4450) || defined(_18F2450)
        #define IN_DATA_BUFFER_ADDRESS 0x480
        #define OUT_DATA_BUFFER_ADDRESS (IN_DATA_BUFFER_ADDRESS + HID_INT_IN_EP_SIZE)
        #define IN_DATA_BUFFER_ADDRESS_TAG @IN_DATA_BUFFER_ADDRESS
        #define OUT_DATA_BUFFER_ADDRESS_TAG @OUT_DATA_BUFFER_ADDRESS
    #elif defined(_16F1459000)  
#warning  se lo metto d� errore RAM! VERIFICARE
        #define IN_DATA_BUFFER_ADDRESS 0x2050
        #define OUT_DATA_BUFFER_ADDRESS (IN_DATA_BUFFER_ADDRESS + HID_INT_IN_EP_SIZE)
        #define IN_DATA_BUFFER_ADDRESS_TAG @IN_DATA_BUFFER_ADDRESS
        #define OUT_DATA_BUFFER_ADDRESS_TAG @OUT_DATA_BUFFER_ADDRESS
    #else
        #define IN_DATA_BUFFER_ADDRESS_TAG
        #define OUT_DATA_BUFFER_ADDRESS_TAG
    #endif
#else
    #define IN_DATA_BUFFER_ADDRESS_TAG
    #define OUT_DATA_BUFFER_ADDRESS_TAG
#endif

#if defined(__XC8)
volatile unsigned char hid_report_in[HID_INT_IN_EP_SIZE] IN_DATA_BUFFER_ADDRESS_TAG;
volatile unsigned char hid_report_out[HID_INT_OUT_EP_SIZE] OUT_DATA_BUFFER_ADDRESS_TAG;
#endif
#define ReceivedDataBuffer hid_report_out
#define ToSendDataBuffer hid_report_in
//unsigned char ReceivedDataBuffer[HID_INT_IN_EP_SIZE];
//unsigned char ToSendDataBuffer[HID_INT_OUT_EP_SIZE];

#if defined(__18CXX)
#pragma udata
#endif


#ifdef USA_USB
USB_HANDLE USBOutHandle = 0;
USB_HANDLE USBInHandle = 0;
#endif


BYTE USBOn=FALSE,MenuMode;
struct SAVED_PARAMETERS configParms;

#if defined(__18CXX)
#ifndef __EXTENDED18__
#pragma udata access ACCESSBANK
#else
#pragma udata gpr0
#define near
#endif
#endif
#if defined(__18CXX)
volatile near unsigned char Displays[3];			// 3 display 
#else
volatile unsigned char Displays[3];			// 3 display 
#endif

#if defined(__18CXX)
#pragma udata gpr0
#endif
volatile BYTE dim=FALSE;
BYTE Buzzer=0;
WORD Voltage=0;
static BYTE firstPassDisplay=20;
volatile WORD tick10=0;
volatile BYTE second_10=0;
#if defined(__XC8)
//volatile PIC24_RTCC_DATE currentDate;
//volatile PIC24_RTCC_TIME currentTime;
#else
//volatile PIC24_RTCC_DATE currentDate={0,0,0};
//volatile PIC24_RTCC_TIME currentTime={0,0,0};
#endif
const BYTE dayOfMonth[12]={31,28,31,30,31,30,31,31,30,31,30,31};
volatile DWORD milliseconds;


#define TickGet() (tick10)

/** PRIVATE PROTOTYPES *********************************************/
static void InitializeSystem(void);
int UserInit(void);
#ifdef USA_USB
void USBDeviceTasks(void);
void ProcessIO(void);
void sendEP1(void);
void USBCBSendResume(void);
#endif


/** DECLARATIONS ***************************************************/

void bumpClock(void) {
	BYTE i;

	while(milliseconds >= 1000) {
		milliseconds -= 1000;
		configParms.currentTime.sec++;
		if(configParms.currentTime.sec >= 60) {
			configParms.currentTime.sec=0;
			configParms.currentTime.min++;
			if(configParms.currentTime.min >= 60) {
				configParms.currentTime.min=0;
				configParms.currentTime.hour++;
				if(configParms.currentTime.hour >= 24) {
					configParms.currentTime.hour=0;
					configParms.currentDate.mday++;
					i=dayOfMonth[configParms.currentDate.mon-1];
					if((i==28) && !(configParms.currentDate.year % 4))
						i++;
					if(configParms.currentDate.mday > i) {		// (rimangono i secoli...)
						configParms.currentDate.mday=0;
						configParms.currentDate.mon++;
						if(configParms.currentDate.mon >= 12) {		// 
							configParms.currentDate.mon=1;
							configParms.currentDate.year++;
							}
						}
					}
				}
			} 
		} 
	}


/********************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *******************************************************************/
#if defined(__18CXX)
void main(void)
#else
int main(void)
#endif
{   
//  unsigned int i;

  InitializeSystem();

/*	TRISA=0;
	while(1) {
		LATA ^= 0xffff;
		ClrWdt();
		}
*/


	UserInit();

#ifdef USA_USB
  #if defined(USB_INTERRUPT)
//    USBDeviceAttach();
  #endif
#endif


  while(1) {

		ClrWdt();

#ifdef USA_USB
    #if defined(USB_POLLING)
// Check bus status and service USB interrupts.
#ifndef __DEBUG
  	USBDeviceTasks(); // Interrupt or polling method.  If using polling, must call
#endif
    				  // this function periodically.  This function will take care
    				  // of processing and responding to SETUP transactions 
    				  // (such as during the enumeration process when you first
    				  // plug in).  USB hosts require that USB devices should accept
    				  // and process SETUP packets in a timely fashion.  Therefore,
    				  // when using polling, this function should be called 
    				  // regularly (such as once every 1.8ms or faster** [see 
    				  // inline code comments in usb_device.c for explanation when
    				  // "or faster" applies])  In most cases, the USBDeviceTasks() 
    				  // function does not take very long to execute (ex: <100 
    				  // instruction cycles) before it returns.
    #endif
				  

  #if defined(USB_INTERRUPT)
    if(USB_BUS_SENSE && (USBGetDeviceState() == DETACHED_STATE)) { 
      USBDeviceAttach(); 
    	} 
    else    // ADDED BY ANGUEL 
    if(!USB_BUS_SENSE && (USBGetDeviceState() != DETACHED_STATE)) { 
      USBDeviceDetach(); 
//			USBDeviceState= DETACHED_STATE;
    	} 
  #endif
#endif


// Application-specific tasks.
// Application related code may be added here, or in the ProcessIO() function.
#ifdef USA_USB
    ProcessIO();        // 

    //if we are in the enumeration process then we don't want to start initializing
    // the FAT library.  It takes too 
    if((USBDeviceState < CONFIGURED_STATE) && (USBDeviceState > ATTACHED_STATE)) {
//      continue; no...
    	}
#endif

//mLED_1^=1;

	UserTasks();

  } //end while
    
  
	} //end main

/********************************************************************
 * Function:        static void InitializeSystem(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        InitializeSystem is a centralize initialization
 *                  routine. All required USB initialization routines
 *                  are called from here.
 *
 *                  User application initialization routine should
 *                  also be called from here.                  
 *
 * Note:            None
 *******************************************************************/
static void InitializeSystem(void) {

	ClrWdt();

#if !defined(__XC8)
	StatusReset();

	OSCCONbits.IRCF0=0;		// 8MHz con l'interno!
	OSCCONbits.IRCF1=1;
	OSCCONbits.IRCF2=1;
	OSCTUNE = 0b00000000;			// provo a scendere verso 6MHz per USB :D ma fa il +-3.2% al max...
//(PLL va solo se 8MHz...)
#else
  OSCTUNE = 0;
  OSCCON = 0xFC;          //16MHz HFINTOSC with 3x PLL enabled (48MHz operation)
  ACTCON = 0x90;          //Enable active clock tuning with USB
#endif

	CM1CON0=0b00000000;
	CM2CON0=0b00000000;
#if defined(__XC8)
	ANSELA=0b00000000;
	ANSELC=0b00000000;
	ANSELAbits.ANSA4=1;
#else
	ANSEL=0b00000000;
	ANSELH=0b00000000;
	ANSELbits.ANS3=1;
#endif


	TRISA = 0b00011011;		// led; ev. AN0 o AN3 = RA4; pulsanti;
	TRISB = 0b00000000;		// catodi; 
	TRISC = 0b00000000;		// display; OLed-I2C; 
//	ODCA  = 0b0000000010000000;		// open collector

  //Set the push button input
  mInitAllSwitches();
  //Set up the LED as output
  mInitAllLEDs();

	LATA = 0b00000000;		// 
	LATB = 0b00000000;		// ; 
	LATC = 0b00000000;		// 

	WPUA=0b00001011;			// usb cmq no...
	WPUB=0b00000000;

// https://forum.microchip.com/s/topic/a5C3l000000MCLJEA4/t277505 porcodio RA0 RA1
#ifndef __XC8
	IOCA= 0b00000011;
#endif


#if defined(__XC8)
  APFCON=0;     // no remapping
#endif

  
#ifdef USA_USB
//	The USB specifications require that USB peripheral devices must never source
//	current onto the Vbus pin.  Additionally, USB peripherals should not source
//	current on D+ or D- when the host/hub is not actively powering the Vbus line.
//	When designing a self powered (as opposed to bus powered) USB peripheral
//	device, the firmware should make sure not to turn on the USB module and D+
//	or D- pull up resistor unless Vbus is actively powered.  Therefore, the
//	firmware needs some means to detect when Vbus is being powered by the host.
//	A 5V tolerant I/O pin can be connected to Vbus (through a resistor), and
// 	can be used to detect when Vbus is high (host actively powering), or low
//	(host is shut down or otherwise not supplying power).  The USB firmware
// 	can then periodically poll this I/O pin to know when it is okay to turn on
//	the USB module/D+/D- pull up resistor.  When designing a purely bus powered
//	peripheral device, it is not possible to source current on D+ or D- when the
//	host is not actively providing power on Vbus. Therefore, implementing this
//	bus sense feature is optional.  This firmware can be made to use this bus
//	sense feature by making sure "USE_USB_BUS_SENSE_IO" has been defined in the
//	HardwareProfile.h file.    
    #if defined(USE_USB_BUS_SENSE_IO)
    tris_usb_bus_sense = INPUT_PIN; // See HardwareProfile.h
    #endif
    
//	If the host PC sends a GetStatus (device) request, the firmware must respond
//	and let the host know if the USB peripheral device is currently bus powered
//	or self powered.  See chapter 9 in the official USB specifications for details
//	regarding this request.  If the peripheral device is capable of being both
//	self and bus powered, it should not return a hard coded value for this request.
//	Instead, firmware should check if it is currently self or bus powered, and
//	respond accordingly.  If the hardware has been configured like demonstrated
//	on the PICDEM FS USB Demo Board, an I/O pin can be polled to determine the
//	currently selected power source.  On the PICDEM FS USB Demo Board, "RA2" 
//	is used for	this purpose.  If using this feature, make sure "USE_SELF_POWER_SENSE_IO"
//	has been defined in HardwareProfile.h, and that an appropriate I/O pin has been mapped
//	to it in HardwareProfile.h.
    #if defined(USE_SELF_POWER_SENSE_IO)
    tris_self_power = INPUT_PIN;	// See HardwareProfile.h
    #endif
#endif


#if !defined(__XC8)
	RCONbits.IPEN=1;				// interrupt in Modalita' Avanzata!
#endif

#if defined(__XC8)
//	T0CON=0b00000011;								//
  OPTION_REG=0b00000100;    // pullup; prescaler 1:32
	T1CON=0b00110001;								// prescaler 1:8
	INTCONbits.T0IE = 1;
	PIE1bits.TMR1IE = 1;
	T3CON=fare 0b00110001;								// prescaler 1:4
#else
	OpenTimer0(TIMER_INT_ON & T0_8BIT & T0_SOURCE_INT & T0_PS_1_32);
										// (la frequenza di TMR0 e' 48/4MHz, divido per 32 )

	OpenTimer1(TIMER_INT_ON & T1_16BIT_RW & T1_SOURCE_INT & T1_PS_1_8 & T1_OSC1EN_OFF & T1_SYNC_EXT_OFF );
#endif

#if !defined(__XC8)
	INTCON2bits.TMR0IP=1;			//high pty Timer 0
	IPR1bits.TMR1IP=0;				//low pty Timer 1
#endif

	ClrWdt();
	

warm_reset:
	INTCONbits.GIE = 1;			// attiva interrupt globali / high pty
#if !defined(__XC8)
	INTCONbits.PEIE = 1;			// attiva low pty interrupt 
#endif

//	WriteTimer0(TMR0BASE);					// inizializzo TMR0
			//WRITETIMER0() dovrebbe essere la macro!

#if defined(__XC8)
  TMR1H=TMR1BASE/256;
	TMR1L=TMR1BASE & 255;
#else
	WriteTimer1(TMR1BASE);					// inizializzo TMR1
#endif


//  CM3CON=0;		// comparatori off (Rxpin) ?? boh


// buzzer 4KHz QUA in software essendo pin led/buzzer
#if !defined(__XC8)
#endif
//	T2CON=0b00000011;								// set prescaler T2
//	PR2=BEEP_STD_FREQ;


#if defined(__XC8)
	ADCON0=0b00001101;	//		; AN3 RA0 ; ON
	ADCON1=0b10100000;	//		; Right justify; FOsc/32; VRef +/-
	ADCON2=0b00000000;	//		; no autoconversion
#else
//	OpenADC(ADC_FOSC_32 & ADC_LEFT_JUST & ADC_12_TAD, ADC_CH3 & ADC_INT_OFF & ADC_VREFPLUS_VDD & ADC_VREFMINUS_VSS,
//	);
	// VERIFICA su questo PIC! 
	ADCON2=0b10001010;	//		; Right justify; 12 Tad, FOsc/32
	ADCON1=0b00000000;	//		; VRef +/-
	ADCON0=0b00001101;	//		; AN3 RA4 ; ON
#endif


#ifdef USA_USB
#ifndef __DEBUG
  USBDeviceInit();
#endif
#endif

	} //end InitializeSystem

int UserInit(void) {
	int i;



	ClrWdt();
//mLED_1=1;
//mLED_2=1;

	if(!sw1 && !sw2) {
//				// tanto per :)
		showText("RRR");
		Delay_S_(10);
		goto resetta;
		}

	{
	char *p=(char *)&configParms.currentTime;
	BYTE i;
	for(i=0; i<sizeof(configParms.currentTime); i++) {
		*p=EEleggi(p);
		p++;
		}
	p=(char *)&configParms.currentDate;
	for(i=0; i<sizeof(configParms.currentDate); i++) {
		*p=EEleggi(p);
		p++;
		}
	configParms.voltRange=EEleggi(&configParms.voltRange);
	}

	if(configParms.currentTime.sec==255) {
resetta:
		configParms.currentTime.sec=0;
		configParms.currentTime.min=0;
		configParms.currentTime.hour=12;
		configParms.currentDate.mday=29;
		configParms.currentDate.mon=11;
		configParms.currentDate.year=23;
		configParms.voltRange=1;
		configParms.clock_correction=0;

#if !defined(__XC8)
		EEscrivi_(&configParms.clock_correction,configParms.clock_correction);
#else
    HEFLASH_writeBlock(0,&configParms,sizeof(struct SAVED_PARAMETERS));
#endif
		}

	configParms.clock_correction=EEleggi(&configParms.clock_correction);

#ifdef USA_USB
  //initialize the variable holding the handle for the last transmission
  USBOutHandle = 0;
  USBInHandle = 0;
#endif

	ClrWdt();
	return 1;
	}

#ifdef USA_USB
void sendEP1(void) {

	USBInHandle = HIDTxPacket(HID_EP,(BYTE*)ToSendDataBuffer,64);
	while(HIDTxHandleBusy(USBInHandle))	{	// QUA DOPO! (xch� il buffer � usato "live")
	ClrWdt();
		#if defined(USB_POLLING)
		USBDeviceTasks()
		#endif
		;
		}
	}
#endif

/********************************************************************
 * Function:        void ProcessIO(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is a place holder for other user
 *                  routines. It is a mixture of both USB and non-USB tasks.
 *
 * Note:            None
 *******************************************************************/
#ifdef USA_USB
void ProcessIO(void) { 
	int i;  

	ClrWdt();
//	mLED_1_Toggle(); 

  // User Application USB tasks
  if((USBDeviceState < CONFIGURED_STATE) || (USBSuspendControl==1)) {
		USBOn=FALSE;
		return;
		}

	USBOn=TRUE;
	if(!HIDRxHandleBusy(USBOutHandle)) {   
	  switch(ReceivedDataBuffer[1]) {			// ReceivedDataBuffer[0] contiene 2 = OUTPUT_REPORT, se ESPLICITAMENTE DEFINITO

			case CMD_NOP:
				break;
			
			case CMD_GETID:						// restituisce ID ASCII ()
				i=0;						// puntatore alla stringa Copyright
			
				do {
					prepOutBuffer(CMD_GETID);					// indicatore
					memcpy((void *)(ToSendDataBuffer+2),(void *)(((char *)CopyrString)+i),HID_INT_IN_EP_SIZE-2);
					sendEP1();
					i+=HID_INT_IN_EP_SIZE-2;
					} while(!memchr((void *)(ToSendDataBuffer+2),0,HID_INT_IN_EP_SIZE-2));
			
				break;
			
			case CMD_GETCAPABILITIES:								// restituisce config. 
				prepOutBuffer(CMD_GETCAPABILITIES);					// indicatore
				ToSendDataBuffer[2]=VERNUMH;
				ToSendDataBuffer[3]=VERNUML;

				sendEP1();
				break;
			
			case CMD_GETCONFIG:											// restituisce config. 
				prepOutBuffer(CMD_GETCONFIG);					// indicatore
				ToSendDataBuffer[14]=LOBYTE(clockTune);
				ToSendDataBuffer[15]=HIBYTE(clockTune);

				ToSendDataBuffer[8]=Buzzer;

				ToSendDataBuffer[26]=configParms.voltRange;

				sendEP1();
				break;
			
			case CMD_GETSTATUS:											
				prepOutBuffer(CMD_GETSTATUS);					// indicatore
				ToSendDataBuffer[2]=0;
				ToSendDataBuffer[3]=0;
				ToSendDataBuffer[5]=USBOn;
//				ToSendDataBuffer[6]=batteryState;
				ToSendDataBuffer[6]=(sw1 ? 0 : 1) | (sw2 ? 0 : 2) | (sw3 ? 0 : 4);

				ToSendDataBuffer[13]=InAlert;

				ToSendDataBuffer[24]=LOBYTE(Voltage);
				ToSendDataBuffer[25]=HIBYTE(Voltage);

				sendEP1();
				break;

			case CMD_SETSTATUS:											// imposta
				prepOutBuffer(CMD_SETSTATUS);					// indicatore
//				USBOn=ReceivedDataBuffer[5];		 :)

// tiemrcount??
//				configParms.timerUnit=ReceivedDataBuffer[26];
//				configParms.timerMode=ReceivedDataBuffer[27];
//				configParms.timerBoot=ReceivedDataBuffer[28];

				sendEP1();
				break;

			case CMD_SETCONFIG:											// imposta
				prepOutBuffer(CMD_SETCONFIG);					// indicatore
//				USBOn=ReceivedDataBuffer[5];		 :)
				CelsiusFahrenheit=ReceivedDataBuffer[5];
				Buzzer=ReceivedDataBuffer[6];
				clockTune=MAKEWORD(ReceivedDataBuffer[14],ReceivedDataBuffer[15]);		//SALVARE in FLASH??

				sendEP1();
				break;


			case CMD_TEST:							// test: accendo/spengo led in base a b0
				mLED_1=ReceivedDataBuffer[2] & 1;
				break;

			case CMD_BEEP:
				prepOutBuffer(CMD_BEEP);
				Beep();			
				sendEP1();
				break;

			case CMD_DISPLAY:
				prepOutBuffer(CMD_DISPLAY);
				if(ReceivedDataBuffer[2] & 1) {

					}

				if(ReceivedDataBuffer[2] & 0x80) {
					}
				else {
					}
			
				sendEP1();
				break;

			case CMD_WRITEEEPROM:					// programmazione EEprom (blocchi SEMPRE da 16 BYTE, con USB nuovo/2005):
				prepOutBuffer(CMD_WRITEEEPROM);			// indicatore
//				memcpy((void *)I2CBuffer,(void *)(ReceivedDataBuffer+5),16);

				if(!(ReceivedDataBuffer[2] & 0x80)) {			// addr a 8 opp. 16? (b6:0 = # BYTEs)
					// lo_addr 
//					I2CWritePage(ReceivedDataBuffer[3],ReceivedDataBuffer[2] & 0x1f);					// gestire ev. errore?
					}

				else {
					// lo_addr & hi_addr
//					I2CWritePage16(MAKEWORD(ReceivedDataBuffer[3],ReceivedDataBuffer[4]),ReceivedDataBuffer[2] & 0x1f);				// gestire ev. errore?
					}
				sendEP1();
				break;

			case CMD_READEEPROM:			// lettura EEprom: 2 BYTE lo/hi addr. (blocchi SEMPRE da 16 BYTE, con USB nuovo/2005):
				prepOutBuffer(CMD_READEEPROM);			// indicatore

				if(!(ReceivedDataBuffer[2] & 0x80)) {			// addr a 8 opp. 16? (b6:0 = # BYTEs)
					// lo_addr
//					I2CRead8Seq(ReceivedDataBuffer[3],ReceivedDataBuffer[2] & 0x1f);
					}
				else {
//					I2CRead16Seq(MAKEWORD(ReceivedDataBuffer[3],ReceivedDataBuffer[4]),ReceivedDataBuffer[2] & 0x1f);			// lo_addr
					}

//				memcpy((void *)ToSendDataBuffer+2,(void *)I2CBuffer,16);

				sendEP1();
				break;

			case CMD_READRTC:			// lettura RTC
				prepOutBuffer(CMD_READRTC);			// indicatore

  			ToSendDataBuffer[3]=configParms.currentDate.mday;		// UNIFORMARE CON SKYUSB!
		    ToSendDataBuffer[4]=configParms.currentDate.mon;
		    ToSendDataBuffer[5]=configParms.currentDate.year;
		    ToSendDataBuffer[6]=configParms.currentTime.hour;
		    ToSendDataBuffer[7]=configParms.currentTime.min;
		    ToSendDataBuffer[8]=configParms.currentTime.sec;
		 
				sendEP1();
				break;

			case CMD_WRITERTC:					// programmazione RTC 
				prepOutBuffer(CMD_WRITERTC);			// indicatore

				i=ReceivedDataBuffer[3];		// UNIFORMARE CON SKYUSB e GSMDEV!
		    configParms.currentDate.mday=i;
				i=ReceivedDataBuffer[4];
				configParms.currentDate.mon=i;
				i=ReceivedDataBuffer[5];
				configParms.currentDate.year=i;			// -2000
				i=ReceivedDataBuffer[6];
	  	  configParms.currentTime.hour=i;
				i=ReceivedDataBuffer[7];
				configParms.currentTime.min=i;
				i=ReceivedDataBuffer[8];
				configParms.currentTime.sec=i;
	

				sendEP1();
				break;

			case CMD_RESET_IF:						// RESET (occhio a Enum. USB)
				Reset();
				break;	
						
			case CMD_DEBUG_ECHO:						// ECHO
				prepOutBuffer(CMD_DEBUG_ECHO);
				memcpy((void *)(ToSendDataBuffer+2),(void *)(ReceivedDataBuffer+2),
			//						min(HID_INT_IN_EP_SIZE,HID_INT_OUT_EP_SIZE)-2);
					HID_INT_OUT_EP_SIZE-2);
			
				sendEP1();
				break;
			}
	  USBOutHandle = HIDRxPacket(HID_EP,(BYTE*)&ReceivedDataBuffer,64);
 		}

	}	//end ProcessIO


void prepOutBuffer(BYTE t) {					// entra W=IDENTificatore

	clearOutBuffer();
	ToSendDataBuffer[0]=1 /*SKYNET_REPORT_ID_INPUT*/;					// dev'essere per forza 1! (ossia ID report di input)
											 													// .. contiene xxx_REPORT_ID_INPUT, se metto esplicitamente un REPORT_ID
	ToSendDataBuffer[1]=t;
	}

void clearOutBuffer(void) {

	memset(ToSendDataBuffer,0,64 /*BUF_SIZE_USB*/);
	}

void prepStatusBuffer(BYTE t) {

	prepOutBuffer(CMD_GETSTATUS);				// indicatore
	ToSendDataBuffer[2]=t;
	}
#endif


#if defined(__XC8)
#define ConvertADC() ADCON0bits.GO=1
#define BusyADC() (ADCON0bits.GO_nDONE)
#define ReadADC() ((((int)ADRESH) << 8 ) | ADRESL)		// usare 10 bit??
#endif
signed int leggi_ana() {
	WORD n;

	ConvertADC();						// inizio conversione AD
	while(BusyADC())
		ClrWdt();
	n=ReadADC();

	return n;
	}


void SetRange(BYTE n) {

	configParms.voltRange=n;		// 
	if(configParms.voltRange>=6)
		configParms.voltRange=0;
	}

void ShowRange(void) {

	switch(configParms.voltRange) {		// ev. altre...
		case 0:
	  	showText("01V");
			break;
		case 1:
	  	showText("5 V");
			break;
		case 2:
	  	showText("10V");
			break;
		case 3:
	  	showText("1HV");		// 100 :)
			break;
		case 4:
	  	showText("1KV");		//
			break;
		case 5:
	  	showText("VAC");
			break;
		}
	}

void UserTasks(void) {
	int i;
	static BYTE inEdit=0,oldSw=7,repeatCounter=0,inEditTime=0,displayDivider=0;
	static BYTE cnt=0,cnt2=0;
	int oldVoltage;
	WORD oldVoltage2;


		switch(configParms.voltRange) {			// 
			case 5:		// AC pi� veloce (...)
				Voltage=leggi_ana();
				break;
			default:
				break;
			}

	if(second_10) {			// 100 mSec 
#warning + rapido se timerCount pi� rapido?? anche per rele'
		second_10=0;



		cnt++;
		cnt2++;

	 	mLED_1 = TickGet() & 2 ? 1 : 0; //


		if(firstPassDisplay) {			// lascia Splash per un po'..
			firstPassDisplay--;
			if(firstPassDisplay>16) {
				showText("---");
				Beep();			
				}
			else if(firstPassDisplay>8) {
#ifdef USA_USB
				showChar(CopyrString[33],2,0);
				showChar(CopyrString[31],1,1);
				showChar(CopyrString[30],0,0);
#else
				showChar(CopyrString[25],2,0);
				showChar(CopyrString[23],1,1);
				showChar(CopyrString[22],0,0);
#endif
				}
			else {
#ifdef USA_USB
				showChar(CopyrString[37],2,1);
				showChar(CopyrString[36],1,0);
				showChar(CopyrString[34],0,1);
#else
				showChar(CopyrString[29],2,1);
				showChar(CopyrString[28],1,0);
				showChar(CopyrString[26],0,1);
#endif
				}
			MenuMode=MENU_VOLT;
			return;
			}
		else {
			ShowRange();
			Delay_S_(7);
			}

//		cnt+=10;			// 


#ifndef USA_USB
		if(USBOn || !sw1 || !sw2 || !sw3) {
	ClrWdt();
			inEditTime=80;			// 8 sec per timeout
			}
#endif


		switch(configParms.voltRange) {			// 
			case 5:		// 
				break;
			default:
				Voltage=leggi_ana();
				break;
			}
		switch(configParms.voltRange) {			// aggiungere 
			case 0:		// 0..0.1
				m_Rele=0;
				Voltage=(((DWORD)Voltage)*100)/1024;
				break;
			case 1:		// 0..5
				m_Rele=0;
				Voltage=(((DWORD)Voltage)*500)/1024;
				break;
			case 2:		// 0..10
				m_Rele=0;
				Voltage=(((DWORD)Voltage)*1000)/1024;
				break;
			case 3:		// 0..10 con divisore (...)
				m_Rele=1;
				Voltage=(((DWORD)Voltage)*1000)/1024;
				break;
			case 4:		// 0..10 o 100 autoranging 
				if(m_Rele) {
					if(Voltage>900) {
						m_Rele=1;
						}
					Voltage=(((DWORD)Voltage)*1000)/1024;
					}
				else {
					if(Voltage<85) {
						m_Rele=0;
						}
					Voltage=(((DWORD)Voltage)*1000)/1024;
					}
				break;
			case 5:		// +- 9.9 AC (...)
				m_Rele=0;
				oldVoltage=Voltage-512;
				oldVoltage=abs(oldVoltage);
				oldVoltage2+=oldVoltage;		// 
				oldVoltage2 /= 2;
				Voltage=oldVoltage2;
				break;
			}

#ifndef USA_USB
		if(!sw1) {		// sw1: cambia range, ++ se in edit
			if(!inEdit) {
				if(oldSw & 1) {
					SetRange(++configParms.voltRange);
					ShowRange();
					Delay_S_(3);
					}
				}
			else {		// inEdit
				switch(MenuMode) {
					case MENU_NOEDIT:
						break;
					case MENU_VOLT:
						break;
					case MENU_SECONDI:
					  configParms.currentTime.sec=0;			// direi :)
						break;
					case MENU_MINUTI:
						if(TickGet() & 2) {
						  configParms.currentTime.min++;
						  if(configParms.currentTime.min>=60)
							  configParms.currentTime.min=0;
							}
						break;
					case MENU_ORA:
						if(TickGet() & 2) {
						  configParms.currentTime.hour++;
						  if(configParms.currentTime.hour>=24)
							  configParms.currentTime.hour=0;
							}
						break;
					case MENU_GIORNO:
						if(TickGet() & 2) {
						  configParms.currentDate.mday++;
							i=dayOfMonth[configParms.currentDate.mon-1];
							if((i==28) && !(configParms.currentDate.year % 4))
								i++;
							if(configParms.currentDate.mday > i) 		// (rimangono i secoli...)
							  configParms.currentDate.mday=1;
							}
						break;
					case MENU_MESE:
						if(TickGet() & 2) {
						  configParms.currentDate.mon++;
						  if(configParms.currentDate.mon>=12)
							  configParms.currentDate.mon=1;
							}
						break;
					case MENU_ANNO:
						if(TickGet() & 2) {
						  configParms.currentDate.year++;
						  if(configParms.currentDate.year>=100)
							  configParms.currentDate.year=0;
							}
						break;
					case MENU_VOLTRANGE:
						if(oldSw & 1) {
							SetRange(++configParms.voltRange);
							}
						break;
					case MENU_MAX:
						break;
					}
				}
			}		//sw1
		else {
			}
fine_sw1:
#else
		if(!sw3) {
			repeatCounter++;
			if((oldSw & 4) || (repeatCounter>5)) {		// x gestire click prolungato
				MenuMode++;
				if(MenuMode==MENU_MAX) {
					MenuMode=MENU_TIMER;
					if(inEdit) {
						inEdit=0;
#ifndef USA_USB
  					goto salva;
#endif
						}
					}
				}
			}		//sw3 qua
		else {
			repeatCounter=0;
			}
				goto fine_sw3;
#endif

#ifndef USA_USB
		if(!sw2) {			// sw2: cambia schermata se noedit , -- se in edit
			if(!inEdit) {
				if(oldSw & 2) {
					MenuMode++;
					if(MenuMode==MENU_MAX)
						MenuMode=MENU_VOLT;
					}
				}
			else {		// inEdit
				switch(MenuMode) {
					case MENU_NOEDIT:
						break;
					case MENU_VOLT:
						break;
					case MENU_SECONDI:
					  configParms.currentTime.sec=0;			// direi :)
						break;
					case MENU_MINUTI:
						if(TickGet() & 2) {
						  if(configParms.currentTime.min>0)
						  	configParms.currentTime.min--;
							else
							  configParms.currentTime.min=59;
							}
						break;
					case MENU_ORA:
						if(TickGet() & 2) {
						  if(configParms.currentTime.hour>0)
							  configParms.currentTime.hour--;
							else
							  configParms.currentTime.hour=23;
							}
						break;
					case MENU_GIORNO:
						if(TickGet() & 2) {
						  if(configParms.currentDate.mday>1)
							  configParms.currentDate.mday--;
							else
							  configParms.currentDate.mday=30;		// qua me ne fotto :D
							}
						break;
					case MENU_MESE:
						if(TickGet() & 2) {
						  if(configParms.currentDate.mon>1)
						  	configParms.currentDate.mon--;
							else
							  configParms.currentDate.mon=12;
							}
						break;
					case MENU_ANNO:
						if(TickGet() & 2) {
						  if(configParms.currentDate.year>0)
							  configParms.currentDate.year--;
// idem!
							}
						break;
					case MENU_VOLTRANGE:
						if(oldSw & 2) {
							SetRange(--configParms.voltRange);
							}
						break;
					case MENU_MAX:
						break;
					}
				}		// inEdit

			}		// sw2
fine_sw2:

		if(!sw3) {			// sw3: lungo entra edit poi corto si muove tra i campi
			if(!inEdit) {
				if(repeatCounter>4) {		// x gestire click prolungato
					inEdit=1;
					MenuMode=MENU_VOLTRANGE;
					}
				}
// else?? boh no						inEdit=0;
			else {
				if(oldSw & 4) {
					MenuMode++;
					if(MenuMode==MENU_MAX) {
						MenuMode=MENU_VOLT;
salva:
						{
#if !defined(__XC8)
						char *p=(char *)&configParms.currentTime;
						BYTE i;
					  showText("SSS");
						for(i=0; i<sizeof(configParms.currentTime); i++) {
							EEscrivi_(p,*p);
							p++;
							}
						p=(char *)&configParms.currentDate;
						for(i=0; i<sizeof(configParms.currentDate); i++) {
							EEscrivi_(p,*p);
							p++;
							}
						EEscrivi_(&configParms.voltRange,configParms.voltRange);
#else
	          HEFLASH_writeBlock(0,&configParms,sizeof(struct SAVED_PARAMETERS));
#endif
						if(Buzzer)
							Beep();
						else
							Delay_S_(10);
						MenuMode=MENU_VOLT;
						inEdit=0;
						}
						}
					}
				}		//sw3
			repeatCounter++;
			}
		else {
			repeatCounter=0;
			}		//sw3
fine_sw3:
/*		if(!sw3)
			dim=1;		// tanto per prova!!
//			Beep();		// tanto per prova!!
		else
			dim=0;*/
  

		switch(MenuMode) {
			case MENU_NOEDIT:
				break;
			case MENU_VOLT:
				displayDivider++;
				if(displayDivider>=3) {		// un pelo + lento
					displayDivider=0;
					switch(configParms.voltRange) {			// aggiungere 
						case 0:
				  		showNumbers(Voltage,0);
							break;
						case 1:
				  		showNumbers(Voltage,2);
							break;
						case 2:
				  		showNumbers(Voltage,1);
							break;
						case 3:
				  		showNumbers(Voltage,0);
							break;
						case 4:
				  		showNumbers(Voltage,0);
							break;
						case 5:
				  		showNumbers((((int)Voltage)-512)/52,1);
							break;
						}
					}
				break;
			case MENU_ORA:
			  showChar('0'+(configParms.currentTime.hour % 10),2,0);
			  showChar('0'+(configParms.currentTime.hour / 10),1,0);
			  showChar('O',0,inEdit); 
				break;
			case MENU_MINUTI:
			  showChar('0'+(configParms.currentTime.min % 10),2,0);
			  showChar('0'+(configParms.currentTime.min / 10),1,0);
			  showChar('M',0,inEdit);
				break;
			case MENU_SECONDI:
			  showChar('0'+(configParms.currentTime.sec % 10),2,0);
			  showChar('0'+(configParms.currentTime.sec / 10),1,0);
			  showChar('S',0,inEdit);
				break;
			case MENU_GIORNO:
			  showChar('0'+(configParms.currentDate.mday % 10),2,0);
			  showChar('0'+(configParms.currentDate.mday / 10),1,0);
			  showChar('G',0,inEdit);
				break;
			case MENU_MESE:
			  showChar('0'+(configParms.currentDate.mon % 10),2,0);
			  showChar('0'+(configParms.currentDate.mon / 10),1,0);
			  showChar('N',0,inEdit);			// sembra m minuscola :)
				break;
			case MENU_ANNO:
			  showChar('0'+(configParms.currentDate.year % 10),2,0);
			  showChar('0'+(configParms.currentDate.year / 10),1,0);
			  showChar('A',0,inEdit);
				break;
			case MENU_VOLTRANGE:
				ShowRange();
				if(inEdit)
					Displays[0] |= 0b10000000;
				break;
			}		//MenuMode

		if(inEditTime) {
			if(!--inEditTime) {
				MenuMode=MENU_VOLT;
				if(inEdit) {
					inEdit=0;
#ifndef USA_USB
					goto salva;
#endif
					}
				}
			}


		if(cnt >= 10   /*150*/) {			// 2 secondi QUA ...
			cnt=0;



#ifdef USA_USB
			if(USBOn) {

				prepOutBuffer(CMD_GETLOG);
			  strcpy((char *)&ToSendDataBuffer[2],buffer);
			
				sendEP1();
				}
#endif

//			}		//1 secondi


			}		// cnt2 > 600 (1 minuto)

		oldSw=(sw1 ? 1 : 0) | (sw2 ? 2 : 0) | (sw3 ? 4 : 0);

		if(!USBOn) {
//		__delay_ms(200);
//			inEdit=0;
//			oldSw=7;
	ClrWdt();


// BOH qua...?
#ifndef USA_USB
			if(!sw1 /* || !sw2*/) {
				oldSw=(sw1 ? 1 : 0) | (sw2 ? 2 : 0) | (sw3 ? 4 : 0);
				}
			else {
	//		ClrWdt();
//				milliseconds+=8192+clockTune; 	//tarato su wdt..
//				bumpClock();
//				second_10=1;		// 
//				cnt+=16;					// 8 sec circa (per mis. temperatura
//				cnt2+=16;					// 8 sec circa (per SDcard
				}
#endif
			}

		}		// second_1

	if(USBOn) {
		ClrWdt();
		}

	}


void Beep(void) {
	int n=2000;		// 1/2 sec

	while(n--) {
		mLED_1_Toggle();
		__delay_us(57 /*125*/);		// 4KHz
		}	
	}



BYTE to_bcd(BYTE n) {
	
	return (n % 10) | ((n / 10) << 4);
	}

BYTE from_bcd(BYTE n) {
	
	return (n & 15) + (10*(n >> 4));
	}


// ---------------------------------------------------------------------------------------
#if !defined(__XC8)
void EEscrivi_(SHORTPTR addr,BYTE n) {		// usare void * ?

	EEADR = (BYTE)addr;
	EEDATA=n;

	EECON1bits.EEPGD=0;		// Point to Data Memory
	EECON1bits.CFGS=0;		// Access EEPROM
	EECON1bits.WREN=1;

	INTCONbits.GIE = 0;			// disattiva interrupt globali... e USB?
	EECON2=0x55;		 // Write 55h
	EECON2=0xAA;		 // Write AAh
	EECON1bits.WR=1;									// abilita write.
	INTCONbits.GIE = 1;			// attiva interrupt globali
	do {
		ClrWdt();
		} while(EECON1bits.WR);							// occupato ? 


	EECON1bits.WREN=0;								// disabilita write.
  }

BYTE EEleggi(SHORTPTR addr) {			// usare void * ?

	EEADR=(BYTE)addr;			// Address to read
	EECON1bits.EEPGD=0;		// Point to Data Memory
	EECON1bits.CFGS=0;		// Access EEPROM
	EECON1bits.RD=1;		// EE Read
	return EEDATA;				// W = EEDATA
	}

void EEscriviWord(SHORTPTR addr,WORD n) {			// usare void * ?

	EEscrivi_(addr++,n & 0xff);
	n >>= 8;
	EEscrivi_(addr,n & 0xff);
	}

WORD EEleggiWord(SHORTPTR addr) {			// usare void * ?
	WORD n;

	n=EEleggi(addr++);
	n <<= 8;
	n|=EEleggi(addr);

	return n;
	}
#else
void EEscrivi_(SHORTPTR addr,BYTE n) {		// usare void * ?
  
  HEFLASH_writeBlock(addr, &n, 1);    // si potrebbero unire pi� byte per ottimizzare...
#warning NON CREDO FUNZIONI COS� serve un blocco!! tipo configParms
  }
BYTE EEleggi(SHORTPTR addr) {			// usare void * ?

  return HEFLASH_readByte(0,addr);
  }
#endif

// -------------------------------------------------------------------------------------
#if !defined(__XC8)
//Delays W microseconds (includes movlw, call, and return) @ 8 or 48MHz
void __delay_us(BYTE uSec) {

#if defined(USA_USB) // 
	// 3/4 di prologo...
	do {
		Delay1TCY();			// 1
		Delay1TCY();			// 1
		Delay1TCY();			// 1
		Delay1TCY();			// 1
		Nop();						// 1
		Nop();						// 1
		Nop();						// 1
		ClrWdt();						// 1; Clear the WDT
		} while(--uSec);		// 3
// dovrebbero essere 12...
  //return             ; 4
#else
	//uSec/=2;
	do {
//		Delay1TCY();			// 1
//		ClrWdt();						// 1; Clear the WDT
		} while(--uSec);		// 4
// dovrebbero essere 2...
#endif

/*Delay		macro	Time				// fare una cosa simile...

  if (Time==1)
		goto	$ + 1		;2
		goto	$ + 1
		nop						;1
  		exitm
  endif

  if (Time==2)
		goto	$ + 1
		goto	$ + 1
		goto	$ + 1
		goto	$ + 1
		goto	$ + 1
  		exitm
  endif

	movlw	Time
	call	Delay_uS

	endm*/

	}




void Delay_S_(BYTE n) {				// circa n*100mSec

	do {
	  Delay_mS(100);
		} while(--n);
	}


void __delay_ms(BYTE n) {				// circa n ms

	do {
		Delay_uS(250);
		Delay_uS(250);
#if defined(USA_USB) // @8MHz � lenta... faccio cos�
		Delay_uS(250);
		Delay_uS(250);
#endif
		} while(--n);
	}

#else

void Delay_S_(BYTE n) {				// circa n*100mSec

	do {
	  __delay_ms(100);
		} while(--n);
	}

#endif


#ifdef USA_USB
// ******************************************************************************************************
// ************** USB Callback Functions ****************************************************************
// ******************************************************************************************************
// The USB firmware stack will call the callback functions USBCBxxx() in response to certain USB related
// events.  For example, if the host PC is powering down, it will stop sending out Start of Frame (SOF)
// packets to your device.  In response to this, all USB devices are supposed to decrease their power
// consumption from the USB Vbus to <2.5mA each.  The USB module detects this condition (which according
// to the USB specifications is 3+ms of no bus activity/SOF packets) and then calls the USBCBSuspend()
// function.  You should modify these callback functions to take appropriate actions for each of these
// conditions.  For example, in the USBCBSuspend(), you may wish to add code that will decrease power
// consumption from Vbus to <2.5mA (such as by clock switching, turning off LEDs, putting the
// microcontroller to sleep, etc.).  Then, in the USBCBWakeFromSuspend() function, you may then wish to
// add code that undoes the power saving things done in the USBCBSuspend() function.

// The USBCBSendResume() function is special, in that the USB stack will not automatically call this
// function.  This function is meant to be called from the application firmware instead.  See the
// additional comments near the function.

/******************************************************************************
 * Function:        void USBCBSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Call back that is invoked when a USB suspend is detected
 *
 * Note:            None
 *****************************************************************************/
void USBCBSuspend(void) {

	//Example power saving code.  Insert appropriate code here for the desired
	//application behavior.  If the microcontroller will be put to sleep, a
	//process similar to that shown below may be used:
	
	//ConfigureIOPinsForLowPower();
	//SaveStateOfAllInterruptEnableBits();
	//DisableAllInterruptEnableBits();
	//EnableOnlyTheInterruptsWhichWillBeUsedToWakeTheMicro();	//should enable at least USBActivityIF as a wake source
	//Sleep();
	//RestoreStateOfAllPreviouslySavedInterruptEnableBits();	//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.
	//RestoreIOPinsToNormal();									//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.

	//IMPORTANT NOTE: Do not clear the USBActivityIF (ACTVIF) bit here.  This bit is 
	//cleared inside the usb_device.c file.  Clearing USBActivityIF here will cause 
	//things to not work as intended.	
	

  #if defined(__C30__)
		_IPL=1; //MAL 3/12			*********** con ethernet questa fa piantare tutto!!!
  #endif
	}


/******************************************************************************
 * Function:        void _USB1Interrupt(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the USB interrupt bit is set
 *					In this example the interrupt is only used when the device
 *					goes to sleep when it receives a USB suspend command
 *
 * Note:            None
 *****************************************************************************/
#if 0
void __attribute__ ((interrupt)) _USB1Interrupt(void) {
  #if !defined(self_powered)
      if(U1OTGIRbits.ACTVIF) {
        IEC5bits.USB1IE = 0;
        U1OTGIEbits.ACTVIE = 0;
        IFS5bits.USB1IF = 0;
      
      //USBClearInterruptFlag(USBActivityIFReg,USBActivityIFBitNum);
      USBClearInterruptFlag(USBIdleIFReg,USBIdleIFBitNum);
      //USBSuspendControl = 0;
      }
  #endif
	}
#endif

/******************************************************************************
 * Function:        void USBCBWakeFromSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The host may put USB peripheral devices in low power
 *					suspend mode (by "sending" 3+ms of idle).  Once in suspend
 *					mode, the host may wake the device back up by sending non-
 *					idle state signalling.
 *					
 *					This call back is invoked when a wakeup from USB suspend 
 *					is detected.
 *
 * Note:            None
 *****************************************************************************/
void USBCBWakeFromSuspend(void) {
	
	// If clock switching or other power savings measures were taken when
	// executing the USBCBSuspend() function, now would be a good time to
	// switch back to normal full power run mode conditions.  The host allows
	// a few milliseconds of wakeup time, after which the device must be 
	// fully back to normal, and capable of receiving and processing USB
	// packets.  In order to do this, the USB module must receive proper
	// clocking (IE: 48MHz clock must be available to SIE for full speed USB
	// operation).

	}

/********************************************************************
 Function:        void USBCB_SOF_Handler(void)

 Description:     The USB host sends out a SOF packet to full-speed
                  devices every 1 ms. This interrupt may be useful
                  for isochronous pipes. End designers should
                  implement callback routine as necessary.

 PreCondition:    None
 
 Parameters:      None
 
 Return Values:   None
 
 Remarks:         None
 *******************************************************************/
void USBCB_SOF_Handler(void) {
  // No need to clear UIRbits.SOFIF to 0 here.
  // Callback caller is already doing that.
	}

/*******************************************************************
 * Function:        void USBCBErrorHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The purpose of this callback is mainly for
 *                  debugging during development. Check UEIR to see
 *                  which error causes the interrupt.
 *
 * Note:            None
 *******************************************************************/
void USBCBErrorHandler(void) {
    // No need to clear UEIR to 0 here.
    // Callback caller is already doing that.

	// Typically, user firmware does not need to do anything special
	// if a USB error occurs.  For example, if the host sends an OUT
	// packet to your device, but the packet gets corrupted (ex:
	// because of a bad connection, or the user unplugs the
	// USB cable during the transmission) this will typically set
	// one or more USB error interrupt flags.  Nothing specific
	// needs to be done however, since the SIE will automatically
	// send a "NAK" packet to the host.  In response to this, the
	// host will normally retry to send the packet again, and no
	// data loss occurs.  The system will typically recover
	// automatically, without the need for application firmware
	// intervention.
	
	// Nevertheless, this callback function is provided, such as
	// for debugging purposes.
	}


/*******************************************************************
 * Function:        void USBCBCheckOtherReq(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        When SETUP packets arrive from the host, some
 * 					firmware must process the request and respond
 *					appropriately to fulfill the request.  Some of
 *					the SETUP packets will be for standard
 *					USB "chapter 9" (as in, fulfilling chapter 9 of
 *					the official USB specifications) requests, while
 *					others may be specific to the USB device class
 *					that is being implemented.  For example, a HID
 *					class device needs to be able to respond to
 *					"GET REPORT" type of requests.  This
 *					is not a standard USB chapter 9 request, and 
 *					therefore not handled by usb_device.c.  Instead
 *					this request should be handled by class specific 
 *					firmware, such as that contained in usb_function_hid.c.
 *
 * Note:            None
 *******************************************************************/
void USBCBCheckOtherReq(void) {

  USBCheckHIDRequest();
	}//end


/*******************************************************************
 * Function:        void USBCBStdSetDscHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USBCBStdSetDscHandler() callback function is
 *					called when a SETUP, bRequest: SET_DESCRIPTOR request
 *					arrives.  Typically SET_DESCRIPTOR requests are
 *					not used in most applications, and it is
 *					optional to support this type of request.
 *
 * Note:            None
 *******************************************************************/
void USBCBStdSetDscHandler(void)
{
    // Must claim session ownership if supporting this request
}//end


/*******************************************************************
 * Function:        void USBCBInitEP(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the device becomes
 *                  initialized, which occurs after the host sends a
 * 					SET_CONFIGURATION (wValue not = 0) request.  This 
 *					callback function should initialize the endpoints 
 *					for the device's usage according to the current 
 *					configuration.
 *
 * Note:            None
 *******************************************************************/
void USBCBInitEP(void) {


  //enable the HID endpoint
  USBEnableEndpoint(HID_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
  //Re-arm the OUT endpoint for the next packet
  USBOutHandle = HIDRxPacket(HID_EP,(BYTE*)&ReceivedDataBuffer,64);

	}

/********************************************************************
 * Function:        void USBCBSendResume(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB specifications allow some types of USB
 * 					peripheral devices to wake up a host PC (such
 *					as if it is in a low power suspend to RAM state).
 *					This can be a very useful feature in some
 *					USB applications, such as an Infrared remote
 *					control	receiver.  If a user presses the "power"
 *					button on a remote control, it is nice that the
 *					IR receiver can detect this signalling, and then
 *					send a USB "command" to the PC to wake up.
 *					
 *					The USBCBSendResume() "callback" function is used
 *					to send this special USB signalling which wakes 
 *					up the PC.  This function may be called by
 *					application firmware to wake up the PC.  This
 *					function will only be able to wake up the host if
 *                  all of the below are true:
 *					
 *					1.  The USB driver used on the host PC supports
 *						the remote wakeup capability.
 *					2.  The USB configuration descriptor indicates
 *						the device is remote wakeup capable in the
 *						bmAttributes field.
 *					3.  The USB host PC is currently sleeping,
 *						and has previously sent your device a SET 
 *						FEATURE setup packet which "armed" the
 *						remote wakeup capability.   
 *
 *                  If the host has not armed the device to perform remote wakeup,
 *                  then this function will return without actually performing a
 *                  remote wakeup sequence.  This is the required behavior, 
 *                  as a USB device that has not been armed to perform remote 
 *                  wakeup must not drive remote wakeup signalling onto the bus;
 *                  doing so will cause USB compliance testing failure.
 *                  
 *					This callback should send a RESUME signal that
 *                  has the period of 1-15ms.
 *
 * Note:            This function does nothing and returns quickly, if the USB
 *                  bus and host are not in a suspended condition, or are 
 *                  otherwise not in a remote wakeup ready state.  Therefore, it
 *                  is safe to optionally call this function regularly, ex: 
 *                  anytime application stimulus occurs, as the function will
 *                  have no effect, until the bus really is in a state ready
 *                  to accept remote wakeup. 
 *
 *                  When this function executes, it may perform clock switching,
 *                  depending upon the application specific code in 
 *                  USBCBWakeFromSuspend().  This is needed, since the USB
 *                  bus will no longer be suspended by the time this function
 *                  returns.  Therefore, the USB module will need to be ready
 *                  to receive traffic from the host.
 *
 *                  The modifiable section in this routine may be changed
 *                  to meet the application needs. Current implementation
 *                  temporary blocks other functions from executing for a
 *                  period of ~3-15 ms depending on the core frequency.
 *
 *                  According to USB 2.0 specification section 7.1.7.7,
 *                  "The remote wakeup device must hold the resume signaling
 *                  for at least 1 ms but for no more than 15 ms."
 *                  The idea here is to use a delay counter loop, using a
 *                  common value that would work over a wide range of core
 *                  frequencies.
 *                  That value selected is 1800. See table below:
 *                  ==========================================================
 *                  Core Freq(MHz)      MIP         RESUME Signal Period (ms)
 *                  ==========================================================
 *                      48              12          1.05
 *                       4              1           12.6
 *                  ==========================================================
 *                  * These timing could be incorrect when using code
 *                    optimization or extended instruction mode,
 *                    or when having other interrupts enabled.
 *                    Make sure to verify using the MPLAB SIM's Stopwatch
 *                    and verify the actual signal on an oscilloscope.
 *******************************************************************/
void USBCBSendResume(void) {
  static WORD delay_count;
    
    //First verify that the host has armed us to perform remote wakeup.
    //It does this by sending a SET_FEATURE request to enable remote wakeup,
    //usually just before the host goes to standby mode (note: it will only
    //send this SET_FEATURE request if the configuration descriptor declares
    //the device as remote wakeup capable, AND, if the feature is enabled
    //on the host (ex: on Windows based hosts, in the device manager 
    //properties page for the USB device, power management tab, the 
    //"Allow this device to bring the computer out of standby." checkbox 
    //should be checked).
    if(USBGetRemoteWakeupStatus() == TRUE) {
        //Verify that the USB bus is in fact suspended, before we send
        //remote wakeup signalling.
      if(USBIsBusSuspended() == TRUE) {
        USBMaskInterrupts();
            
        //Clock switch to settings consistent with normal USB operation.
        USBCBWakeFromSuspend();
        USBSuspendControl = 0; 
        USBBusIsSuspended = FALSE;  //So we don't execute this code again, 
                                        //until a new suspend condition is detected.

        //Section 7.1.7.7 of the USB 2.0 specifications indicates a USB
        //device must continuously see 5ms+ of idle on the bus, before it sends
        //remote wakeup signalling.  One way to be certain that this parameter
        //gets met, is to add a 2ms+ blocking delay here (2ms plus at 
        //least 3ms from bus idle to USBIsBusSuspended() == TRUE, yeilds
        //5ms+ total delay since start of idle).
        delay_count = 3600U;        
        do {
          delay_count--;
          } while(delay_count);
            
        //Now drive the resume K-state signalling onto the USB bus.
        USBResumeControl = 1;       // Start RESUME signaling
        delay_count = 1800U;        // Set RESUME line for 1-13 ms
        do {
          delay_count--;
        	} while(delay_count);
        USBResumeControl = 0;       //Finished driving resume signalling

        USBUnmaskInterrupts();
        }
    }
	}


/*******************************************************************
 * Function:        BOOL USER_USB_CALLBACK_EVENT_HANDLER(
 *                        USB_EVENT event, void *pdata, WORD size)
 *
 * PreCondition:    None
 *
 * Input:           USB_EVENT event - the type of event
 *                  void *pdata - pointer to the event data
 *                  WORD size - size of the event data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called from the USB stack to
 *                  notify a user application that a USB event
 *                  occured.  This callback is in interrupt context
 *                  when the USB_INTERRUPT option is selected.
 *
 * Note:            None
 *******************************************************************/
BOOL USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, WORD size) {

  switch(event) {
    case EVENT_TRANSFER:
      //Add application specific callback task or callback function here if desired.
      break;
    case EVENT_SOF:
      USBCB_SOF_Handler();
      break;
    case EVENT_SUSPEND:
      USBCBSuspend();
      break;
    case EVENT_RESUME:
      USBCBWakeFromSuspend();
      break;
    case EVENT_CONFIGURED: 
      USBCBInitEP();
      break;
    case EVENT_SET_DESCRIPTOR:
      USBCBStdSetDscHandler();
      break;
    case EVENT_EP0_REQUEST:
      USBCBCheckOtherReq();
      break;
  	case EVENT_BUS_ERROR:
      USBCBErrorHandler();
      break;
	  case EVENT_TRANSFER_TERMINATED:
	      //Add application specific callback task or callback function here if desired.
	      //The EVENT_TRANSFER_TERMINATED event occurs when the host performs a CLEAR
	      //FEATURE (endpoint halt) request on an application endpoint which was 
	      //previously armed (UOWN was = 1).  Here would be a good place to:
	      //1.  Determine which endpoint the transaction that just got terminated was 
	      //      on, by checking the handle value in the *pdata.
	      //2.  Re-arm the endpoint if desired (typically would be the case for OUT 
	      //      endpoints).
	    break;
	  default:
	    break;
    }

  return TRUE; 
	}

BOOL MyUSBSleepOnSuspend(void) {
  unsigned int U1EIE_save, U1IE_save, U1OTGIE_save,i;
  unsigned char USB1IE_save;


	return FALSE;		//se mai-sleep, NO! (v. anche usb_hal_pic24.c)





  return TRUE;
	}

#else			// serve se non ho 2 progetti diversi, perch� linkata da robba usb...

#if !defined(__XC8)
BOOL USER_USB_CALLBACK_EVENT_HANDLER(int /*USB_EVENT*/ event, void *pdata, WORD size) {
	}
#endif

#endif


/*****************************************************************************
  Function:
	BYTE void uitoa(WORD Value, char * Buffer)			modifiche GD gen 2011!

  Summary:
	Converts an unsigned integer to a decimal string.
	
  Description:
	Converts a 16-bit unsigned integer to a null-terminated decimal string.
	
  Precondition:
	None

  Parameters:
	Value	- The number to be converted
	Buffer	- Pointer in which to store the converted string

  Returns:
  	# of digits written
  ***************************************************************************/
BYTE uitoa(WORD Value, char *Buffer) {
	BYTE i;
	WORD Digit;
	WORD Divisor;
	BOOL Printed = FALSE;
	char *oldBuffer=Buffer;

	if(Value)	{
		for(i=0, Divisor=10000; i<5; i++) {
			Digit = Value/Divisor;
			if(Digit || Printed) {
				*Buffer++ = '0' + Digit;
				Value -= Digit*Divisor;
				Printed = TRUE;
				}
			Divisor /= 10;
			}
		}
	else {
		*Buffer++ = '0';
		}

	*Buffer = 0;

	return Buffer-oldBuffer;
	}			    

void showChar(char n,BYTE pos,BYTE dp) {

	if(n>='0' && n<='9')
		Displays[pos]=table_7seg[n-'0'+1];
	else if(n=='-')
		Displays[pos]=table_7seg[11];
	else if(n>='A' && n<='Z')
		Displays[pos]=table_7seg[n-'A'+11+1];
	else if(n>=58 && n<=63)		// hex
		Displays[pos]=table_7seg[n-58+11+1];
	else if(!n || n==' ')
		Displays[pos]=table_7seg[0];
	if(dp)
		Displays[pos] |= 0b10000000;
	}

void showNumbers(int n,BYTE prec) {
	char myBuf[8];

	myBuf[2]= (n % 10) + '0';
	showChar(myBuf[2],2,0 /*!prec MAI */);
	n /= 10;
	prec--;
	myBuf[1]= (n % 10) + '0';
	showChar(myBuf[1],1,!prec);
	n /= 10;
	prec--;
	if(n<0) {
		showChar('-',0,!prec);
		}
	else {
		myBuf[0]= (n % 10) + '0';
		showChar(myBuf[0] != '0' ? myBuf[0] : 0,0,!prec);
		}
	}

void showText(const rom char *s) {

	showChar(*s++,0,0);
	if(*s) {
		showChar(*s++,1,0);
		showChar(*s,2,0);		// questo � ok cmq!
		}
	else {
		showChar(0,1,0);
		showChar(0,2,0);
		}
	}



// ---------------------------------------------------------------------
#if defined(__XC8)
#define ConvertADC() ADCON0bits.GO=1
#define BusyADC() (ADCON0bits.GO_nDONE)
#define ReadADC() ((((int)ADRESH) << 8 ) | ADRESL)		// usare 10 bit??
#endif
signed int leggi_tempAna() {			// in decimi �C
	WORD n;

	ConvertADC();						// inizio conversione AD
	while(BusyADC())
		ClrWdt();
	n=ReadADC();

  
//  	return n;

  
  
	if(n<400 || n>800)		// segnalo errore se chip guasto (-90..+300)
		return INVALID_READOUT;

//	n= (((DWORD)n*100) +7680L /*-51200*/)/256L;		//1K partitore, 10bit, 1/2Vcc @ 20V
	n= (n-492   -20 )*10;		//1K partitore, 10bit, 1/2Vcc @ 20V
// sembra variare di 2/3�C anzich� 1, il che avrebbe un senso dato il partitore... (ci vorrebbe corrente costante credo)
// partendo da 22�C ambiente, corpo umano=32 e frigo=6, freezer=2
	
//n=ADRESH;

	return n;
	}

#ifdef USA_TC74 
signed int leggi_temp(BYTE q) {			// in decimi �C
	WORD n;

/*
    StartI2C2(); 
    IdleI2C2();
    MasterWriteI2C2(0x90 | (q << 1)); //Send command
    IdleI2C2();
    MasterWriteI2C2(0); //Read X command
    IdleI2C2();
//    StopI2C1();
//    IdleI2C1();
    __delay_us(50);   //xx microsecond delay
    StartI2C2();
    IdleI2C2();
    MasterWriteI2C2(0x91 | (q << 1));  //Read values
    IdleI2C2();
    n = MasterReadI2C2();  //High 8 bits
    AckI2C2();
    IdleI2C2();  
		n <<= 8;    
    n |= MasterReadI2C2();  //Low 4 bits
    NotAckI2C2();
    IdleI2C2();
    StopI2C2();
    IdleI2C2();
*/

	DoI2CStart();
	DoI2CMO(0x90 | (q << 1));		// mando address (FORZO WR)
	DoI2CMO(0);
	DoI2CStart();
	DoI2CMO(0x91 | (q << 1));		// mando address (FORZO RD)
	n=DoI2CMI(0);
	DoI2CStop();

//	if(!n || n==0xffff)		// segnalo errore se chip guasto su TC74 no!
//		return INVALID_READOUT;

	n*=10;

	return n;
  }
#endif


#ifdef USA_MCP9800 
signed int leggi_tempMCP(BYTE q) {
	WORD n;


/*    StartI2C1(); 
    IdleI2C1();
    MasterWriteI2C1(WRITE_ADD); //Send command
    IdleI2C1();
    MasterWriteI2C1(MEAS_X); //Read X command
    IdleI2C1();
    StopI2C1();
    IdleI2C1();
    delay100();   //100 microsecond delay
    StartI2C1();
    IdleI2C1();
    MasterWriteI2C1(READ_ADD);  //Read values
    IdleI2C1();
    bhi = MasterReadI2C1();  //High 8 bits
    AckI2C1();
    IdleI2C1();      
    blow = MasterReadI2C1();  //Low 4 bits
    NotAckI2C1();
    IdleI2C1();
    StopI2C1();
    IdleI2C1();
*/

	DoI2CStart();
	DoI2CMO(0x90 | (q << 1));		// mando address (FORZO WR)
	DoI2CMO(0);
	DoI2CStart();
	DoI2CMO(0x91 | (q << 1));		// mando address (FORZO RD)
	n=DoI2CMI(1);
	n <<= 8;
	n |= DoI2CMI(0);
	DoI2CStop();

	if(!n || n==0xffff)		// segnalo errore se chip guasto
		return INVALID_READOUT;

	n >>= 4;		// @ 12 bit
	n *= 10;
	n /= 16;

	return n;
  }
#endif

#ifdef USA_HDI
WORD leggi_press(BYTE q) {		// v. SGTA
	WORD n;
	long n1;
	BYTE subAddress=0xf0;		// HDI

	DoI2CStart();
//		DoI2CMO(subAddress /* | (0*2) non disponibile qua */);		// mando address (FORZO WR)
//		DoI2CStart();
	DoI2CMO(subAddress | 1 /* | (0*2) non disponibile qua */);		// mando address (FORZO RD)
	n=DoI2CMI(1);
	n <<= 8;
	n |= DoI2CMI(0);
// secondo il datasheet qua ci pu� essere anche una temperatura!! 16 bit...

	DoI2CStop();

	if(!n || n==0xffff)		// segnalo errore se chip guasto
		return INVALID_READOUT;

	n1=(signed int)n-2731  +2 /*tarat.*/;			// v. doc. pag. 6/11
	if(n1<0)
		n1=0;
	n1 *= 5*1000L;
	n1 /= (21845  /* /5 */ /* modello 2 bar!! */);
	n=n1;

	return n;
  }
#endif

#ifdef USA_SHT21 
signed int leggi_tempSHT21(BYTE q) {		// in decimi �C
	WORD n;
	double d;

	OCCHIO a SIGNED!!

	I2CSTART();
	I2CTXByte(0b10000000);		// SHT21
	I2CTXByte(0xE3);					// read T w/hold

	I2CSTART();
	I2CTXByte(0b10000001);		// 

	setClockHigh();			// aspetto conversione

	n=I2CRXByte();
	I2CBITOUT(0);						// to continue transmission
	n <<= 8;
	n |= I2CRX1Byte();
	I2CSTOP();

	n >>= 2;		// ultimi 2 LSB sono "STAT"
	d=0.011072*n;
	d -= 46.8375;
	d += ((-0.000000021233*n)*n);
	n=d*10.0;

	return n;
  }

WORD leggi_umidSHT(BYTE q,signed int temp_comp /*�C*/) {			// in decimi %; v. SGTA per press. compens.
	WORD n;
	double d;

	OCCHIO a SIGNED!!

	I2CSTART();
	I2CTXByte(0b10000000);		// SHT21
	I2CTXByte(0xE5);					// read P w/hold

	I2CSTART();
	I2CTXByte(0b10000001);		// 

	setClockHigh();			// aspetto conversione

	n=I2CRXByte();
	I2CBITOUT(0);						// to continue transmission
	n <<= 8;
	n |= I2CRX1Byte();
	I2CSTOP();

	n >>= 4;		// ultimi 2 LSB sono "STAT" e poi � 12bit
	d=0.038219*n;
	d -= -7.7239;
	d += ((-0.0000036634*n)*n);

	n=d*10.0;

	return n;
  }
#endif

#ifdef USA_VOLT
WORD leggi_tensione(BYTE q) {		// fare...
	WORD n;


	n=0;

	return n;
	}
#endif

