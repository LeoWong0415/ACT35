/*
 *  Monitor.c - Interface between TW_Terminal2 and Firmware.
 *
 *  Copyright (C) 2011 Intersil Corporation
 *
 */

//*****************************************************************************
//
//								Monitor.c
//
//*****************************************************************************
//
//
//#include <intrins.h>

#include "config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "cpu.h"
#include "util.h"
#include "printf.h"			  
#include "i2c.h"
#include "osd.h"
#include "monitor.h"
#include "monitor_MCU.h"
#include "monitor_SPI.h"
#include "monitor_MENU.h"
#include "main.h"
#include "Measure.h"
#include "global.h"
#include "Settings.h"
#include "Remo.h"
#include "scaler.h"
#ifdef SUPPORT_DELTA_RGB
#include "DeltaRGB.h"
#endif

#include "spi.h"
#include "InputCtrl.h"
#include "ImageCtrl.h"
#include "OutputCtrl.h"
#include "TouchKey.h"
#include "measure.h"
#include "HDMI_EP9351.h"
#include "VAdc.h"
#include "DTV.h"
#include "EEPROM.H"
#include "main.h"
#include "menu8835B.h"

		BYTE 	DebugLevel = 0;
XDATA	BYTE	MonAddress = TW88I2CAddress;	
XDATA	BYTE	MonIndex;
XDATA	BYTE	MonRdata, MonWdata;
XDATA	BYTE	monstr[40];				// buffer for input string
XDATA	BYTE 	*argv[10];
XDATA	BYTE	argc=0;
		bit		echo=1;
		bit		access=1;


//=============================================================================
//
//=============================================================================
void Prompt(void)
{
#ifdef BANKING
	if ( MonAddress == TW88I2CAddress )
		Printf("\n[B%02bx]MCU_I2C[%02bx]>", BANKREG, MonAddress);
	else
#else
	if ( MonAddress == TW88I2CAddress )
		Printf("\nMCU_I2C[%02bx]>", MonAddress);
	else
#endif
		Printf("\nI2C[%02bx]>", MonAddress);
}

#if 0
void WaitUserInput(void)
{
	Printf("\nPress any key...");
	while ( !RS_ready() );
	Puts("\n");
}
#endif

void Mon_tx(BYTE ch)
{
	RS_tx(ch);
}
//=============================================================================
//
//=============================================================================

void SetMonAddress(BYTE addr)
{
	MonAddress = addr;
}

