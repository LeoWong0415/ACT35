/**
 * @file
 * settings.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	setup system 
*/

//input			
//	CVBS	YIN0
//	SVIDEO	YIN1, CIN0
//	aRGB	G:YIN2 B:CIN0 R:VIN0 
//	aYUV	G:YIN2 B:CIN0 R:VIN0

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
#include "SOsd.h"
#include "FOsd.h"
#include "Scaler.h"
#include "decoder.h"
#include "InputCtrl.h"
#include "EEPROM.h"
#include "ImageCtrl.h"
#include "decoder.h"
#include "InputCtrl.h"
#include "OutputCtrl.h"
#include "Settings.h"
#include "measure.h"
//#include "SOsdMenu.h"


#include "Data\DataInitPC.inc"
#include "Data\DataInitMonitor.inc"


#if 0
void SW_Reset(void)
{
	DECLARE_LOCAL_page
	ReadTW88Page(page);
	WriteTW88Page(0);
	WriteTW88(REG006, ReadTW88(REG006) | 0x80);	//SW RESET
	WriteTW88Page(page);
}
#endif	


//=============================================================================
//	INPUT CLOCKS			                                               
//=============================================================================

//NeedClock ?
//LLPLL
//PCLKO
//-----------------------------------------------------------------------------
/**
* set High speed clock. only for test
*/
void ClockHigh(void)
{
	dPrintf("\nHigh");	

	WriteTW88Page(PAGE0_SSPLL );
	WriteTW88(REG0F6, 0x00 );	// PCLK div by 1

	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG20D, 0x81 );	// PCLKO div by 2

	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E1, 0xe0 );	// Source=PCLK, Delay=1, Edge=1

	SPI_SetReadModeByRegister(0x05);	// SPI mode QuadIO, Match DMA mode with SPI-read
}

//-----------------------------------------------------------------------------
/**
* set Low speed clock. only for test
*/
void ClockLow(void)
{
	dPrintf("\nLow");	

	WriteTW88Page(PAGE0_SSPLL );
	WriteTW88(REG0F6, 0x00 );	// PCLK div by 1

	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG20D, 0x80 );	// PCLKO div by 1

	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E1, 0x20 );	// Source=PCLK, Delay=0, Edge=0

	SPI_SetReadModeByRegister(0x05);	// SPI mode QuadIO, Match DMA mode with SPI-read
}

//-----------------------------------------------------------------------------
/**
* set 27MHz clock. only for test
*/
void Clock27(void)
{
	dPrintf("\n27MHz");	

	WriteTW88Page(PAGE0_SSPLL );
	WriteTW88(REG0F6, 0x00 );	// PCLK div by 1

	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG20D, 0x80 );	// PCLKO div by 1

	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E1, 0x00 );	// Source=27M

	SPI_SetReadModeByRegister(0x05);	// SPI mode QuadIO, Match DMA mode with SPI-read
}

//=========================================
// SSPLL
//=========================================

//-----------------------------------------------------------------------------
/**
* power up the SSPLL
*/
void SSPLL_PowerUp(BYTE fOn)
{
	WriteTW88Page(PAGE0_SSPLL);
	if(fOn)	WriteTW88(REG0FC, ReadTW88(REG0FC) & ~0x80);
	else	WriteTW88(REG0FC, ReadTW88(REG0FC) |  0x80);
}
//-----------------------------------------------------------------------------
/**
* get PPF(PLL Pixel Frequency) value. SSPLL value
*
* oldname:SspllGetPCLK
*
*	FPLL = REG(0x0f8[3:0],0x0f9[7:0],0x0fa[7:0])
*	POST = REG(0x0fd[7:6])
*	PLL Osc Freq = 108MHz * FPLL / 2^17 / 2^POST
*/
DWORD SspllGetPPF(BYTE fHost)
{
	DWORD ppf, FPLL;
	BYTE  i;

	//read PLL center frequency
	FPLL = SspllGetFreqReg(fHost);

	#ifdef DEBUG_PC
	dPrintf("\r\n(GetFBDN) :%ld", FPLL);
	#endif

	i= SspllGetPost(fHost);
	ppf = SspllFPLL2FREQ(FPLL, i);
// 	dPrintf("\r\n(GetPPF) :%ld", ppf);

	//test
	//FPLL = SspllFREQ2FPLL(ppf, i);
 	//dPrintf(" test FPLL :0x%lx", FPLL);


#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	//FPGA do not return the correct ppf value. type "PCLK 666" for 66.6MKz
	ppf = 66560180;
	dPrintf("->%ld", ppf);
#endif
	return ppf;
}

//desc
// PLL = 108MHz *FPLL / 2^17
// FPLL = PLL * 2^17 / 108MHz
//		= PLL * 131072 / 108MHz
//		= PLL * 131072 / (108 * 100000 * 10)Hz
//      = (PLL / 100000) * (131072 / 108)* (1/10)
//      = (PLL / 100000) * (1213.6296) * (1/10)
//      = (PLL / 100000) * (1213.6296 *2 ) * (1/10*2) 
//      = (PLL / 100000) * (2427.2592) / 20 
//      = (PLL / 100000) * (2427) / 20 
//@param
//	_PPF: input pixel clock
//oldname:ChangeInternPLL@TW8816
//nickname: SetPclk

//		#define REG0F8_FPLL0			0xF8
//		#define REG0F9_FPLL1			0xF9
//		#define REG0FA_FPLL2			0xFA
//		#define REG0FD_SSPLL_ANALOG	0xFD

//-----------------------------------------------------------------------------
/**
* set SSPLL register value
*/
void SspllSetFreqReg(DWORD FPLL)
{
	dPrintf("\nSspllSetFreqReg(%lx)",FPLL);
	WriteTW88Page(PAGE0_SSPLL);
	WriteTW88(REG0FA_FPLL2, (BYTE)FPLL );
	WriteTW88(REG0F9_FPLL1, (BYTE)(FPLL>>8));
	WriteTW88(REG0F8_FPLL0, (ReadTW88(REG0F8_FPLL0)&0xF0) | (FPLL>>16));
}
//-----------------------------------------------------------------------------
/**
* get SSPLL register value
*
*	register
*	R0F8[3:0]	FPLL[19:16]
*	R0F9[7:0]	FPLL[15:8]
*	R0FA[7:0]	FPLL[7:0]
*/
DWORD SspllGetFreqReg(BYTE fHost)
{
	DWORD dFPLL;
	
	if(fHost) {
		WriteHostPage(PAGE0_SSPLL);
		dFPLL = ReadHost(REG0F8_FPLL0)&0x0F;
		dFPLL <<=8;
		dFPLL |= ReadHost(REG0F9_FPLL1);
		dFPLL <<=8;
		dFPLL |= ReadHost(REG0FA_FPLL2);
	}
	else {
		WriteTW88Page(PAGE0_SSPLL);
		dFPLL = ReadTW88(REG0F8_FPLL0)&0x0F;
		dFPLL <<=8;
		dFPLL |= ReadTW88(REG0F9_FPLL1);
		dFPLL <<=8;
		dFPLL |= ReadTW88(REG0FA_FPLL2);
	}
	return dFPLL;
}

//-----------------------------------------------------------------------------
/**
* set SSPLL AnalogControl register
*
*	register
*	R0FD[7:6] POST
*	R0FD[5:4] VCO
*	R0FD[2:0] ChargePump
*/
void SspllSetAnalogControl(BYTE value)
{
	WriteTW88Page(PAGE0_SSPLL);
	WriteTW88(REG0FD_SSPLL_ANALOG, value );
}

//-----------------------------------------------------------------------------
/**
* get SSPLL Post value
*/
BYTE SspllGetPost(BYTE fHost)
{
	BYTE post;

	if(fHost) {
		WriteHostPage(PAGE0_SSPLL);
		post = ReadHost(REG0FD);
	}
	else {
		WriteTW88Page(PAGE0_SSPLL);
		post = ReadTW88(REG0FD);
	}
	return ((post>>6) & 0x03);
}

