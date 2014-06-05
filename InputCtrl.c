/**
 * @file
 * InputCtrl.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	see video input control 
 *
*/
/*
* video input control 
*
*	+-----+ LoSpeed  +-----+  +---------+   +-+           +------+
*	|     | Decoder  |     |=>| decoder |==>| |==========>|      |
*	|     | =======> |     |  +---------+   | |           |      |
*	|     |          |     |                | |           |      |
*	|     | HiSpeed  |inMux|                | |           |      |
*	|     | ARGB     |     |  +---------+   | |           |      |
*	|INPUT| =======> |     |=>|  ARGB   |==>| |==========>|Scaler|
*	|     |          +-----+  +---------+   | |           |      |
*	|     | Digital                         | |           |      |
*	|     | DTV                             | |  +-----+  |      |
*	|     | ===============================>| |=>| DTV |=>|      |
*	+-----+                                 +-+  +-----+  +------+
*	                                         |    +--------+
*	                                         +==> |Measure |
*	                                              +--------+
*/

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
#include "spi.h"

#include "main.h"
#include "SOsd.h"
#include "FOsd.h"
#include "decoder.h"
#include "Scaler.h"
#include "InputCtrl.h"
#include "EEPROM.h"
#include "ImageCtrl.h"
#include "Settings.h"
#include "measure.h"
#include "vadc.h"
#include "dtv.h"
#include "InputCtrl.h"
#include "OutputCtrl.h"
#include "SOsdMenu.h"
#ifdef SUPPORT_HDMI_SiIRX
#include "hdmi_SIL9127.H"
#endif
#ifdef SUPPORT_HDMI_EP9351
#include "hdmi_EP9351.H"
#endif

/*

SDTV 480i/60M
	 576i/50	
	 480p SMPTE 267M-1995
HDTV 1080i/60M
	 1080i/50
	 720p/60M
	 720p/50
	 1080p = SMPTE 274M-1995 1080p/24 & 1080p/24M
	                         1080p/50 1080p/60M


			scan lines	 field1 field2	 half
480i/60M	525			 23~262 285~524	 142x
576i/50		625			 23~310 335~622
1080i		1125
720p		750

standard
480i/60M	SMPTE 170M-1994.
			ITU-R BT.601-4
			SMPTE 125M-1995
			SMPTE 259M-1997
*/

//=============================================================================
// INPUT CONTROL
//=============================================================================
// Input Module
// start from 0x040
//0x040~0x049
//R040[1:0]	Input Select		0:InternalDecoder,1:ARGB/YUV(YPbPr),2:DTV(BT656)
//R041[0]	Input data format	0:YCbCr 1:RGB
//=============================================================================

XDATA	BYTE	InputMain;
XDATA	BYTE	InputSubMode;

//-----------------------------------------------------------------------------
/**
* Get InputMain value
*
* friend function.
* Other bank, specially Menu Bank(Bank2) needs this InputMain global variable.
*/
BYTE GetInputMain(void)
{
	return InputMain;
}
//-----------------------------------------------------------------------------
/**
* Set InputMain value
*
* @see GetInputMain
*/
void SetInputMain(BYTE input)
{
	InputMain = input;
	//update EE
}

#ifdef MODEL_TW8835_EXTI2C
#define VBLANK_WAIT_VALUE	0x0100 
#else
#define VBLANK_WAIT_VALUE	0xFFFE 
#endif

//-----------------------------------------------------------------------------
/**
* wait Vertical Blank
*
* You can use this function after you turn off the PD_SSPLL(REG0FC[7]).
* 0xFFFE value uses max 40ms on Cache + 72MHz.
*/
void WaitVBlank(BYTE cnt)
{
	XDATA	BYTE i;
	WORD loop;
	DECLARE_LOCAL_page

	ReadTW88Page(page);
	WriteTW88Page(PAGE0_GENERAL );

	for ( i=0; i<cnt; i++ ) {
		WriteTW88(REG002, 0xff );
		loop = 0;
		while (!( ReadTW88(REG002 ) & 0x40 ) ) {
			// wait VBlank
			loop++;
			if(loop > VBLANK_WAIT_VALUE  ) {
				wPrintf("\nERR:WaitVBlank");
				break;
			}
		}		
	}
	WriteTW88Page(page);
}

//-----------------------------------------------------------------------------
/**
* wait Vertical Blank
*
* @see WaitVBlank
*/
void Wait1VBlank(void)
{
	WORD loop;
	DECLARE_LOCAL_page

	ReadTW88Page(page);
	WriteTW88Page(PAGE0_GENERAL );

	WriteTW88(REG002, 0xff );
	loop = 0;
	while (!( ReadTW88(REG002 ) & 0x40 ) ) {
		// wait VBlank
		loop++;
		if(loop > VBLANK_WAIT_VALUE  ) {
			wPrintf("\nERR:WaitVBlank");
			break;
		}
	}
	WriteTW88Page(page);
}

