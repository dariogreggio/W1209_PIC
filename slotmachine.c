/********************************************************************
 FileName:     main.c
 Dependencies: See INCLUDES section
 Processor:		PIC18, PIC24, and PIC32 USB Microcontrollers
 Hardware:		This demo is natively intended to be used on Microchip USB demo
 				boards supported by the MCHPFSUSB stack.  See release notes for
 				support matrix.  This demo can be modified for use on other hardware
 				platforms.
 Complier:  	Microchip C18 (for PIC18), C30 (for PIC24), C32 (for PIC32)
 Company:		Microchip Technology, Inc.

 Software License Agreement:

********************************************************************
 File Description:

 Change History:
  Rev   Description
  ----  -----------------------------------------
  1.0   5/2022 Initial release, basata su LX336 (n. 65/1979) di Nuova Elettronica
  1.1   8/2023 adattata a W1209PIC :) e con aggiunte giochi vari


Dedicato alla dolce guerra :) (il nasdaq muore, le atomiche arrivano...)

********************************************************************/

#ifndef MAIN_C
#define MAIN_C

/** INCLUDES *******************************************************/

#include <generictypedefs.h>
#include <delays.h>
#include <timers.h>
#include <reset.h>
#include <pwm.h>
#include <portb.h>
#include <stdio.h>
#include <stdlib.h>
#include "slotmachine.h"
#include "hardwareprofile - pic18F.h"

/** CONFIGURATION **************************************************/


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


#pragma romdata
static rom const char CopyrString[]= {'C','y','b','e','r','d','y','n','e',' ','(','A','D','P','M',')',' ','-',' ','G','i','o','c','h','i','n','i',' ','7','s','e','g',' ',
	VERNUMH+'0','.',VERNUML/10+'0',(VERNUML % 10)+'0', ' ','2','8','/','0','8','/','2','3', 0 };

const rom BYTE table_7seg[]={0, // .GFEDCBA
															0b00111111,0b00000110,0b01011011,0b01001111,0b01100110,
												 			0b01101101,0b01111101,0b00000111,0b01111111,0b01101111,
															0b01000000,			// il segno meno...
//slot
															0b01001001,			// no palle; 12
															0b01101011,			// palla alta
															0b01011101,			// palla bassa
															0b01111111,			// due palle
//dadi
															0b01000000,			// 1; 16
															0b00001001,			// 2
															0b01001001,			// 3
															0b00110110,			// 4
															0b01110110,			// 5
															0b00111111,			// 6
															};

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

#pragma udata

#define BASE_FIRST 6
BYTE first=BASE_FIRST;

BYTE gameMode=0;
BYTE tipoGioco=0;		//0=Slot, 1=Dadi, 2=tombola, 3=cascade, 4=auto, 5=atterraggio, 6=sette e mezzo
#define MAX_GIOCHI 7
BYTE durataGioco,durataGioco2;
WORD Punti,totPunti;
BYTE posPlayer=1;
WORD mazzoDiCarte[4];		// 1bit per carta estratta, 10 opp. 13 x 4



volatile BYTE tmr_cnt,second_1,second_100;
volatile BYTE Timer10;
BYTE dividerBeep;
BYTE dividerUI;


BYTE Displays[3][2];		// 3 display (display 3digit);

unsigned char dMode;


/** PRIVATE PROTOTYPES *********************************************/
static void InitializeSystem(void);
void ProcessIO(void);
void UserInit(void);
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();


/** VECTOR REMAPPING ***********************************************/
	
	#pragma code HIGH_INTERRUPT_VECTOR = 0x08
	void High_ISR(void)
	{
  _asm 
	  goto YourHighPriorityISRCode 
	_endasm
	}
	#pragma code LOW_INTERRUPT_VECTOR = 0x18
	void Low_ISR(void)
	{
  _asm 
		goto YourLowPriorityISRCode
	_endasm
	}
	

#pragma code

	
//These are your actual interrupt handling routines.
#pragma interrupt YourHighPriorityISRCode
void YourHighPriorityISRCode() {		//
	static BYTE dividerMux=0b00010000;
	signed char mux_corr;
		
	if(INTCONbits.TMR0IE && INTCONbits.TMR0IF) {		// (occhio USB)
// 4.1 mSec 2/8/23 con ottimizzazioni

	
		LATB= (LATB & 0b10001111) | dividerMux;
		switch(dividerMux) {
			case 0b00110000:
				LATC=Displays[2][dMode];
				dividerMux = 0b01010000;
				break;
			case 0b01010000:
				LATC=Displays[1][dMode];
				dividerMux = 0b01100000;
				break;
			case 0b01100000:
				LATC=Displays[0][dMode];
				dividerMux = 0b00110000;
				break;

			default:
				dividerMux = 0b01100000;
				break;
			}

#define MUX_CORR_STEP 5
		mux_corr=0;		// correggo tempo mux per dare più tempo se c'è più assorbimento!
		if((LATC & 1))		// catodo comune
			mux_corr+=MUX_CORR_STEP;
		if((LATC & 2))
			mux_corr+=MUX_CORR_STEP;
		if((LATC & 4))
			mux_corr+=MUX_CORR_STEP;
		if((LATC & 8))
			mux_corr+=MUX_CORR_STEP;
		if((LATC & 16))
			mux_corr+=MUX_CORR_STEP;
		if((LATC & 32))
			mux_corr+=MUX_CORR_STEP;
		if((LATC & 64))
			mux_corr+=MUX_CORR_STEP;

//	m_Led2Bit ^= 1; //check timer	


		INTCONbits.TMR0IF = 0;

//		WriteTimer0(TMR0BASE);					// inizializzo TMR0; no funzioni per overhead
		// WRITETIMER0(0) dovrebbe essere la macro!
//		TMR0H=Tmr0Base / 256;					// reinizializzo TMR0
		TMR0L=TMR0BASE-mux_corr;					// reinizializzo TMR0
// mah, non sembra fare molto a livello visivo...


		}
	
	}	//This return will be a "retfie fast", since this is in a #pragma interrupt section 