/*
	PLL Osc Freq = 108MHz * FPLL / 2^17 / 2^POST
	FREQ			= 27000000 * 4 * FPLL / 2^17  / 2^POST
    FPLL 			= FREQ *((2^15) * (2^POST)) / 27000000 			   			
    FPLL 			= (FREQ / 1000) *((2^15) * (2^POST)) / 27000 			   			
    FPLL 			= (FREQ / 1000) *((2^12) * (2^POST)) * (2^3  / 27000)
    FPLL 			= (FREQ / 1000) *((2^12) * (2^POST)) / (3375) 			   			
*/
#if 1 //GOOD
//-----------------------------------------------------------------------------
/**
* get FPLL value from freq
*/
DWORD SspllFREQ2FPLL(DWORD FREQ, BYTE POST)
{
	DWORD FPLL;
	FPLL = FREQ/1000L;
	FPLL <<= POST;
	FPLL <<= 12;
	FPLL = FPLL / 3375L;
	return FPLL;
}
#endif
#if 0	//BKFYI:example
/*
    FPLL 			= FREQ *((2^15) * (2^POST)) / 27000000 			   			
	FPLL 			= FREQ *(32768 * (2^POST)) / 27000000
	FPLL 			= FREQ *(512 * (2^POST)) / 421875
    FPLL            = FREQ * 16 / 421875 * 32 * (2^POST)
*/
//DWORD SspllFREQ2FPLL(DWORD FREQ, BYTE POST)
//{
//	DWORD FPLL;
//	FPLL = FREQ * 16; 
//	FPLL /= 421875;
//	FPLL *= 32;
//	FPLL <<= POST;
//	return FPLL;
//}
#endif

#if 0 //BKFYI: example 
/*
	FREQ			= 27000000 * 4 * FPLL / 2^17  / 2^POST
    Simpilfied FREQ	= 824L * FPLL * 2^POST
*/
//DWORD SspllFPLL2FREQ(DWORD FPLL, BYTE POST)
//{
//	DWORD FREQ;
//	FREQ = FPLL * 824L;
//	FREQ >>= POST;
//	return FREQ;
//}
#endif

/*
    FREQ 			= 27000000 * FPLL / ( (2^15) * (2^POST) )
    FREQ 			= 27000000 * FPLL / ( (2^15) * (2^POST) )
					= 421875 * 64 * FPLL / (64 * 2^9 *(2^POST))
					= 421875 * FPLL / (512 *(2^POST))
	   				= FPLL / 64 * 421875 / 8 / (2^POST)
*/
//-----------------------------------------------------------------------------
/**
* get freq from FPLL
*/
DWORD SspllFPLL2FREQ(DWORD FPLL, BYTE POST)
{
	DWORD FREQ;
	FREQ = FPLL / 64;
	FREQ *= 421875;
	FREQ /= 8;
	FREQ >>= POST;
	return FREQ;
}


//SSPLL Set Frequency And PLL
//R0F8
//R0F9
//R0FA
//R0FD
//R0FC
//R20D[4]	Pixel clock polarity control
//R20D[1:0]	Pixel clock output frequency division control
//
//if PPF is 108M, POST=0. VCO:3 CURR=4

//-----------------------------------------------------------------------------
/**
* set SSPLL freq and Pll
*/
void SspllSetFreqAndPll(DWORD _PPF)
{
	BYTE	ppf, CURR, VCO, POST;
	DWORD	FPLL;
	
	dPrintf("\nSspllSetFreqAndPll(%ld)",_PPF);
	ppf = _PPF/1000000L;		//base:1MHz

	//----- Frequency Range --------------------
	if     ( ppf < 27 )  { VCO=2; CURR=0; POST=2; }		// step = 0.5MHz
	else if( ppf < 54 )  { VCO=2; CURR=1; POST=1; }		// step = 1.0MHz
	else if( ppf < 108 ) { VCO=2; CURR=2; POST=0; }		// step = 1.0MHz
	else                 { VCO=3; CURR=3; POST=0; }		// step = 1.0MHz

	CURR = VCO+1;	//BK110721. Harry Suggest.

	//----- Get FBDN
	FPLL = SspllFREQ2FPLL(_PPF, POST);

	//----- Setting Registers
	SspllSetFreqReg(FPLL);
	SspllSetAnalogControl((VCO<<4) | (POST<<6) | CURR);

	dPrintf("\nPOST:%bx VCO:%bx CURR:%bx",POST, VCO, CURR);

	//adjust pclk divider
	if(ppf >=150) {
		ppf /= 2;
		PclkSetDividerReg(1);	//div2
	}
	else {
		PclkSetDividerReg(0);	//div1:default
	}

	//adjust pclko divider. see SetDefaultPClock()
	PclkoSetDiv( (ppf+39) / 40 - 1); //pixel clock polarity : Invert 0:div1, 1:div2, 2:div3
										//BKTODO:move pixel clock polarity...	
	PclkSetPolarity(1);	//invert

}



//=========================================
// PCLK
//=========================================

//-----------------------------------------------------------------------------
/**
* set PCLK divider
*/
void PclkSetDividerReg(BYTE divider)
{
	WriteTW88Page(0);
	WriteTW88(REG0F6, (ReadTW88(REG0F6) & 0xF8) | divider);
}

//-----------------------------------------------------------------------------
/**
* get PCLK frequency
*/
DWORD PclkGetFreq(BYTE fHost,DWORD sspll)
{
	BYTE divider;
	DWORD temp32;
	if(fHost) {
		WriteHostPage(0);
		divider = ReadHost(REG0F6) & 0x03;
	}
	else {
		WriteTW88Page(0);
		divider = ReadTW88(REG0F6) & 0x03;
	}
	switch(divider) {
	case 0:	temp32 = sspll;			break;
	case 1:	temp32 = sspll / 2;		break;
	case 2:	temp32 = sspll / 4;		break;
	case 3:	temp32 = sspll / 8;		break;
	}
	return temp32;
}
//-----------------------------------------------------------------------------
/**
* get PCLKO frequency
*/
DWORD PclkoGetFreq(BYTE fHost, DWORD pclk)
{
	BYTE divider;
	DWORD temp32;
	if(fHost) {
		WriteHostPage(2);
		divider = ReadHost(REG20D) & 0x03;
	}
	else {
		WriteTW88Page(2);
		divider = ReadTW88(REG20D) & 0x03;
	}
	divider++;
	temp32 = pclk / divider;
	return temp32;
}

//-----------------------------------------------------------------------------
/**
* set PCLKO divider and CLK polarity
*
*	R20D[4]	Pixel clock polarity control
*	R20D[1:0]	Poxel clock output frequency division control

* @param div - Pixel clock output frequency division
*	0:div 1,	1:div 2,	2:div 3,	3:div 4.
*
*/
void PclkoSetDiv(/*BYTE pol,*/ BYTE div)
{
	BYTE value;
	WriteTW88Page(PAGE2_SCALER);
	value = ReadTW88(REG20D) & 0xFC;
	WriteTW88(REG20D, value | div);
}
//-----------------------------------------------------------------------------
/**
* set PCLK polarity
*
* @param pol - Pixel clock output polarity
*	-0:	no inversion
*	-1:	inversion
*	- 0xFF: do not change it. Use previous value
*/
void PclkSetPolarity(BYTE pol)
{
	BYTE value;
	WriteTW88Page(PAGE2_SCALER);
	value = ReadTW88(REG20D);
	if(pol)	value |=  0x10;
	else	value &= ~0x10;
	WriteTW88(REG20D, value);
}


//=========================================
// CLKPLL
//=========================================