//-----------------------------------------------------------------------------
//class:Input
/**
* Set input path & color domain
*
*	register
*	REG040[1:0]
*	REG041[0]
*
* @param path: input mode
*		- 0:InternalDecoder
*		- 1:AnalogRGB/Component. PC or Component
*		- 2:BT656(DTV). Note, HDTV use BT709.
*		- 3:DTV2 ??   <--TW8835 do not support                                  
* @param format: data format.
*		- 0:YCbCr 1:RGB
*/
void InputSetSource(BYTE path, BYTE format)
{
	BYTE r040, r041;

	WriteTW88Page( PAGE0_GENERAL );
	r040 = ReadTW88(REG040_INPUT_CTRL_I) & ~0x17;	//clear [2] also.
	r041 = ReadTW88(REG041_INPUT_CTRL_II) & ~0x3F;
	r040 |= path;
	r041 |= format;

	if(path==INPUT_PATH_DECODER) {		//InternalDecoder
		r041 |= 0x0C;					//input sync detion edge control. falling edge
	}
	else if(path==INPUT_PATH_VADC) {	//ARGB(PC or Component)
		r040 |= 0x10;					//invert clock
		if(InputMain==INPUT_COMP) {
			r041 |= 0x20;				//progressive
			r041 |= 0x10;				//implicit DE mode.(Component, don't care)
			r041 |= 0x0C;				//input sync detion edge control. falling edge
			r041 |= 0x02;				//input field inversion
		}
		else {
			//??r041 |= 0x20;			//progressive
			r041 |= 0x10;				//implicit DE mode.(Component, don't care)
			r041 |= 0x0C;				//input sync detion edge control. falling edge
		}
	}
	else if(path==INPUT_PATH_DTV) {		//DTV
										//clock normal
		r040 |= 0x08;					//INT_4 pin is turn into dtvde pin
		//r041 |= 0x20;					// progressive
		r041 |= 0x10;					//implicit DE mode
		//r041 |= 0x0C;					//input sync detion edge control. falling edge
	}
	else if(path==INPUT_PATH_BT656) {
		//target r040:0x06 r041:0x00
	}
	dPrintf("\nInputSetSource r040:%bx r041:%bx",r040,r041);
	WriteTW88(REG040_INPUT_CTRL_I,r040);
	WriteTW88(REG041_INPUT_CTRL_II,r041);
}
#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
//class:Input
void InputSetProgressiveField(fOn)
{
	WriteTW88Page(PAGE0_INPUT);
	if(fOn)	WriteTW88(REG041_INPUT_CTRL_II, ReadTW88(REG041_INPUT_CTRL_II) | 0x20);	    //On Field for Prog
	else	WriteTW88(REG041_INPUT_CTRL_II, ReadTW88(REG041_INPUT_CTRL_II) & ~0x20);	//Off Field for Prog
}
#endif

#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
//class:Input
void InputSetPolarity(BYTE V,BYTE H, BYTE F)
{
	BYTE r041;
	WriteTW88Page( PAGE0_INPUT );

	r041 = ReadTW88(REG041_INPUT_CTRL_II ) & ~0x0E;
	if(V)	r041 |= 0x08;
	if(H)	r041 |= 0x04;
	if(F)	r041 |= 0x02;
	WriteTW88(REG041_INPUT_CTRL_II, r041);
}
//-----------------------------------------------------------------------------
//class:Input
BYTE InputGetVPolarity(void)
{
	BYTE r041;
	WriteTW88Page( PAGE0_INPUT );

	r041 = ReadTW88(REG041_INPUT_CTRL_II );
	if(r041 & 0x08)	return ON;		//detect falling edge
	else			return OFF;		//detect rising edge
}
//-----------------------------------------------------------------------------
//class:Input
BYTE InputGetHPolarity(void)
{
	BYTE r041;
	WriteTW88Page( PAGE0_INPUT );

	r041 = ReadTW88(REG041_INPUT_CTRL_II );
	if(r041 & 0x04)	return ON;		//detect falling edge
	else			return OFF;		//detect rising edge
}
//-----------------------------------------------------------------------------
//class:Input
BYTE InputGetFieldPolarity(void)
{
	BYTE r041;
	WriteTW88Page( PAGE0_INPUT );

	r041 = ReadTW88(REG041_INPUT_CTRL_II );
	if(r041 & 0x02)	return ON;		//input field inversion
	else			return OFF;		//
}
#endif

#if defined(SUPPORT_COMPONENT)
//-----------------------------------------------------------------------------
//class:Input
/**
* set Field Polarity
*
* R041[1] input field control. 1:inversion
*/
void InputSetFieldPolarity(BYTE fInv)
{
	BYTE r041;
	WriteTW88Page( PAGE0_INPUT );

	r041 = ReadTW88(REG041_INPUT_CTRL_II );
	if(fInv)	WriteTW88(REG041_INPUT_CTRL_II, r041 | 0x02);
	else 		WriteTW88(REG041_INPUT_CTRL_II, r041 & ~0x02);
}
#endif

#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
//class:Input
/*
* R041[0] Input data format selection 1:RGB
*/
BYTE InputGetColorDomain(void)
{
	BYTE r041;
	WriteTW88Page( PAGE0_INPUT );

	r041 = ReadTW88(REG041_INPUT_CTRL_II );
	if(r041 & 0x01)	return ON;		//RGB color
	else			return OFF;		//YUV color
}
#endif

//-----------------------------------------------------------------------------
//class:Input
/**
* set input crop
*
* input cropping for implicit DE.
* NOTE:InternalDecoder is not an implicit DE.
*
*	register
*	REG040[7:6]REG045[7:0]	HCropStart
*			   REG043[7:0]	VCropStart
*	REG042[6:4]REG044[7:0]	VCropLength
*	REG042[3:0]REG046[7:0]	HCropLength
*/
void InputSetCrop( WORD x, WORD y, WORD w, WORD h )
{
	WriteTW88Page( PAGE0_INPUT );

	WriteTW88(REG040_INPUT_CTRL_I, (ReadTW88(REG040_INPUT_CTRL_I) & 0x3F) | ((x & 0x300)>>2) );
	WriteTW88(REG045, (BYTE)x);
	WriteTW88(REG043, (BYTE)y);

	WriteTW88(REG042, ((h&0xF00) >> 4)|(w >>8) );
	WriteTW88(REG044, (BYTE)h);
	WriteTW88(REG046, (BYTE)w);
	//dPrintf("\nInput Crop Window: x = %d, y = %d, w = %d, h = %d", x, y, w, h );
}

#if 0
//-----------------------------------------------------------------------------
//class:Input
void InputSetCropStart( WORD x, WORD y)
{
	WriteTW88Page( PAGE0_INPUT );
	WriteTW88(REG040, (ReadTW88(REG040) & 0x3F) | ((x & 0xF00)>>2) );
	WriteTW88(REG045, (BYTE)x);
	WriteTW88(REG043, (BYTE)y);
	//dPrintf("\nInput Crop Window: x = %d, y = %d", x, y);
}
#endif

