/*
 *  Monitor.c - Interface between TW_Terminal2 and Firmware.
 *
 *  Copyright (C) 2012 Intersil Corporation
 *
 */

//*****************************************************************************
//
//								Monitor.c	MASTER VERSION
//
//*****************************************************************************
//
//
//#include <intrins.h>

#include "config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "global.h"
#include "cpu.h"
#include "printf.h"	
#include "util.h"
#include "monitor.h"
#include "monitor_MCU.h"
#include "monitor_SPI.h"
#include "monitor_MENU.h"
		  
#include "i2c.h"
#include "spi.h"

#include "main.h"
#include "SOsd.h"
#include "FOsd.h"
#include "Measure.h"
#include "Settings.h"
#include "Remo.h"
#include "scaler.h"
#ifdef SUPPORT_DELTA_RGB
#include "DeltaRGB.h"
#endif
#include "InputCtrl.h"
#include "ImageCtrl.h"
#include "OutputCtrl.h"
#include "TouchKey.h"
#include "measure.h"
#include "HDMI_EP9351.h"
#include "VAdc.h"
#include "DTV.h"
#include "EEPROM.H"
#include "SOsdMenu.h"

		BYTE 	DebugLevel = 0;
XDATA	BYTE	MonAddress = TW88I2CAddress;	
XDATA	BYTE	MonIndex;
XDATA	BYTE	MonRdata, MonWdata;
XDATA	BYTE	monstr[50];				// buffer for input string
XDATA	BYTE 	*argv[12];
XDATA	BYTE	argc=0;
		bit		echo=1;
		bit		access=1;

extern BYTE i2ccmd_reg_start;



#if defined(MODEL_TW8835_MASTER) && defined(SUPPORT_EXTMCU_ISP)
//desc:
//	download bin image from Host(or Master) to the SPIFlash on the slave.
//  Current TW8835 Host(or Master) cannot hold the big binary image,
//	So, if we assign a NULL on the *ptr, parameter, 
//	this I2cSpiProg() will download a test pattern to SPIFlash on the slave.
void DownloadSlaveSpiFlash(void)
{
	DWORD start;
	DWORD len;
	BYTE ret;
#ifdef USE_EXTMCU_ISP_GPIO
	BYTE reg4E1;
#endif
	if( argc < 3 ) {
		Printf("\nInvalue command...i2cisp start_addr len"); 
		return;
	} 
#ifdef USE_EXTMCU_ISP_GPIO
	//======================================
	// trick for PLAY mode(27MHz) on the Slave board. (Move to CPKPLL.72MHz)
	// FYI.
	// TW8835 I2C slave uses a MCUSPI clock. The default is a 27MHz.
	// TW8835 I2C slave can supports 400kHz on 27MHz MCUSPI clock.
	// If we change up the MCUSPI clock, TW8835 I2C slave can support more higher frequency.
	//
	// TW8835 I2C Master is implemented by SW.
	// If we use 72MHz MCUSPI clock with a cache, I2C routine uses more then 400KHz.
	//
	// If the slave is a 27MHz clock and the master is a 72MHz clock with cache,
	// the I2C routine will not be work.
	//
	// To draw the SPIOSD progress bar, we need high MCUSPI clock then 27MHz.
	// We need to change the MCUSPI clock before we go into the stop mode.
	//
	// so, we makes a slave fast in here.
	//======================================
#if 0	//method 1: working.
	PORT_I2CCMD_GPIO_MASTER=0;  //Start
	WriteSlowI2CByte(TW88I2CAddress, 0xFF,4);
	WriteSlowI2CByte(TW88I2CAddress,REG4E1,0x21);	//make salve fast
	PORT_I2CCMD_GPIO_MASTER=1;	 //Stop
#endif
#if 1	//method 2: working.
	reg4E1 = ReadTW88(REG4E1);
	if(reg4E1 & 0x20) {
		WriteTW88(REG4E1, reg4E1 & 0xCF);	//make me slow
	}
	WriteI2CByte(TW88I2CAddress, 0xFF,4);
	WriteI2CByte(TW88I2CAddress,REG4E1,0x21);	//make slave fast

	WriteTW88(REG4E1, reg4E1 | 0x20);		//make me fast
#endif
	//======================================
	// start
	// Make PORT_EXTMCU_ISP as low, it will make a stop mode on the slave board.
	// If it is a Host/Slave mode, the internal MCU was OFF on the slave mode, 
	// this I2cSpiProg should be work also.
	//======================================
	PORT_EXTMCU_ISP = 0;
	delay1ms(50);
#endif	//..USE_EXTMCU_ISP_GPIO

	start = a2h( argv[1] );
	len = a2h( argv[2] );
	ret = I2cSpiProg(start, NULL, len);
	if(ret)
		Printf("\nI2cSpiProg Failed [%bx]",ret);	
	//----------------------
	// If you want to download the multi bin file
	// use I2CSpiProg() again
	//----------------------
	//else {
	//	start += 0x10000;	//test 2nd bank	
	//	ret = I2cSpiProg(start, NULL, len);
	//	if(ret)
	//		Printf("\nI2cSpiProg Failed [%bx]",ret);	
	//}
	//----------------------
#ifdef USE_EXTMCU_ISP_GPIO
	PORT_EXTMCU_ISP = 1;	//recover
	//======================================
	// END
	// Set REG0D4[0]=1 to reboot the slave system.
	//======================================
	WriteI2CByte(TW88I2CAddress, 0xFF,0);
	WriteI2CByte(TW88I2CAddress, REG0D4, ReadI2CByte(TW88I2CAddress, REG0D4) | 0x01);


#if 1	//method 2: 
	//if I was slow, make me slow
	if((reg4E1 & 0x20) == 0)
		WriteTW88(REG4E1, reg4E1 & 0xCF);	//make me slow
#endif
#endif //..USE_EXTMCU_ISP_GPIO
}
#endif