//-----------------------------------------------------------------------------
/**
* select ClkPLL input
*/
void ClkPllSetSelectReg(BYTE ClkPllSel)
{
	WriteTW88Page(4);
	if(ClkPllSel) WriteTW88(REG4E0, ReadTW88(REG4E0) |  0x01);
	else		  WriteTW88(REG4E0, ReadTW88(REG4E0) & ~0x01);
}
//-----------------------------------------------------------------------------
/**
* set ClkPLL divider
*/
void ClkPllSetDividerReg(BYTE divider)
{
	WriteTW88Page(4);
	WriteTW88(REG4E1, (ReadTW88(REG4E1) & ~0x07) | divider);	//CLKPLL Divider
}

//-----------------------------------------------------------------------------
/**
* set ClkPLL input and ClkPLL divider
* 
* only from monitor
*/
void ClkPllSetSelDiv(BYTE ClkPllSel, BYTE ClkPllDiv)
{
	BYTE mcu_sel;
	DWORD clkpll,spi_clk;
	BYTE i=0;

	//check & move MCU CLK source to 27M 
	mcu_sel = McuSpiClkReadSelectReg();
	if(mcu_sel==MCUSPI_CLK_PCLK)
		McuSpiClkSelect(MCUSPI_CLK_27M);	
	//
	//Now, MCU uses 27M or 32K. You can change CLKPLL register without a system hang
	//

	ClkPllSetSelectReg(ClkPllSel);
	do {
		ClkPllSetDividerReg(ClkPllDiv);
		ClkPllDiv++;
		clkpll =ClkPllGetFreq(0);
		spi_clk=SpiClkGetFreq(0, clkpll);
		i++;
	} while(spi_clk > 75000000L);	//MAX SPICLK
	if(i!=1)
		ePrintf("\nClkPllSetSelDiv div encreased:%d",i-1);

	//restore MCU CLK source
	if(mcu_sel==MCUSPI_CLK_PCLK)
		McuSpiClkSelect(MCUSPI_CLK_PCLK);	
}

//-----------------------------------------------------------------------------
/**
* get ClkPLL frequency
*/
DWORD ClkPllGetFreq(BYTE fHost)
{
	BYTE temp8;
	DWORD clkpll;
	DWORD temp32;
	if(fHost) {
		WriteHostPage(4);
		temp8 = ReadHost(REG4E0) & 0x01;
	}
	else {
		WriteTW88Page(4);
		temp8 = ReadTW88(REG4E0) & 0x01;
	}
	if(temp8==0) {
		temp32 = SspllGetPPF(fHost);
		clkpll = PclkGetFreq(fHost, temp32);
	}
	else {
		clkpll=108000000L;
	}
	if(fHost) {
		WriteHostPage(4);
		temp8 = ReadHost(REG4E1) & 0x07;
	}
	else {
		WriteTW88Page(4);
		temp8 = ReadTW88(REG4E1) & 0x07;
	}
	switch(temp8) {
	case 0:	temp32 = clkpll;		break;
	case 1:	temp32 = clkpll*2/3;	break;
	case 2:	temp32 = clkpll*2;		break;
	case 3:	temp32 = clkpll*2/5;	break;
	case 4:	temp32 = clkpll*3;		break;
	case 5:	temp32 = clkpll*2/7;	break;
	case 6:	temp32 = clkpll*4;		break;
	case 7:	temp32 = clkpll*5;		break;
	}
	return temp32;
}


//=========================================
// MCPSPI
//=========================================
//-----------------------------------
// McuSpiClkToPclk & McuSpiClkRestore
//-----------------------------------

/*
result  register        stepA	stepB	stepC	stepD	stepE
------	--------		-----	-----	-----	-----	-----
fail	PCLK_SEL  		PLL				 		PCLK
		MCUSPI_CLK_SEL		PCLK	27M						PCLK
		MCUSPI_CLK_DIV		1		   		0				1

OK		PCLK_SEL  		PLL				 		PCLK
		MCUSPI_CLK_SEL		PCLK	27M						PCLK
		MCUSPI_CLK_DIV		1		   		 				1

OK		PCLK_SEL  		PLL				 		PCLK
		MCUSPI_CLK_SEL		PCLK
		MCUSPI_CLK_DIV		1
---------------------------------------------------------------
*/
BYTE shadow_r4e0;
BYTE shadow_r4e1;

//-----------------------------------------------------------------------------
/**
* select PCLK for McuSpi
*
* NOTE: SPIDMA needs a PCLK source
* @see McuSpiClkRestore
*/
BYTE McuSpiClkToPclk(BYTE divider)
{
//#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
//	BYTE temp;
//	//do not change divider. I will use 65MHz PCLK with divider 1.
//	temp = divider;			
//	return 0;
//#endif
#ifdef MODEL_TW8835_EXTI2C
	BYTE temp;
	temp = divider;			
	WriteTW88Page(PAGE4_CLOCK);
	shadow_r4e0 = ReadTW88(REG4E0);
	shadow_r4e1 = ReadTW88(REG4E1);

	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG002, 0xff );				
	while((ReadTW88(REG002) & 0x40) ==0);	//wait vblank  I2C_WAIT_VBLANK
//PORT_DEBUG = 0;
 	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E0, shadow_r4e0 & 0xFE);	//select PCLK.
	WriteTW88(REG4E1, 0x20 | divider);
//PORT_DEBUG = 1;
	return 0;
#else
	WriteTW88Page(PAGE4_CLOCK);
	shadow_r4e0 = ReadTW88(REG4E0);
	shadow_r4e1 = ReadTW88(REG4E1);

	WriteTW88(REG4E0, shadow_r4e0 & 0xFE);	//select PCLK.
	WriteTW88(REG4E1, 0x20 | divider);		//CLKPLL + divider.

	return 0;
#endif
}

//-----------------------------------------------------------------------------
/**
* restore MCUSPI clock
*
* @see McuSpiClkToPclk
*/

void McuSpiClkRestore(void)
{
//#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
//	//do not change. I will use 65MHz PCLK with divider 1.
//	return;
//#endif
	//Printf("\nMcuSpiClkRestore REG4E0:%bx REG4E1:%bx",shadow_r4e0,shadow_r4e1);
#ifdef MODEL_TW8835_EXTI2C	//I2C_WAIT_VBLANK
	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG002, 0xff );				
	while((ReadTW88(REG002) & 0x40) ==0); //wait vblank
#endif
//PORT_DEBUG = 0;
 	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E0, shadow_r4e0);
//#ifdef MODEL_TW8835_EXTI2C
//#else
	WriteTW88(REG4E1, shadow_r4e1);
//#endif
//PORT_DEBUG = 1;
	//Printf("-Done");
}

//-----------------------------------------------------------------------------
/**
* read MCUSPI clock mode
*/
BYTE McuSpiClkReadSelectReg(void)
{
	BYTE value;
	WriteTW88Page(PAGE4_CLOCK);
	value = ReadTW88(REG4E1) & 0x30;
	return (value >> 4);
}

//-----------------------------------------------------------------------------
/**
* Select McuSpi clock source
*
*	register
*	R4E1[5:4]
* @param McuSpiClkSel clock source
*	-0:27MHz
*	-1:32KHz
*	-2:CLKPLL. << (PLL Divider) << (PCLK or 108M) << (if PCLK, SSPLL with PCLK Divider)
*/
void McuSpiClkSelect(BYTE McuSpiClkSel)
{
	BYTE value;
	WriteTW88Page(PAGE4_CLOCK);
	value = ReadTW88(REG4E1) & 0x0F;
	WriteTW88(REG4E1, (McuSpiClkSel << 4) | value);
} 

#if 0
//OLD code
//!void McuSpiClkSet(BYTE McuSpiClkSel, BYTE ClkPllSel, BYTE ClkPllDiv) 
//!{
//!	WriteTW88Page(PAGE4_CLOCK);
//!	if(ClkPllSel)	WriteTW88(REG4E0, ReadTW88(REG4E0) |  0x01);
//!	else			WriteTW88(REG4E0, ReadTW88(REG4E0) & ~0x01); 
//!	WriteTW88(REG4E1, (McuSpiClkSel << 4) | ClkPllDiv);
//!}
#endif