volatile BYTE divider1s;

#pragma interruptlow YourLowPriorityISRCode
void YourLowPriorityISRCode()	{
		//Check which interrupt flag caused the interrupt.
		//Service the interrupt
		//Clear the interrupt flag
		//Etc.


// 100.001 mSec 22/5/2020; ha perso 7 minuti in 11.5 ore ossia 420:41400, 1%


//	if(PIE1bits.TMR1IE && PIR1bits.TMR1IF) {

//		WriteTimer1(TMR1BASE);					// inizializzo TMR0
		TMR1H=TMR1BASE/256;
		TMR1L=TMR1BASE & 255;

//	m_Led2Bit ^= 1; //check timer	

		Timer10++;
		second_100=1;					// flag

		divider1s++;
		if(divider1s==10) {		// per RealTimeClock
			divider1s=0;
			second_1=1;					// flag
			}


		PIR1bits.TMR1IF = 0;
//		}


	
	}	//This return will be a "retfie", since this is in a #pragma interruptlow section 




/** DECLARATIONS ***************************************************/
#pragma code

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
void main(void) {   

//	STKPTR=0;		// risparmia una posizione di Stack, xché main() è stata CALLed!

  InitializeSystem();

  while(1) {

		// Application-specific tasks.

		if(second_100) {
			second_100=0;
    	handle_events();        
			updateUI();
			}

    }	//end while
	}	//end main


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

	StatusReset();

	ClrWdt();

	OSCCONbits.IRCF0=0;		// 8MHz con l'interno!
	OSCCONbits.IRCF1=1;
	OSCCONbits.IRCF2=1;
	OSCTUNE = 0b00100000;			// 

	OSCCONbits.SCS0 = 0;
	OSCCONbits.SCS1 = 0;

	OSCCONbits.IDLEN=0;

	__delay_ms(100);		// sembra sia meglio aspettare un pizzico prima di leggere la EEPROM.. (v. forum 2006)

	CM1CON0=0b00000000;
	CM2CON0=0b00000000;

	ANSEL=0b00000000;
	ANSELH=0b00000000;


  UserInit();
    
	}	//end InitializeSystem



/******************************************************************************
 * Function:        void UserInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine should take care of all of the demo code
 *                  initialization that is required.
 *
 * Note:            
 *
 *****************************************************************************/
void UserInit(void) {
	BYTE i;

	ClrWdt();

	srand(PORTA+PORTB+PORTC);

	EnablePullups();

	TRISA = 0b00011011;		// led; pulsanti; ev paddle :)
	TRISB = 0b00000000;		// catodi; 
	TRISC = 0b00000000;		// display; 
//	ODCA  = 0b0000000010000000;		// open collector

	LATA=0b00000000;		// tutto spento
	LATB=0b00000000;
	LATC=0b00000000;

  //Set the push button input
  mInitAllSwitches();
  //Set up the LED as output
  mInitAllLEDs();

	LATA = 0b00000000;		// 
	LATB = 0b00000000;		// ; 
	LATC = 0b00000000;		// 

	WPUA=0b00001011;			// 
	WPUB=0b00000000;

// https://forum.microchip.com/s/topic/a5C3l000000MCLJEA4/t277505 porcodio RA0 RA1
	IOCA= 0b00000011;

	OpenTimer0(TIMER_INT_ON & T0_8BIT & T0_SOURCE_INT & T0_PS_1_64);			// mux display
	// v. ReadTimer sotto...
	OpenTimer1(TIMER_INT_ON & T1_16BIT_RW & T1_SOURCE_INT & T1_OSC1EN_OFF & T1_SYNC_EXT_OFF & T1_PS_1_8);		// RTC (100mS opp 32768Hz)
//	T1CON=0b10110101;
//	PIE1bits.TMR1IE=1;

	dividerBeep=2;		//tanto per... all'accensione!

	RCONbits.IPEN=1;				// interrupt in Modalita' avanzata (18X)
	INTCON2bits.TMR0IP = 1;			// Timer-0 high Pty
	IPR1bits.TMR1IP = 0;				// Timer-1 low Pty, ev. crystal RTC
//	IPR1bits.TMR2IP = 0;				// Timer-2 Low Pty NON USATO

	TMR0L=TMR0BASE & 255;					// 
//	TMR1H=TMR1BASE / 256;					// inizializzo TMR1
//	TMR1L=TMR1BASE & 255;					// 
	WriteTimer1(TMR1BASE);					// inizializzo TMR1
  
	INTCONbits.GIEL = 1;			// attiva interrupt LO-pri
	INTCONbits.GIEH = 1;			// attiva interrupt HI-pri

	Displays[0][0]=Displays[1][0]=Displays[2][0]=0x00;
	Displays[0][1]=Displays[1][1]=Displays[2][1]=0x00;

	dMode=0;


	dividerUI=0;

	gameMode=0;
	totPunti=0;
	totPunti=EEleggi(&totPunti);
	totPunti |= ((WORD)EEleggi(1+&totPunti)) << 8;
	if(totPunti ==0xffff)
		totPunti=0;			// viene poi scritto al primo click!


	}		//end UserInit


