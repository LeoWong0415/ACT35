/**
 * @file
 * Host.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	ExtI2C host only file
 * @section DESCRIPTION
 *	Host mode code for MODEM_TW8836_EXTI2C or MODEL_TW8836_MASTER
 * 
 ******************************************************************************
 */

#include <intrins.h>
#include "Config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "Global.h"
#include "CPU.h"
#include "Printf.h"
#include "util.h"
#include "Monitor.h"

#include "I2C.h"
#include "SPI.h"

#include "main.h"
#include "host.h"	//<<==
#include "Remo.h"
#include "TouchKey.h"
#include "eeprom.h"

#include "Settings.h"
#include "InputCtrl.h"
#include "decoder.h"
#include "Scaler.h"
#include "ImageCtrl.h"
#include "OutputCtrl.h"
#include "VAdc.h"
#include "DTV.h"
#include "measure.h"

#ifdef SUPPORT_HDMI_SiIRX
#include <hal_cp9223.h>
#include <SiI_config.h>
#include <amf.h>
#include <infofrm.h>
#include <registers.h>
#include <debug_cmd.h>
#include <CEC.h>
#endif
#ifdef SUPPORT_HDMI_EP9351
#include "hdmi_ep9351.h"
#endif

#include "SOsd.h"
#include "FOsd.h"
#include "SpiFlashMap.h"
#include "SOsdMenu.h"
#include "Demo.h"


extern BYTE SysNoInitMode;



#ifndef MODEL_TW8835_EXTI2C
//----------------------------
/**
* Trick for Bank Code Segment
*/
//----------------------------
CODE BYTE DUMMY_HOST_CODE;
void Dummy_HOST_func(void)
{
	BYTE temp;
	temp = DUMMY_HOST_CODE;
}
#else //..MODEL_TW8835_EXTI2C
//=============================================================================
// from MAIN.C
//=============================================================================

//=============================================================================
// Init QuadIO SPI Flash			                                               
//=============================================================================

