typedef unsigned char * SHORTPTR;


#define SERNUM      1000
#define VERNUMH     1
#define VERNUML     10



enum {
	BEEP_STD_FREQ=60,		// 4KHz @8MHz  QUA in software!
	BEEP_ERR_FREQ=100
	};



// il Timer0 conta ogni 125nSec*prescaler... (@8MHz CPUCLK => 2MHz) su PIC18
//#define TMR0BASE ((256-206)-1)			// 300Hz per timer/orologio/mux (a
#define TMR0BASE (256-100)		//   3.24mS=~300Hz per mux 26/8/23
// il Timer1 conta ogni 250nSec*prescaler... (@16MHz CPUCLK => 8MHz) su PIC18
//#define TMR1BASE ((65536-(50000-525))+3)		// 10Hz per orologio 5x10000 correzione
#define TMR1BASE (65536-25000)		//   10Hz per timer




void EEscrivi_(SHORTPTR addr,BYTE n);
BYTE EEleggi(SHORTPTR addr);
void EEscriviDword(SHORTPTR addr,DWORD n);
DWORD EEleggiDword(SHORTPTR addr);
void EEscriviWord(SHORTPTR addr,WORD n);
WORD EEleggiWord(SHORTPTR addr);
#define EEcopiaARAM(p) { *p=EEleggi(p); }
#define EEcopiaAEEPROM(p) EEscrivi_(p,*p)



	
void __delay_us(BYTE );
void __delay_ms(BYTE );
void Delay_S_(BYTE );
#define Delay_S() Delay_S_(10)
#define SPI_DELAY() Delay_SPI()



void handle_events(void);
void updateUI(void);
WORD getPuntiDaDisplay(void);
BYTE getPuntiDaCarta40(BYTE );
void showNumbers(int,BYTE);
void show2Numbers(BYTE,BYTE,char);
void showAllNumbers(int,BYTE);
void showChar(char,char,BYTE,BYTE);
void showOverFlow(BYTE);
void showFruits(BYTE n1,BYTE n2,BYTE n3);
void showDadi(BYTE n1,BYTE n2);
void showCarta(BYTE n);
void mischiaMazzo(void);
signed char estraiCarta40(void);


void SetBeep(BYTE);
void StdBeep(void);
void ErrorBeep(void);