BYTE dividerT;
void handle_events(void) {
	static BYTE oldPuls1Bit=3,timePressed1;
	WORD n;

	ClrWdt();

		dividerUI++;
		
		if(first)
			return;
	
		if(!sw2) {
			if(oldPuls1Bit & 2) {
//				SetBeep(2);		// tanto per...
	
				}

			switch(tipoGioco) {
				case 0:
					break;
				case 1:
					break;
				case 2:
					break;
				case 3:
					if(gameMode==3)
						if(posPlayer<2)
							posPlayer++;
					break;
				case 4:
					if(gameMode==3)
						if(posPlayer<2)
							posPlayer++;
					break;
				case 5:
					if(gameMode==3)
						if(durataGioco>0)
							durataGioco-=3;		// lo tratto come signed
// gestire carburante??
					break;
				case 6:
					if(gameMode==3) {		// fine giocate
						gameMode=4;
						}
					break;
				}

			if(!sw1 && gameMode<=1) {		// solo se gioco fermo!
				StdBeep();		// 
				totPunti=0;
				gameMode=1;
				switch(tipoGioco) {
					case 0:
						EEscrivi_(&totPunti,LOBYTE(totPunti));
						EEscrivi_(1+&totPunti,HIBYTE(totPunti));
						break;
					case 1:
						break;
					case 2:
						break;
					case 3:
						break;
					case 4:
						break;
					case 5:
						break;
					case 6:
						break;
					}
				dMode=1;
				Displays[2][1] = 0b00001000;
				showAllNumbers(totPunti,0);
				while(!sw1)			// debounce per evitare che lo prenda sotto!
					ClrWdt();
				Displays[2][1] = 0b00000000;
				}

			oldPuls1Bit &= ~2;
			}
		else {
	
//			Displays[2] &= ~0b01111001;
			oldPuls1Bit |= 2;
			}

		if(!sw1) {
			if(oldPuls1Bit & 1) {
				timePressed1=0;
				}
			switch(tipoGioco) {
				case 0:
					if(gameMode<2)
						gameMode=2;
					break;
				case 1:
					if(gameMode<2)
						gameMode=2;
					break;
				case 2:
					if(gameMode<2)
						gameMode=2;
					break;
				case 3:
					if(gameMode==3)
						if(posPlayer>0)
							posPlayer--;
					break;
				case 4:
					if(gameMode==3)
						if(posPlayer>0)
							posPlayer--;
					break;
				case 5:
					if(gameMode==3)
						if(durataGioco>0)
							durataGioco-=3;		// lo tratto come signed
// gestire carburante??
					break;
				case 6:
					break;
				}

			oldPuls1Bit &= ~1;
			}
		else {
			if(!(oldPuls1Bit & 1)) {
				switch(tipoGioco) {
					case 0:
						if(gameMode<3) {
							gameMode=3;
							durataGioco=durataGioco2=(timePressed1*2) + (rand() & 7) + 5;
		//					durataGioco=durataGioco2=20;
							if(timePressed1<30)			// 3 secondi max
								timePressed1++;
						//	else si potrebbe innescare lancio cmq!
							}
						break;
					case 1:
						if(gameMode<3) {
							gameMode=3;
							durataGioco=durataGioco2=(timePressed1*2) + (rand() & 7) + 5;
		//					durataGioco=durataGioco2=20;
							if(timePressed1<30)			// 3 secondi max
								timePressed1++;
						//	else si potrebbe innescare lancio cmq!
							}
						break;
					case 2:
						if(gameMode<3) {
							gameMode=3;
							durataGioco=durataGioco2=(timePressed1*2) + (rand() & 7) + 5;
		//					durataGioco=durataGioco2=20;
							if(timePressed1<30)			// 3 secondi max
								timePressed1++;
						//	else si potrebbe innescare lancio cmq!
							}
						break;
					case 3:
						if(gameMode<2)
							gameMode=2;
						break;
					case 4:
						if(gameMode<2) 
							gameMode=2;
						break;
					case 5:
						if(gameMode<2) 
							gameMode=2;
						break;
					case 6:
						if(gameMode==3) {
							BYTE i;

									i=estraiCarta40();
									Punti+=getPuntiDaCarta40(i);
									showCarta(i);
									showNumbers(Punti,1);
									dMode=1;

							gameMode=4;
							}
						break;
					}
				}
			oldPuls1Bit |= 1;
			}


		if(!sw3) {
			if(oldPuls1Bit & 4) {
				if(gameMode<2) {		// solo se gioco fermo!

				tipoGioco++;
				if(tipoGioco>=MAX_GIOCHI)
					tipoGioco=0;
					dividerT=24;		// forza scritta aggiornata
					}
				}

			oldPuls1Bit &= ~4;
			}
		else {
			if(!(oldPuls1Bit & 4)) {
				}
			oldPuls1Bit |= 4;
			}

	}