void MonReadI2CByte(void)
{
	////////////////////////////////////
	BYTE len;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
			
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if( argc>=2 ) 
		for(len=0; len<10; len++) 
			if( argv[1][len]==0 ) 
				break;

	if( len>2 ) {
		MonWdata = a2h( argv[1] ) / 0x100;
		MonIndex = 0xff;

		Printf("\nWrite %02bxh:%02bxh ", MonIndex, MonWdata);
		if ( MonAddress == TW88I2CAddress ) {
			WriteTW88Byte(MonIndex, MonWdata);
			MonRdata = ReadTW88Byte(MonIndex);
		}
		else {
			WriteI2CByte(MonAddress, MonIndex, MonWdata);
			MonRdata = ReadI2CByte(MonAddress, MonIndex);
		}
   		Printf("==> Read %02bxh:%02bxh", MonIndex, MonRdata);
		if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
	}
#endif
	////////////////////////////////////

	if( argc>=2 ) {
#ifndef SUPPORT_8BIT_CHIP_ACCESS
		for(len=0; len<10; len++) 
			if( argv[1][len]==0 ) 
				break;
		if( len>2 ) {
			MonPage = a2h( argv[1] );	 	//page+index
			MonRdata = ReadTW88(MonPage);
			if( echo )
				Printf("\nRead %03xh:%02bxh", MonPage, MonRdata);
			return;	
		}
#endif	
		MonIndex = a2h( argv[1] );
	}
	else	{
		Printf("   --> Missing parameter !!!");
		return;
	}

#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if ( MonAddress == TW88I2CAddress )
		MonRdata = ReadTW88(MonIndex);
#else
	if ( MonAddress == TW88I2CAddress )	{
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
#endif
	else
		MonRdata = ReadI2CByte(MonAddress, MonIndex);
	if( echo )
		Printf("\nRead %02bxh:%02bxh", MonIndex, MonRdata);	
	
	MonWdata = MonRdata;
}



void MonWriteI2CByte(void) 
{
	////////////////////////////////////
	BYTE len;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif

#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if( argc>=2 ) 
		for(len=0; len<10; len++) 
			if( argv[1][len]==0 ) 
				break;

	if( len>2 ) {
		MonWdata = a2h( argv[1] ) / 0x100;
		MonIndex = 0xff;

		Printf("\nWrite %02bxh:%02bxh ", MonIndex, MonWdata);
		if ( MonAddress == TW88I2CAddress ) {
			if(MonIndex==0xff) 	{ WriteTW88BytePage(MonWdata); }
			else				WriteTW88Byte(MonIndex, MonWdata);
			MonRdata = ReadTW88Byte(MonIndex);
		}
		else {
			WriteI2CByte(MonAddress, MonIndex, MonWdata);
			MonRdata = ReadI2CByte(MonAddress, MonIndex);
		}
   		Printf("==> Read %02bxh:%02bxh", MonIndex, MonRdata);
		if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
	}
#endif
	////////////////////////////////////

	if( argc<3 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	MonIndex = a2h( argv[1] );
	MonWdata = a2h( argv[2] );

#ifndef SUPPORT_8BIT_CHIP_ACCESS
	for(len=0; len<10; len++) 
		if( argv[1][len]==0 ) 
			break;
	if( len>2 ) {
		MonPage = a2h( argv[1] );	 	//page+index
		WriteTW88(MonPage,MonWdata);
		if( echo ) {
			Printf("\nWrite %03xh:%02bxh", MonPage, MonWdata);
			MonRdata = ReadTW88(MonPage);			
	   		Printf("==> Read %03xh:%02bxh", MonPage, MonRdata);
			if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
		}
		return;	
	}
#endif	
	
	if( echo ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
		Printf("\nWrite %02bxh:%02bxh ", MonIndex, MonWdata);
		if ( MonAddress == TW88I2CAddress ) {
			WriteTW88(MonIndex, MonWdata);
			MonRdata = ReadTW88(MonIndex);
		}
#else
		MonPage = ReadTW88Byte(0xff) << 8;
		Printf("\nWrite %03xh:%02bxh ", MonPage | MonIndex, MonWdata);
		if ( MonAddress == TW88I2CAddress ) {
			WriteTW88(MonPage | MonIndex, MonWdata);

			MonPage = ReadTW88Byte(0xff) << 8;
			MonRdata = ReadTW88(MonPage | MonIndex);
		}
#endif
		else {
			WriteI2CByte(MonAddress, MonIndex, MonWdata);
			MonRdata = ReadI2CByte(MonAddress, MonIndex);
		}
   		Printf("==> Read %02bxh:%02bxh", MonIndex, MonRdata);
		if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
	}
	else {
		if ( MonAddress == TW88I2CAddress ) {
			if(MonIndex==0xff) 	{ WriteTW88Page(MonWdata); }
			else				WriteTW88(MonIndex, MonWdata);
		}
		else {
			WriteI2CByte(MonAddress, MonIndex, MonWdata);
		}
	}
}

void MonIncDecI2C(BYTE inc)
{
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
	switch(inc){
		case 0:  MonWdata--;	break;
		case 1:  MonWdata++;	break;
		case 10: MonWdata-=0x10;	break;
		case 11: MonWdata+=0x10;	break;
	}

	if ( MonAddress == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
		WriteTW88(MonIndex, MonWdata);
		MonRdata = ReadTW88(MonIndex);
#else
		MonPage = ReadTW88Byte(0xff) << 8;
		WriteTW88(MonPage | MonIndex, MonWdata);
		MonRdata = ReadTW88(MonPage | MonIndex);
#endif
	}
	else {
		WriteI2CByte(MonAddress, MonIndex, MonWdata);
		MonRdata = ReadI2CByte(MonAddress, MonIndex);
	}

	if( echo ) {
		Printf("Write %02bxh:%02bxh ", MonIndex, MonWdata);
		Printf("==> Read %02bxh:%02bxh", MonIndex, MonRdata);
	}

	Prompt();
}

void MonDumpI2C(void)
{
	BYTE ToMonIndex;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
	int  cnt=7;

	if( argc>=2 ) MonIndex   = a2h(argv[1]);
	if( argc>=3 ) ToMonIndex = a2h(argv[2]);
	else          ToMonIndex = MonIndex+cnt;
	
	if ( ToMonIndex < MonIndex ) ToMonIndex = 0xFF;
	cnt = ToMonIndex - MonIndex + 1;

	if( echo ) {
		if ( MonAddress == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonIndex);
				Printf("\n==> Read %02bxh:%02bxh", MonIndex, MonRdata);
				MonIndex++;
			}
#else
			MonPage = ReadTW88Byte(0xff) << 8;
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonPage | MonIndex);
				Printf("\n==> Read %02bxh:%02bxh", MonIndex, MonRdata);
				MonIndex++;
			}
#endif
		}
		else {
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadI2CByte(MonAddress, MonIndex);
				Printf("\n==> Read %02bxh:%02bxh", MonIndex, MonRdata);
				//if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
				MonIndex++;
			}
		}
	}
	else {
		if ( MonAddress == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonIndex);
				MonIndex++;
			}