//-----------------------------------------------------------------------------
/**
* get MCU clock frequency
*/
DWORD McuGetClkFreq(BYTE fHost)
{
	BYTE temp8;
	DWORD temp32;

	if(fHost) {
		WriteHostPage(4);
		temp8 = ReadHost(REG4E1) >> 4;
	}
	else {
		WriteTW88Page(4);
		temp8 = ReadTW88(REG4E1) >> 4;
	}
	switch(temp8) {
	case 0: temp32 = 27000000L;				break;
	case 1:	temp32 = 32000L;				break;
	case 2:	temp32 = ClkPllGetFreq(fHost);	break;
	default: //unknown
			temp32 = 27000000L;				break;
	}
	return temp32;
}

//=========================================
// SPI CLK
//=========================================

//-----------------------------------------------------------------------------
/**
* get SPI clock frequency
*/
DWORD SpiClkGetFreq(BYTE fHost, DWORD mcu_clk)
{
	BYTE divider;
	DWORD temp32;
	if(fHost) {
		WriteHostPage(0);
		divider = ReadHost(REG0F6) >> 4;
	}
	else {
		WriteTW88Page(0);
		divider = ReadTW88(REG0F6) >> 4;
	}
	divider++;
	temp32 = mcu_clk / divider;
	return temp32;
}

//-----------------------------------------------------------------------------
/**
* set LLPLL clock source
*
* use 27M OSD or PLL 
*/
void LLPLLSetClockSource(BYTE use_27M)
{
	WriteTW88Page(PAGE1_LLPLL);
	if(use_27M)	WriteTW88(REG1C0, ReadTW88(REG1C0) | 0x01); 
	else		WriteTW88(REG1C0, ReadTW88(REG1C0) & ~0x01);
}

/*
example
#if 0
	WriteTW88Page(PAGE4_CLOCK);
	value = ReadTW88(REG4E1);
	WriteTW88(REG4E1, value & ~0x30);	//select 27M first
	WriteTW88(REG4E0, ReadTW88(REG4E0) | 0x01);		//select 108M PLL clock.
	WriteTW88(REG4E1, (value & ~0x30) | 0x01);			//SPI clock Source=PCLK, Delay=1, Edge=0, PLL(72M) divider:1.5
	WriteTW88(REG4E1, 0x21);	 					//SPI clock Source=PCLK, Delay=1, Edge=0, PLL(72M) divider:1.5
#endif
	do {
		BYTE r4e0,r4e1;
		WriteTW88Page(PAGE4_CLOCK);
		r4e0 = ReadTW88(REG4E0);
		r4e1 = ReadTW88(REG4E1);
		WriteTW88(REG4E1, r4e1 & 0x0F);	//27M
		delay1ms(10);
		WriteTW88(REG4E1, 0x00);			//27M with div1.0
		WriteTW88(REG4E0, r4e0 | 0x01);
		WriteTW88(REG4E1, 0x21);			
		delay1ms(10);
// 		Printf("\nline:%d",__LINE__);
	} while(0);
	//
*/



//-----------------------------------------------------------------------------
//set default SSPLL clock
#if 0
void SetDefaultPClock(void)
{
	WriteTW88Page(PAGE0_SSPLL);
	WriteTW88(REG0F9, 0x50);	  	//SSPLL 70MKz. 0x50:70MHz, 0x3C:66.6MHz..Pls use SspllSetFreqReg(0x015000);
	WriteTW88(REG0F6, 0x00); 		//PCLK div:1. SPI CLK div:1

	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG20D, 0x81 );	// PCLKO div by 2. ??Polarity
}
#endif

//-----------------------------------------------------------------------------
/**
* print current clock info
*/
void DumpClock(BYTE fHost)
{
	DWORD ppf, FPLL;
	BYTE  i;
	DWORD pclk,pclko;
	DWORD clkpll, mcu_clk,spi_clk;

	//read PLL center frequency
	FPLL = SspllGetFreqReg(fHost);
	i= SspllGetPost(fHost);
	ppf = SspllFPLL2FREQ(FPLL, i);

	pclk = PclkGetFreq(fHost, ppf);
	pclko = PclkoGetFreq(fHost, pclk);

	clkpll =ClkPllGetFreq(fHost);

	mcu_clk=McuGetClkFreq(fHost);
	spi_clk=SpiClkGetFreq(fHost, mcu_clk);

	dPrintf("\nCLOCK SSPLL:%lx POST:%bx FREQ:%ld", FPLL,i,ppf);
	dPrintf("\n      PCLK:%ld PCLKO:%ld",	pclk, pclko);
	dPrintf("\n      CLKPLL:%ld",clkpll);
	dPrintf("\n      MCU:%ld SPI:%ld",mcu_clk,spi_clk);
}


//=============================================================================
//				                                               
//=============================================================================
//global


/*
//==========================
// GPIO EXAMPLE
//==========================
//!GPIO_EN	Enable(active high)
//!GPIO_OE	Output Enable(active high)
//!GPIO_OD	Output Data
//!GPIO_ID	Input Data
//!
//!		GPIO_EN	GPIO_OE	GPIO_OD	GPIO_ID
//!GPIO0	R080	R088	R090	R098
//!GPIO1	R081	R089	R091	R099
//!GPIO2	R082	R08A	R092	R09A
//!GPIO3	R083	R08B	R093	R09B
//!GPIO4	R084	R08C	R094	R09C
//!GPIO6	R085	R08D	R095	R09D
//!GPIO7	R086	R08E	R096	R09E
//!
//!bit readGpioInputData(BYTE gpio, BYTE b)
//!{
//!	BYTE reg;
//!	reg = 0x98+gpio;
//!	value = ReadTW88(reg);
//!	if(value & (1<<b))	return 1;
//!	else				return 0;
//!}
//!bit readGpioOutputData(BYTE gpio, BYTE b)
//!{
//!	BYTE reg;
//!	reg = 0x90+gpio;
//!	value = ReadTW88(reg);
//!	if(value & (1<<b))	return 1;
//!	else				return 0;
//!}
//!void writeGpioOutputData(BYTE gpio, BYTE b, BYTE fOnOff)
//!{
//!	BYTE reg;
//!	reg = 0x90+gpio;
//!	value = ReadTW88(reg);
//!	if(fOnOff) value |= (1<<b);
//!	else       value &= ~(1<<b);
//!	WriteTW88(reg,value);
//!}
*/
	


//
//BKFYI110909.
//	We merge step0 and step1, and check the status only at step2.
//	
//	step0 check_status : OK
//	step1 check_status : OK
//	step2 check_status : OK
//
//  step0 & step1 check_status : fail 20%
//	step2         check_status : OK
//-------------------------------------- 

//-----------------------------------------------------------------------------
/**
* turn on DCDC
*/
BYTE DCDC_On(BYTE step)
{
	BYTE i;
	//-------------
	//DCDC ON
	WriteTW88Page(PAGE0_DCDC);
	switch(step) {
	case 0:
#if 0
		WriteTW88(REG0E8, 0x72);	//default. & disable OverVoltage
		WriteTW88(REG0E8, 0x12);	//disable OverCurrent, disable UnderCurrent
		WriteTW88(REG0E8, 0x13);	//enable DC convert digital block
#endif
		WriteTW88(REG0E8, 0xF2);	//Printf("\nREG0E8:F2[%bd]",ReadTW88(REG0EA)>>4);
		WriteTW88(REG0E8, 0x02);	//Printf("\nREG0E8:02[%bd]",ReadTW88(REG0EA)>>4);
		WriteTW88(REG0E8, 0x03);	//Printf("\nREG0E8:03[%bd]",ReadTW88(REG0EA)>>4);
		WriteTW88(REG0E8, 0x01);	//Printf("\nREG0E8:01[%bd]",ReadTW88(REG0EA)>>4);
		break;
	case 1:
		WriteTW88(REG0E8, 0x11);	//powerup DC sense block
		break;
	case 2:
		WriteTW88(REG0E8, 0x71);	//turn on under current feedback control
									//0x11->0x51->0x71
		break;
	//default:
	//	ePuts("\nBUG");
	//	return;
	}
	for(i=0; i < 10; i++) {
		if((ReadTW88(REG0EA) & 0x30)==0x30) {
			//dPrintf("\nDCDC(%bd):%bd",step,i);
			return ERR_SUCCESS;	//break;
		}
		delay1ms(2);
	}
	Printf("\nDCDC_On(%bd) FAIL",step);
	return ERR_FAIL;
}