void updateUI(void) {
	BYTE i;


	if(dividerUI>=5) {		//500mS

		mLED_1 ^= 1;
	
		dividerUI=0;
	
		if(dividerBeep) {
			dividerBeep--;
			if(!dividerBeep) {
	//			T2CONbits.TMR2ON=0;
//				ClosePWM1(); qua SW
				}
		
			}
	
		if(first) {			// lascia Splash per un po'..
			first--;
			
			if(first>((BASE_FIRST/2)-1)) {
				showChar(0,CopyrString[38],0,0);
				showChar(0,CopyrString[39],1,1);
				showChar(0,CopyrString[41],2,0);
				}
			else {
				showChar(0,CopyrString[42],0,1);
				showChar(0,CopyrString[44],1,0);
				showChar(0,CopyrString[45],2,1);
				}
	

			if(!first) {
				Displays[2][0] = 0b00000000;		//off
				dividerT=0;
				}
	
			return;
			}
		}
	
	if(first) 
		return;

	dividerT++;

	switch(gameMode) {
		case 0:		// demo/fermo
			dMode=0;
			switch(dividerT) {
				case 0:
				case 10:
					Displays[0][0] = 0b00000001;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					break;
				case 1:
				case 11:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000001;
					Displays[2][0] = 0b00000000;
					break;
				case 2:
				case 12:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000001;
					break;
				case 3:
				case 13:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000010;
					break;
				case 4:
				case 14:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000100;
					break;
				case 5:
				case 15:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00001000;
					break;
				case 6:
				case 16:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00001000;
					Displays[2][0] = 0b00000000;
					break;
				case 7:
				case 17:
					Displays[0][0] = 0b00001000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					break;
				case 8:
				case 18:
					Displays[0][0] = 0b00010000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					break;
				case 9:
				case 19:
					Displays[0][0] = 0b00100000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					break;
				case 20:
				case 21:
				case 22:
				case 23:
				case 24:
				case 25:
				case 26:
				case 27:
				case 28:
					Displays[0][0] = 0b01110011;		// PLy
					Displays[1][0] = 0b00111000;
					Displays[2][0] = 0b01101110;
					break;
				case 29:
				case 31:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					break;
				case 30:
				case 32:
					Displays[0][0] = 0b01000000;
					Displays[1][0] = 0b01000000;
					Displays[2][0] = 0b01000000;
					break;
				case 33:
				case 34:
				case 35:
				case 36:
				case 37:
					switch(tipoGioco) {
						case 0:
							Displays[0][0] = 0b01101101;		// SLO
							Displays[1][0] = 0b00111000;
							Displays[2][0] = 0b00111111;
							break;
						case 1:
							Displays[0][0] = 0b01011110;		// dAd
							Displays[1][0] = 0b01110111;
							Displays[2][0] = 0b01011110;
							break;
						case 2:
							Displays[0][0] = 0b01111000;		// tom
							Displays[1][0] = 0b01011100;
							Displays[2][0] = 0b01010100;
							break;
						case 3:
							Displays[0][0] = 0b00111001;		// CAS
							Displays[1][0] = 0b01110111;
							Displays[2][0] = 0b01101101;
							break;
						case 4:
							Displays[0][0] = 0b01110111;		// AUt
							Displays[1][0] = 0b00111110;
							Displays[2][0] = 0b01111000;
							break;
						case 5:
							Displays[0][0] = 0b01101101;		// SPC
							Displays[1][0] = 0b01110011;
							Displays[2][0] = 0b00111001;
							break;
						case 6:
							Displays[0][0] = 0b00000111;		// 712
							Displays[1][0] = 0b00000110;
							Displays[2][0] = 0b01011011;
							break;
						}
					break;
				case 38:
				case 40:
				case 42:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					break;
				case 39:
				case 41:
				case 43:
					Displays[0][0] = 0b01000000;
					Displays[1][0] = 0b01000000;
					Displays[2][0] = 0b01000000;
					break;
				case 44:
				case 46:
				case 48:
					switch(tipoGioco) {
						case 0:
#warning mettere random figure slot
							Displays[0][0] = 0b00000000;
							Displays[1][0] = 0b00000000;
							Displays[2][0] = 0b00000000;
							break;
						case 1:
							Displays[0][0] = 0b00111111;
							Displays[1][0] = 0b01000000;
							Displays[2][0] = 0b00111111;
							break;
						case 2:
							Displays[0][0] = 0b00111111;
							Displays[1][0] = 0b01101111;
							Displays[2][0] = 0b00111111;
							break;
						case 3:
							Displays[0][0] = 0b00001000;
							Displays[1][0] = 0b00001000;
							Displays[2][0] = 0b00001000;
							break;
						case 4:
							Displays[0][0] = 0b00000000;
							Displays[1][0] = 0b00001000;
							Displays[2][0] = 0b00000000;
							break;
						case 5:
							Displays[0][0] = 0b00000001;		// linea a scendere
							Displays[1][0] = 0b01000000;
							Displays[2][0] = 0b00001000;
							break;
						case 6:
							Displays[0][0] = 0b00111001;		// Come Quando Fuori :)
							Displays[1][0] = 0b01100111;
							Displays[2][0] = 0b01110001;
							break;
						}
					break;
				case 45:
				case 47:
				case 49:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					break;
				case 50:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					break;
				case 51:
					gameMode=1;
					dividerT=0;
					break;
				}
			break;

		case 1:		// demo/punti
			dMode=1;
			switch(tipoGioco) {
				case 0:
					showAllNumbers(totPunti,0);
					break;
				case 1:

			dividerT=12;
					break;
				case 2:

			dividerT=12;
					break;
				case 3:

					showAllNumbers(durataGioco,0);		// bah astronavi
					break;
				case 4:

					showAllNumbers(durataGioco,0);		// bah tempo residuo
					break;
				case 5:

					showAllNumbers(durataGioco2,0);		// bah carburante residuo o tempo impiegato
					break;
				case 6:
					break;
				}
			if(dividerT>=12) {
				dividerT=0;
				gameMode=0;
				}
			break;

		case 2:		// prerolling/pregioco
			dMode=0;
			switch(tipoGioco) {
				BYTE n;
				case 0:
					n=dividerT % 3;
					switch(n) {
		/*				case 0:
							Displays[1][0] = 0b00001000;
							Displays[2][0] = 0b00001000;
							Displays[3][0] = 0b00001000;
							break;
						case 1:
							Displays[1][0] = 0b01000000;
							Displays[2][0] = 0b01000000;
							Displays[3][0] = 0b01000000;
							break;
						case 2:
							Displays[1][0] = 0b00000001;
							Displays[2][0] = 0b00000001;
							Displays[3][0] = 0b00000001;
							break;*/
						case 0:
							Displays[0][0] = 0b01001001;
							Displays[1][0] = 0b01001001;
							Displays[2][0] = 0b01001001;
							break;
						case 1:
							Displays[0][0] = 0b01011101;
							Displays[1][0] = 0b01011101;
							Displays[2][0] = 0b01011101;
							break;
						case 2:
							Displays[0][0] = 0b01101011;
							Displays[1][0] = 0b01101011;
							Displays[2][0] = 0b01101011;
							break;
						}
					break;
				case 1:
					n=dividerT & 1;
					switch(n) {
						case 0:
							Displays[0][0] = 0b01000000;
							Displays[1][0] = 0b00000000;
							Displays[2][0] = 0b00111111;
							break;
						case 1:
							Displays[0][0] = 0b00111111;
							Displays[1][0] = 0b00000000;
							Displays[2][0] = 0b01000000;
							break;
						}
					break;
				case 2:
					showNumbers((rand() % 89)+1,0);
					dMode=1;
					break;
				case 3:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					durataGioco=3;		// astronavi
					posPlayer=1;
					if(dividerT > 5) {
						gameMode=3;
						dividerT=0;
						}
					break;
				case 4:
					Displays[0][0] = 0b00000000;
					Displays[1][0] = 0b00000000;
					Displays[2][0] = 0b00000000;
					durataGioco=90     *2;		// secondi
					posPlayer=1;
					if(dividerT > 5) {
						gameMode=3;
						dividerT=0;
						}
					break;
				case 5:
					Punti=900;		// metri
					durataGioco=0;
					durataGioco2=0;
					showNumbers(Punti,0);
					dMode=1;
					if(dividerT > 5) {
						gameMode=3;
						dividerT=0;
						}
					break;
				case 6:
					switch(dividerT) {
						case 0:
							Punti=0;		// 
							durataGioco=0;		// se banco o giocatore prima
							durataGioco2=0;
							break;
						case 1:
							mischiaMazzo();		// tutto finto :D quasi
							showNumbers((rand() % 52 /*40*/)+1,0);
							dMode=1;
							break;
						case 10:
							durataGioco=rand() & 1;
							if(durataGioco) {
								Displays[0][0] = 0b00110000;		// Io
								Displays[1][0] = 0b01011100;
								Displays[2][0] = 0b00000000;
								}
							else {
								Displays[0][0] = 0b01111000;		// tU
								Displays[1][0] = 0b00111110;
								Displays[2][0] = 0b00000000;
								}
							break;
						case 20:
							if(durataGioco) {
								if(Punti < 50 || (Punti < 70 && rand()<5000)) {		// strategia diciamo :)
									i=estraiCarta40();
									Punti+=getPuntiDaCarta40(i);
									showCarta(i);
									}
								else {
									showNumbers(Punti,1);
									dMode=1;
									}
								}
							break;
						case 30:
							dividerT=19;
							break;
						case 40:
							gameMode=3;
							dividerT=0;
							break;
						}
					break;
				}
			break;

		case 3:		// rolling/giocando
			dMode=0;
			switch(tipoGioco) {
				BYTE n;
				case 0:
					if(durataGioco2) {
						if(durataGioco2>=20)
							n=1;
						else if(durataGioco2>=12)
							n=2;
						else if(durataGioco2>=6)
							n=3;
						else if(durataGioco2>=3)
							n=4;
						else
							n=5;
						if(dividerT >= n) {
							WORD d=rand();
							showFruits((d >> 8) & 3,(d >> 4) & 3,d & 3);
		// forse si potrebbe rendere il frutto "3" più raro degli altri... ma va benone anche così
							durataGioco2--;
							dividerT=0;
							}
						}
					else {
						if(dividerT > 20) {
							gameMode=4;
							dividerT=0;
							}
						}
					break;

				case 1:
					if(durataGioco2) {
						if(durataGioco2>=20)
							n=1;
						else if(durataGioco2>=12)
							n=2;
						else if(durataGioco2>=6)
							n=3;
						else if(durataGioco2>=3)
							n=4;
						else
							n=5;
						if(dividerT >= n) {
							WORD d=rand();
							showDadi((d >> 3) % 6,d % 6);
							durataGioco2--;
							dividerT=0;
							}
						}
					else {
						if(dividerT > 20) {
							gameMode=4;
							dividerT=0;
							}
						}
					break;

				case 2:			// qua si potrebbe anche cambiare/semplificare...
					if(durataGioco2) {
						if(durataGioco2>=20)
							n=1;
						else if(durataGioco2>=12)
							n=2;
						else if(durataGioco2>=6)
							n=3;
						else if(durataGioco2>=3)
							n=4;
						else
							n=5;
						if(dividerT >= n) {
							WORD d=(rand() % 89)+1;
							showNumbers(d,0);
							for(i=0; i<3; i++)
								Displays[i][0] = Displays[i][1];			// bah faccio così qua, non tocco dMode
							durataGioco2--;
							dividerT=0;
							}
						}
					else {
						if(dividerT > 20) {
							gameMode=4;
							dividerT=0;
							}
						}
					break;

				case 3:
					Displays[0][0] &= ~0b10001000;			// pulisco astronave 
					Displays[1][0] &= ~0b10001000;
					Displays[2][0] &= ~0b10001000;
					if(dividerT>3) {		// accelerare all'aumentare quadro...
						for(i=0; i<3; i++) {
							if(Displays[i][0] & 0b01000000) {			// controllo segmento di mezzo...
								if(posPlayer==i) {			//.. se c'è l'astronave, punto..
									Displays[posPlayer][0] |= 0b10000000;
									Punti++;
// se ce ne sono più d'uno... scoppio lo stesso!									break;
									}
								else {
									if(durataGioco)				// ..altrimenti, esplode!
										durataGioco--;
									Displays[0][0] |= 0b10000000;
									Displays[1][0] |= 0b10000000;
									Displays[2][0] |= 0b10000000;
									}
								Displays[i][0] &= ~0b01000000;
								}
							if(Displays[i][0] & 0b00000001) {			// quindi scrollo da segmento alto a mezzo
								Displays[i][0] |= 0b01000000;
								Displays[i][0] &= ~0b00000001;
								}
							}

						if(rand()>25000) {
							/*n=rand() & 7;
							if(n & 1)
								Displays[0][0] |= 0b00000001;
							else
								Displays[0][0] &= ~0b00000001;
							if(n & 2)
								Displays[1][0] |= 0b00000001;
							else
								Displays[1][0] &= ~0b00000001;
							if(n & 4)
								Displays[2][0] |= 0b00000001;
							else
								Displays[2][0] &= ~0b00000001;*/
// beh direi solo una colonna alla volta!
							n=rand() % 3;
							if(n == 0)
								Displays[0][0] |= 0b00000001;
							if(n == 1)
								Displays[1][0] |= 0b00000001;
							if(n == 2)
								Displays[2][0] |= 0b00000001;
							}
						dividerT=0;
						}

					Displays[0][0] |= (posPlayer==0 ? 0b00001000 : 0b00000000);
					Displays[1][0] |= (posPlayer==1 ? 0b00001000 : 0b00000000);
					Displays[2][0] |= (posPlayer==2 ? 0b00001000 : 0b00000000);
					if(!durataGioco) {
						gameMode=4;
						dividerT=0;
						}
					break;

				case 4:
					Displays[0][0] &= ~0b00001000;
					Displays[1][0] &= ~0b00001000;
					Displays[2][0] &= ~0b00001000;
					if(dividerT> (durataGioco<20 ? 2 : 3)) {		// accelero a fine tempo...
						WORD n2;

						for(i=0; i<3; i++) {
							Displays[i][0] &= ~0b00010100;				// pulisco parte bassa
							if(Displays[i][0] & 0b00100000) 			// scrollo da zona alta a zona bassa, strada sx
								Displays[i][0] |= 0b00010000;
//							Displays[i][0] &= ~0b00100000;
							if(Displays[i][0] & 0b00000010)				// strada dx
								Displays[i][0] |= 0b00000100;
//							Displays[i][0] &= ~0b00000010;
							if(Displays[i][0] & 0b01000000) {			// altra auto in mezzo PRIMA!
								if(Displays[i][0] & 0b00001000) {		// controllare schianto!
									ErrorBeep();
									}
								Displays[i][0] |= 0b00001000;
								Displays[i][0] &= ~0b01000000;
								}
							if(Displays[i][0] & 0b00000001) {			// altra auto in alto
								Displays[i][0] |= 0b01000000;
								Displays[i][0] &= ~0b00000001;
								}
							}
						if(rand()>25000) {
							n2=rand();
							switch(durataGioco2) {
								case 0:			// strada larga
									if(n2>RAND_MAX/2)
										durataGioco2=1;
									else
										durataGioco2=2;
									break;
								case 1:			// strada stretta sx
									if(n2>((DWORD)RAND_MAX*2)/3)
										durataGioco2=3;
									else if(n2>RAND_MAX/3)
										durataGioco2=4;
									else 
										durataGioco2=0;
									break;
								case 2:			// strada stretta dx
									if(n2>((DWORD)RAND_MAX*2)/3)
										durataGioco2=5;
									else if(n2>RAND_MAX/3)
										durataGioco2=4;
									else 
										durataGioco2=0;
									break;
								case 3:			// strada strettissima sx
									durataGioco2=1;
									break;
								case 4:			// strada strettissima centro
									if(rand()>RAND_MAX/2)
										durataGioco2=1;
									else 
										durataGioco2=2;
									break;
								case 5:			// strada strettissima dx
									durataGioco2=2;
									break;
								}
							}
						if(rand()>30000) {			// inserisco anche altre macchine! :D
// RIMETTERE (sporca troppo...			Displays[rand() % 3][0] |= 0b00000001;
							}

						Displays[0][0] &= ~0b00100010;
						Displays[1][0] &= ~0b00100010;
						Displays[2][0] &= ~0b00100010;
						switch(durataGioco2) {
							case 0:			// strada larga
								Displays[0][0] |= 0b00100000;
								Displays[2][0] |= 0b00000010;
								break;
							case 1:			// strada stretta sx
								Displays[0][0] |= 0b00100000;
								Displays[1][0] |= 0b00000010;
								break;
							case 2:			// strada stretta dx
								Displays[1][0] |= 0b00100000;
								Displays[2][0] |= 0b00000010;
								break;
							case 3:			// strada strettissima sx
								Displays[0][0] |= 0b00100010;
								break;
							case 4:			// strada strettissima centro
								Displays[1][0] |= 0b00100010;
								break;
							case 5:			// strada strettissima dx
								Displays[2][0] |= 0b00100010;
								break;
							}
						Displays[0][0] ^= 0b10000000;
						Displays[2][0] ^= 0b10000000;
						dividerT=0;
						}

					for(i=0; i<3; i++) {
						if(Displays[i][0] & 0b00001000 && posPlayer==i) {			// se ti sei spostato, meno punti/più lento
							Punti++;
							break;
							}
						}
					Displays[0][0] |= (posPlayer==0 ? 0b00001000 : 0b00000000);
					Displays[1][0] |= (posPlayer==1 ? 0b00001000 : 0b00000000);
					Displays[2][0] |= (posPlayer==2 ? 0b00001000 : 0b00000000);
					switch(durataGioco2) {
						case 0:			// strada larga (se sei fuori strada, no punti per un po')
							Punti++;			// 
							break;
						case 1:			// strada stretta sx
							if(posPlayer==0 || posPlayer==1) 			// 
								Punti++;
							break;
						case 2:			// strada stretta dx
							if(posPlayer==1 || posPlayer==2) 			// 
								Punti++;
							break;
						case 3:			// strada strettissima sx
							if(posPlayer==0) 			// 
								Punti++;
							break;
						case 4:			// strada strettissima centro
							if(posPlayer==1) 			// 
								Punti++;
							break;
						case 5:			// strada strettissima dx
							if(posPlayer==2) 			// 
								Punti++;
							break;
						}

					durataGioco--;
					if(!durataGioco) {
						gameMode=4;
						dividerT=0;
						}
					break;

				case 5:
					durataGioco+=1;		// 0.98 ossia 9.8m/s in 100mS
					if(Punti>durataGioco)
						Punti-=(signed char)durataGioco;
					else
						Punti=0;

					showNumbers(Punti,0);
					for(i=0; i<3; i++) {
						Displays[i][0] = Displays[i][1];			// bah faccio così qua, non tocco dMode
						if((signed char)durataGioco<0) 		// retrorazzi, accendo puntini
							Displays[i][0] |= 0b10000000;
						}

					if(Punti>=1000) {
						Displays[0][0] = 0b00111111;		// OUt
						Displays[1][0] = 0b00111110;
						Displays[2][0] = 0b01111000;
						ErrorBeep();
						gameMode=4;
						}
					else if(Punti>5) {
						durataGioco2++;
						}
					else {
						if((signed char)durataGioco<5) {
							Displays[0][0] = 0b00111111;		// OK :)
							Displays[1][0] = 0b01110110;
							Displays[2][0] = 0b00000000;
							StdBeep();
							}
						else {
							Displays[0][0] = 0b00111001;		// CrS
							Displays[1][0] = 0b01010000;
							Displays[2][0] = 0b01101101;
							ErrorBeep();
							}
						gameMode=4;
						dividerT=0;
						}
					break;

				case 6:
					switch(dividerT) {
						case 0:
							break;
						case 1:
							break;
						}
					if(dividerT>3) {
						Displays[2][0] ^= 0b10000000;		// indica attesa giocatore :)
						}
					break;
				}
			break;

		case 4:		// fine rolling/punteggio
			dMode=1;
			if(dividerT == 1) {
				switch(tipoGioco) {
					case 0:
						Punti=getPuntiDaDisplay();
						showNumbers(Punti,0);
						totPunti += Punti;
						if(Punti) {		// giustamente :)
// bah tolgo per ora... finire per tutti, array boh							EEscrivi_(&totPunti,LOBYTE(totPunti));
//							EEscrivi_(1+&totPunti,HIBYTE(totPunti));
							}
						break;
					case 1:
						Punti=getPuntiDaDisplay();
						showNumbers(Punti,0);
						break;
					case 2:
//						Punti=getPuntiDaDisplay();
//						showNumbers(Punti,0);
// a posto così
						StdBeep();
						break;
					case 3:
						showNumbers(Punti,0);
						break;
					case 4:
						showNumbers(Punti,0);
						break;
					case 5:
						showNumbers(Punti,0);
						break;
					case 6:
						showNumbers(Punti,0);
						break;
					}
				}
			if(dividerT >= 20) {
				gameMode=1;
				dMode=1;
				dividerT=0;
				}
			break;
		}
		
	}