#ifdef SUPPORT_PC
//-----------------------------------------------------------------------------
//class:Input
/**
* set Horizontal Start at InputCrop
*/
void InputSetHStart( WORD x)
{
	WriteTW88Page( PAGE0_INPUT );

	WriteTW88(REG040, (ReadTW88(REG040) & 0x3F) | ((x & 0xF00)>>2) );
	WriteTW88(REG045, (BYTE)x);
	//dPrintf("\nInput Crop Window: x = %d", x);
}
#endif
//-----------------------------------------------------------------------------
//class:Input
/**
* get Horizontal Start at InputCrop
*/
WORD InputGetHStart(void)
{
	WORD wValue;
	WriteTW88Page( PAGE0_INPUT );

	wValue = ReadTW88(REG040) & 0xC0;
	wValue <<= 2;
	wValue |=  ReadTW88(REG045);
	return wValue;
}

#if 0
//-----------------------------------------------------------------------------
//class:Input
void InputSetVStart( WORD y)
{
	WriteTW88Page( PAGE0_INPUT );

	WriteTW88(REG043, (BYTE)y);
	dPrintf("\nInput Crop Window: y = %d", y);
}
//-----------------------------------------------------------------------------
//class:Input
WORD InputGetVStart(void)
{
	WORD wValue;
	WriteTW88Page( PAGE0_INPUT );

	wValue = ReadTW88(REG043 );
	return wValue;
}
#endif

#if 0
//-----------------------------------------------------------------------------
//class:Input
WORD InputGetHLen(void)
{
	WORD len;
	WriteTW88Page( PAGE0_INPUT );
	len =ReadTW88(REG042) & 0x0F;
	len <<=8;
	len |= ReadTW88(REG046);
	return len;
}
//-----------------------------------------------------------------------------
//class:Input
WORD InputGetVLen(void)
{
	WORD len;
	WriteTW88Page( PAGE0_INPUT );
	len =ReadTW88(REG042) & 0x70;
	len <<=4;
	len |= ReadTW88(REG044);
	return len;
}
#endif

#if 0
//-----------------------------------------------------------------------------
//class:BT656Input
//register
//	R047[7]	BT656 input control	0:External input, 1:Internal pattern generator
void BT656InputSetFreerun(BYTE fOn)
{
	WriteTW88Page(PAGE0_INPUT);
	if(fOn)	WriteTW88(REG047,ReadTW88(REG047) | 0x80);
	else	WriteTW88(REG047,ReadTW88(REG047) & ~0x80);
}
#endif
//-----------------------------------------------------------------------------
//class:BT656Input
/**
* set Freerun and invert clock flag on BT656
*
*	R047[7]
*	R047[5]
*/
void BT656InputFreerunClk(BYTE fFreerun, BYTE fInvClk)
{
	BYTE value;
	WriteTW88Page(PAGE0_INPUT);
	value = ReadTW88(REG047);
	if(fFreerun)	value |= 0x80;
	else			value &= ~0x80;
	
	if(fInvClk)		value |= 0x20;
	else			value &= ~0x20;
	WriteTW88(REG047, value);
}

//-----------------------------------------------------------------------------
/**
* print Input string
*/
void PrintfInput(BYTE Input, BYTE debug)
{
	if(debug==3) {
		dPuts("\nInput:");
		switch(Input) {
		case 0: dPrintf("CVBS"); 					break;
		case 1: dPrintf("SVIDEO"); 					break;
		case 2: dPrintf("Component"); 				break;
		case 3: dPrintf("PC"); 						break;
		case 4: dPrintf("DVI"); 					break;
		case 5: dPrintf("HDMIPC"); 					break;
		case 6: dPrintf("HDMITV"); 					break;
		case 7: dPrintf("BT656");					break;
		default: dPrintf("unknown:%02bd",Input); 	break;
		}
	}
	else {
		Puts("\nInput:");
		switch(Input) {
		case 0: Printf("CVBS"); 					break;
		case 1: Printf("SVIDEO"); 					break;
		case 2: Printf("Component"); 				break;
		case 3: Printf("PC"); 						break;
		case 4: Printf("DVI"); 						break;
		case 5: Printf("HDMIPC"); 					break;
		case 6: Printf("HDMITV"); 					break;
		case 7: Printf("BT656");					break;
		default: Printf("unknown:%02bd",Input); 	break;
		}
	}
}

//-----------------------------------------------------------------------------
/**
* Change Video Input.
*
* @param mode
*	- INPUT_CVBS : ChangeCVBS
*	- INPUT_SVIDEO: ChangeCVBS
*	- INPUT_COMP : ChangeCOMPONENT
*	- INPUT_PC :  ChangePC
*	- INPUT_DVI : ChangeDVI
* 	- INPUT_HDMIPC:
*	- INPUT_HDMITV:	ChangeHDMI
*	- INPUT_BT656: ChangeBT656
* @see ChangeCVBS
*/
void ChangeInput( BYTE mode )
{
	dPrintf("\nChangeInput:%02bx", mode);

	if(getNoSignalLogoStatus())
		RemoveLogo();


	PrintfInput(mode,3);
	switch ( mode ) {
#ifdef SUPPORT_CVBS
		case INPUT_CVBS:
			ChangeCVBS();
			break;
#endif
#ifdef SUPPORT_SVIDEO
		case INPUT_SVIDEO:
			ChangeSVIDEO();
			break;
#endif
#ifdef SUPPORT_COMPONENT
		case INPUT_COMP:
			ChangeCOMPONENT();
			break;
#endif
#ifdef SUPPORT_PC
		case INPUT_PC:
			ChangePC();
			break;
#endif
#ifdef SUPPORT_DVI
		case INPUT_DVI:
			ChangeDVI();
			break;
#endif
#if defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
		case INPUT_HDMIPC:
		case INPUT_HDMITV:
			ChangeHDMI();
			break;
#endif
#ifdef SUPPORT_BT656
		case INPUT_BT656:
			ChangeBT656();
			break;
#endif
		default:
			ChangeCVBS();
			break;
	}
}
//-----------------------------------------------------------------------------
/**
* move to next video input
*/
void	InputModeNext( void )
{
	BYTE next_input;

#if defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
	if(InputMain==INPUT_HDMIPC)
		next_input = InputMain + 2;
	else
#endif
	next_input = InputMain + 1;

	do {
		if(next_input == INPUT_TOTAL)
			next_input = INPUT_CVBS;	
#ifndef SUPPORT_CVBS
		if(next_input==INPUT_CVBS)
			next_input++;	
#endif
#ifndef SUPPORT_SVIDEO
		if(next_input==INPUT_SVIDEO)
			next_input++;	
#endif
#ifndef SUPPORT_COMPONENT
		if(next_input==INPUT_COMP)
			next_input++;	
#endif
#ifndef SUPPORT_PC
		if(next_input==INPUT_PC)
			next_input++;	
#endif
#ifndef SUPPORT_DVI
		if(next_input==INPUT_DVI)
			next_input++;	
#endif
#if defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
		if(next_input==INPUT_HDMIPC)
			next_input+=2;	
		else if(next_input==INPUT_HDMITV)
			next_input++;
#endif
#ifndef SUPPORT_BT656
		if(next_input==INPUT_COMP)
			next_input++;	
#endif
#if defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
		if(next_input==INPUT_HDMIPC) {
			if(GetHdmiModeEE())  next_input = INPUT_HDMITV;
		}
#endif
	} while(next_input==INPUT_TOTAL);

	ChangeInput(next_input);
}