//GPIO43 or expender GPIO[1]
//-----------------------------------------------------------------------------
/**
* set FP_Bias
*/
void FP_BiasOnOff(BYTE fOn)
{
#ifdef EVB_10
	WriteTW88Page(PAGE0_GENERAL);
	if(fOn) {
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
#else
		WriteTW88(REG084, ReadTW88(REG084) | 0x08);		//enable 
		WriteTW88(REG08C, ReadTW88(REG08C) | 0x08);		//output enable
		delay1ms(2);
#endif
	}
	else {
		WriteTW88(REG08C, ReadTW88(REG08C) & ~0x08);	//output disable
		WriteTW88(REG084, ReadTW88(REG084) & ~0x08);	//disable
	}
#else
	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG084, 0x00);	//disable
#ifdef PICO_GENERIC
	dPrintf("\nFP_BiasOnOff(%bd) skip",fOn);
#else
	if(fOn) {
		WriteSlowI2CByte( I2CID_SX1504, 1, 0 );		// output enable
		WriteSlowI2CByte( I2CID_SX1504, 0, ReadSlowI2CByte(I2CID_SX1504, 0) & 0xFD );		// FPBIAS enable.
	}
	else {
		WriteSlowI2CByte( I2CID_SX1504, 1, 0 );		// output enable
		WriteSlowI2CByte( I2CID_SX1504, 0, ReadSlowI2CByte(I2CID_SX1504, 0) | 0x02 );		// FPBIAS disable
	}
#endif
#endif	
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88Page(PAGE0_GENERAL);
	if(fOn) WriteTW88(REG0F1, ReadTW88(REG0F1) |  0x40);		
	else	WriteTW88(REG0F1, ReadTW88(REG0F1) & ~0x40);		
#endif
}

//FrontPanel PowerControl ON - GPIO42. or expender GPIO[0]
//-----------------------------------------------------------------------------
/**
* set FP_PWC
*/
void FP_PWC_OnOff(BYTE fOn)
{
#ifdef EVB_10
	WriteTW88Page(PAGE0_GENERAL);
	if(fOn) {
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
#else
		WriteTW88(REG08C, ReadTW88(REG08C) | 0x04);		//output FPPWC enable
		WriteTW88(REG084, ReadTW88(REG084) | 0x04);		//enable GPIO
#endif
	}
	else {
		WriteTW88(REG08C, ReadTW88(REG08C) & ~0x04);	//output FPPWC disable
		WriteTW88(REG084, ReadTW88(REG084) & ~0x04);	//disable GPIO
	}
#else
	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG084, 0x00);											//disable
#ifdef PICO_GENERIC
	dPrintf("\nFP_PWC_OnOff(%bd) skip",fOn);
#else
	if(fOn) {
		WriteSlowI2CByte( I2CID_SX1504, 1, 0 );									// output enable
		WriteSlowI2CByte( I2CID_SX1504, 0, ReadSlowI2CByte(I2CID_SX1504, 0) & 0xFE );	// FPPWC enable
	}
	else {
		WriteSlowI2CByte( I2CID_SX1504, 1, 0 );									// output enable
		WriteSlowI2CByte( I2CID_SX1504, 0, ReadSlowI2CByte(I2CID_SX1504, 0) | 0x01 );	// FPPWC disable
	}
#endif
#endif

#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88Page(PAGE0_GENERAL);
	if(fOn) WriteTW88(REG0F1, ReadTW88(REG0F1) |  0x80);		
	else	WriteTW88(REG0F1, ReadTW88(REG0F1) & ~0x80);		
#endif

	
}

//-----------------------------------------------------------------------------
/**
* set GPIO for FP
*/
void FP_GpioDefault(void)
{
	WriteTW88Page(PAGE0_GPIO);
	WriteTW88(REG084, 0x00);	//FP_BiasOnOff(OFF);
	WriteTW88(REG08C, 0x00);	//FP_PWC_OnOff(OFF);
	WriteTW88(REG094, 0x00);	//output

#ifndef EVB_10
	if(access) {
		//turn off FPPWC & FPBias. make default
		//	0x40 R0 R1 is related with FP_PWC_OnOff
#ifndef PICO_GENERIC
		WriteSlowI2CByte( I2CID_SX1504, 0, 3 );	//RegData:	FPBias OFF. FPPWC disable.
		WriteSlowI2CByte( I2CID_SX1504, 1, 3 );	//RegDir:	input 
#endif
		//Printf("\nI2CID_SX1504 0:%02bx 1:%bx",ReadSlowI2CByte(I2CID_SX1504, 0), ReadSlowI2CByte(I2CID_SX1504, 1));
	}
#endif
}

//-----------------------------------------------------------------------------
/**
* init default NTSC value
*/
void InitWithNTSC(void)
{
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
#else
	//dump default registers
	if(DebugLevel >=3) {
//		DumpRegister(0);
		//DumpRegister(1);
		//DumpRegister(2);
		//DumpRegister(3);
//		DumpRegister(4);
		//DumpRegister(5);
	}
#endif

	//---------------------------
	//clock
	//---------------------------
	WriteTW88Page(PAGE4_CLOCK);
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88(REG4E1, 0x01);			//SPI_CK_DIV[2:0]=1. SPI_CK_SEL[5:4]=0(27MHz) will be changed later 
	WriteTW88(REG4E0, 0x00);			// PCLK_SEL[0]=0(PCLK)
#else
	if(SpiFlashVendor==SFLASH_VENDOR_MX)//if Macronix SPI Flash
		WriteTW88(REG4E1, 0x02);		//SPI_CK_DIV[2:0]=2(%2). SPI_CK_SEL[5:4]=0(27MHz) will be changed later 
	else
		WriteTW88(REG4E1, 0x01);		//SPI_CK_DIV[2:0]=1(%1.5).SPI_CK_SEL[5:4]=0(27MHz) will be changed later
#ifdef MODEL_TW8835_EXTI2C_USE_PCLK
	WriteTW88(REG4E0, 0x00);			// PCLK_SEL[0]=0(PCLK)
#else
	WriteTW88(REG4E0, 0x01);			// PCLK_SEL[0]=1(PLL108M)
#endif
#endif

	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG0D6, 0x01);
							//SSPLL FREQ R0F8[3:0]R0F9[7:0]R0FA[7:0]
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88(REG0F8, 0x01);	//65MHz
	WriteTW88(REG0F9, 0x34);	
	WriteTW88(REG0FA, 0x26);	
	WriteTW88(REG0F6, 0x00); 	//PCLK_div_by_1 SPICLK_div_by_1
	WriteTW88(REG0FD, 0x34);	//SSPLL Analog Control
								//		POST[7:6]=0 VCO[5:4]=3 IPMP[2:0]=4
#else
	WriteTW88(REG0F8, 0x02);	//108MHz
	WriteTW88(REG0F9, 0x00);	
	WriteTW88(REG0FA, 0x00);	
	WriteTW88(REG0F6, 0x00); 	//PCLK_div_by_1 SPICLK_div_by_1
	WriteTW88(REG0FD, 0x34);	//SSPLL Analog Control
								//		POST[7:6]=0 VCO[5:4]=3 IPMP[2:0]=4
#endif

	WriteTW88Page(PAGE2_SCALER);
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88(REG20D, 0x01);  	//pclko div 2. 	65/2 = 32.5MHz 
#else
	WriteTW88(REG20D, 0x02);	//pclko div 3. 	108/3=36MHz