WORD getPuntiDaDisplay(void) {
	WORD n;
	BYTE t[3];
	BYTE i,j;

	switch(tipoGioco) {
		case 0:
			t[0]=t[1]=t[2]=0;
			for(i=0; i<3; i++) {
				switch(Displays[i][0]) {
					case 0b01001001 /*12*/:			// non è il massimo ma ok..
						t[0]++;
						break;
					case 0b01101011 /*12+1*/:
						t[1]++;
						break;
					case 0b01011101 /*12+2*/:
						t[2]++;
						break;
					case 0b01111111 /*12+3*/:
						t[3]++;
						break;
					}
				}
		
			switch(t[3]) {
				case 3:
					return 50;
					break;
				case 2:
					if(t[1]>0 || t[2]>0) {
						return 20;
						}
					else {
						return 10;
						}
					break;
				case 1:
					if(t[1]==2 || t[2]==2) {
						return 5;
						}
					else if(t[1]==1 || t[2]==1) {
						return 3;
						}
					else {
						return 2;
						}
					break;
				case 0:
					if(t[1]==3 || t[2]==3) {
						return 3;
						}
					else if(t[1]==2 || t[2]==2) {
						return 1;
						}
					break;
				}
			break;
		case 1:
			t[0]=0;
			for(i=0; i<3; i+=2) {
				for(j=0b01000000; j; j>>=1) {
					if(Displays[i][0] & j) {
						t[0]++;
						}
					}
				}
			return t[0];
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		}

	return 0;
	}

