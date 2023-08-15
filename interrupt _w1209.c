#include <stdio.h>

#if defined(__18CXX)
#include <timers.h>
#include <portb.h>
#endif

#include "HardwareProfile.h"
#include "w1209_pic.h"

#include "swi2c.h"


#if defined(__18CXX)
//These are your actual interrupt handling routines.
void YourHighPriorityISRCode(void);
void YourLowPriorityISRCode(void);
#endif



extern volatile BYTE dim;
extern volatile WORD tick10;
extern volatile BYTE second_10;
volatile BYTE divider1s;
extern volatile DWORD milliseconds;
signed char clock_correction=0;

const rom char days_month[2][12] = {
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	};

#if defined(__18CXX)
#ifndef __EXTENDED18__
#pragma udata access ACCESSBANK
#else
#pragma udata gpr1
#define near
#endif
#endif

#ifndef OLED
#if defined(__18CXX)
extern near volatile unsigned char Displays[3];			// 3 display 
#else
extern volatile unsigned char Displays[3];			// 3 display 
#endif
#endif

#if defined(__18CXX)
#pragma udata gpr0
#endif



/** VECTOR REMAPPING ***********************************************/
#if defined(__18CXX)
	//Note: If this project is built while one of the bootloaders has
	//been defined, but then the output hex file is not programmed with
	//the bootloader, addresses 0x08 and 0x18 would end up programmed with 0xFFFF.
	//As a result, if an actual interrupt was enabled and occured, the PC would jump
	//to 0x08 (or 0x18) and would begin executing "0xFFFF" (unprogrammed space).  This
	//executes as nop instructions, but the PC would eventually reach the REMAPPED_RESET_VECTOR_ADDRESS
	//(0x1000 or 0x800, depending upon bootloader), and would execute the "goto _startup".  This
	//would effective reset the application.
	
	//To fix this situation, we should always deliberately place a 
	//"goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS" at address 0x08, and a
	//"goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS" at address 0x18.  When the output
	//hex file of this project is programmed with the bootloader, these sections do not
	//get bootloaded (as they overlap the bootloader space).  If the output hex file is not
	//programmed using the bootloader, then the below goto instructions do get programmed,
	//and the hex file still works like normal.  The below section is only required to fix this
	//scenario.

	//On PIC18 devices, addresses 0x00, 0x08, and 0x18 are used for
	//the reset, high priority interrupt, and low priority interrupt
	//vectors.  However, the current Microchip USB bootloader 
	//examples are intended to occupy addresses 0x00-0x7FF or
	//0x00-0xFFF depending on which bootloader is used.  Therefore,
	//the bootloader code remaps these vectors to new locations
	//as indicated below.  This remapping is only necessary if you
	//wish to program the hex file generated from this project with
	//the USB bootloader.  If no bootloader is used, edit the
	//usb_config.h file and comment out the following defines:
	//#define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER
	//#define PROGRAMMABLE_WITH_USB_LEGACY_CUSTOM_CLASS_BOOTLOADER
	
	#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)
		#define REMAPPED_RESET_VECTOR_ADDRESS			0x1000
		#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x1008
		#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x1018
	#elif defined(PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER)	
		#define REMAPPED_RESET_VECTOR_ADDRESS			0x800
		#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x808
		#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x818
	#else	
		#define REMAPPED_RESET_VECTOR_ADDRESS			0x00
		#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x08
		#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x18
	#endif
	
#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)||defined(PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER)
extern void _startup (void);        // See c018i.c in your C18 compiler dir
#pragma code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS
void _reset (void) {
  _asm goto _startup _endasm
	}
#endif
#pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS
void Remapped_High_ISR(void) {
  _asm 
#ifdef SILICON_REV_A3
	CALL Foo, 1 // store current value of WREG, BSR, STATUS for a second time
	RETFIE 1					// FAST
Foo:
	POP 	// clears return address of Foo call
	goto YourHighPriorityISRCode 
				// insert high priority ISR code here
#else
	goto YourHighPriorityISRCode 
#endif
	_endasm
	}
#pragma code
#pragma code REMAPPED_LOW_INTERRUPT_VECTOR = REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS
void Remapped_Low_ISR(void) {
  _asm 
	goto YourLowPriorityISRCode
	_endasm
	}
#endif	

#if defined(__18CXX)
#pragma code
#endif


/** D E C L A R A T I O N S **************************************************/
/******************************************************************************
 * Function:        void YourHighPriorityISRCode(void)
 * PreCondition:    None
 * Input:
 * Output:
 * Side Effects:
 * Overview:
 *****************************************************************************/