//=============================================================================
// Input Control routine
//=============================================================================

extern CODE BYTE DataInitNTSC[];


//-----------------------------------------------------------------------------
/**
* prepare video input register after FW download the default init values.
*
*	select input path
*	turnoff freerun manual & turnon freerun auto.
*	assign default freerun Htotal&Vtotal
*
* @see I2CDeviceInitialize
*/		
void InitInputAsDefault(void)
{
	//---------------------------------
	//step1:
	//Before FW starts the ChangeInput, 
	//		link ISR & turnoff signal interrupt & NoSignal task,
	//		turn off LCD.
	FOsdIndexMsgPrint(FOSD_STR5_INPUTMAIN);		//prepare InputMain string

	LinkCheckAndSetInput();						//link CheckAndSetInput
	Interrupt_enableVideoDetect(OFF);			//turnoff Video Signal Interrupt
	TaskNoSignal_setCmd(TASK_CMD_DONE);			//turnoff NoSignal Task
	LedBackLight(OFF);							//turnoff LedBackLight

	//---------------------------------
	//step2:
	//set system default
	//	Download the default register values.
	//	set sspll
	//	select MCU/SPI Clock
 	dPuts("\nI2CDownload DataInitNTSC");
	I2CDeviceInitialize( DataInitNTSC, 0 );	 //Pls, do not use this ....
#ifdef MODEL_TW8835_EXTI2C_USE_PCLK
	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E0, 0x00);				//use PCLK
#endif

	if(SpiFlashVendor==SFLASH_VENDOR_MX) {
		WriteTW88Page(PAGE4_CLOCK);
		WriteTW88(REG4E1, (ReadTW88(REG4E1) & 0xF8) | 0x02);	//if Macronix SPI Flash, SPI_CK_DIV[2:0]=2
	}		
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	//If you want CKLPLL, select MCUSPI_CLK_PCLK 
	McuSpiClkSelect(MCUSPI_CLK_PCLK);
#endif


	//---------------------------------
	//step3:
	//	InputSource=>InMux=>Decoder=>VAdc=>BT656=>DTV=>Scaler=>Measure
	//-------------------

	//InputSource
	switch(InputMain) {
	case INPUT_CVBS:
	case INPUT_SVIDEO:
		InputSetSource(INPUT_PATH_DECODER,INPUT_FORMAT_YCBCR);
		break;
	case INPUT_COMP:	//target R040:31 R041:3E
		InputSetSource(INPUT_PATH_VADC,INPUT_FORMAT_YCBCR);		
		break;
	case INPUT_PC:		//target R040:31 R041:1D
		InputSetSource(INPUT_PATH_VADC,INPUT_FORMAT_RGB);		
		break;
	case INPUT_DVI:		//target R040:2A R041:11. Note:DtvInitDVI() overwite R040.
		InputSetSource(INPUT_PATH_DTV,INPUT_FORMAT_RGB);		
		break;
	case INPUT_HDMIPC:
	case INPUT_HDMITV:	//target R040: R041:
		InputSetSource(INPUT_PATH_DTV,INPUT_FORMAT_RGB);		
		break;
	case INPUT_BT656:	//target R040:2A R041:00
		InputSetSource(INPUT_PATH_BT656,INPUT_FORMAT_YCBCR);	 
		break;
	}

	//InMux
	InMuxSetInput(InputMain);

	//Decoder	
	DecoderFreerun(DECODER_FREERUN_AUTO);	//component,pc,dvi removed

	//aRGB(VAdc)
	VAdcSetDefaultFor();

	if(InputMain==INPUT_BT656)
		BT656OutputEnable(ON,0);			//R007[3]=1.DataInitNTSC clear it.
	else
		BT656OutputEnable(OFF, 1);

	//BT656Input
	switch(InputMain) {
	case INPUT_CVBS:
	case INPUT_SVIDEO:
	case INPUT_COMP:
	case INPUT_PC:
		break;
	case INPUT_DVI:
		BT656InputFreerunClk(OFF, OFF);		//BT656 turnoff FreeRun
		break;
	case INPUT_HDMIPC:
	case INPUT_HDMITV:
		BT656InputFreerunClk(OFF, OFF);		//BT656 turnoff FreeRun
		break;
	case INPUT_BT656:
		BT656InputFreerunClk(OFF, ON);		//off freerun, on invert_clk
		break;
	}

	//DTV
	switch(InputMain) {
	case INPUT_CVBS:
	case INPUT_SVIDEO:
	case INPUT_COMP:
	case INPUT_PC:
		break;
#ifdef SUPPORT_DVI
	case INPUT_DVI:
		DtvSetClockDelay(1);
		DtvSetVSyncDelay(4);

		DtvSetFieldDetectionRegion(ON,0x11);	// set Det field by WIN
		DtvSetPolarity(0,0);
		DtvSetRouteFormat(DTV_ROUTE_YPbPr,DTV_FORMAT_RGB565);
#ifdef MODEL_TW8835FPGA
		SetExtVAdcI2C(0x9A, 0);	//AD9883. VGA
	//	SetExtVAdcI2C(0x9A, 6);		//480P
#endif
		break;
#endif
	case INPUT_HDMIPC:
	case INPUT_HDMITV:
#if defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)

#ifdef SUPPORT_HDMI_EP9351 
		//default is RGB565. test RGB24.
#ifdef SUPPORT_HDMI_24BIT
		DtvSetRouteFormat(DTV_ROUTE_PrYPb,DTV_FORMAT_RGB); //RGB24. 120720
#else
		DtvSetRouteFormat(DTV_ROUTE_YPbPr,DTV_FORMAT_RGB565);
#endif
#endif
#ifdef SUPPORT_HDMI_SiIRX
		DtvSetRouteFormat(DTV_ROUTE_RGB, DTV_FORMAT_RGB);
#endif
#ifdef MODEL_TW8835FPGA
		SetExtVAdcI2C(0x9A, 0);	//AD9883. VGA
	//	SetExtVAdcI2C(0x9A, 6);		//480P
#endif
		DtvSetClockDelay(1);	//BK111201
		DtvSetVSyncDelay(4);	//BK111201

		DtvSetFieldDetectionRegion(ON,0x11);	// set Det field by WIN
#endif
		break;
	case INPUT_BT656:
#ifdef SUPPORT_BT656
		DtvSetRouteFormat(DTV_ROUTE_PrYPb,DTV_FORMAT_INTERLACED_ITU656);
#endif
#ifdef MODEL_TW8835FPGA
		SetExtVAdcI2C(0x9A, 0);	//AD9883. VGA
	//	SetExtVAdcI2C(0x9A, 6);		//480P
#endif
		break;
	}