BYTE getPuntiDaCarta40(BYTE n) {		// in decimi
	BYTE i,j;

	i=n / 4;
	j=n % 10;
	if(j>=8)
		return 5;
	else
		return j*10;
	}

void showChar(char q,char n,BYTE pos,BYTE dp) {

	if(n>='0' && n<='9')
		Displays[pos][q]=table_7seg[n-'0'+1];
	else if(n>='A' && n<='Z')
		Displays[pos][q]=table_7seg[n-'A'+10+1];
	else if(n=='-')
		Displays[pos][q]=table_7seg[11];
	else if(!n)
		Displays[pos][q]=table_7seg[0];
	else 
		Displays[pos][q]=table_7seg[n];
	if(dp)
		Displays[pos][q] |= 0b10000000;
	}

void showFruits(BYTE n1,BYTE n2,BYTE n3) {

	showChar(0,n1+12,2,0);
	showChar(0,n2+12,1,0);
	showChar(0,n3+12,0,0);
	}

void showDadi(BYTE n1,BYTE n2) {

	showChar(0,n2+16,2,0);
	Displays[1][0] = 0b00000000;
	showChar(0,n1+16,0,0);
	}

void showCarta(BYTE n) {
	BYTE i,j;

	i=n / 4;
	j=n % 10;
	switch(i) {
		case 0:
			showChar(0,'C',1,0);
			break;
		case 1:
			showChar(0,'Q',1,0);
			break;
		case 2:
			showChar(0,'F',1,0);
			break;
		case 3:
			showChar(0,'P',1,0);
			break;
		}
	Displays[1][0] = 0b00000000;
	switch(j) {
		case 7:
			showChar(0,'F',0,0);
			break;
		case 8:
			showChar(0,'D',0,0);
			break;
		case 9:
			showChar(0,'R',0,0);
			break;
		default:
			showChar(0,j+'1',0,0);
			break;
		}
	}

