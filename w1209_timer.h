
#include "GenericTypeDefs.h"
#include "Compiler.h"

#define SERNUM      1000
#define VERNUMH     1
#define VERNUML     0


//#define FLASH_TIME 7
#define USA_SW_RTC 1
//#define USA_USB 1 // v. progetto , in MPLABX / PIC16

//#define USA_TC74 1
//#define USA_MCP9800 1
//#define USA_SHT21 1
//#define USA_HDI 1
//#define USA_VOLT 1
//#define USA_ANALOG 1
//#define SUB_ADDRESS 0


#define INVALID_READOUT 9999		//meglio di ffff !!

// il Timer0 conta ogni 125nSec*prescaler... (@8MHz CPUCLK => 2MHz) su PIC18
#define TMR0BASE (256-100)		//   3.24mS=~300Hz per mux 2/8/23 #cancrofitch
//((3240000L)/(1000000000.0/(GetPeripheralClock())*32))
#define TMR1BASE (65536-25000)		//   10Hz per timer
//((100000000L)/(1000000000.0/(GetPeripheralClock())*8))
#define TMR3BASE_1 (65536-50000)		//   1Hz per timerCount di BASE
#define TMR3BASE_10 (65536-5000)		//   10Hz
// finire prescaler 4
//((100000000L)/(1000000000.0/(GetPeripheralClock())*4))

typedef unsigned char *SHORTPTR;

#define BEEP_STD_FREQ 100

#define EEscrivi(a,b) EEscrivi_(a,b)			// per lcd
void EEscrivi_(BYTE *addr,BYTE n);
BYTE EEleggi(BYTE *addr);
void EEscriviDword(BYTE *addr,DWORD n);
DWORD EEleggiDword(BYTE *addr);
void EEscriviWord(BYTE *addr,WORD n);
WORD EEleggiWord(BYTE *addr);
#define EEcopiaARAM(p) { *p=EEleggi(p); }
#define EEcopiaAEEPROM(p) EEscrivi_(p,*p)




void prepOutBuffer(BYTE );
void clearOutBuffer(void);
void prepStatusBuffer(BYTE);
#ifdef USA_SW_RTC 
void bumpClock(void);
#endif

#if defined(__18CXX)
void __delay_us(BYTE uSec);
void __delay_ms(BYTE mSec);
#endif
#if !defined(__18CXX)
#define rom
#endif
void Delay_S_(BYTE );				// circa n*100mSec

	
#define DELAY_SPI_FAST 5
#define Delay08()	__delay_us(TIME_GRANULARITY)			// 1 bit-time 
#define Delay_SPI() __delay_us(DELAY_SPI_FAST /*DelaySPIVal*/)
#define Delay_1uS() __delay_us(1)
#define Delay_uS(x) __delay_us(x)
#define Delay_mS(x) __delay_ms(x)
#define Delay_1mS() Delay_mS(1)
void Delay_mS_LP(BYTE);
void I2CDelay(void);





void StartTimer(void);
void Beep(void);
void showChar(char n,BYTE pos,BYTE dp);
void showNumbers(int n,BYTE prec);
void showText(const rom char *s);


void UserTasks(void);
signed int leggi_tempAna(void);
signed int leggi_temp(BYTE);
signed int leggi_tempMCP(BYTE );
WORD leggi_press(BYTE );
signed int leggi_tempSHT(BYTE );
WORD leggi_umidSHT(BYTE q,signed int temp_comp /*°C*/);
WORD leggi_tensione(BYTE );


    // PIC24 RTCC Structure
typedef union {
  struct {
    unsigned char   mday;       // BCD codification for day of the month, 01-31
    unsigned char   mon;        // BCD codification for month, 01-12
    unsigned char   year;       // BCD codification for years, 00-99
    unsigned char   reserved;   // reserved for future use. should be 0
  	};                              // field access	
  unsigned char       b[4];       // byte access
  unsigned short      w[2];       // 16 bits access
  unsigned long       l;          // 32 bits access
	} PIC24_RTCC_DATE;