void MonitorI2CCMD_Help(void)
{
	Puts("\n=======================================================");
	Puts("\n>>>     Welcome to Intersil I2CCMD  Rev 1.00       <<<");
	Puts("\n=======================================================");
	Puts("\n00 00               :Get current InputMain");
	Puts("\n00 01               :Get menu level");
	Puts("\n00 02               :Get Debug Level");
	Puts("\n00 03               :Get access value");
	Puts("\n00 04               :Get watchdog");
	Puts("\n00 05               :Get SPI_Buffer addr(12bit)");
	Puts("\n00 06               :Get SPI_Buffer len(12bit)");
	Puts("\n");
	Puts("\n01 00~0E            :ChangeInput");
	Puts("\n01 0F               :Next Input");
	Puts("\n01 10               :CheckAndSet");
	//Puts("\n01 11 00	:access=0. test only");
	//Puts("\n01 11 01	:access=1");
	Puts("\n01 12 [0|1|2|3]     : set debug level");
	Puts("\n01 80               :reset");
#ifdef USE_EXTMCU_ISP_I2CCMD
	Puts("\n01 8A               :goto stop mode");
#endif
	Puts("\n01 90               :disable watchdog");
	Puts("\n01 91               :enable watchdog");
	Puts("\n");
	Puts("\n02 00 addr          : read SFR");
	Puts("\n02 01 addr data     : write SFR");
	Puts("\n");
	Puts("\n03 00 addr addr     : read EEPROM");
	Puts("\n03 01 addr addr data: write EEPROM");
	Puts("\n");
	Puts("\n04 01               : NAVI KEY ENTER");
	Puts("\n04 02               : NAVI KEY UP");
	Puts("\n04 03               : NAVI KEY DOWN");
	Puts("\n04 04               : NAVI KEY LEFT");
	Puts("\n04 05               : NAVI KEY RIGHT");
	Puts("\n04 06               : MENU START");
	Puts("\n04 07               : MENU EXIT");
	Puts("\n=======================================================");
	Puts("\n");
}