void showOverFlow(BYTE t) {

	if(t) {
		Displays[0][1] |= 0b10000000;
		Displays[1][1] |= 0b10000000;
		Displays[2][1] |= 0b10000000;
		}
	}

void show2Numbers(BYTE n1,BYTE n2,char m) {
	char myBuf[8];

	myBuf[3]= (n1 / 10) + '0';
	showChar(1,myBuf[3],2,0 /*!prec MAI */);
	myBuf[2]= (n1 % 10) + '0';
	showChar(1,myBuf[2],1,0);
	myBuf[0]= (n2 % 10) + '0';
	showChar(1,myBuf[0],0,0);
	if(m)
		Displays[2][1] |= 0b00000110;
	else
		Displays[2][1] &= ~0b00000110;
	}

void showNumbers(int n,BYTE prec) {
	char myBuf[8];
	char sign;		// 1 se negativo

	if(n<0) {
		sign=1;
		n=-n;
		}
	else
		sign=0;
	myBuf[0]= (n % 10) + '0';
	showChar(1,myBuf[0],2,0 /*!prec MAI */);
	n /= 10;
	prec--;
	myBuf[1]= (n % 10) + '0';
	showChar(1,myBuf[1],1,!prec);
	n /= 10;
	prec--;
	if(sign) {
//		myBuf[0]='-';
		showChar(1,'-',0,!prec);
		if(n>=1)
			showOverFlow(1);
		}
	else {
		myBuf[2]= (n % 10) + '0';
		showChar(1,myBuf[2] != '0' ? myBuf[2] : 0,0,!prec);
		if(n>=10)
			showOverFlow(1);
		}
	}