// PIC24 RTCC Structure
typedef union {
  struct {
    unsigned char   sec;        // BCD codification for seconds, 00-59
    unsigned char   min;        // BCD codification for minutes, 00-59
    unsigned char   hour;       // BCD codification for hours, 00-24
    unsigned char   weekday;    // BCD codification for day of the week, 00-06
  	};                              // field access
  unsigned char       b[4];       // byte access
  unsigned short      w[2];       // 16 bits access
  unsigned long       l;          // 32 bits access
	} PIC24_RTCC_TIME;

BYTE to_bcd(BYTE );
BYTE from_bcd(BYTE );

struct __attribute__((__packed__)) SAVED_PARAMETERS {
	volatile PIC24_RTCC_DATE currentDate;
	volatile PIC24_RTCC_TIME currentTime;
	WORD timerCount;
	BYTE timerUnit /* 0,1,... */;
	signed char clock_correction;
	BYTE timerOptions;			// b0=boot; b1=protect; b2=mode; b3: retriggerabile; (b4: azzerabile SEMPRE se non protect)
  };

extern struct SAVED_PARAMETERS configParms;


//cmd #
enum {
	CMD_NOP=0,
	CMD_GETID=1,
	CMD_GETCAPABILITIES=2,
	CMD_GETCONFIG=3,
	CMD_SETCONFIG,

	CMD_GETSTATUS=8,
	CMD_SETSTATUS,

	CMD_TEST=16,
	CMD_BEEP=17,
	CMD_DISPLAY=18,
	CMD_GETLOG,

	CMD_WRITEEEPROM=24,
	CMD_WRITERTC=25,
	CMD_READEEPROM=26,
	CMD_READRTC=27,

	CMD_RESET_IF=30,
	CMD_DEBUG_ECHO=31,
	CMD_READ_IDLOCS=63
	};


// MANUFACTURER_ID=0x115f (ADPM) = 4447

// PRODUCT_IDs
// 0x101=tastiera Mac-Ktronic
// 0x102=tastiera/mouse Ktronic
// 0x103=mouse Interlink (anche PS/2 autosensing) Ktronic
// 0x104=tastiera 6 tasti Ktronic e anche Remote Keyboard ADPM; e Consilium
// 0x105=Terminale USB Ktronic 
// 0x106=tastiera Logitech-Ktronic Multimediale
// 0x107=tastiera Ktronic con canale dati per riconfigurazione ecc.
// 0x108=Composite device (MSC+CDC)KT-ED12 Boagno
// 0x109=Composite device (MSC+CDC)KT-ED11 Boagno
// 0x10a=Tester generico KTronic
// 0x10b=tastiera capacitiva 
// 0x10c=tastiera IR

// 0x54=demo MSD HID (geco)

// 0x201=Skynet USB RS485
// 0x202=Skynet USB Onde convogliate
// 0x203=Skynet USB Wireless CC900
// 0x204=Skynet USB Wireless Aurel
// 0x20a=Skynet USB simil-ethernet
// 0x210=SkyPIC USB per Eprom tradizionali/parallele/PIC
// 0x220=USB_RELE scheda I/O USB e anche BT switch
// 0x230=PELUCHEUSB Giocattolo con MP3/musica, e anche GECO
// 0x231=USBSTEP per pannelli solari
// 0x240=Analizzatore logico USB
// 0x241=USBPower Alimentatore; anche BTAGallerini
// 0x250=VGA clock
// 0x251=TettoRGB e led rgb Rudi
// 0x252=LivioPizzuti's device
// 0x253=USBTime
// 0x254=USBRadio
// 0x255=Oriol's device
// 0x256=Geco tuning device
// 0x257=Manjeevan's device
// 0x258=joystick cockpit auto
// 0x259=joystick + keyboard nicola mogicato insta 2021
// 0x260=GSMDevice
// 0x261=Geiger (MSD + HID)
// 0x262=USBthermo (MSD + HID); anche bluetooth ktronic
// 0x263=Caricabatteria Pb(MSD + HID)
// 0x264=W1209 PIC (HID)