#if defined(__18CXX)
#pragma interrupt YourHighPriorityISRCode nosave=section("MATH_DATA"),section(".tmpdata")
void YourHighPriorityISRCode() {		//
#endif
#if defined(__XC8)
void interrupt YourHighPriorityISRCode() {		//
#endif
// dev'essere 5mS
	static BYTE dividerMux=0b00010000;
	signed char mux_corr;
		
#ifdef USA_USB
  #if defined(USB_INTERRUPT)
    USBDeviceTasks();
  #endif
#endif

	if(INTCONbits.TMR0IE && INTCONbits.TMR0IF) {		// (occhio USB)
// 4.1 mSec 2/8/23 con ottimizzazioni

	
#ifndef OLED
		LATB= (LATB & 0b10001111) | dividerMux;
		switch(dividerMux) {
			case 0b01100000:
				LATC=Displays[0];
//LATD=0b00000001;
				if(dim)
					dividerMux = 0b11111111;
				else
					dividerMux = 0b01010000;
				break;
			case 0b01010000:
				LATC=Displays[1];
//				LATD |= 0b10000000;		// dp fisso per ora!
//LATD=0b00000010;
				if(dim)
					dividerMux = 0b11111111;
				else
					dividerMux = 0b00110000;
				break;
			case 0b00110000:
				LATC=Displays[2];
//LATD=0b00000100;
				if(dim)
					dividerMux = 0b11111111;
				else
					dividerMux = 0b01100000;
				break;

			case 0b01011111:
			case 0b01001111:
			case 0b00101111:
				LATBbits.LATB4=1;
				LATBbits.LATB5=1;
				LATBbits.LATB6=1;
				dividerMux++;
				if(dividerMux > 128)
					dividerMux = 0b01100000;
				break;
			default:
				dividerMux = 0b01100000;
				break;
			}

#define MUX_CORR_STEP 5
		mux_corr=0;		// correggo tempo mux per dare più tempo se c'è più assorbimento!
		if(!(LATC & 1))
			mux_corr+=MUX_CORR_STEP;
		if(!(LATC & 2))
			mux_corr+=MUX_CORR_STEP;
		if(!(LATC & 4))
			mux_corr+=MUX_CORR_STEP;
		if(!(LATC & 8))
			mux_corr+=MUX_CORR_STEP;
		if(!(LATC & 16))
			mux_corr+=MUX_CORR_STEP;
		if(!(LATC & 32))
			mux_corr+=MUX_CORR_STEP;
		if(!(LATC & 64))
			mux_corr+=MUX_CORR_STEP;

//	m_Led2Bit ^= 1; //check timer	
#endif


		INTCONbits.TMR0IF = 0;

//		WriteTimer0(TMR0BASE);					// inizializzo TMR0; no funzioni per overhead
		// WRITETIMER0(0) dovrebbe essere la macro!
//		TMR0H=Tmr0Base / 256;					// reinizializzo TMR0
#if defined(__18CXX)
		TMR0L=TMR0BASE-mux_corr;					// reinizializzo TMR0
#endif
#if defined(__XC8)
		TMR0=TMR0BASE-mux_corr;					// reinizializzo TMR0
#endif
// mah, non sembra fare molto a livello visivo...


	
		}
	
#if defined(__XC8)
	if(PIE1bits.TMR1IE && PIR1bits.TMR1IF) {				// Timer 1 ..  
		PIR1bits.TMR1IF=0;					// clear bit IRQ

//2 mS 27/11/10 @ 44Mhz
//	WriteTimer1(TMR1BASE);					// inizializzo TMR1
// in questo modo evito overhead nell'interrupt... causa chiamata di funzione!
		TMR1H=TMR1BASE/256;
		TMR1L=TMR1BASE & 255;

		//Check which interrupt flag caused the interrupt.
		//Service the interrupt
		//Clear the interrupt flag
		//Etc.


// 100.000 mSec 2/8/23 #leucemiafitch
// 	mLED_1 ^= 1; //check timer	
// dev'essere 10Hz


	tick10++;
	divider1s++;

	if(divider1s == 5 || divider1s == 10)
		second_10=1;					// flag; 500mS per debounce migliore!
	if(divider1s==10) {		// per RealTimeClock
		divider1s=0;
#ifdef USA_SW_RTC 
		milliseconds+=1000;
		bumpClock();
#endif
		}


		PIR1bits.TMR1IF = 0;

		}
#endif
    
	}	//This return will be a "retfie fast", since this is in a #pragma interrupt section 


#if defined(__18CXX)
/******************************************************************************
 * Function:        void low_isr(void)
 * PreCondition:    None
 * Input:
 * Output:
 * Side Effects:
 * Overview:
 *****************************************************************************/
#pragma interruptlow YourLowPriorityISRCode
void YourLowPriorityISRCode()	{

//	if(PIE1bits.TMR1IE && PIR1bits.TMR1IF) {

	if(PIR1bits.TMR1IF) {				// Timer 1 ..  
		PIR1bits.TMR1IF=0;					// clear bit IRQ

//2 mS 27/11/10 @ 44Mhz
//	WriteTimer1(TMR1BASE);					// inizializzo TMR1
// in questo modo evito overhead nell'interrupt... causa chiamata di funzione!
		TMR1H=TMR1BASE/256;
		TMR1L=TMR1BASE & 255;

		//Check which interrupt flag caused the interrupt.
		//Service the interrupt
		//Clear the interrupt flag
		//Etc.


// 100.000 mSec 2/8/23 #leucemiafitch
// 	mLED_1 ^= 1; //check timer	
// dev'essere 10Hz


	tick10++;
	divider1s++;

	if(divider1s == 5 || divider1s == 10)
		second_10=1;					// flag; 500mS per debounce migliore!
	if(divider1s==10) {		// per RealTimeClock
		divider1s=0;
#ifdef USA_SW_RTC 
		milliseconds+=1000;
		bumpClock();
#endif
		}


		PIR1bits.TMR1IF = 0;
//		}

		}
	}
#endif