void showAllNumbers(int n,BYTE prec) {
	char myBuf[8];
	char sign;		// 1 se negativo

	if(n<0) {
		sign=1;
		n=-n;
		}
	else
		sign=0;
	myBuf[0]= (n % 10) + '0';
	showChar(1,myBuf[0],2,0 /*!prec MAI */);
	n /= 10;
	prec--;
	myBuf[1]= (n % 10) + '0';
	showChar(1,myBuf[1],1,!prec);
	n /= 10;
	prec--;
	if(sign) {
//		myBuf[0]='-';
		showChar(1,'-',0,!prec);
		if(n>=1)
			showOverFlow(1);
		}
	else {
		myBuf[3]= (n % 10) + '0';
		showChar(1,myBuf[3],0,!prec);
		if(n>=10)
			showOverFlow(1);
		}
	}

void StdBeep(void) {
	int n=3000;		// ~1 sec

	while(n--) {
		mLED_1_Toggle();
		__delay_us(57 /*125*/);		// 4KHz
		}	
	}

void SetBeep(BYTE n) {

// fare se serve...

	dividerBeep=1 /*n*/;		// FINIRE SISTEMARE tempi sopra!

	}

void ErrorBeep(void) {
	int n=2000;		// 1 sec

	while(n--) {
		mLED_1_Toggle();
		__delay_us(100);		// 
		}	
	}


void mischiaMazzo(void) {

	mazzoDiCarte[0]=mazzoDiCarte[1]=mazzoDiCarte[2]=mazzoDiCarte[3]=0;
	}

signed char estraiCarta40(void) {
	BYTE i,j,n;

	for(i=0; i<4; i++) {
		for(j=0; j<10; j++) {
			if(!(mazzoDiCarte[i] & 1<<j))
				goto libera;
			}
		}
	return -1;

libera:
	n=rand() % 40;
	i=n / 4;
	j=n % 10;
	if(!(mazzoDiCarte[i] & 1<<j)) {
		mazzoDiCarte[i] |= 1<<j;
		return n;
		}
	else {
		n++;
		goto libera;
		}
	}


// -------------------------------------------------------------------------------------
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


// -------------------------------------------------------------------------------------
//Delays W microseconds (includes movlw, call, and return) @ 8MHz
void __delay_us(BYTE uSec) {

	//uSec/=2;
	do {
//		Delay1TCY();			// 1
//		ClrWdt();						// 1; Clear the WDT
		} while(--uSec);		// 4
// dovrebbero essere 2...

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
	  __delay_ms(100);
		} while(--n);
	}


void __delay_ms(BYTE n) {				// circa n ms

	do {
		__delay_us(250);
		__delay_us(250);
		__delay_us(250);
		__delay_us(250);
		} while(--n);
	}


/** EOF main.c *************************************************/