#endif


#if 0	//if you want CLKPLL 72MHz or 54MHz
	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG0FC, ReadTW88(REG0FC) & ~0x80);	//Turn off SSPLL PowerDown first.
	WriteTW88(REG0F6, ReadTW88(REG0F6) | 0x20); 	//SPICLK_div_by_3

	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E1, ReadTW88(REG4E1) | 0x20);		//SPI_CK_SEL[5:4]=2. CLKPLL
#endif

	//---------------------------
	WriteTW88Page(PAGE1_DECODER);
	WriteTW88(REG1C0, 0x01);		//LLPLL input def 0x00
	WriteTW88(REG105, 0x2f);		//Reserved def 0x0E


	//---------------------------
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG21E, 0x03);		//BLANK def 0x00.	--enable screen blanking with AUTO

	//---------------------------
	WriteTW88Page(PAGE0_LEDC);
	WriteTW88(REG0E0, 0xF2);		//LEDC. default. 
									//	R0E0[1]=1	Analog block power down
									//	R0E0[0]=0	LEDC digital block disable
	//---------------------------
									//DCDC.	default. HW:0xF2
									//	R0E8[1]=1	Sense block power down
									//	R0E8[0]=0	DC converter digital block disable.

	WriteTW88(REG0E8, 0x70);		//Masami
	//WriteTW88(REG0E8, 0xF2);
	WriteTW88(REG0EA, 0x3F);		//Van. 110909

												//SSPLL_PowerDown
												//TWTerCmd:	"b 8a fc 77 ff"
	//WriteTW88Page(PAGE0_SSPLL);				//DocDefault:0x30. HWDefault:0xB0
	//WriteTW88(REG0FC, ReadTW88(REG0FC) | 0x80);	//	R0FC[7]=1	SSPLL power down

#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	//set external decoder
	WriteSlowI2CByte(I2CID_TW9910, 0x03, 0x82);
	WriteSlowI2CByte(I2CID_TW9910, 0x1A, 0x02);
	WriteSlowI2CByte(I2CID_TW9910, 0x1F, 0x01);
#endif



	//=====================
	//All Register setting
	//=====================

	//-------------------
	// PAGE 0
	//-------------------
	WriteTW88Page(PAGE0_GENERAL);

//R000 74	Product ID Code Register

//R002 0A	IRQ
//R003 FE	IMASK
//R004 01	Status

//R006 00	SRST

//R007 00	Output Ctrl I
//	[7]	SWPPRT
//	[6]	SWPNIT
//	[5:4]	SHFT2
//	[3]		EN656OUT
//	[2:0]	TCONSEL						
#if defined(PANEL_SRGB)
	WriteTW88(REG007, 0x01);	// Serial RGB
#elif defined(PANEL_FP_LSB)	
	WriteTW88(REG007, 0x02);	// FP LSB
#else
	WriteTW88(REG007, 0x00);	// TCON
#endif			
//R008 30	Output Ctrl II	
//	[7:6]	TCKDRV	=10	8mA	
//	[5]		TRI_FPD	=1	1:Tristate all FP data pins.
//	[4]		TRI_EN	=0	1:Tristate all output pins
//	[3:0]	GPOSEL	=9	sDE(Serial RGB DE)
//	SW use 0xA9. TRI_FPD will turn on at FP module.

#ifdef MODEL_TW8835_EXTI2C	
	WriteTW88(REG008, 0xA0);	//GPOSEL for IRQ. 0=Negative IRQ(Use).1=Positive IRQ
#elif defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88(REG008, 0x89);		
#else
	WriteTW88(REG008, 0xA9);		
#endif

//R00F 00	INT0 WRITE PORT
//R01F 00	TEST
													
//R040 00	INPUT Control I
//	[7:6]	IPHDLY	Input H cropping position control in implicit DE mode. This is a 10-bit register.
//	[4]		CKINP	Scaler input clock polarity control
//					1 = Inversion 0 = no inversion
//	[3]		DTVDE_EN
//					1 = INT_4 pin is turn into dtvde pin 
//					0 = INT_4 pin is normal operation
//	[2]		DTVCK2_EN
//					1 = dtv_vs pin is turn into 2nd DTV clock pin 
//					0 = dtv_vs pin is normal operation
//	[1:0]	IPSEL	Input selection
//					0 = Internal decoder 1 = Analog RGB/YUV 2 = DTV
	WriteTW88(REG040, 0x10);		

//R041 00	INPUT CONTROL II
//	[5]		PROG	Field control for progressive input
//	[4]		IMPDE	1 = implicit DE mode. It is only available in DTV input mode
//	[3]		IPVDET	Input V sync detection edge control
//				1 = falling edge 0 = rising edge
//	[2]		IPHDET	Input H sync detection edge control
//				1 = falling edge 0 = rising edge
//	[1]		IPFD	Input field control
//				1 = inversion 0 = no inversion
//	[0]		RGBIN	Input data format selection
//				1 = RGB 0 = YCbCr
	WriteTW88(REG041, 0x0C);

//R042 02	INPUT CROP_HI
//	[6:4]	IPVACT_HI	
//				Input V cropping length control in number of input lines for 
//				use in implicit DE mode. This is an 11-bit register.
//	[3:0]	IPHACT_HI	
//				Input H cropping length control in number of input pixels for 
//				use in implicit DE mode. This is a 12-bit register.
//R043 20	INPUT V CROP Position
//	[7:0]	IPVDLY
//				Input V cropping starting position in number of lines relative to the V sync. 
//				This is used in implicit DE mode
//R044 F0	INPUT V CROP LENGTH LO
//	[7:0]	IPVACT_LO
//				Input V cropping length control in number of input lines for 
//				use in implicit DE mode. This is an 11-bit register.
//R045 20	IINPUT H CROP POSITION LO
//	[7:0]	IPHDLY_LO
//				Input H cropping position control relative to leading edge of 
//				H sync in implicit DE mode. This is a 10-bit register.
//R046 D0	INPUT H CROP LENGTH LO
//	[7:0]	IPHACT_LO
//				Input H cropping length control in number of input pixels for 
//				use in implicit DE mode. This is a 12-bit register.
//---------------------------
//	input h start 0x082	length 0x2D0 
//  input v start 0x10	length 0x0F0	
	WriteTW88(REG042, 0x02);	
	WriteTW88(REG043, 0x10);	
	WriteTW88(REG044, 0xF0);	
	WriteTW88(REG045, 0x82);	
	WriteTW88(REG046, 0xD0);	

//R047 00	BT656 DECODER CONTROL I
//	[7]		FRUN	BT656 input control
//				0 = External input 1 = Internal pattern generator
//	[6]		HZ50	Internal pattern generator field frequency control.
//				0 = 60 Hz 1 = 50 Hz
//	[5]		DTVCKP	BT656 input clock control
//				0 = no inversion 1 = Inversion
//	[4:0]	EVDELAY	BT656 input V delay control in number of lines.
//R048 00	BT646 DECODER CONTROL II
//	[7]		NONSTA	Non-standard BT656 signal decoding.
//	[5:0]	EHDELAY	BT656 input H delay control in number of pixels.
//R049 00	BT656 Status I
//	[7:0]	LNPF656[9:2]	BT656 Input line count per frame status.
//R04A 01	BT656 STATUS II
//	[2:1]	LNPF656[1:0]
//	[0]		VDET656		BT656 input video loss detection.
//				0 = video detected 1 = no video input
	WriteTW88(REG047, 0x80);	
	//WriteTW88(REG048, 0x00);
	WriteTW88(REG049, 0x41);	//def 0x00

//R050~R05F	DTV
	//WriteTW88(REG05F, 0x00);
	
//R080~R09E	GPIO

//R0A0~ MBIST

//R0B0~ TSC CONTROL
	WriteTW88(REG0B1, 0xC0); //disable Touch Ready&Pen interrupt

//R0D4