#else
			MonPage = ReadTW88Byte(0xff) << 8;
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonPage | MonIndex);
				MonIndex++;
			}
#endif
		}
		else {
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadI2CByte(MonAddress, MonIndex);
				MonIndex++;
			}
		}
	}
}

//-----------------------------------------------------------------------------

void MonNewReadI2CByte(void)
{
	BYTE Slave;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif

	if( argc>=3 ) MonIndex = a2h( argv[2] );
	else	{
		Printf("   --> Missing parameter !!!");
		return;
	}
	Slave = a2h(argv[1]);

#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if ( Slave == TW88I2CAddress )
		MonRdata = ReadTW88(MonIndex);
#else
	if ( Slave == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
#endif	
	else
		MonRdata = ReadI2CByte(Slave, MonIndex);

	if( echo )
		Printf("\n<R>%02bx[%02bx]=%02bx", Slave, MonIndex, MonRdata);
	MonWdata = MonRdata;
}

void MonNewWriteI2CByte(void)
{
	BYTE Slave;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif

	if( argc<4 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	
	Slave    = a2h( argv[1] );
	MonIndex = a2h( argv[2] );
	MonWdata = a2h( argv[3] );
	
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if ( Slave == TW88I2CAddress ) {
		WriteTW88(MonIndex, MonWdata);
		MonRdata = ReadTW88(MonIndex);
	}
#else
	if ( Slave == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		WriteTW88(MonPage | MonIndex, MonWdata);
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
#endif
	else {
		WriteI2CByte(Slave, MonIndex, MonWdata);
		MonRdata = ReadI2CByte(Slave, MonIndex);
   	}
	if( echo ) {
		Printf("\n<R>%02bx[%02bx]=%02bx", Slave, MonIndex, MonRdata);
	}
}

void MonNewDumpI2C(void)
{
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
	BYTE 	ToMonIndex, Slave;
	WORD	i;
	
	if( argc>=2 ) MonIndex = a2h(argv[2]);
	if( argc>=3 ) ToMonIndex = a2h(argv[3]);
	Slave = a2h(argv[1]);

	if( echo ) {
		if ( Slave == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
			for(i=MonIndex; i<=ToMonIndex; i++) {
				MonRdata = ReadTW88(i);
        		Printf("\n<R>%02bx[%02x]=%02bx", Slave, i, MonRdata);
			}
#else
			MonPage = ReadTW88Byte(0xff) << 8;
			for(i=MonIndex; i<=ToMonIndex; i++) {
				MonRdata = ReadTW88(MonPage | i);
        		Printf("\n<R>%02bx[%02x]=%02bx", Slave, i, MonRdata);
			}
#endif
		}
		else {
			for(i=MonIndex; i<=ToMonIndex; i++) {
				MonRdata = ReadI2CByte(Slave, i);
        		Printf("\n<R>%02bx[%02x]=%02bx", Slave, i, MonRdata);
			}
		}
	}
	else {
		if ( Slave == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
			for(i=MonIndex; i<=ToMonIndex; i++)
				MonRdata = ReadTW88(i);
#else
			MonPage = ReadTW88Byte(0xff) << 8;
			for(i=MonIndex; i<=ToMonIndex; i++)
				MonRdata = ReadTW88(MonPage | i);
#endif
		}
		else {
			for(i=MonIndex; i<=ToMonIndex; i++) {
				MonRdata = ReadI2CByte(Slave, i);
			}
		}
	}
}

//format:
// 	b 88 index startbit|endbit data
void MonWriteBit(void)
{
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
	BYTE mask, i, FromBit, ToBit,  MonMask, val;
	BYTE Slave;

	if( argc<5 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	Slave = a2h(argv[1]);

	MonIndex = a2h( argv[2] );
	FromBit  =(a2h( argv[3] ) >> 4) & 0x0f;
	ToBit    = a2h( argv[3] )  & 0x0f;
	MonMask  = a2h( argv[4] );

	if( FromBit<ToBit || FromBit>7 || ToBit>7) {
		Printf("\n   --> Wrong range of bit operation !!!");
		return;
	}
	
	mask = 0xff; 
	val=0x7f;
	for(i=7; i>FromBit; i--) {
		mask &= val;
		val = val>>1;
	}

	val=0xfe;
	for(i=0; i<ToBit; i++) {
		mask &= val;
		val = val<<1;
	}

#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if ( Slave == TW88I2CAddress ) {
		MonRdata = ReadTW88(MonIndex);
		MonWdata = (MonRdata & (~mask)) | (MonMask & mask);
				
		WriteTW88Byte(MonIndex, MonWdata);
		MonRdata = ReadTW88(MonIndex);
	}
#else
	if ( Slave == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
		MonWdata = (MonRdata & (~mask)) | (MonMask & mask);
				
		WriteTW88(MonPage | MonIndex, MonWdata);
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
#endif
	else {
		MonRdata = ReadI2CByte(Slave, MonIndex);
		MonWdata = (MonRdata & (~mask)) | (MonMask & mask);
				
		WriteI2CByte(Slave, MonIndex, MonWdata);
		MonRdata = ReadI2CByte(Slave, MonIndex);
	}
	if( echo )
		//TW_TERMINAL need this syntax
		Printf("\n<R>%02bx[%02bx]=%02bx", Slave, MonIndex, MonRdata);
}
// wait reg mask result max_wait
void MonWait(void)
{
	WORD i,max;
	BYTE reg, mask, result;
	if( argc<5 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	reg = a2h( argv[1] );
	mask = a2h( argv[2] );
	result = a2h( argv[3] );
	max = a2h( argv[4] );
	for(i=0; i < max; i++) {
		if((ReadTW88(reg) & mask)==result) {
			Printf("=>OK@%bd",i);
			break;
		}
		delay1ms(2);
	}
	if(i >= max)
		Printf("=>fail wait %bx %bx %bx %d->fail",reg,mask,result,max);
}

//=============================================================================
//			Help Message
//=============================================================================
void MonHelp(void)
{
	Puts("\n=======================================================");
	Puts("\n>>>     Welcome to Intersil Monitor  Rev 1.01       <<<");
	Puts("\n=======================================================");
	Puts("\n   R ii             ; Read data.(");
	Puts("\n   W ii dd          ; Write data.)");
	Puts("\n   D [ii] [cc]      ; Dump.&");
	Puts("\n   B AA II bb DD    ; Bit Operation. bb:StartEnd");
	Puts("\n   C aa             ; Change I2C address");
	Puts("\n   Echo On/Off      ; Terminal Echoing On/Off");
#ifdef EVB_30
	Puts("\n   HDMI ii nn       ; HDMI register ii read nn bytes");
	Puts("\n   HDINIT           ; HDMI initialize");
	Puts("\n   HDINFO           ; HDMI ouput timing");
#endif
	Puts("\n=======================================================");
	Puts("\n=== DEBUG ACCESS time init MCU SPI EE menu task [on] ====");
	Puts("\nM [CVBS|SVIDEO|COMP|PC|DVI|HDMI|BT656]      ; Change Input Mode");
	Puts("\nselect [CVBS|SVIDEO|COMP|PC|DVI|HDMI|BT656] ; select Input Mode");
	Puts("\ninit default                               : default for selected input");
	Puts("\nCheckAndSet                                ; CheckAndSet selected input");
	Puts("\n=======================================================");
#ifdef SUPPORT_I2CCMD_TEST_SLAVE
	Puts("\ntesti2cslave mode sec");
	Puts("\n	mode:0	read");
	Puts("\n	mode:1	read with SFR_EA");
	Puts("\n	mode:2	read and write");
	Puts("\n	mode:3	write");
	Puts("\n	mode:4	write x 20 times");
	Puts("\n	mode:5	read R4DC");
	Puts("\n	mode:6	write R4DC=0xAB");
	Puts("\ntesti2cpage page           ; set compare page");
	Puts("\n=======================================================");
#endif
	Puts("\n");

}
//=============================================================================
//
//=============================================================================
BYTE MonGetCommand(void)
{
	static BYTE comment=0;
	static BYTE incnt=0, last_argc=0;
	BYTE i, ch;
	BYTE ret=0;

	if( !RS_ready() ) return 0;
	ch = RS_rx();

	//----- if comment, echo back and ignore -----
	if( comment ) {
		if( ch=='\r' || ch==0x1b ) comment = 0;
		else { 
			Mon_tx(ch);
			return 0;
		}
	}
	else if( ch==';' ) {
		comment = 1;
		Mon_tx(ch);
		return 0;
	}

	//=====================================
	switch( ch ) {

	case 0x1b:
		argc = 0;
		incnt = 0;
		comment = 0;
		Prompt();
		return 0;

	//--- end of string
	case '\r':

		if( incnt==0 ) {
			Prompt();
			break;
		}

		monstr[incnt++] = '\0';
		argc=0;

		for(i=0; i<incnt; i++) if( monstr[i] > ' ' ) break;

		if( !monstr[i] ) {
			incnt = 0;
			comment = 0;
			Prompt();
			return 0;
		}
		argv[0] = &monstr[i];
		for(; i<incnt; i++) {
			if( monstr[i] <= ' ' ) {
				monstr[i]='\0';
     			 //Printf("(%s) ",  argv[argc]);
				i++;
				while( monstr[i]==' ' ) i++;
				argc++;
				if( monstr[i] ){
     			 argv[argc] = &monstr[i];
				}
			}
		}

		ret = 1;
		last_argc = argc;
		incnt = 0;
		
		break;

	//--- repeat command
	case '/':
		argc = last_argc;
		ret = 1;
		break;

	//--- repeat command without CR
	case '`':
	{
		BYTE i, j, ch;

		for(j=0; j<last_argc; j++) {
			for(i=0; i<100; i++) {
				ch = argv[j][i];
				if( ch==0 ) {
					if( j==last_argc-1 ) return 0;
					ch = ' ';
					RS_ungetch( ch );
					break;
				}
				else {
					RS_ungetch( ch );
				}
			}
		}
		break;
	}

	//--- back space
	case 0x08:
		if( incnt ) {
			incnt--;
			Mon_tx(ch);
			Mon_tx(' ');
			Mon_tx(ch);
		}
		break;

	//--- decreamental write
	case ',':
		if( incnt ) {
			Mon_tx(ch);
			monstr[incnt++] = ch;
		}
		else
			MonIncDecI2C(0);
		break;

	case '<':
		if( incnt ) {
			Mon_tx(ch);
			monstr[incnt++] = ch;
		}
		else
			MonIncDecI2C(10);
		break;

	//--- increamental write
	case '.':
		if( incnt ) {
			Mon_tx(ch);
			monstr[incnt++] = ch;
		}
		else
			MonIncDecI2C(1);
		break;

	case '>':
		if( incnt ) {
			Mon_tx(ch);
			monstr[incnt++] = ch;
		}
		else
			MonIncDecI2C(11);
		break;

	default:
		Mon_tx(ch);
		monstr[incnt++] = ch;
		break;
	}

	if( ret ) {
		comment = 0;
		last_argc = argc;
		return ret;
	}
	else {
		return ret;
	}
}

//*****************************************************************************
//				Monitoring Command
//*****************************************************************************

BYTE *MonString = 0;
extern CODE BYTE DataInitADC[];
extern CODE BYTE DataInitYUV[];
extern CODE BYTE DataInitNTSC[];
extern CODE BYTE DataInitDTV[];
extern CODE BYTE DataInitTCON[];

void Monitor(void)
{
	WORD wValue;
	DWORD dValue;

	if( MonString ) {																				  
		RS_ungetch( *MonString++ );
		if( *MonString==0 ) MonString = 0;
	}

	if( !MonGetCommand() ) return;

	//---------------- Write Register -------------------
	if( !stricmp( argv[0], "W" ) ) {
		MonWriteI2CByte();
	}
	else if( !stricmp( argv[0], ")" ) ) {
		MonNewWriteI2CByte();
	}
	//---------------- Read Register --------------------
	else if ( !stricmp( argv[0], "R" ) ) {
		MonReadI2CByte();
	}
	else if ( !stricmp( argv[0], "(" ) ) {
		MonNewReadI2CByte();
	}
	//---------------- Dump Register --------------------
	else if( !stricmp( argv[0], "D" ) ) {
		Puts("\ndump start");
		MonDumpI2C();
	}
	else if( !stricmp( argv[0], "&" ) ) {
		MonNewDumpI2C();
	}

	//---------------- Bit Operation --------------------
	else if( !stricmp( argv[0], "B" ) ) {// Write bits - B AA II bb DD
		MonWriteBit();
	}
	//---------------- Change I2C -----------------------
	else if( !stricmp( argv[0], "C" ) ) {
		MonAddress = a2h( argv[1] );
	}
	//---------------- wait -----------------------
	else if( !stricmp( argv[0], "wait" ) ) {
		MonWait();
	}
	//---------------- delay -----------------------
	else if( !stricmp( argv[0], "delay" ) ) {
		wValue = a2i( argv[1] );
		delay1ms(wValue);
	}
	//---------------- Help -----------------------------
	else if( !stricmp( argv[0], "H" ) || !stricmp( argv[0], "HELP" ) || !stricmp( argv[0], "?" ) ) {
		MonHelp();

	}
	//---------------- Read/Write Register for slow I2C  ------
#if 0
//!	//==>to test SW I2C Slave
//!	else if ( !stricmp( argv[0], "RR" ) ) {
//!		MonReadSlowI2CByte();	
//!	}
//!	else if( !stricmp( argv[0], "WW" ) ) {
//!		MonWriteSlowI2C();	
//!	}
#endif
#if 0
//!	//---------------- Read/Write Register for internal register ------
//!	else if ( !stricmp( argv[0], "RR" ) ) {
//!		MonReadInternalReg();	//==>see "mcu Rx addr"
//!	}
//!	else if( !stricmp( argv[0], "WW" ) ) {
//!		MonWriteInternalReg();	//==>see "mcu Wx addr"
//!	}
#endif

#ifdef SW_I2C_SLAVE
	else if( !stricmp( argv[0], "i2c" ) ) {

		extern BYTE dbg_sw_i2c_index[];
		extern BYTE dbg_sw_i2c_devid[];
		extern BYTE dbg_sw_i2c_regidx[];
		extern BYTE dbg_sw_i2c_data[];
		BYTE i;
		extern BYTE i2c_delay_start /*= 160*/;
		extern BYTE i2c_delay_restart /*= 2*/;
		extern BYTE i2c_delay_datasetup /*= 32*/;
		extern BYTE i2c_delay_clockhigh /*= 32*/;
		extern BYTE i2c_delay_datahold /*= 32*/;


		if(!stricmp(argv[1],"?")) {
			Printf("\nSW i2c ");

		}
		if(!stricmp(argv[1],"reset")) {
			dbg_sw_i2c_sda_count = 0;
			dbg_sw_i2c_scl_count = 0;
			sw_i2c_regidx = 0;
			for(i=0; i < 4; i++) {
				dbg_sw_i2c_index[i] = 0;
				dbg_sw_i2c_devid[i] = 0;
				dbg_sw_i2c_regidx[i] = 0;
				dbg_sw_i2c_data[i] = 0; 
			}
			i2c_delay_start = 160;
			i2c_delay_restart = 2;
			i2c_delay_datasetup = 32;
			i2c_delay_clockhigh = 32;
			i2c_delay_datahold = 32;
		}


		if(!stricmp(argv[1],"delay_start")) {
			if( argc>=3 ) 
				i2c_delay_start = (BYTE)a2h( argv[2] );
		}
		if(!stricmp(argv[1],"delay_restart")) {
			if( argc>=3 ) 
				i2c_delay_restart = (BYTE)a2h( argv[2] );
		}
		if(!stricmp(argv[1],"delay_datasetup")) {
			if( argc>=3 ) 
				i2c_delay_datasetup = (BYTE)a2h( argv[2] );
		}
		if(!stricmp(argv[1],"delay_clockhigh")) {
			if( argc>=3 ) 
				i2c_delay_clockhigh = (BYTE)a2h( argv[2] );
		}
		if(!stricmp(argv[1],"delay_datahold")) {
			if( argc>=3 ) 
				i2c_delay_datahold = (BYTE)a2h( argv[2] );
		}
		Printf("\ni2c_delay start:%bx restart:%bx datasetup:%bx clockhigh:%bx datahold:%bx",
			i2c_delay_start,i2c_delay_restart,i2c_delay_datasetup, i2c_delay_clockhigh,i2c_delay_datahold);  	


		Printf("\nsda_count:%bd scl_count:%d",dbg_sw_i2c_sda_count, dbg_sw_i2c_scl_count);
		Printf("\nsw_i2c_index:%bx sw_i2c_devid:%bx",sw_i2c_index, sw_i2c_devid);

		//Printf("\nstart:%bd",sw_i2c_index);
		//Printf(" devid:%02bx index:%02bx", sw_i2c_devid,	sw_i2c_regidx);
		for(i=0; i < dbg_sw_i2c_sda_count; i++) {
			Printf("\n%bd::start:%bd id:%02bx index:%02bx data:%02bx",i,dbg_sw_i2c_index[i],dbg_sw_i2c_devid[i], dbg_sw_i2c_regidx[i],  dbg_sw_i2c_data[i]);
		}
	}
#endif
	//---------------------------------------------------
/*
	else if( !stricmp( argv[0], "*" ) ) {
			
				if( argc==1 ) {
					Printf("\n  * 0 : Program default Loader");
					Printf("\n  * 1 : Program external Loader");
					Printf("\n  * 2 : Execute Loader");
				}
				else { 
					BYTE mode;
					mode = a2h(argv[1]);
					//Loader(mode);
				}
	}
*/
	//----------------------------------------------------
	//---------------- Init ------------------------------
	//----------------------------------------------------
	else if(!stricmp( argv[0], "init" ) ) {
	}
	//---------------- SPI Debug -------------------------
	else if( !stricmp( argv[0], "SPI" ) ) {
		MonitorSPI();
	}
	else if( !stricmp( argv[0], "SPIC" ) ) {
		MonitorSPIC();
	}
	//---------------- EEPROM Debug -------------------------
	//---------------- MENU Debug -------------------------
	else if( !stricmp( argv[0], "menu" ) ) {
	}
	//---------------- Debug Level ---------------------
	else if ( !stricmp( argv[0], "DEBUG" ) ) {
		if( argc==2 ) {
			DebugLevel = a2h(argv[1]);
		}
		Printf("\nDebug Level = %2bx", DebugLevel);
	}
	//---------------- Echo back on/off -----------------
	else if ( !stricmp( argv[0], "echo" ) ) {
		if( !stricmp( argv[1], "off" ) ) {
			echo = 0;
			Printf("\necho off");
		}
		else {
			echo = 1;
			Printf("\necho on");
		}
	}
	//---------------- Echo back on/off -----------------
	else if ( !stricmp( argv[0], "ACCESS" ) ) {
		if( !stricmp( argv[1], "0" ) ) {
			access = 0;
			Printf("\nAccess off");
			//disable interrupt.
			WriteTW88Page(PAGE0_GENERAL );
			WriteTW88(REG003, 0xFE );	// enable only SW interrupt
		}
		else {
			access = 1;
			Printf("\nAccess on");
		}
	}
	//---------------- MCU Debug -------------------------
	else if( !stricmp( argv[0], "MCU" ) ) {
		MonitorMCU();
	}
	
	//---------------- pclk -------------------------
	//	pclk 1080 means 108MHz
	//	pclk 27 means 27MHz
	else if( !stricmp( argv[0], "pclk" ) ) {
	}

	//====================================================
	// OTHER TEST ROUTINES
	//====================================================

	

	//==========================================
	// FontOSD Test
	//==========================================

	//==========================================
	// SpiOSD Test
	//==========================================


	//---------------- TOUCH Debug -------------------------
	//---------------- Touch Calibration -------------------------
	//---------------- Delta RGB Panel Test -------------------------
	//---------------- HDMI -------------------	
	//----------------------------------------------------
	//keep compiler happy.
	//----------------------------------------------------	
	else if(!stricmp( argv[0], "compiler" )) {
	}
	//----------------------------------------------------
	else {
		Printf("\nInvalid command...");
	}
	Prompt();
}