//monitor I2CCMD
//used registers
//	REG009	result.
//	REG00F	I2C Interrupt. write 0x50
//	REG4DB	cmd0
//	REG4DC	cmd1
//	REG4DD	cmd2
//	REG4DE	cmd3
//	REG4DF	cmd4 or read data
//
void MonitorI2CCMD(void)
{
	BYTE value;
	volatile BYTE vData;
	BYTE i;
	BYTE cmd0,cmd1;
	WORD wvalue;

	if(argc < 3) {
		MonitorI2CCMD_Help();
		return;		
	}
	//-------------------
	// check BUSY
	WriteI2CByteToTW88(0xFF,0);	
	for(i=0; i < 100; i++) {
		vData = ReadI2CByteFromTW88(0x009); 
		delay1ms(2);
		if(vData != 0xB0)
			break;
	}
	if(i) 	Printf("I2CCMD wait %dms",(WORD)i*10);
	WriteI2CByteToTW88(0x009, 0x00); //clear


	WriteI2CByteToTW88(0xFF,4);
	cmd0 = a2h( argv[1] );
	WriteI2CByteToTW88(i2ccmd_reg_start+0/*I2CCMD_REG0*/,cmd0);
	cmd1 = a2h( argv[2] );
	WriteI2CByteToTW88(i2ccmd_reg_start+1,cmd1);
	if(argc >= 4) {
		value = a2h( argv[3] );
		WriteI2CByteToTW88(i2ccmd_reg_start+2,value);
	}
	if(argc >= 5) {
		value = a2h( argv[4] );
		WriteI2CByteToTW88(i2ccmd_reg_start+3,value);
	}
	if(argc >= 6) {
		value = a2h( argv[5] );
		WriteI2CByteToTW88(i2ccmd_reg_start+4,value);
	}	
	WriteI2CByteToTW88(0xFF,0);
	WriteI2CByteToTW88(REG00F,0x50);	//start

	//------------------------
	// read response
	WriteI2CByteToTW88(0xFF,0);
	for(i=0; i < 100; i++) {
		vData = ReadI2CByteFromTW88(0x009);
		delay1ms(5);
		if(vData != 0xB0 && vData != 0x00)
			break;
	}

	Printf("\nResult:%bx",vData);
	vData &= 0xF0;
	if(vData == 0xA0)	Puts(" ACK");
	if(vData == 0xB0)	Puts(" BUSY");
	if(vData == 0xC0)	Puts(" CHECK");
	if(vData == 0xD0)	Puts(" DONE");
	if(vData == 0xE0)	Puts(" ERR");
	if(vData == 0xF0)	Puts(" FAIL");

	Printf(" %dms",(WORD)i*5); //normally, use 5ms

	//------------------------
	// read result data
	if(cmd0==0x00) {
		WriteI2CByteToTW88(0xFF,4);
		if(cmd1==0x05 || cmd1==0x06) {
			wvalue = ReadI2CByteFromTW88(i2ccmd_reg_start+3); wvalue <<= 8;
			wvalue |= ReadI2CByteFromTW88(i2ccmd_reg_start+4);
			Printf(" read:%x",wvalue);
			
		}
		else {
			value = ReadI2CByteFromTW88(i2ccmd_reg_start+4);
			Printf(" read:%bx",value);
		}
	}
	else if(cmd0==0x02 || cmd0==0x03) {
		if(cmd1==0x00) {
			WriteI2CByteToTW88(0xFF,4);
			value = ReadI2CByteFromTW88(i2ccmd_reg_start+4);	
			Printf(" read:%bx",value);
		}
	}
}

//=============================================================================
// I2C TEST
//=============================================================================
#if defined(MODEL_TW8835_MASTER) && defined(SUPPORT_I2CCMD_TEST)