//BKTODO:120423. power down the external chip.
//#if defined(SUPPORT_HDMI_EP9351)
//	if(InputMain != INPUT_HDMIPC && InputMain != INPUT_HDMITV) {
//		//power down
//	}
//#endif


	//scaler
	ScalerSetFreerunManual( OFF );		//component,pc,dvi removed

	//measure
	switch(InputMain) {
	case INPUT_CVBS:
	case INPUT_SVIDEO:
	case INPUT_COMP:
	case INPUT_PC:
		//BKFYI. CVBS&SVIDEO was MeasSetWindow( 0, 0, 2200,1125 );
		MeasSetWindow( 0, 0, 0xfff, 0xfff );	//set dummy window. 1600x600
		MeasSetField(2);						// field:Both
		MeasEnableDeMeasure(OFF);				// Disable DE Measure
		MeasSetThreshold(0x40);					// Threshold active detection
		break;
	case INPUT_DVI:
	case INPUT_HDMIPC:
	case INPUT_HDMITV:
		MeasSetWindow( 0, 0, 0xfff, 0xfff );	//set dummy window. 1600x600
		MeasSetField(2);						//field:Both
		MeasEnableDeMeasure(ON);				//Enable DE Measure
		MeasSetThreshold(0x40);					//Threshold active detection
		MeasSetErrTolerance(4);					//tolerance set to 32
		MeasEnableChangedDetection(ON);			//set EN. Changed Detection
		break;
	case INPUT_BT656:
		break;
	}

	//image effect
	SetImage(InputMain);	//set saved image effect(contrast,....)
}


//-----------------------------------------------------------------------------
/**
* enable video Output 
*
* call when CheckAndSet[Input] is successed.
*/
void VInput_enableOutput(BYTE fRecheck)
{
	if(fRecheck) {
		dPrintf("====Found Recheck:%d",VH_Loss_Changed);
		// do not turn on here. We need a retry.
	}
	else {
		ScalerSetMuteManual( OFF );		//TurnOn Video Output. Remove Black Video
		ScalerSetFreerunManual( OFF );	//FreeRunOff,UseVideoInputSource
		ScalerSetFreerunValue(0);		//caclulate freerun value

		SpiOsdSetDeValue();
		FOsdSetDeValue();

		LedBackLight(ON);				//TurnOn Display
	}
	TaskNoSignal_setCmd(TASK_CMD_DONE);
	
	Interrupt_enableVideoDetect(ON);

	if(InputMain == INPUT_DVI
	|| InputMain == INPUT_HDMIPC
	|| InputMain == INPUT_HDMITV
	|| InputMain == INPUT_BT656 ) {	
		//digital input.
		; //SKIP
	}
	else
		Interrupt_enableSyncDetect(ON);
#ifdef PICO_GENERIC
	FLCOS_toggle();
#endif
}

//-----------------------------------------------------------------------------
/**
* goto Freerun move
*
* call when CheckAndSet[Input] is failed.
* oldname: VInputGotoFreerun
* input
*	reason
*		0: No Signal
*		1: No STD
*		2: Out of range
*/
void VInput_gotoFreerun(BYTE reason)
{
	ScalerCheckPanelFreerunValue();

	//Freerun
	if(InputMain == INPUT_BT656) {
		//??WHY
	}
	else {
		DecoderFreerun(DECODER_FREERUN_60HZ);
	}
	ScalerSetFreerunManual( ON );

	if(InputMain == INPUT_HDMIPC 
	|| InputMain == INPUT_HDMITV
	|| InputMain == INPUT_BT656) {
		//??WHY
	}
	else {
		ScalerSetMuteManual( ON );
	}
	// Prepare NoSignal Task...
	if(reason==0 && MenuGetLevel()==0) { //0:NoSignal 1:NO STD,...
		if(access) {
			FOsdSetDeValue();
			FOsdIndexMsgPrint(FOSD_STR2_NOSIGNAL);
			tic_task = 0;

#ifdef NOSIGNAL_LOGO
			if(getNoSignalLogoStatus() == 0)
				InitLogo1();						
#endif
			TaskNoSignal_setCmd(TASK_CMD_WAIT_VIDEO);
		}
	}

	if(InputMain == INPUT_PC) {
		//BK111019. I need a default RGB_START,RGB_VDE value for position menu.
		RGB_HSTART = InputGetHStart();
		RGB_VDE = ScalerReadVDEReg();
	}

	LedBackLight(ON);

	Interrupt_enableVideoDetect(ON);
}