void InitHostCore(BYTE fPowerUpBoot)
{
	if(fPowerUpBoot) {
		//check port 1.5. if high, it is a skip(NoInit) mode.
		SysNoInitMode = SYS_MODE_NORMAL;
#ifdef MODEL_TW8836FPGA
		if(1)	
#else
		if(PORT_NOINIT_MODE == 1)	
#endif
		{
			SysNoInitMode = SYS_MODE_NOINIT;
			//turn on the SKIP_MODE.
			access = 0;
					
			McuSpiClkSelect(MCUSPI_CLK_27M);
			return;
		}
	}

	Puts("\nInitHostCore");	
	//----- Set SPI mode
	SpiFlashVendor = SPIHost_QUADInit();
	SPIHost_SetReadModeByRegister(SPI_READ_MODE);		// Match DMA READ mode with SPI-read

	//----- Enable Chip Interrupt

	WriteHostPage(PAGE0_GENERAL );
	WriteHost(REG002, 0xFF );	// Clear Pending Interrupts
	WriteHost(REG003, 0xFE );	// enable SW. disable all other interrupts

	//enable remocon interrupt.
	EnableRemoInt();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void InitHostSystem(BYTE fPowerUpBoot)
{
	BYTE ee_mode;

	//check EEPROM
	ee_mode = CheckEEPROM();
	if(ee_mode==1) {
		//---------- if FW version is not matched, initialize EEPROM data -----------
		InitHostWithNTSC();
		
		DebugLevel = 3;
	  
		#ifdef USE_SFLASH_EEPROM
		EE_Format();
		EE_FindCurrInfo();
		#endif
	
		InputMain = 0xff;	// start with saved input
		InitializeEE();	 	//save all default EE values.
	
		DebugLevel = 0;
		SaveDebugLevelEE(DebugLevel);
	 
		ee_mode = 0;
	}

	//read debug level
	DebugLevel = GetDebugLevelEE();
	if((DebugLevel==0) && (fPowerUpBoot))
		Printf("\n===> Debugging was OFF (%02bx)", DebugLevel);
	else 
		ePrintf("\n===> Debugging is ON (%02bx)", DebugLevel);

	ePrintf("\nInitHostSystem(%bd)",fPowerUpBoot);

	//Init HW with default
	InitHostWithNTSC();
	FP_Host_GpioDefault();
	SSPLL_Host_PowerUp(ON);
	PrintSystemClockMsg("SSPLL_Host_PowerUp");

	WriteHostPage(0);
	WriteHost(REG008, 0x89);	//Output enable


#if 0
	//TODO: current method does not working.
	//		try it with stable SSPLL code. See the ...

	//McuSpiClkSelect(MCUSPI_CLK_PCLK);
	WriteHostPage(PAGE4_CLOCK);
	WriteHost(REG4E1, 0x20 | (ReadHost(REG4E1) & 0x0F));
#endif

	//dump clock
	DumpClock(1);

	Printf("\nNow, enable the slave system....");
}


//=============================================================================
// from SPI.C
//=============================================================================
BYTE SPIHOSTCMD_x_READ      = 0x03;
BYTE SPIHOSTCMD_x_BYTES		= 5;
BYTE SpiFlashHostVendor;

#define SPIHOSTDMA_READDATA(n)		ReadHost(REG4D0+n)

//========================================================
// SpiFlashHost
//========================================================
//void SpiFlashHostCmdRead(BYTE dest)
//{
//	WriteHost(REG4C3, dest << 6 | SPIHOSTCMD_x_BYTES);
//	WriteHost(REG4CA, SPIHOSTCMD_x_READ);
//}

void SpiFlashHostDmaDestType(BYTE dest, BYTE access_mode)
{
	BYTE dat;
	dat = ReadHost(REG4C3) & 0x0F;
	dat |= (dest << 6);
	dat |= (access_mode << 4);
	WriteHost(REG4C3, dat);
}

void SpiFlashHostSetCmdLength(BYTE len)
{
	WriteHost(REG4C3, (ReadHost(REG4C3) & 0xF0) | len);
}
void SpiFlashHostDmaStart(BYTE fWrite, BYTE fBusy)
{
	BYTE dat;
  
	dat = 0x01;					//start
	if(fWrite) 	dat |= 0x02;	//write
	if(fBusy)	dat |= 0x04;	//busy
	WriteHost(REG4C4, dat);
}
void SpiFlashHostDmaBuffAddr(WORD addr)
{
	WriteHost(REG4C6, (BYTE)(addr >> 8));	//page
	WriteHost(REG4C7, (BYTE)addr);			//index
}
void SpiFlashHostDmaReadLen(DWORD len)
{
	WriteHost(REG4DA, len>>16 );
	WriteHost(REG4C8, len>>8 );
	WriteHost(REG4C9, len );
}
void SpiFlashHostDmaReadLenByte(BYTE len_l)
{
	WriteHost(REG4C9, len_l );
}
void SpiFlashHostCmd(BYTE cmd, BYTE cmd_len)
{
	WriteHost(REG4CA, cmd);
	SpiFlashHostSetCmdLength(cmd_len);
}
void SpiFlashHostDmaFlashAddr(DWORD addr)
{
	WriteHost(REG4CB, (BYTE)(addr >> 16));
	WriteHost(REG4CC, (BYTE)(addr >> 8));
	WriteHost(REG4CD, (BYTE)(addr));
}
#if 0
void SpiFlashHostDmaRead(BYTE dest_type,WORD dest_loc, DWORD src_loc, WORD size)
{
	WriteHostPage(PAGE4_SPI);
	SpiFlashHostDmaDestType(dest_type,0);
	SpiFlashHostCmd(SPIHOSTCMD_x_READ, SPIHOSTCMD_x_BYTES);
	SpiFlashHostDmaFlashAddr(src_loc);
	SpiFlashHostDmaBuffAddr((WORD)dest_loc);
	SpiFlashHostDmaReadLen(size);						
	SpiFlashHostDmaStart(SPIDMA_READ, SPIDMA_BUSYCHECK);
}
#endif
void SpiFlashHostDmaRead2XMem(BYTE *dest_loc, DWORD src_loc, WORD size)
{
	WriteHostPage(PAGE4_SPI);
	SpiFlashHostDmaDestType(DMA_DEST_MCU_XMEM,0);
	SpiFlashHostCmd(SPIHOSTCMD_x_READ, SPIHOSTCMD_x_BYTES);
	SpiFlashHostDmaFlashAddr(src_loc);
	SpiFlashHostDmaBuffAddr((WORD)dest_loc);
	SpiFlashHostDmaReadLen(size);						
	SpiFlashHostDmaStart(SPIDMA_READ, SPIDMA_BUSYCHECK);
}
/**
* only for (dest_type==DMA_DEST_MCU_XMEM)
*/
//void SpiFlashDmaReadForXMem(BYTE dest_type,BYTE *dest_loc, DWORD src_loc, WORD size)
void SpiFlashDmaReadForXMem(/*BYTE dest_type,*/BYTE *dest_loc, DWORD src_loc, WORD size)
{
	WORD i,j,  r_size;
	BYTE *ptr;
	BYTE dat;
	BYTE r4c1;

#ifdef DEBUG_SPI
	//dPrintf("\nSpiFlashDmaReadForXMem(%bx,%lx,%lx,%x)",dest_type,dest_loc,src_loc,size);
	dPrintf("\nSpiFlashDmaReadForXMem(%lx,%lx,%x)",dest_loc,src_loc,size);
#endif
	//if(dest_type !=DMA_DEST_MCU_XMEM)
	//	return;

	ptr = (BYTE *)dest_loc;
	dPrintf("  ptr:%lx",ptr);



	WriteTW88Page(PAGE4_SPI);
	r4c1 = ReadTW88(REG4C1);
	WriteTW88(REG4C1,r4c1 & ~0x01); 
	//WriteTW88(REG4C1,r4c1 | 0x01); 
	SpiFlashDmaDestType(DMA_DEST_CHIPREG,0);
	SpiFlashCmd(0x0B, 0x05 /*SPICMD_x_READ, SPICMD_x_BYTES*/);	  //SPICMD_FASTREAD
	SpiFlashDmaBuffAddr(DMA_BUFF_REG_ADDR);
	for(i=0; i < size; i+=8) {
		SpiFlashDmaFlashAddr(src_loc+i);
		if(i+8 <= size)  r_size = 8;
		else			 r_size = size - i;
		SpiFlashDmaReadLen(r_size);
		//WaitVBlank(1);
		WriteTW88(REG4C0, (ReadTW88(REG4C0) & ~0x07) | 1);
		WriteTW88(REG4C4, 0x05);	//or 0x01						//DMA Read

		while(ReadI2CByte(TW88I2CAddress,REG4C4) & 0x01);		//SpiFlashDmaWait

		WriteTW88(REG4C0, (ReadTW88(REG4C0) & ~0x07) | 5);
		dPrintf("\n%06lx:%x:",src_loc+i,r_size);
		for(j=0; j < r_size; j++) {
			*ptr++ = dat = ReadTW88(REG4D0+j);
			dPrintf("%02bx ",dat);
		}
	}
	WriteTW88(REG4C1,r4c1);
	return;
}

//#define SpiFlashHostCmdRead(dest)  SpiFlashCmdRead(dest)
//#define SpiFlashHostDmaDestType(dest, access_mode)  SpiFlashDmaDestType(dest, access_mode)
//#define SpiFlashHostDmaRead(dest_type,dest_loc, src_loc, size)	SpiFlashDmaRead(dest_type,dest_loc, src_loc, size)

//=============================================================================
//	SPIHOST_
//=============================================================================
void SPIHost_SetReadModeByRegister( BYTE mode )
{
	WriteHostPage(PAGE4_SPI);
	WriteHost(REG4C0, (ReadHost(REG4C0) & ~0x07) | mode);

	switch( mode ) {
		case 0:	//--- Slow
			SPIHOSTCMD_x_READ	= 0x03;		//normal read	
			SPIHOSTCMD_x_BYTES	= 4;
			break;
		case 1:	//--- Fast
			SPIHOSTCMD_x_READ	= 0x0b;		//SPICMD_FASTREAD	
			SPIHOSTCMD_x_BYTES	= 5;
			break;
		case 2:	//--- Dual
			SPIHOSTCMD_x_READ	= 0x3b;
			SPIHOSTCMD_x_BYTES	= 5;
			break;
		case 3:	//--- Quad
			SPIHOSTCMD_x_READ	= 0x6b;	
			SPIHOSTCMD_x_BYTES	= 5;
			break;
		case 4:	//--- Dual-IO
			SPIHOSTCMD_x_READ	= 0xbb;	
			SPIHOSTCMD_x_BYTES	= 5;
			break;
		case 5:	//--- Quad-IO
			SPIHOSTCMD_x_READ	= 0xeb;	 
			SPIHOSTCMD_x_BYTES	= 7;
			break;
 	}
}

void SPIHost_WriteEnable(void)
{
	WriteHostPage(PAGE4_SPI );					// Set Page=4
	SpiFlashHostDmaDestType(DMA_DEST_CHIPREG,0);
	SpiFlashHostCmd(SPICMD_WREN, 1);				//SPI Command = WRITE_ENABLE
	SpiFlashHostDmaReadLen(0);
	SpiFlashHostDmaStart(SPIDMA_WRITE,0);
}

void SPIHost_SectorErase( DWORD spiaddr )
{
	dPrintf("\nSPIHost_SectorErase %06lx",spiaddr);
  
	SPIHost_WriteEnable();

	WriteTW88Page(PAGE4_SPI);					// Set Page=4
	SpiFlashHostDmaDestType(DMA_DEST_CHIPREG,0);
	SpiFlashHostCmd(SPICMD_SE, 4);
	SpiFlashHostDmaFlashAddr(spiaddr);
	SpiFlashHostDmaReadLen(0);
	SpiFlashHostDmaStart(SPIDMA_WRITE,SPIDMA_BUSYCHECK);
}

void SPIHost_PageProgram( DWORD spiaddr, WORD xaddr, WORD cnt )
{
	SPIHost_WriteEnable();

	WriteHostPage(PAGE4_SPI );					// Set Page=4
	SpiFlashHostDmaDestType(DMA_DEST_MCU_XMEM,0);
	SpiFlashHostCmd(SPICMD_PP, 4);
	SpiFlashHostDmaFlashAddr(spiaddr);
	SpiFlashHostDmaBuffAddr(xaddr);
	SpiFlashHostDmaReadLen(cnt);
	SpiFlashHostDmaStart(SPIDMA_WRITE,SPIDMA_BUSYCHECK);
}

//=====================================
//for Host
BYTE SPIHost_QUADInit(void)
{
	BYTE dat0,dat1;
	BYTE vid;
	BYTE cid;
	BYTE ret;
	BYTE temp;
	
	WriteHostPage(4);						 
	SpiFlashHostDmaDestType(DMA_DEST_CHIPREG, 0);
	SpiFlashHostDmaBuffAddr(DMA_BUFF_REG_ADDR);
	SpiFlashHostDmaReadLen(0);	//clear high & middle bytes 

	SpiFlashHostCmd(SPICMD_RDID, 1);	
	SpiFlashHostDmaReadLenByte(3);
	SpiFlashHostDmaStart(SPIDMA_READ,0/*, __LINE__*/);
	vid  = SPIHOSTDMA_READDATA(0);
	dat1 = SPIHOSTDMA_READDATA(1);
	cid  = SPIHOSTDMA_READDATA(2);

	Printf("\n\tSPI JEDEC ID: %02bx %02bx %02bx", vid, dat1, cid );

	if(vid == 0x1C)			ret = SFLASH_VENDOR_EON;
	else if(vid == 0xC2) 	ret = SFLASH_VENDOR_MX; 
	else if(vid == 0xEF)	ret = SFLASH_VENDOR_WB;
	//else if(vid == 0x20)	ret = SFLASH_VENDOR_MICRON; //numonyx
	else {
		Printf(" UNKNOWN SPIFLASH !!");
		return 0;
	}
	//QUAD ?

	if (vid == 0xEF) {					// WB
		//if(cid == 0x18) {				//Q128 case different status read command
			SpiFlashHostCmd(SPICMD_RDSR2, 1);
			SpiFlashHostDmaReadLenByte(1);
			SpiFlashHostDmaStart(SPIDMA_READ,0/*, __LINE__*/);
			dat0 = SPIHOSTDMA_READDATA(0);
			Printf("\nStatus2 before QUAD: %02bx", dat0);
			temp = dat0;									  //dat0[1]:QE
	}
	else {
		SpiFlashHostCmd(SPICMD_RDSR, 1);
		SpiFlashHostDmaReadLenByte(1);
		SpiFlashHostDmaStart(SPIDMA_READ,0 /*, __LINE__*/);
		dat0 = SPIHOSTDMA_READDATA(0);
		temp = dat0 & 0x40;	//if 0, need to enable quad
	}
	if(temp==0) {
		Printf("\nHostFlash is not QUAD. Please enable it first.");
	}
	
	
	
		
	return ret;

}

//=============================================================================
// setting.c
//=============================================================================
void SSPLL_Host_PowerUp(BYTE fOn)
{
	WriteHostPage(PAGE0_SSPLL);
	if(fOn)	WriteHost(REG0FC, ReadHost(REG0FC) & ~0x80);
	else	WriteHost(REG0FC, ReadHost(REG0FC) |  0x80);
}

void FP_Host_GpioDefault(void)
{
	WriteHostPage(PAGE0_GPIO);
	WriteHost(REG084, 0x00);	//FP_BiasOnOff(OFF);
	WriteHost(REG08C, 0x00);	//FP_PWC_OnOff(OFF);
	WriteHost(REG094, 0x00);	//output
}

void InitHostWithNTSC(void)
{
	//---------------------------
	//clock
	//---------------------------
	WriteHostPage(PAGE4_CLOCK);
	if(SpiFlashVendor==SFLASH_VENDOR_MX)	// MCUSPI Clock : Select 27MHz with
		WriteHost(REG4E1, 0x02);			//if Macronix SPI Flash, SPI_CK_DIV[2:0]=2
	else
		WriteHost(REG4E1, 0x01);			//SPI_CK_SEL[5:4]=2. PCLK, SPI_CK_DIV[2:0]=1
	WriteHost(REG4E0, 0x01);				// PCLK_SEL[0]=1

	WriteHostPage(PAGE0_GENERAL);
											//SSPLL FREQ R0F8[3:0]R0F9[7:0]R0FA[7:0]
	WriteHost(REG0F8, 0x02);				//108MHz
	WriteHost(REG0F9, 0x00);	
	WriteHost(REG0FA, 0x00);	
	WriteHost(REG0F6, 0x00); 				//PCLK_div_by_1 SPICLK_div_by_1
	WriteHost(REG0FD, 0x34);				//SSPLL Analog Control
											//		POST[7:6]=0 VCO[5:4]=3 IPMP[2:0]=4
	WriteHostPage(PAGE2_SCALER);
	WriteHost(REG20D, 0x02);				//pclko div 3. 	108/3=36MHz

#if 0	//if you want CLKPLL 72MHz or 54MHz
	WriteHostPage(PAGE0_GENERAL);
	WriteHost(REG0FC, ReadHost(REG0FC) & ~0x80);	//Turn off SSPLL PowerDown first.
	WriteHost(REG0F6, ReadHost(REG0F6) | 0x20); 	//SPICLK_div_by_3

	WriteHostPage(PAGE4_CLOCK);
	WriteHost(REG4E1, ReadHost(REG4E1) | 0x20);		//SPI_CK_SEL[5:4]=2. CLKPLL
#endif

	//---------------------------
	WriteHostPage(PAGE1_DECODER);
	WriteHost(REG1C0, 0x01);				//LLPLL input def 0x00
	WriteHost(REG105, 0x2f);				//Reserved def 0x0E


	//---------------------------
	WriteHostPage(PAGE2_SCALER);
	WriteHost(REG21E, 0x03);				//BLANK def 0x00.	--enable screen blanking with AUTO

	//---------------------------
	WriteHostPage(PAGE0_LEDC);
	WriteHost(REG0E0, 0xF2);				//LEDC. default. 
											//	R0E0[1]=1	Analog block power down
											//	R0E0[0]=0	LEDC digital block disable
											//DCDC.	default. HW:0xF2
											//	R0E8[1]=1	Sense block power down
											//	R0E8[0]=0	DC converter digital block disable.
	WriteHost(REG0E8, 0x70);
	WriteHost(REG0EA, 0x3F);

	//=====================
	//All Register setting
	//=====================

	//-------------------
	// PAGE 0
	//-------------------
	WriteHostPage(PAGE0_GENERAL);

	WriteHost(REG007, 0x00);	// TCON
	WriteHost(REG008, 0xA9);	//<<<======	

	WriteHost(REG040, 0x10);		

	WriteHost(REG041, 0x0C);

	WriteHost(REG042, 0x02);	
	WriteHost(REG043, 0x10);	
	WriteHost(REG044, 0xF0);	
	WriteHost(REG045, 0x82);	
	WriteHost(REG046, 0xD0);	

	WriteHost(REG047, 0x80);	
	WriteHost(REG049, 0x41);	//def 0x00

	WriteHost(REG0ED, 0x40);	//def:0x80
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteHost(REG0F1, 0xC0);	//[7]FP_PWC [6]FP_BIAS		
#endif

	//-------------------
	// PAGE 1
	//-------------------
	WriteHostPage(PAGE1_DECODER);
	WriteHost(REG11C, 0x0F);	//*** Disable Shadow
	WriteHost(REG102, 0x40);
	WriteHost(REG106, 0x03);	//ACNTL def 0x00
	WriteHost(REG107, 0x02);	//Cropping Register High(CROP_HI) def 0x12
	WriteHost(REG109, 0xF0);	//Vertical Active Register,Low(VACTIVE_LO) def 0x20
	WriteHost(REG10A, 0x0B);	//Horizontal Delay,Low(HDELAY_LO) def 0x10
	WriteHost(REG111, 0x5C);	//CONTRAST def 0x64
	WriteHost(REG117, 0x80);	//Vertical Peaking Control I	def 0x30
	WriteHost(REG11E, 0x00);	//Component video format: def 0x08
	WriteHost(REG121, 0x22);	//Individual AGC Gain def 0x48
	WriteHost(REG127, 0x38);	//Clamp Position(PCLAMP)	def 0x2A
	WriteHost(REG128, 0x00);
	WriteHost(REG12B, 0x44);	//Comb Filter	def -100-100b
	WriteHost(REG130, 0x00);
	WriteHost(REG134, 0x1A);
	WriteHost(REG135, 0x00);
								//ADCLLPLL
	WriteHost(REG1C6, 0x20); //WriteHost(REG1C6, 0x27);	//WriteHost(REG1C6, 0x20);  Use VAdcSetFilterBandwidth()
	WriteHost(REG1CB, 0x3A);	//WriteHost(REG1CB, 0x30);

	//-------------------
	// PAGE 2
	//-------------------
	WriteHostPage(PAGE2_SCALER);
	//WriteHost(REG201, 0x00);
	//WriteHost(REG202, 0x20);
	WriteHost(REG203, 0xCC);		//XSCALE: 0x1C00. def:0x2000
	WriteHost(REG204, 0x1C);		//XSCALE:
	WriteHost(REG205, 0x8A);		//YSCALE: 0x0F8A. def:0x2000
	WriteHost(REG206, 0x0F);		//YSCALE:
	WriteHost(REG207, 0x40);		//PXSCALE[11:4]	def 0x800
	WriteHost(REG208, 0x20);		//PXINC	def 0x10
	WriteHost(REG209, 0x00);		//HDSCALE:0x0400 def:0x0100
	WriteHost(REG20A, 0x04);		//HDSCALE
	WriteHost(REG20B, 0x08);		//HDELAY2 def:0x30
	//WriteHost(REG20C, 0xD0);	//HACTIVE2_LO
	WriteHost(REG20D, 0x90 | (ReadHost(REG20D) & 0x03));  //LNTT_HI def 0x80. 
	WriteHost(REG20E, 0x20);		//HPADJ def 0x0000.  20E[6:4] HACTIVE2_HI
	WriteHost(REG20F, 0x00);
	WriteHost(REG210, 0x21);		//HA_POS	def 0x10
	
	//HA_LEN		Output DE length control in number of the output clocks.
	//	R212[3:0]R211[7:0] = PANEL_H+1
	WriteHost(REG211, 0x21);		//HA_LEN	def 0x0300
	WriteHost(REG212, 0x03);		//PXSCALE[3:0]	def:0x800 HALEN_H[3:0] def:0x03
	WriteHost(REG213, 0x00);		//HS_POS	def 0x10
	WriteHost(REG214, 0x20);		//HS_LEN
	WriteHost(REG215, 0x2E);		//VA_POS	def 0x20
	//VA_LEN	Output DE control in number of the output lines.
	//	R217[7:0]R216[7:0] = PANEL_V
	WriteHost(REG216, 0xE0);		//VA_LEN_LO	def 0x0300
	WriteHost(REG217, 0x01);		//VA_LEN_HI
	
	WriteHost(REG21C, 0x42);	//PANEL_FRUN. def 0x40
	WriteHost(REG21E, 0x03);		//BLANK def 0x00.	--enable screen blanking. SW have to remove 21E[0]
	WriteHost(REG280, 0x20);		//Image Adjustment register
	WriteHost(REG28B, 0x44);
	WriteHost(REG2E4, 0x21);		//--dither

	//-------------
	//Enable TCON
	//TW8835					TW8832
	WriteHostPage(PAGE2_TCON);
	WriteHost(REG240, 0x10);		//WriteHost(REG240, 0x11);
	WriteHost(REG241, 0x00);		//WriteHost(REG241, 0x0A);
	WriteHost(REG242, 0x05);		//WriteHost(REG242, 0x05);
	WriteHost(REG243, 0x01);		//WriteHost(REG243, 0x01);
	WriteHost(REG244, 0x64);		//WriteHost(REG244, 0x64);
	WriteHost(REG245, 0xF4);		//WriteHost(REG245, 0xF4);
	WriteHost(REG246, 0x00);		//WriteHost(REG246, 0x00);
	WriteHost(REG247, 0x0A);		//WriteHost(REG247, 0x0A);
	WriteHost(REG248, 0x36);		//WriteHost(REG248, 0x36);
	WriteHost(REG249, 0x10);		//WriteHost(REG249, 0x10);
	WriteHost(REG24A, 0x00);		//WriteHost(REG24A, 0x00);
	WriteHost(REG24B, 0x00);		//WriteHost(REG24B, 0x00);
	WriteHost(REG24C, 0x00);		//WriteHost(REG24C, 0x00);
	WriteHost(REG24D, 0x44);		//WriteHost(REG24D, 0x44);
	WriteHost(REG24E, 0x04);		//WriteHost(REG24E, 0x04);

	WriteHostPage(PAGE0_GENERAL);
	WriteHost(REG006, 0x06);	//display direction. TSCP,TRSP
	WriteHost(REG007, 0x00);	// TCON
}	

#endif //..MODEL_TW8835_EXTI2C