//R0E0 LEDC CONTROL
//	WriteTW88(REG0F7, 0x16); def
//	WriteTW88(REG0F8, 0x01); def
//									
//;DC/DC setting
//	WriteTW88(REG0e8, 0xf1);  DC sense powerup. DC digital enable. DCDC-on. DONOT change it here. 
//	WriteTW88(REG0e9, 0x06);  def
//	WriteTW88(REG0ea, 0x0a);	??
//	WriteTW88(REG0eb, 0x40);	def
//	WriteTW88(REG0ec, 0x20);	def
	WriteTW88(REG0ED, 0x40);	//def:0x80
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88(REG0F1, 0xC0);	//[7]FP_PWC [6]FP_BIAS		
#endif
//
//							SSPLL FREQ R0F8[3:0]R0F9[7:0]R0FA[7:0]
//	WriteTW88(REG0F9, 0x50);	//70MHz NOT HERE
//	WriteTW88(REG0FA, 0x00);	//def
//	WriteTW88(REG0FB, 0x40);	//def
//;FPCLK_PD
//	WriteTW88(REG0FC, 0x23);	En.SSPLL
//	WriteTW88(REG0FD, 0x00);

	//-------------------
	// PAGE 1
	//-------------------
	WriteTW88Page(PAGE1_DECODER);
	WriteTW88(REG11C, 0x0F);	//*** Disable Shadow
	WriteTW88(REG102, 0x40);
	//WriteTW88(REG104, 0x00);
	//moveup		WriteTW88(REG105, 0x2f);	//Reserved def 0x0E
	WriteTW88(REG106, 0x03);	//ACNTL def 0x00
	WriteTW88(REG107, 0x02);	//Cropping Register High(CROP_HI) def 0x12
	//WriteTW88(REG108, 0x12);
	WriteTW88(REG109, 0xF0);	//Vertical Active Register,Low(VACTIVE_LO) def 0x20
	WriteTW88(REG10A, 0x0B);	//Horizontal Delay,Low(HDELAY_LO) def 0x10
	//WriteTW88(REG10B, 0xD0);
	//WriteTW88(REG10C, 0xCC);
	//WriteTW88(REG110, 0x00);
	WriteTW88(REG111, 0x5C);	//CONTRAST def 0x64
	//WriteTW88(REG112, 0x11);
	//WriteTW88(REG113, 0x80);
	//WriteTW88(REG114, 0x80);
	//WriteTW88(REG115, 0x00);
	WriteTW88(REG117, 0x80);	//Vertical Peaking Control I	def 0x30
	//WriteTW88(REG118, 0x44);
	//WriteTW88(REG11D, 0x7F);	 //BK120418. Masami request 0x01(NTSC only) on the RCD mode.
	WriteTW88(REG11E, 0x00);	//Component video format: def 0x08
	//WriteTW88(REG120, 0x50);	
	WriteTW88(REG121, 0x22);	//Individual AGC Gain def 0x48
	//WriteTW88(REG122, 0xF0);
	//WriteTW88(REG123, 0xD8);
	//WriteTW88(REG124, 0xBC);
	//WriteTW88(REG125, 0xB8);
	//WriteTW88(REG126, 0x44);
	WriteTW88(REG127, 0x38);	//Clamp Position(PCLAMP)	def 0x2A
	WriteTW88(REG128, 0x00);
	//WriteTW88(REG129, 0x00);
	//WriteTW88(REG12A, 0x78);
	WriteTW88(REG12B, 0x44);	//Comb Filter	def -100-100b
	//WriteTW88(REG12C, 0x30);
	//WriteTW88(REG12D, 0x14);
	//WriteTW88(REG12E, 0xA5);
	//WriteTW88(REG12F, 0xE0);
	WriteTW88(REG130, 0x00);
	//WriteTW88(REG133, 0x05);	//CLMD
	WriteTW88(REG134, 0x1A);
	WriteTW88(REG135, 0x00);
								//ADCLLPLL
	//moveup	WriteTW88(REG1C0, 0x01);		//LLPLL input def 0x00
	//WriteTW88(REG1C2, 0x01);
	//WriteTW88(REG1C3, 0x03);
	//WriteTW88(REG1C4, 0x5A);
	//WriteTW88(REG1C5, 0x00);
	WriteTW88(REG1C6, 0x20); //WriteTW88(REG1C6, 0x27);	//WriteTW88(REG1C6, 0x20);  Use VAdcSetFilterBandwidth()
	//WriteTW88(REG1C7, 0x04);
	//WriteTW88(REG1C8, 0x00);
	//WriteTW88(REG1C9, 0x06);
	//WriteTW88(REG1CA, 0x06);
	WriteTW88(REG1CB, 0x3A);	//WriteTW88(REG1CB, 0x30);
	//WriteTW88(REG1CC, 0x00);
	//WriteTW88(REG1CD, 0x54);
	//WriteTW88(REG1D0, 0x00);
	//WriteTW88(REG1D1, 0xF0);
	//WriteTW88(REG1D2, 0xF0);
	//WriteTW88(REG1D3, 0xF0);
	//WriteTW88(REG1D4, 0x00);
	//WriteTW88(REG1D5, 0x00);
	//WriteTW88(REG1D6, 0x10);
	//WriteTW88(REG1D7, 0x70);
	//WriteTW88(REG1D8, 0x00);
	//WriteTW88(REG1D9, 0x04);
	//WriteTW88(REG1DA, 0x80);
	//WriteTW88(REG1DB, 0x80);
	//WriteTW88(REG1DC, 0x20);
	//WriteTW88(REG1E2, 0xD9);
	//WriteTW88(REG1E3, 0x07);
	//WriteTW88(REG1E4, 0x33);
	//WriteTW88(REG1E5, 0x31);
	//WriteTW88(REG1E6, 0x00);
	//WriteTW88(REG1E7, 0x2A);

	//-------------------
	// PAGE 2
	//-------------------
	WriteTW88Page(PAGE2_SCALER);
	//WriteTW88(REG201, 0x00);
	//WriteTW88(REG202, 0x20);
	WriteTW88(REG203, 0xCC);		//XSCALE: 0x1C00. def:0x2000
	WriteTW88(REG204, 0x1C);		//XSCALE:
	WriteTW88(REG205, 0x8A);		//YSCALE: 0x0F8A. def:0x2000
	WriteTW88(REG206, 0x0F);		//YSCALE:
	WriteTW88(REG207, 0x40);		//PXSCALE[11:4]	def 0x800
	WriteTW88(REG208, 0x20);		//PXINC	def 0x10
	WriteTW88(REG209, 0x00);		//HDSCALE:0x0400 def:0x0100
	WriteTW88(REG20A, 0x04);		//HDSCALE
	WriteTW88(REG20B, 0x08);		//HDELAY2 def:0x30
	//WriteTW88(REG20C, 0xD0);	//HACTIVE2_LO
	WriteTW88(REG20D, 0x90 | (ReadTW88(REG20D) & 0x03));  //LNTT_HI def 0x80. 
	WriteTW88(REG20E, 0x20);		//HPADJ def 0x0000.  20E[6:4] HACTIVE2_HI
	WriteTW88(REG20F, 0x00);
	WriteTW88(REG210, 0x21);		//HA_POS	def 0x10
	
	//HA_LEN		Output DE length control in number of the output clocks.
	//	R212[3:0]R211[7:0] = PANEL_H+1
#if 0
	WriteTW88(REG211, 0x21);		//HA_LEN	def 0x0300
	WriteTW88(REG212, 0x03);		//PXSCALE[3:0]	def:0x800 HALEN_H[3:0] def:0x03
#else
	WriteTW88(REG211, (BYTE)(PANEL_H+1));		//HA_LEN	def 0x0300
	WriteTW88(REG212, (PANEL_H+1)>>8);		//PXSCALE[3:0]	def:0x800 HALEN_H[3:0] def:0x03