//=============================================================================
// Change to DECODER. (CVBS & SVIDEO)
//=============================================================================

//-----------------------------------------------------------------------------
/**
* check and set the decoder input
*
* @return
*	0: success
*	1: VDLOSS
*	2: No Standard
*	3: Not Support Mode
*
* extern
*	InputSubMode
*/
BYTE CheckAndSetDecoderScaler( void )
{
	BYTE	mode;
	DWORD	vPeriod, vDelay;
	BYTE vDelayAdd;
	DWORD x_ratio, y_ratio;

	dPrintf("\nCheckAndSetDecoderScaler start.");

	if ( DecoderCheckVDLOSS(100) ) {
		ePuts("\nCheckAndSetDecoderScaler VDLOSS");
		DecoderFreerun(DECODER_FREERUN_60HZ);
		ScalerSetFreerunManual( ON );
		return( 1 );
	}
	//get standard
	mode = DecoderCheckSTD(100);
	if ( mode == 0x80 ) {
	    ePrintf("\nCheckAndSetDecoderScaler NoSTD");
		return( 2 );
	}
	mode >>= 4;
	InputSubMode = mode;

	VideoAspect = GetAspectModeEE();

	//read VSynch Time+VBackPorch value
	vDelay = DecoderGetVDelay();

	//reduce VPeriod to scale up.
	//and adjust V-DE start.

	//720x240 => 800x480
	x_ratio = PANEL_H;
	x_ratio *=100;
	x_ratio /= 720;
	y_ratio = PANEL_V;
	y_ratio *=100;
	y_ratio /= 480;
	dPrintf("\nXYRatio X:%ld Y:%ld",x_ratio,y_ratio);

	if(VideoAspect==VIDEO_ASPECT_ZOOM) {
		if(x_ratio > y_ratio) {
			dPrintf(" use x. adjust Y");
			y_ratio = 0;
		}
		else {
			dPrintf(" use y. adjust X");	
			x_ratio = 0;
		}
	}
	else if(VideoAspect==VIDEO_ASPECT_NORMAL) {
		if(x_ratio > y_ratio) {
			dPrintf(" use y. adjust X");
			x_ratio = 0;
		}
		else {
			dPrintf(" use x. adjust Y");
			y_ratio = 0;
		}
	}
	else {
		x_ratio = 0;
		y_ratio = 0;
	}
	//720x288 => 800x480

	if ( mode == 0 ) {				// NTSC(M)
		vPeriod = 228;				// NTSC line number.(reduced from 240 to 228)
		//vDelay += 12; 			// (6 = ( 240-228 ) / 2) times 2 because. 240->480
		//vDelay += 27;				// for V Back Porch & V top border
		vDelayAdd = 39;

		if(VideoAspect==VIDEO_ASPECT_ZOOM) {
			vDelayAdd += 30;
			vDelayAdd += 5; //???
		}

		DecoderSetVActive(240);		//set:240 0x0F0

		//prepare info
		FOsdSetInputMainString2FOsdMsgBuff();	//GetInputMainString(FOsdMsgBuff);									 	
		TWstrcat(FOsdMsgBuff," NTSC");			//BK110811. call FOsdCopyMsgBuff2Osdram(OFF); to draw
	}
	else if ( mode == 1 ) {			 //PAL(B,D,G,H,I)
		vPeriod = 275;				// PAL line number,(Reduced from 288 to 275)
#if 0
		//vDelay += 7; 				// 6.7 = ( 288-275 ) / 2
		//vDelay += 2;				// add some more for V Back Porch & V Top border
		vDelayAdd = 25;
#else
		//vDelay += 14; 			// (6.7 = ( 288-275 ) / 2  ) * 2
		//vDelay += 25;				// add some more for V Back Porch & V Top border
		vDelayAdd = 39;
#endif
		if(VideoAspect==VIDEO_ASPECT_ZOOM)
			vDelayAdd += 33;

		DecoderSetVActive(288);		//set:288. 0x120
		 
		//prepare info
		FOsdSetInputMainString2FOsdMsgBuff();	//GetInputMainString(FOsdMsgBuff);									 	
		TWstrcat(FOsdMsgBuff," PAL");			//BK110811. call FOsdCopyMsgBuff2Osdram(OFF); to draw
	}
	//BKTODO: Support more mode
	//0 = NTSC(M)          
	//1 = PAL (B,D,G,H,I)          
	//2 = SECAM          
	//3 = NTSC4.43
	//4 = PAL (M)            
	//5 = PAL (CN)                     
	//6 = PAL 60  
	else if ( mode == 3 //MTSC4
		   || mode == 4 //PAL-M
	       || mode == 6 //PAL-60			 
	) {				
		vPeriod = 228;
		vDelayAdd = 39;
 		if(VideoAspect==VIDEO_ASPECT_ZOOM) {
			vDelayAdd += 30;
			vDelayAdd += 5; //???
		}
 		DecoderSetVActive(240);		//set:240 0x0F0

		//prepare info
		FOsdSetInputMainString2FOsdMsgBuff();									 	
		if(mode==3) TWstrcat(FOsdMsgBuff," NTSC4");		
		if(mode==4) TWstrcat(FOsdMsgBuff," PAL-M");		
		if(mode==6) TWstrcat(FOsdMsgBuff," PAL-60");		
   }     
	else if ( mode == 2 //SECAM
		  ||  mode == 4 //PAL-CN
	) {	
		vPeriod = 275;
		vDelayAdd = 39;			
		if(VideoAspect==VIDEO_ASPECT_ZOOM)
			vDelayAdd += 33;

		DecoderSetVActive(288);		//set:288. 0x120
		 
		//prepare info
		FOsdSetInputMainString2FOsdMsgBuff();									 	
		if(mode==2) TWstrcat(FOsdMsgBuff," SECAM");
		if(mode==4) TWstrcat(FOsdMsgBuff," PAL-CN");
	}
	else {
		ePrintf( "\nCheckAndSetDecoderScaler Input Mode %bd does not support now", mode );
		return(3);
	}
	
	ScalerSetLineBufferSize(720);	//BK120116	- temp location. pls remove

	if(y_ratio) ScalerSetHScaleWithRatio(720, (WORD)y_ratio);
	else		ScalerSetHScale(720);					//PC->CVBS need it.
	if(x_ratio)	ScalerSetVScaleWithRatio(vPeriod, (WORD)x_ratio);
	else 		ScalerSetVScale(vPeriod);				//R206[7:0]R205[7:0]	= vPeriod
	
	ScalerWriteVDEReg(vDelay+vDelayAdd);			//R215[7:0]=vDelay, R217[3:0]R216[7:0]=PANEL_V

 	//dPrintf( "\nInput_Mode:%02bx VDE_width:%ld, vBackPorch:%ld", mode, vPeriod, vDelay );
	ePrintf( "\nInput_Mode:%s VDE_width_for_scaler:%ld, V-DE:%ld+%bd", mode ? "PAL":"NTSC", vPeriod, vDelay,vDelayAdd );
	
	return(0);
}