//------------------
// i2ctest.	old name was testi2c.
//
//------------------
void I2CTestRoutine(void)
{
	BYTE dat[8],accum;
	BYTE value;
	BYTE i,k,j=1;
	BYTE loop_cnt;
	BYTE cmd;
	
	if ( argc > 1 )	loop_cnt = a2h(argv[1]);
	else			loop_cnt = 1;
	for(i=0;i<loop_cnt; i++) {
		//---------------------
		//prepare test pattern
		dat[0] = i;
		dat[1] = i+1;
		dat[2] = i+2;
		dat[3] = i+3;
		dat[4] = i+4;
		dat[5] = (i<<4) | (i & 0x0F);
		dat[6] = 0xFF - i;
		dat[7] = 0x7F - i -1;
		for(k=0,accum=0; k < 8; k++)
			accum += dat[k];

		Printf("\nW %02bx %02bx %02bx %02bx %02bx %02bx %02bx %02bx:%02bx::",
			dat[0],dat[1],dat[2],dat[3],dat[4],dat[5],dat[6],dat[7],accum);

		//---------------
		// start
		//---------------
		WriteI2CByte(TW88I2CAddress, 0xFF, 0x00);
		value = ReadI2CByte(TW88I2CAddress,0xFF);
		if(value != 0x00) {
			Printf("\nREG_FF W:00 R:%02bx",value);
			WriteI2CByte(TW88I2CAddress,0xFF, 0x00);
			value = ReadI2CByte(TW88I2CAddress,0xFF);
			if(value != 0x00) {
				Printf(" W:00 R:%02bx",value);
				goto LABEL_FAIL;
			}
		}
		cmd = SW_INTR_EXTERN | 7;	//(8-1) byte + CHKSUM
		WriteI2CByte(TW88I2CAddress, 0x0F, cmd);
		delay1ms(1);
		value = ReadI2CByte(TW88I2CAddress,0x0F);
		if(value != cmd) {
			Printf("\nREG_0F W:%02bx R:%02bx",cmd,value);
			goto LABEL_FAIL;
		}
		for(j=0; j < 100; j++) {
			value = ReadI2CByte(TW88I2CAddress,0x09); 
			if(value == EXT_I2C_ACK1) //?ACK
				break;
		}
		Printf(" Start_ACK:%bd",j);

		//---------------
		// write
		//---------------
		WriteI2CByte(TW88I2CAddress,0xFF,4);
		for(j=0; j < 8; j++) {
			WriteI2CByte(TW88I2CAddress,0xD0+j,dat[j]);
		}
		WriteI2CByte(TW88I2CAddress,0xDF,accum);
		//----------------
		//check
		for(j=0; j < 8; j++) {
			value=ReadI2CByte(TW88I2CAddress,0xD0+j);
			if(value!=dat[j]) {
				Printf(" %bx:W%bx:R%bx",j,dat[j],value);
				value=ReadI2CByte(TW88I2CAddress,0xD0+j);
				if(value!=dat[j])
					Printf(" %bx:W%bx:R%bx",j,dat[j],value);
			}
		}
		value=ReadI2CByte(TW88I2CAddress,0xDF);
		if(value!=accum) {
			Printf(" F:W%bx:R%bx",accum,value);
			value=ReadI2CByte(TW88I2CAddress,0xDF);
			if(value!=accum) 
				Printf(" F:W%bx:R%bx",accum,value);
		}


		//---------------
		// Write DONE, request answer
		//---------------
		WriteI2CByte(TW88I2CAddress, 0xFF, 0x00);
		value = ReadI2CByte(TW88I2CAddress,0xFF);
		if(value != 0x00) {
			Printf("\nREG_FF W:00 R:%02bx",value);
			WriteI2CByte(TW88I2CAddress,0xFF, 0x00);
			value = ReadI2CByte(TW88I2CAddress,0xFF);
			if(value != 0x00) {
				Printf(" REG_FF W:00 R:%02bx",value);
				goto LABEL_FAIL;
			}
		}
		cmd = SW_INTR_EXTERN | EXT_I2C_REQ_ANSWER;
		WriteI2CByte(TW88I2CAddress, 0x0F, cmd);
		value = ReadI2CByte(TW88I2CAddress,0x0F);
		if(value != cmd) {
			Printf(" REG_0F W:%02bx R:%02bx",cmd,value);
		} 
		for(j=0; j < 100; j++) {
			value = ReadI2CByte(TW88I2CAddress,0x09); 
			if(value == EXT_I2C_ACK2) { //?ACK
				Printf(" Done_ACK:%bd",j);
				break;
			}
			else if(value == EXT_I2C_NAK2) {//NAK
				Printf(" Done_NAK:%bd",j);
				break;
			}
		}
		//------------------------
		//release
		// after this, do not use WriteI2CByte(TW88I2CAddress,,) ReadI2CByte(TW88I2CAddress,).
		if(loop_cnt==1) {
			//skip. to check timer expire.
		}
		else {
			cmd = SW_INTR_EXTERN | EXT_I2C_DONE;
			WriteI2CByte(TW88I2CAddress, 0x0F, cmd);
		}
	}
LABEL_FAIL:
  	delay1ms(1);

}
#endif


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
	BYTE len;
	WORD MonPage;
			
	if( argc>=2 ) {
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
		MonIndex = a2h( argv[1] );
	}
	else	{
		Printf("   --> Missing parameter !!!");
		return;
	}

	if ( MonAddress == TW88I2CAddress )	{
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
#ifdef MODEL_TW8835_MASTER
	else if( MonAddress == TW88SalveI2CAddress )
		MonRdata = ReadI2CByte(MonAddress+2, MonIndex);
#endif
	else
		MonRdata = ReadI2CByte(MonAddress, MonIndex);
	if( echo )
		Printf("\nRead %02bxh:%02bxh", MonIndex, MonRdata);	
	
	MonWdata = MonRdata;
}