#endif
	WriteTW88(REG213, 0x00);		//HS_POS	def 0x10
	WriteTW88(REG214, 0x20);		//HS_LEN
	WriteTW88(REG215, 0x2E);		//VA_POS	def 0x20
	//VA_LEN	Output DE control in number of the output lines.
	//	R217[7:0]R216[7:0] = PANEL_V
#if 0
	WriteTW88(REG216, 0xE0);		//VA_LEN_LO	def 0x0300
	WriteTW88(REG217, 0x01);		//VA_LEN_HI
#else
	WriteTW88(REG216, PANEL_V);		//VA_LEN_LO	def 0x0300
	WriteTW88(REG217, PANEL_V>>8);	//VA_LEN_HI
#endif
	
	//WriteTW88(REG218, 0x00);	//VA_LEN_POS
	//WriteTW88(REG219, 0x00);
	//WriteTW88(REG21A, 0x00);
	//WriteTW88(REG21B, 0x00);
	WriteTW88(REG21C, 0x42);	//PANEL_FRUN. def 0x40
	//WriteTW88(REG21D, 0x00);
	WriteTW88(REG21E, 0x03);		//BLANK def 0x00.	--enable screen blanking. SW have to remove 21E[0]
	WriteTW88(REG280, 0x20);		//Image Adjustment register
	//WriteTW88(REG281, 0x80);
	//WriteTW88(REG282, 0x80);
	//WriteTW88(REG283, 0x80);
	//WriteTW88(REG284, 0x80);
	//WriteTW88(REG285, 0x80);
	//WriteTW88(REG286, 0x80);
	//WriteTW88(REG287, 0x80);
	//WriteTW88(REG288, 0x80);
	//WriteTW88(REG289, 0x80);
	//WriteTW88(REG28A, 0x80);
	WriteTW88(REG28B, 0x44);
	//WriteTW88(REG28C, 0x00);
	//WriteTW88(REG2B0, 0x10);	//Image Adjustment register	
	//WriteTW88(REG2B1, 0x40);	
	//WriteTW88(REG2B2, 0x40);
	//WriteTW88(REG2B6, 0x67);
	//WriteTW88(REG2B7, 0x94);
	//WriteTW88(REG2BF, 0x00);	//Test Pattern Generator Register
	//WriteTW88(REG2E0, 0x00);	//--disable gamma
	WriteTW88(REG2E4, 0x21);		//--dither
	//WriteTW88(REG2F8, 0x00);	//8bit Panel interface
	//WriteTW88(REG2F9, 0x00);	//8bit Panel interface	TW8832:0x80

	//-------------
	//Enable TCON
	//TW8835					TW8832
	WriteTW88Page(PAGE2_TCON);
	WriteTW88(REG240, 0x10);		//WriteTW88(REG240, 0x11);
	WriteTW88(REG241, 0x00);		//WriteTW88(REG241, 0x0A);
	WriteTW88(REG242, 0x05);		//WriteTW88(REG242, 0x05);
	WriteTW88(REG243, 0x01);		//WriteTW88(REG243, 0x01);
	WriteTW88(REG244, 0x64);		//WriteTW88(REG244, 0x64);
	WriteTW88(REG245, 0xF4);		//WriteTW88(REG245, 0xF4);
	WriteTW88(REG246, 0x00);		//WriteTW88(REG246, 0x00);
	WriteTW88(REG247, 0x0A);		//WriteTW88(REG247, 0x0A);
	WriteTW88(REG248, 0x36);		//WriteTW88(REG248, 0x36);
	WriteTW88(REG249, 0x10);		//WriteTW88(REG249, 0x10);
	WriteTW88(REG24A, 0x00);		//WriteTW88(REG24A, 0x00);
	WriteTW88(REG24B, 0x00);		//WriteTW88(REG24B, 0x00);
	WriteTW88(REG24C, 0x00);		//WriteTW88(REG24C, 0x00);
	WriteTW88(REG24D, 0x44);		//WriteTW88(REG24D, 0x44);
	WriteTW88(REG24E, 0x04);		//WriteTW88(REG24E, 0x04);

	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG006, 0x06);	//display direction. TSCP,TRSP
#if defined(PANEL_SRGB)
	WriteTW88(REG007, 0x01);	// Serial RGB
#elif defined(PANEL_FP_LSB)	
	WriteTW88(REG007, 0x02);	// FP LSB
#else
	WriteTW88(REG007, 0x00);	// TCON
#endif

#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88Page(4);
	WriteTW88(REG4E1, 0x21);			//SPI_CK_DIV[2:0]=1. SPI_CK_SEL[5:4]=2(CLKPLLMHz) will be changed later 
#endif

			
}

//-----------------------------------------------------------------------------
/**
* turn on DCDC
*
*	DCDC startup step
*	SSPLL ON
*	FP PWC ON
*	call DCDC_on(0) & DCDC_on(1)
*	delay
*	wait VBlank
*	Enable FP Data Out
*	delay
*	call DCDC_on(2)
*	FP Bias On
*
* DCDC data out needs more then 200ms delay after SSPLL_PowerUp(ON).
*/
BYTE DCDC_StartUP_sub(void)
{
	BYTE ret;
#if 0
		ret = ReadTW88(REG008);
		Printf("\nREG008:%bx FPData:%bd Output:%bx",ret, ret&0x20?0:1, ret&0x10?0:1 );
		Printf("\nREG0E8:%bx", ReadTW88(REG0E8));
		Printf("\nREG0EA:%bx", ReadTW88(REG0EA));
		ret = ReadTW88(REG0FC);
		Printf("\nREG0FC:%bx PD_SSPLL:%bd", ret, ret & 0x80);
		Printf("\nREG21E:%bx", ReadTW88(REG21E));
#endif

	//-------------
	//FPPWC ON
	FP_PWC_OnOff(ON);

	ret=DCDC_On(0);
	ret=DCDC_On(1);

	//-------------
	// wait
#ifdef EVB_10
	delay1ms(100);
#endif

	WaitVBlank(1);
	//-------------
	//FP Data Out
	OutputEnablePin(ON,ON);		//Output enable. FP data: enable


#ifdef EVB_10
	delay1ms(15);
#endif

	//DCDC final
	ret=DCDC_On(2);

	//-------------
	//FPBIAS ON 
	FP_BiasOnOff(ON);

	//disable Blank
	//WriteTW88Page(PAGE2_SCALER);
	//WriteTW88(REG21E, ReadTW88(REG21E) & ~0x01);

	PrintSystemClockMsg("DCDC_StartUp END");
	if(ret!=ERR_SUCCESS) {
		Puts(" FAIL");

		//WriteTW88Page(PAGE0_DCDC);
		//WriteTW88(REG0E8, 0xF2);	Printf("\nREG0E8:F2[%bd]",ReadTW88(REG0EA)>>4);
		//WriteTW88(REG0E8, 0x02);	Printf("\nREG0E8:02[%bd]",ReadTW88(REG0EA)>>4);
		//WriteTW88(REG0E8, 0x03);	Printf("\nREG0E8:03[%bd]",ReadTW88(REG0EA)>>4);
		//WriteTW88(REG0E8, 0x01);	Printf("\nREG0E8:01[%bd]",ReadTW88(REG0EA)>>4);
		//WriteTW88(REG0E8, 0x11);	Printf("\nREG0E8:11[%bd]",ReadTW88(REG0EA)>>4);
		//WriteTW88(REG0E8, 0x71);	Printf("\nREG0E8:71[%bd]",ReadTW88(REG0EA)>>4);
	}
	return ret;
}

//-----------------------------------------------------------------------------
/**
* turn on DCDC
*
* @see DCDC_StartUP_sub
*/
BYTE DCDC_StartUP(void)
{
	BYTE ret;
	ret=DCDC_StartUP_sub();
	if(ret == ERR_SUCCESS)
		return ERR_SUCCESS;

	ret=DCDC_StartUP_sub();
	if(ret == ERR_SUCCESS)
		return ERR_SUCCESS;

	ret=DCDC_StartUP_sub();
	return ret;
}