//-----------------------------------------------------------------------------
/**
* Change to Decoder
*
* extern
*	InputMain
*	InputSubMode
* @param
*	fSVIDEO		0:CVBS, 1:SVIDEO
* @return
*	- 0: success
*	- 1: No Update happen
*	- 2: No Signal or unknown video sidnal.
*	- 3: NO STD
* @see InitInputAsDefault
* @see CheckAndSetDecoderScaler
* @see VInput_enableOutput
* @see VInput_gotoFreerun
*/
BYTE ChangeDecoder(BYTE fSVIDEO)
{
	BYTE ret;

	if(fSVIDEO) {
		if ( InputMain == INPUT_SVIDEO ) {
			dPrintf("\nSkip ChangeSVIDEO");
			return(1);
		}
		InputMain = INPUT_SVIDEO;
	}
	else {
		if ( InputMain == INPUT_CVBS ) {
			dPrintf("\nSkip ChangeCVBS");
			return(1);
		}
		InputMain = INPUT_CVBS;
	}
	InputSubMode = 7; //N/A

	if(GetInputEE() != InputMain) 	
		SaveInputEE( InputMain );

	//----------------
	// initialize video input
	InitInputAsDefault();


	//BKFYI: We need a delay before call DecoderCheckVDLOSS() on CheckAndSetDecoderScaler()
	//But, if fRCDMode, InputMode comes from others, not CVBS, not SVIDEO. We don't need a delay 
	delay1ms(350);

	//
	// Check and Set 
	//
	ret = CheckAndSetDecoderScaler();	//same as CheckAndSetInput()
	if(ret==ERR_SUCCESS) {
		//success
		VInput_enableOutput(0);
		return 0;
	}
	//------------------
	// NO SIGNAL
	//------------------
	VInput_gotoFreerun(ret-1);	//1->0:NoSignal 2->1:NO STD

	return (ret+1);	 //2:NoSignal 3:NO STD
}

//-----------------------------------------------------------------------------
/**
* Change to CVBS
*
* @return
*	- 0: success
*	- 1: No Update happen
*	- 2: No Signal or unknown video sidnal.
*	- 3: NO STD
* @see ChangeDecoder
*/
BYTE ChangeCVBS( void )
{
	return ChangeDecoder(0);
}

//-----------------------------------------------------------------------------
/**
* Change to SVIDEO
*
* @return
*	- 0: success
*	- 1: No Update happen
*	- 2: No Signal or unknown video sidnal.
*	- 3: NO STD
* @see ChangeDecoder
*/
BYTE	ChangeSVIDEO( void )
{
	return ChangeDecoder(1);
}

//=============================================================================
// Change to COMPONENT (YPBPR)
//=============================================================================


#ifdef SUPPORT_COMPONENT
//-----------------------------------------------------------------------------
/**
* Change to Component
*
* @return
*	- 0: success
*	- 1: No Update happen
*	- 2: No Signal or unknown video sidnal.
*/
BYTE	ChangeCOMPONENT( void )
{
	BYTE ret;

	if ( InputMain == INPUT_COMP ) {
		dPrintf("\nSkip ChangeCOMPONENT");
		return(1);
	}
		
	InputMain = INPUT_COMP;
	InputSubMode = 7; //N/A.Note:7 is a correct value.

 	if(GetInputEE() != InputMain)
		SaveInputEE( InputMain );

	//----------------
	// initialize video input
	InitInputAsDefault();

	//
	// Check and Set with measure
	//
	//
	// Check and Set VADC,mesaure,Scaler for Component input
	//
	ret = CheckAndSetComponent();		//same as CheckAndSetInput()
	if(ret==ERR_SUCCESS) {
		//success
		VInput_enableOutput(0);
		return 0;
	}
	//------------------
	// NO SIGNAL

	//InputVAdcMode = 0;

	//start recover & force some test image.
	VInput_gotoFreerun(0);

	return(2);  //fail
}
#endif
//=============================================================================
// Change to PC
//=============================================================================