void MonWriteI2CByte(void) 
{
	BYTE len;
	WORD MonPage;

	if( argc<3 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	MonIndex = a2h( argv[1] );
	MonWdata = a2h( argv[2] );

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
	
	if( echo ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		Printf("\nWrite %03xh:%02bxh ", MonPage | MonIndex, MonWdata);
		if ( MonAddress == TW88I2CAddress ) {
			WriteTW88(MonPage | MonIndex, MonWdata);

			MonPage = ReadTW88Byte(0xff) << 8;
			MonRdata = ReadTW88(MonPage | MonIndex);
		}
#ifdef MODEL_TW8835_MASTER
		else if( MonAddress == TW88SalveI2CAddress ) {
			WriteI2CByte(MonAddress+2, MonIndex, MonWdata);
			MonRdata = ReadI2CByte(MonAddress+2, MonIndex);
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
#ifdef MODEL_TW8835_MASTER
		else if( MonAddress == TW88SalveI2CAddress )
			WriteI2CByte(MonAddress+2, MonIndex, MonWdata);
#endif
		else {
			WriteI2CByte(MonAddress, MonIndex, MonWdata);
		}
	}
}

void MonIncDecI2C(BYTE inc)
{
	WORD MonPage;

	switch(inc){
		case 0:  MonWdata--;	break;
		case 1:  MonWdata++;	break;
		case 10: MonWdata-=0x10;	break;
		case 11: MonWdata+=0x10;	break;
	}

	if ( MonAddress == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		WriteTW88(MonPage | MonIndex, MonWdata);
		MonRdata = ReadTW88(MonPage | MonIndex);
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
	WORD MonPage;
	int  cnt=7;

	if( argc>=2 ) MonIndex   = a2h(argv[1]);
	if( argc>=3 ) ToMonIndex = a2h(argv[2]);
	else          ToMonIndex = MonIndex+cnt;
	
	if ( ToMonIndex < MonIndex ) ToMonIndex = 0xFF;
	cnt = ToMonIndex - MonIndex + 1;

	if( echo ) {
		if ( MonAddress == TW88I2CAddress ) {
			MonPage = ReadTW88Byte(0xff) << 8;
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonPage | MonIndex);
				Printf("\n==> Read %02bxh:%02bxh", MonIndex, MonRdata);
				MonIndex++;
			}
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
			MonPage = ReadTW88Byte(0xff) << 8;
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonPage | MonIndex);
				MonIndex++;
			}
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
	WORD MonPage;

	if( argc>=3 ) MonIndex = a2h( argv[2] );
	else	{
		Printf("   --> Missing parameter !!!");
		return;
	}
	Slave = a2h(argv[1]);

	if ( Slave == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
	else
		MonRdata = ReadI2CByte(Slave, MonIndex);

	if( echo )
		Printf("\n<R>%02bx[%02bx]=%02bx", Slave, MonIndex, MonRdata);
	MonWdata = MonRdata;
}

void MonNewWriteI2CByte(void)
{
	BYTE Slave;
	WORD MonPage;

	if( argc<4 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	
	Slave    = a2h( argv[1] );
	MonIndex = a2h( argv[2] );
	MonWdata = a2h( argv[3] );
	
	if ( Slave == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		WriteTW88(MonPage | MonIndex, MonWdata);
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
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
	WORD MonPage;
	BYTE 	ToMonIndex, Slave;
	WORD	i;
	
	if( argc>=2 ) MonIndex = a2h(argv[2]);
	if( argc>=3 ) ToMonIndex = a2h(argv[3]);
	Slave = a2h(argv[1]);

	if( echo ) {
		if ( Slave == TW88I2CAddress ) {
			MonPage = ReadTW88Byte(0xff) << 8;
			for(i=MonIndex; i<=ToMonIndex; i++) {
				MonRdata = ReadTW88(MonPage | i);
        		Printf("\n<R>%02bx[%02x]=%02bx", Slave, i, MonRdata);
			}
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
			MonPage = ReadTW88Byte(0xff) << 8;
			for(i=MonIndex; i<=ToMonIndex; i++)
				MonRdata = ReadTW88(MonPage | i);
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
	WORD MonPage;

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

	if ( Slave == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
		MonWdata = (MonRdata & (~mask)) | (MonMask & mask);
				
		WriteTW88(MonPage | MonIndex, MonWdata);
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
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
	Puts("\n=======================================================");
	Puts("\n=== DEBUG ACCESS time MCU ====");
	Puts("\nI2CCMD ");
#if defined(MODEL_TW8835_MASTER) && defined(SUPPORT_EXTMCU_ISP)
	Puts("\ni2cisp addr len		; download test pattern to SPIFlash(test only)");
#endif
#if defined(MODEL_TW8835_MASTER) && defined(SUPPORT_I2CCMD_TEST)
	Puts("\ni2ctest 	loop		;normal. use i2ctest cmd on slave also");
#endif
	Puts("\n=======================================================");
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

#if 1 // OSPOSD Move test
void WaitVBlank1(void)
{
	//XDATA	BYTE i;
	WORD loop;
	volatile BYTE vdata;

	WriteI2CByte(0x8a,0xff,0x00);

	PORT_DEBUG = 0;
	WriteI2CByte( 0x8a,0x02, 0xff ); //clear
	loop = 0;
	while(1) {
		vdata = ReadI2CByte( 0x8a,0x02 );
		//Printf("\nREG002:%bx", vdata);		
		if(vdata & 0x40) 
			break;
		loop++;
		if(loop > 0xFFFE) {
			ePrintf("\nERR:WaitVBlank");
			break;
		}
	}
	PORT_DEBUG = 1;
	//Printf("\nWaitVBlank1 loop:%d", loop);
}
#endif

//*****************************************************************************
//				Monitoring Command
//*****************************************************************************

BYTE *MonString = 0;

void Monitor(void)
{
	WORD wValue;

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

	//----------------------------------------------------
	//---------------- Init ------------------------------
	//----------------------------------------------------
	//-----input select------------------------------------------
	//---------------- Change Input Mode ---------------------
	// M [CVBS|SVIDEO|COMP|PC|DVI|HDMI|BT656]
	//--------------------------------------------------------      
	//---------------- CheckAndSet ---------------------
	//---------------- check -------------------------

#if defined(MODEL_TW8835_MASTER)
	//---------------- I2CCMD ---------------------
	else if ( !stricmp( argv[0], "I2CCMD" ) ) {
		MonitorI2CCMD();
	}
#endif
#if defined(MODEL_TW8835_MASTER) && defined(SUPPORT_I2CCMD_TEST)
	//---------------- I2CCMD ---------------------
	else if ( !stricmp( argv[0], "I2CTEST" ) ) {
		I2CTestRoutine();
	}
	
#endif
	//---------------- SPI Debug -------------------------
	//---------------- EEPROM Debug -------------------------
	//---------------- MENU Debug -------------------------
	//---------------- Font Osd Debug -------------------------
	//---------------- SPI Osd Debug -------------------------
	//---------------- Debug Level ---------------------
	//---------------- Echo back on/off -----------------
	//---------------- Echo back on/off -----------------
	//---------------- task on/off -----------------
	//---------------- System Clock Display -----------------
	else if ( !stricmp( argv[0], "time" ) ) {
			Printf("\nSystem Clock: %ld:%5bd", SystemClock, tic01);
	}
	//---------------- MCU Debug -------------------------
	//---------------- HDMI test -------------------------
#if defined(MODEL_TW8835_MASTER) && defined(SUPPORT_EXTMCU_ISP)
	//---------------- Host(or Master) to Slave SPIFLASH download -------------------------
	else if( !stricmp( argv[0], "I2CISP" ) ) {	//flash download
		DownloadSlaveSpiFlash();
	}
#endif
	

	//==========================================
	// FontOSD Test
	//==========================================
	//==========================================
	// SpiOSD Test
	//==========================================
#if 1 // OSPOSD Move test
	else if(!stricmp( argv[0], "move")) {
		WORD i;
		BYTE xpositionH,xpositionL;
		WORD delay;
		BYTE win;

		if(argc < 2) {
			win = 1;
			if(SFR_CACHE_EN) 	delay = 50;
			else				delay=10;
		}
		else {
			win = a2h(argv[1]);
			if(win==0)	win = 1;
			if(argc < 3)
				delay = a2i(argv[2]);

		}

		for(i = 0; i< 801; i+= 1) {
			xpositionH = i >>8;
			xpositionL = (BYTE)i & 0xff;
			
			WaitVBlank1();
			//delay1ms(8); //test
			PORT_DEBUG = 0;
			WriteI2CByte(0x8a,0xff, 0x04);
			WriteI2CByte(0x8a,REG431+0x10*win, xpositionH);	//WriteI2CByte(0x8a,0x41, xpositionH);
			WriteI2CByte(0x8a,REG432+0x10*win, xpositionL);	//WriteI2CByte(0x8a,0x42, xpositionL);
			WriteI2CByte(0x8a,0xff, 0x00);
			PORT_DEBUG = 1;

			delay1ms(delay);
		}
	}
	else if(!stricmp( argv[0], "move1")) {
		WORD i;
		BYTE xpositionH,xpositionL;
		WORD delay;
		BYTE win;

		if(argc < 2) {
			win = 1;
			if(SFR_CACHE_EN) 	delay = 50;
			else				delay=10;
		}
		else {
			win = a2h(argv[1]);
			if(win==0)	win = 1;
			if(argc < 3)
				delay = a2i(argv[2]);

		}

		//for(i = 0; i< 801; i+= 1) {
		i =0;
		while(!RS_ready()) {
			i+=4;
			i %= 16;

			xpositionH = i >>8;
			xpositionL = (BYTE)i & 0xff;
			
			WaitVBlank1();
			//delay1ms(8); //test
			PORT_DEBUG = 0;
			WriteI2CByte(0x8a,0xff, 0x04);
			WriteI2CByte(0x8a,REG431+0x10*win, xpositionH);	//WriteI2CByte(0x8a,0x41, xpositionH);
			WriteI2CByte(0x8a,REG432+0x10*win, xpositionL);	//WriteI2CByte(0x8a,0x42, xpositionL);
			WriteI2CByte(0x8a,0xff, 0x00);
			PORT_DEBUG = 1;

			delay1ms(delay);
		}
	}
	else if(!stricmp( argv[0], "move2")) {
		WORD i;
		BYTE xpositionH,xpositionL;
		WORD delay;
		BYTE win;

		if(argc < 2) {
			win = 1;
			if(SFR_CACHE_EN) 	delay = 50;
			else				delay=10;
		}
		else {
			win = a2h(argv[1]);
			if(win==0)	win = 1;
			if(argc < 3)
				delay = a2i(argv[2]);

		}

		//for(i = 0; i< 801; i+= 1) {
		i =0;
		while(!RS_ready()) {
			i+=4;
			i %= 16;

			xpositionH = i >>8;
			xpositionL = (BYTE)i & 0xff;
			
			WaitVBlank1();
			//delay1ms(8); //test
			PORT_DEBUG = 0;
			WriteI2CByte(0x8a,0xff, 0x04);
			WriteI2CByte(0x8a,REG431+0x10*win, xpositionH);	//WriteI2CByte(0x8a,0x41, xpositionH);
			WriteI2CByte(0x8a,REG432+0x10*win, xpositionL);	//WriteI2CByte(0x8a,0x42, xpositionL);
			WriteI2CByte(0x8a,0xff, 0x00);
			PORT_DEBUG = 1;

			delay1ms(delay);
		}
	}
#endif
	//---------------- TOUCH Debug -------------------------
	//---------------- Delta RGB Panel Test -------------------------
	
	//----------------------------------------------------
	//make compiler happy.
	//Please, DO NOT EXECUTE
	//----------------------------------------------------	
	else if(!stricmp( argv[0], "compiler" )) {
		BYTE temp1,temp2;

		//util.c
		TWitoa(temp1, &temp2);
		TWhtos(temp1, &temp2);
		TWstrlen(&temp1);
		TWstrcpy(&temp1,&temp2);
		TWstrcat(&temp1,&temp2);
		IsDigit(temp1);

		//printf.c
#ifdef DEBUG
		dPrintf("\n");
		wPrintf("\n");
		dPuts("\n");
		wPuts("\n");
		ePuts("\n");
#endif

		//i2c.c
		ReadSlowI2CByte(0x8A, 0x00);
		WriteSlowI2CByte(0x8A, 0x00, 0x00);
		I2CDeviceInitialize(&temp1,temp2);

		//spi.c
		SPI_SectorErase( 0 );
		SPI_BlockErase(0);
		SPI_PageProgram(0, 0/*NULL*/, 0);
		//SPI_dump(0);
		//SPI_Status(); 
		SpiFlashDmaRead(0,0,0,0);
		SpiFlashDmaRead2XMem(0,0,0);
	}
	//----------------------------------------------------
	else {
		Printf("\nInvalid command...");
	}
	Prompt();
}