//-----------------------------------------------------------------------------
//BYTE last_position_h;
//BYTE last_position_v;
//BYTE temp_position_h;
//BYTE temp_position_v;
#ifdef SUPPORT_PC
//-----------------------------------------------------------------------------
/**
* Change to PC
*
* @return
*	- 0: success
*	- 1: No Update happen
*	- 2: No Signal or unknown video sidnal.
*/
BYTE ChangePC( void )
{
	BYTE ret;

	if ( InputMain == INPUT_PC ) {
		dPrintf("\nSkip ChangePC");
		return(1);
	}

	InputMain = INPUT_PC;
	InputSubMode = 0; //N/A

	if(GetInputEE() != InputMain)
		SaveInputEE( InputMain );

	//----------------
	// initialize video input
	InitInputAsDefault();

	//
	// Check and Set VADC,mesaure,Scaler for Analog PC input
	//
	ret = CheckAndSetPC();		//same as CheckAndSetInput()
	if(ret==ERR_SUCCESS) {
		//success
		VInput_enableOutput(0);
		return 0;
	}


	//------------------
	// NO SIGNAL
	// Prepare NoSignal Task...


	//free run		
	//start recover & force some test image.
	VInput_gotoFreerun(0);


	return 2;	//fail..
}
#endif

//=============================================================================
// Change to DVI
//=============================================================================

#ifdef SUPPORT_DVI
//-----------------------------------------------------------------------------
/**
* Change to DVI
*
* linked with SIL151
* @return
*	- 0: success
*	- 1: No Update happen
*	- 2: No Signal or unknown video sidnal.
*/
BYTE	ChangeDVI( void )
{
	BYTE ret;

	if ( InputMain == INPUT_DVI ) {
		dPrintf("\nSkip ChangeDVI");
		return(1);
	}

	InputMain = INPUT_DVI;

	if(GetInputEE() != InputMain)
		SaveInputEE( InputMain );

	//----------------
	// initialize video input
	InitInputAsDefault();

	//
	// Check and Set VADC,mesaure,Scaler for Analog PC input
	//
	ret = CheckAndSetDVI();		//same as CheckAndSetInput()
	if(ret==0) {
		//success
		VInput_enableOutput(0);
		return 0;
	}

	//------------------
	// NO SIGNAL
	// Prepare NoSignal Task...
	VInput_gotoFreerun(0);

	//dPrintf("\nChangeDVI--END");
	return(2);
}
#endif

//=============================================================================
// Change to HDMI
//=============================================================================

//-----------------------------------------------------------------------------
/**
* Change to HDMI
*
* linked with EP9351
* @return
*	- 0: success
*	- 1: No Update happen
*	- 2: No Signal or unknown video sidnal.
*/
BYTE ChangeHDMI(void)
{
	BYTE ret;
#ifdef SUPPORT_HDMI_EP9351
//	BYTE i;
//	volatile BYTE r3C, r3D;
#endif

	if ( InputMain == INPUT_HDMIPC || InputMain == INPUT_HDMITV ) {
		dPrintf("\nSkip ChangeHDMI");
		return(1);
	}

	if(GetHdmiModeEE())  InputMain = INPUT_HDMITV;
	else 				 InputMain = INPUT_HDMIPC;

	if(GetInputEE() != InputMain)
		SaveInputEE( InputMain );

	dPrintf("\nChangeHDMI InputMain:%02bx",InputMain);

	//----------------
	// initialize video input
	InitInputAsDefault();

#ifdef SUPPORT_HDMI_SiIRX
	HdmiCheckDeviceId();
	HdmiInitReceiverChip();
#endif
#ifdef SUPPORT_HDMI_EP9351
	//wakeup(turn off the power down)
	HdmiInitEp9351Chip();
#endif

	//
	// Check and Set 
	//
#if defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
	ret = CheckAndSetHDMI();
#else
	ret=ERR_FAIL;
#endif
	if(ret==ERR_SUCCESS) {
		//success
		VInput_enableOutput(0);
		return 0;
	}

	//------------------
	// NO SIGNAL
	// Prepare NoSignal Task...
	VInput_gotoFreerun(0);
	//dPrintf("\nChangeHDMI--END");
	return(2);
}


//=============================================================================
// Change to BT656
//=============================================================================
#ifdef SUPPORT_BT656
//-----------------------------------------------------------------------------
/**
* Check and Set BT656
*
* @return
*	- 0: success
*	- other: error
* @todo REG04A[0] does not work.
*/
BYTE CheckAndSetBT656(void)
{
	BYTE value;

#if 0
	WriteTW88Page(PAGE0_BT656);
	value=ReadTW88(REG04A);
	if(value & 0x01)	return 1;	//NO Signal
	else				return 0;	//found signal
#else

	value = ScalerCalcHDE();
	ScalerWriteHDEReg(value+3);

	//R04A[0] is not working
	return 0;
#endif
}
#endif

//-----------------------------------------------------------------------------
//		BYTE	ChangeBT656( void )
//-----------------------------------------------------------------------------
#ifdef SUPPORT_BT656
//-----------------------------------------------------------------------------
/**
* Change to BT656
*
* @return
*	- 0: success
*	- 1: No Update happen
*	- 2: No Signal or unknown video sidnal.
*/
BYTE ChangeBT656(void)
{
	BYTE ret;

	if ( InputMain == INPUT_BT656 ) {
		dPrintf("\nSkip ChangeBT656");
		return(1);
	}
	InputMain = INPUT_BT656;

	if(GetInputEE() != InputMain)
		SaveInputEE( InputMain );

	//----------------
	// initialize video input
	InitInputAsDefault();

	//
	// Check and Set VADC,mesaure,Scaler for Analog PC input
	//
	ret = CheckAndSetBT656();		//same as CheckAndSetInput()
	//dPrintf("\nBT656 input Detect: %s", ret ? "No" : "Yes" );

 	if(ret==0) {
		//success
		VInput_enableOutput(0);

		return 0;
	}

	//------------------
	// NO SIGNAL
	// Prepare NoSignal Task...

	VInput_gotoFreerun(0);

	return(2);
}
#endif

