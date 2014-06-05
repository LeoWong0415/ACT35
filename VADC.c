/**
 * @file
 * VADC.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	Video HighSpeed ADC module for aRGB 
 *
 *  VADC means Video ADC. we also call it as aRGB.
 *  VADC consist of "SYNC Processor" + "LLPLL" + "ADC".
 *  Component & PC inputs use VADC module. 	
 ******************************************************************************
 */

//-------------------------------------------------------------------
// global function
//	CheckAndSetComponent
//	CheckAndSetPC
//	VAdcSetDefault
//-------------------------------------------------------------------
#include "Config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "Printf.h"
#include "Monitor.h"
#include "I2C.h"
#include "CPU.h"
#include "Scaler.h"

#include "InputCtrl.h"

#include "measure.h"
#include "PC_modes.h"
	
#include "vadc.h"
#include "eeprom.h"
#include "settings.h"

#ifdef SUPPORT_COMPONENT
#include "data\DataComponent.inc"
#endif


#ifdef DEBUG_PC_COLOR
#define pcPrintf	dPrintf
#else 
#define pcPrintf	nullFn
#endif

XDATA	BYTE	InputVAdcMode;

#ifdef MODEL_TW8835FPGA
//AD9888 & AD9883 60Hz table
CODE BYTE AD9888_table[20][12] = {
/*idx   VGA  SVGA  XGA	SXGA  480i  576i  480p  576p  1080i 720p  1080p */
{0x01, 0x31, 0x41, 0x53, 0x69, 0x35, 0x35, 0x35, 0x35, 0x89, 0x67, 0x89}, 	//PLL_HIGH
{0x02, 0xF0, 0xF0, 0xF0, 0x70, 0x90, 0xF0, 0x90, 0xF0, 0x70, 0x10, 0x70}, 	//PLL_LOW
{0x03, 0x10, 0x28, 0x60, 0x98, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40}, 	//VCO & Charge Pump Current
//{0x03, 0x48, 0x48, 0x50, 0x98, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40}, 	//VCO & Charge Pump Current
{0x04, 0x80, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//Phase
{0x05, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}, 	//same
{0x06, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14}, 	//same
{0x07, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}, 	//same
{0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x09, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0A, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0B, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0C, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0D, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0E, 0x42, 0x42, 0x42, 0x42, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb}, 	//sync control
{0x0F, 0x6A, 0x4A, 0x6A, 0x6A, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02}, 	//sync control
{0x10, 0x88, 0x88, 0x88, 0x88, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e}, 	//SOG, Red clamp, Blue clamp
{0x11, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}, 	//same
{0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 	//same
{0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 	//same
{0x15, 0x46, 0x46, 0x46, 0x46, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e} 
};

//AD9888 & AD9883 60Hz table
CODE BYTE AD9883_table[20][12] = {
/*idx   VGA  SVGA  XGA	SXGA  480i  576i  480p  576p  1080i 720p  1080p */
{0x01, 0x31, 0x41, 0x53, 0x69, 0x35, 0x35, 0x35, 0x35, 0x89, 0x67, 0x89}, 	//PLL_HIGH
{0x02, 0xF0, 0xF0, 0xF0, 0x70, 0x90, 0xF0, 0x90, 0xF0, 0x70, 0x10, 0x70}, 	//PLL_LOW
{0x03, 0x10, 0x28, 0x60, 0x98, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40}, 	//VCO & Charge Pump Current
{0x04, 0x80, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//Phase
{0x05, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}, 	//same
{0x06, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14}, 	//same
{0x07, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}, 	//same
{0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x09, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0A, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0B, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0C, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0D, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, 	//same
{0x0E, 0x42, 0x42, 0x42, 0x42, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f, 0x9f}, 	//sync control
{0x0F, 0x6A, 0x4A, 0x6A, 0x6A, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02}, 	//sync control
{0x10, 0x88, 0x88, 0x88, 0x88, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e}, 	//SOG, Red clamp, Blue clamp
{0x11, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}, 	//same
{0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 	//same
{0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 	//same
{0x15, 0x46, 0x46, 0x46, 0x46, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e, 0x4e} 
};
void SetExtVAdcI2C(BYTE addr, BYTE mode)
{
	BYTE i;
	
	dPrintf("\nSetExtVAdcI2C addr:%bx mode:%bd",addr,mode);
	if(addr != 0x98 && addr != 0x9a)
		return;
	if(mode>10)
		return;

	if(addr==0x98) {
		for(i=0; i < 20; i++) {
			WriteI2CByte(addr, AD9888_table[i][0],AD9888_table[i][mode+1]);
		}
	}
	if(addr==0x9a) {
		for(i=0; i < 20; i++) {
			WriteI2CByte(addr, AD9883_table[i][0],AD9883_table[i][mode+1]);
		}
	}
	delay1ms(100);
}
#endif

//---------------------------------------
//R1C2[5:4] - VCO range
//				0 = 5  ~ 27MHz
//				1 = 10 ~ 54MHz
//				2 = 20 ~ 108MHz
//				3 = 40 ~ 216MHz
//R1C2[2:0] - Charge pump
//				0 = 1.5uA
//				1 = 2.5uA
//				2 = 5.0uA
//				3 = 10uA
//				4 = 20uA
//				5 = 40uA
//				6 = 80uA
//				7 = 160uA
//----------------------------------------

#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
//parmeter
//	_IPF: Input PixelClock = Htotal*Vtotal*Hz
//
//
//BYTE SetVCORange(DWORD _IPF)
//
//need pixel clock & POST divider value.
//
//pixel clock = Htotal * Vtotal * Freq
//HFreq = Vtotal * Freq
//pixel clock = Htotal * HFreq
//
//-----------------------------------------------------------------------------
//==>LLPLLSetVcoRange
BYTE VAdcSetVcoRange(DWORD _IPF)
{
	BYTE VCO_CURR, value, chged=0;
	WORD val;
	
	val = _IPF / 1000000L;

	dPrintf("\nVAdcSetVcoRange _IPF:%lx val:%dMHz",_IPF,val);  
												//   +------BUG
												//    ??pump value		
	if     ( val < 15 )		VCO_CURR = 0x01;	// 00 000
	else if( val < 34 )		VCO_CURR = 0x01;	// 00 000
	else if( val < 45 )		VCO_CURR = 0x11;	// 01 000
	else if( val < 63 )		VCO_CURR = 0x11;	// 01 000
	else if( val < 70 )		VCO_CURR = 0x21;	// 10 000
	else if( val < 80 )		VCO_CURR = 0x21;	// 10 000
	else if( val <100 )		VCO_CURR = 0x21;	// 10 000
	else if( val <110 )		VCO_CURR = 0x21;	// 10 000
	else					VCO_CURR = 0x31;	// 11 000
	VCO_CURR |= 0xC0;	//POST div 1
	
	WriteTW88Page(PAGE1_VADC);
	value = ReadTW88(REG1C2);
	if( VCO_CURR != value) {
		chged = 1;
		dPrintf(" R1C2:%bx->%bx", value, VCO_CURR );
		WriteTW88(REG1C2, VCO_CURR);			// VADC_VCOCURR
		delay1ms(1);					// time to stabilize
	}


//	#ifdef DEBUG_PC_MEAS
//	dPrintf("\r\nSetVCO=%02bx, changed=%bd", VCO_CURR, chged );
//	#endif
	return chged;
}
#endif


#if defined(SUPPORT_COMPONENT) || defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
/**
* Read aRGB(VAdc) InputStauts
*
*	register
*	R1C1	LLPLL Input Detection Register
*	R1C1[7] - VSync input polarity 
*	R1C1[6]	- HSync input polarity
*	R1C1[5]	- VSYNC pulse detection status. 1=detected
*	R1C1[4]	- HSYNC pulse detection status. 1=detected
*	R1C1[3]	- Composite Sync detection status	
*	R1C1[2:0] Input source format detection in case of composite sync.
*				0:480i	1:576i	3:480p	3:576p
*				4:1080i	5:720p	6:1080p	7:fail
*/
BYTE VAdcGetInputStatus(void)
{
	BYTE value;
	WriteTW88Page(PAGE1_VADC);
	value = ReadTW88(REG1C1);
	return value;
}
#endif

#if defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
/**
* Read HSync&VSync input polarity status register.
*
* And, Set LLPLL input polarity & VSYNC output polarity.
*
*	Read R1C1[6] and set R1C0[2]
*	Read R1C1[7] and set R1CC[1]
*
*	PC uses VAdcSetPolarity(0) and Component uses VAdcSetPolarity(1).
*
* register
*	R1C0[2]	- LLPLL input polarity. Need Negative. CA_PAS need a normal
*	R1C1[6]	- HSync input polarity
*	R1C1[7] - VSync input polarity 
*	R1CC[1] - VSYNC output polarity. Need Positive

* ==>ARGBSetPolarity
* othername PolarityAdjust
*
* @param
*	fUseCAPAS.	If "1", R1C0[2] always use 0.
*				component use fUseCAPAS=1. 
*
*/
void VAdcSetPolarity(BYTE fUseCAPAS)
{
	BYTE r1c1;

	WriteTW88Page(PAGE1_VADC );
	r1c1 = ReadTW88(REG1C1);
	if(fUseCAPAS) {
		//CA_PAS need a normal
		WriteTW88(REG1C0, ReadTW88(REG1C0) & ~0x04);
		WriteTW88(REG1CC, ReadTW88(REG1CC) & ~0x02);	//if active high, no inv.
	}
	else {
		//check HS_POL.		Make LLPLL input polarity Negative
		if(r1c1 & 0x40) WriteTW88(REG1C0, ReadTW88(REG1C0) | 0x04);		//if active high, invert. make negative
		else			WriteTW88(REG1C0, ReadTW88(REG1C0) & ~0x04);	//if active low, normal. keep negative
		//check VS_POL.		Make VS output polarity Positive
		if(r1c1 & 0x80) WriteTW88(REG1CC, ReadTW88(REG1CC) & ~0x02);	//if active high, normal.
		else			WriteTW88(REG1CC, ReadTW88(REG1CC) | 0x02);		//if active low, inv
	}
}
#endif

#if defined(SUPPORT_COMPONENT) || defined(SUPPORT_PC)
#define LLPLL_POST_8		0x00
#define LLPLL_POST_4		0x40
#define LLPLL_POST_2		0x80
#define LLPLL_POST_1		0xC0 //*
#define LLPLL_VCO_40TO216	0x30 //*
#define LLPLL_PUMP_5		0x02 //*
//-----------------------------------------------------------------------------
/**
* Set LLPLL Control
*
*	register
*	R1C2[7:6]	PLL post divider
*	R1C2[5:4]	VCO range select
*	R1C2[2:0]	Charge pump current
*/
void VAdcSetLLPLLControl(BYTE value)
{
	WriteTW88Page(PAGE1_VADC );
	WriteTW88(REG1C2, value);
}
#endif


//-----------------------------------------------------------------------------
// LLPLL Divider
//-----------------------------------------------------------------------------
#if defined(SUPPORT_COMPONENT) || defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
/**
* Write LLPLL divider
*
* other name: SetCoarse(WORD i)
*
*	register
*		R1C3[3:0]R1C4[7:0] - LLPLL Divider. PLL feedback divider. A 12-bit register 
* @param	value: PLL value. Use (Htotal-1)
* @param	fInit:	init flag
*/
void VAdcLLPLLSetDivider(WORD value, BYTE fInit)
{
	volatile BYTE mode;

	WriteTW88Page(PAGE1_VADC );
	Write2TW88(REG1C3,REG1C4, value);
	if(fInit) {	
		WriteTW88(REG1CD, ReadTW88(REG1CD) | 0x01);		// PLL init
		//wait
		do {
			mode = TW8835_R1CD;
		} while(mode & 0x01);
	}
#ifdef MODEL_TW8835FPGA
	WriteI2CByte(0x98, 1, value>>4);
	WriteI2CByte(0x98, 2, value<<4);
#endif
}
#endif

#if defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
/**
* Read LLPLL divider value
*
* other name: GetCoarse(void)
*/
WORD VAdcLLPLLGetDivider(void)
{
	WORD value;

	WriteTW88Page(PAGE1_VADC);
	Read2TW88(REG1C3,REG1C4,value);
	return value & 0x0FFF;
}
#endif

//-----------------------------------------------------------------------------
// LLPLL Clock PHASE
//-----------------------------------------------------------------------------
#if defined(SUPPORT_COMPONENT) || defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
/**
* set Phase value
*
*	register
*	R1C5[4:0]
* @param value: Phase value
* @param fInit:	init flag
*/
void VAdcSetPhase(BYTE value, BYTE fInit)
{
	volatile BYTE mode;

	WriteTW88Page(PAGE1_VADC);
	WriteTW88(REG1C5, value&0x1f);
#ifdef MODEL_TW8835FPGA
	WriteI2CByte(0x98, 4, value<<3);
#endif
	if(fInit) {
		WriteTW88(REG1CD, ReadTW88(REG1CD) | 0x01);	// PLL init
		//wait
		do {
			mode = TW8835_R1CD;
		} while(mode & 0x01);
	}
}
#endif
#if defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
/**
* get Phase value
*/
//-----------------------------------------------------------------------------
BYTE VAdcGetPhase(void)
{
	WriteTW88Page(PAGE1_VADC);
	return ReadTW88(REG1C5) & 0x1f;		//VADC_PHASE
}
#endif



//-----------------------------------------------------------------------------
// LLPLL Filter BandWidth
//---------------------------
//register
//	R1C6[2:0]	R1C6 default: 0x20.
//-----------------------------------------------------------------------------
/**
* set filter bankwidth
*/
void VAdcSetFilterBandwidth(BYTE value, WORD delay)
{
	if(delay)
		delay1ms(delay);
	WriteTW88Page(PAGE1_VADC);
	WriteTW88(REG1C6, (ReadTW88(REG1C6) & 0xF8) | value);
}


//-----------------------------------------------------------------------------
//desc: check input
//@param
//	type	0:YPbPr, 1:PC
//		YPbPr	Use CompositeSync(CSYNC) with clamping & Slicing
//		PC		Use a seperate HSYNC & VSYNC PIN
//
//for YPbPr
//	0x1C0[7:6]	= 0		default
//	0x1C0[3] = 0		Select Clamping output(not HSYNC)
//	0x1C0[4] = 1		Select CS_PAS
//
//for PC(aRGB)
//preprocess
//	0x1C0[3] = 1	Select HSYNC
//	0x1C0[4] = 0	Select HSYNC(or Slice, Not a CS_PAS)
//	
//detect
//	0x1C1[6]	Detected HSYNC polarity
//	0x1C1[4]	HSYNC detect status
//postprocess
//	0x1C0[2]	PLL reference input polarity	
//
//return
//	0: fail
//	else: R1C1 value		
#ifdef UNCALLED_SEGMENT
BYTE VAdcCheckInput(BYTE type)
{
//	BYTE value;
	volatile BYTE rvalue;
	BYTE check;
	BYTE i;

	ePrintf("\nVAdcCheckInput(%bx) %s",type,type ? "PC": "YPbPr" );

	WriteTW88Page(PAGE1_VADC );

//	//power up PLL, SOG,....
//	value = 0x40;										// powerup PLL
//	if(type==0)	value |= 0x80;							// powerup SOG
//	WriteTW88(REG1CB, (ReadTW88(REG1CB) & 0x1F) | value );	// keep SOG Slicer threshold & coast
		
	if(type==0) check = 0x08;	//check CompositeSynch
	else 		check = 0x30;	//check HSynch & VSynch

	//(YPbPr need more then 370ms, PC need 200ms). max 500ms wait
	for(i=0; i < 50; i++) {
		rvalue = TW8835_R1C1;
		dPrintf(" %02bx",rvalue);

		if((rvalue & check) == check) {
			ePrintf("->success:%bd",i);
			return rvalue;	
		}
		delay1ms(10);
	}
	ePrintf("->fail");

//	WriteTW88(REG1CB, ReadTW88(REG1CB) & 0x1F);	//PowerDown

	return 0;	//No detect		
}
#endif


//-----------------------------------------------------------------------------
//only for test
//-----------------------------------------------------------------------------
#ifdef UNCALLED_SEGMENT
BYTE VAdcDoubleCheckInput(BYTE detected)
{
	BYTE i, count;
	BYTE rvalue;
	BYTE old = detected;

	ePrintf("\nVAdcDoubleCheckInput");
	count=0;

	WriteTW88Page(PAGE1_VADC );
	for(i=0; i < 200; i++) {
		rvalue = ReadTW88(REG1C1 );
		if(rvalue == old) count++;
		else {
			dPrintf(" %02bx@%bd", rvalue, i);
			old = rvalue;
			count=0;
		}
		if(count >= 30) {
			ePrintf("->success");
			return rvalue;
		}
		delay1ms(10);
	}
	ePrintf("->fail");
	return 0;
}
#endif


//0x1C1 - LLPLL Input Detection Register 



#ifdef UNCALLED_SEGMENT
#if defined(SUPPORT_COMPONENT) || defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
//power down SOG,PLL,Coast
//register
//	R1CB[7]	SOG power down.	1=Powerup
//	R1CB[6]	PLL power down.	1=Powerup
//	R1CB[5]	PLL coast function. 1=Enable
//-----------------------------------------------------------------------------
void VAdcSetPowerDown(void)
{
	WriteTW88Page(PAGE1_VADC );
	WriteTW88(REG1CB, (ReadTW88(REG1CB) & 0x1F));	
}
#endif
#endif

//-----------------------------------------------------------------------------
//register
//	R1CB[4:0]  SOG Slicer Threshold Register
//-----------------------------------------------------------------------------
#ifdef UNCALLED_SEGMENT
void VADcSetSOGThreshold(BYTE value)
{
	WriteTW88Page(PAGE1_VADC );
	WriteTW88(REG1CB, ReadTW88(REG1CB) & ~0x1F | value);	
}
#endif

//-----------------------------------------------------------------------------
//gain control
//R1D0[2]R1D1[7:0]	Y/G channel gain
//R1D0[1]R1D2[7:0]	C/B channel gain
//R1D0[0]R1D3[7:0]	V/R channel gain
//-----------------------------------------------------------------------------
#ifdef SUPPORT_PC
void VAdcSetChannelGainReg(WORD GainG,WORD GainB,WORD GainR)
{
	WriteTW88Page(PAGE1_VADC );
	WriteTW88(REG1D1, GainG );
	WriteTW88(REG1D2, GainB );
	WriteTW88(REG1D3, GainR );
	WriteTW88(REG1D0, (GainR >> 8)+ ((GainB >> 7) & 2) + ((GainG >> 6) & 4 ));
}
WORD VAdcReadGChannelGainReg(void)
{
	WORD wTemp;
	WriteTW88Page(PAGE1_VADC );
	wTemp = ReadTW88(REG1D0) & 0x04;
	wTemp <<= 6;
	wTemp |= ReadTW88(REG1D1);
	return wTemp;
}
WORD VAdcReadBChannelGainReg(void)
{
	WORD wTemp;
	WriteTW88Page(PAGE1_VADC );
	wTemp = ReadTW88(REG1D0) & 0x02;
	wTemp <<= 7;
	wTemp |= ReadTW88(REG1D2);
	return wTemp;
}
WORD VAdcReadRChannelGainReg(void)
{
	WORD wTemp;
	WriteTW88Page(PAGE1_VADC );
	wTemp = ReadTW88(REG1D0) & 0x01;
	wTemp <<= 8;
	wTemp |= ReadTW88(REG1D3);
	return wTemp;
}
#endif

//-----------------------------------------------------------------------------
//register
//	R1D4[7:0]
//-----------------------------------------------------------------------------
#if 0
void VAdcSetClampMode(BYTE value)
{
	WriteTW88Page(PAGE1_VADC );
	WriteTW88(REG1D4, value );
}
#endif

//-----------------------------------------------------------------------------
//register
//	R1D4[5]
//-----------------------------------------------------------------------------
#ifdef SUPPORT_COMPONENT
/**
* set clamp mode and HSync Edge
*/
void VAdcSetClampModeHSyncEdge(BYTE fOn)
{
	WriteTW88Page(PAGE1_VADC );
	if(fOn)	WriteTW88(REG1D4, ReadTW88(REG1D4) | 0x20 );
	else	WriteTW88(REG1D4, ReadTW88(REG1D4) & ~0x20 );
}
#endif

//-----------------------------------------------------------------------------
//register
//	R1D7[7:0]
//-----------------------------------------------------------------------------
#ifdef SUPPORT_COMPONENT
/**
* set clamp position
*/
void VAdcSetClampPosition(BYTE value)
{
	WriteTW88Page(PAGE1_VADC );
	WriteTW88(REG1D7, value );	// ADC clamp position from HSync edge by TABLE ClampPos[]
}
#endif

//-----------------------------------------------------------------------------
//register
//	R1E6[5]	PGA control	0=low speed operation. 1=high speed operation
//-----------------------------------------------------------------------------
#ifdef UNCALLED_SEGMENT
/**
* set PGA control
*/
void VAdcSetPGAControl(BYTE fHigh)
{
	WriteTW88Page(PAGE1_VADC );
	if(fHigh)	WriteTW88(REG1E6, ReadTW88(REG1E6) | 0x20);		//HighSpeed
	else		WriteTW88(REG1E6, ReadTW88(REG1E6) & ~0x20);	//LowSpeed
}
#endif

//===================================================================
//
//===================================================================
//-----------------------------------------------------------------------------
/**
* set default VAdc for PC & Component.
*
* If input is not PC or Component, powerdown VAdc.
*	R1C0[]	10
*	R1C2[]	d2
*	* R1C6	20
*	R1CB[]
*	R1CC[]
*	R1D4[]	00	20
*	R1D6[]	10	10
*	R1D7[]		00
*	R1DA[]	80	01
*	R1DB[]	80	01
*	R1E6[]	00  20		PGA high
* external
*  InputMain
* @todo pls, remove or refind. it is too big.
*/

void VAdcSetDefaultFor(void)
{
#if defined(SUPPORT_COMPONENT) || defined(SUPPORT_PC)
	BYTE rvalue;
#endif
	//dPrintf("\nVAdcSetDefaultFor()");

	WriteTW88Page(PAGE1_VADC );
	if ( InputMain == INPUT_COMP ) {
#ifdef SUPPORT_COMPONENT
		WriteTW88(REG1C0,0x10);	// mode for SOG slicer
		WriteTW88(REG1C2,0xD2);	// ==> VCO Charge pump		POST:1. VCO:10~54MHz Pump:5uA
		WriteTW88(REG1C6,0x20);	// PLL loop control
		WriteTW88(REG1C9,0x00);	// Pre-coast = 0
		WriteTW88(REG1CA,0x00);	// Post-coast = 0
		WriteTW88(REG1CB,0xD6);	// Power up PLL, SOG
#ifdef MODEL_TW8835FPGA
		WriteTW88(REG1CC,(ReadTW88(REG1CC) & 0xE0) | 0x0B);	//1CC[3:2]=10b. HS Pin.1CC[1]=1. VSYNC inversion, 1CC[0]=1. HSYNC inversion
#else
		WriteTW88(REG1CC,0x00);	// ==> Sync selection
#endif

		WriteTW88(REG1D0,0x00);	// ADC gain
		WriteTW88(REG1D1,0xF0);	// 
		WriteTW88(REG1D2,0xF0);	// 
		WriteTW88(REG1D3,0xF0);	// 

		WriteTW88(REG1D4,0x20);	// clamp mode
		WriteTW88(REG1D5,0x00);	// clamp start
		WriteTW88(REG1D6,0x10);	// clamp stop
		WriteTW88(REG1D7,0x00);	// clamp pos.
		WriteTW88(REG1D9,0x02);	// clamp Y level
		WriteTW88(REG1DA,0x80);	// clamp U level
		WriteTW88(REG1DB,0x80);	// clamp V level
		WriteTW88(REG1DC,0x10);	// HS width

		WriteTW88(REG1E2,0x59);	//***
		WriteTW88(REG1E3,0x37);	//***
		WriteTW88(REG1E4,0x55);	//***
		WriteTW88(REG1E5,0x55);	//***

		WriteTW88(REG1E6,0x20);	// PGA high speed

#ifdef MODEL_TW8835FPGA
		SetExtVAdcI2C(0x98, 4);	 //AD9888..default480i
#endif
		//set default divider(856-1. for 480i or 480p) & phase. 
		VAdcLLPLLSetDivider(0x035A, 1);
		//rvalue=GetPhaseEE(EE_YUVDATA_START+0);
		//if(rvalue==0xff)
			rvalue=0;
		VAdcSetPhase(rvalue, 0);
#endif
	}
	else if ( InputMain == INPUT_PC ) {
#ifdef SUPPORT_PC
		WriteTW88(REG1C0,0x08);	// mode for HV sync
		WriteTW88(REG1C2,0xD2);	// ==> VCO Charge pump		POST:1. VCO:10~54MHz Pump:5uA
		WriteTW88(REG1C6,0x20);	// PLL loop control
		WriteTW88(REG1C9,0x00);	// Pre-coast = 0
		WriteTW88(REG1CA,0x00);	// Post-coast = 0
		WriteTW88(REG1CB,0x56);	// Power up PLL
#ifdef MODEL_TW8835FPGA
		WriteTW88(REG1CC,(ReadTW88(REG1CC) & 0xE0) | 0x18);	////1CC[4]:1  VSYNC: input pin. 1CC[3:2]=10b.
#else
		WriteTW88(REG1CC,0x12);	// ==> Sync selection
#endif

		WriteTW88(REG1D0,0x00);	// ADC gain
		WriteTW88(REG1D1,0xF0);	// 
		WriteTW88(REG1D2,0xF0);	// 
		WriteTW88(REG1D3,0xF0);	// 

		WriteTW88(REG1D4,0x20);	// clamp mode
		WriteTW88(REG1D5,0x00);	// clamp start
		WriteTW88(REG1D6,0x10);	// clamp stop
		WriteTW88(REG1D7,0x00);	// clamp pos.
		WriteTW88(REG1D9,0x02);	// clamp G/Y level
		WriteTW88(REG1DA,0x01);	// clamp B/U level
		WriteTW88(REG1DB,0x01);	// clamp R/V level
		WriteTW88(REG1DC,0x10);	// HS width

		WriteTW88(REG1E2,0x59);	//***
		WriteTW88(REG1E3,0x37);	//***
#ifdef CHIP_MANUAL_TEST
		WriteTW88(REG1E4,0x53);	//***
#else
		WriteTW88(REG1E4,0x55);	//***
#endif
		WriteTW88(REG1E5,0x55);	//***

		WriteTW88(REG1E6,0x20);	// PGA high speed

#ifdef MODEL_TW8835FPGA
		SetExtVAdcI2C(0x98, 0);		 //AD9888..default VGA
#endif
		//set default divider(1056, for SVGA) & phase. 
		VAdcLLPLLSetDivider(0x0420, 1);	
		rvalue=GetPhaseEE(5);	//SVGA.
		if(rvalue==0xff)
			rvalue=0;
		VAdcSetPhase(rvalue, 0); //VGA
#endif
	}
	else {
		//power down SOG,PLL,Coast
		//same as VAdcSetPowerDown();	
		WriteTW88(REG1CB, (ReadTW88(REG1CB) & 0x1F));
		LLPLLSetClockSource(1);			//select 27MHz. R1C0[0]
	}	
}




//-----------------------------------------------------------------------------
//R1D0
//R1D1 Y channel gain
//R1D2 C channel gain
//R1D3 V channel gain
//read RGB max value from meas and adjust color gain value on VAdc.
//-----------------------------------------------------------------------------

#if defined(SUPPORT_COMPONENT) || defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
//extern BYTE WaitStableLLPLL(WORD delay);
//BYTE VAdcSetupLLPLL(WORD divider, /*BYTE ctrl,*/ BYTE fInit, BYTE delay)
//-----------------------------------------------------------------------------
/**
* update LLPLL divider
*
*/
BYTE VAdcLLPLLUpdateDivider(WORD divider, /*BYTE ctrl,*/ BYTE fInit, BYTE delay)
{
	BYTE ret;
	
	ret = ERR_SUCCESS;
	
	VAdcSetFilterBandwidth(0, 0);

//	//LLPLL Control.
//	VAdcSetLLPLLControl(ctrl);

	VAdcLLPLLSetDivider(divider, fInit);
	if(fInit) {
		if(WaitStableLLPLL(delay))
			ret = ERR_FAIL;
	}
	VAdcSetFilterBandwidth(7, 0);	//restore
	
	return ret;
}
#endif


#if defined(SUPPORT_COMPONENT) || defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
/**
* wait stable LLPLL input.
*
* @return
*	0:success. ERR_SUCCESS.
*	1:fail. ERR_FAIL
*/
BYTE	WaitStableLLPLL(WORD delay)
{
	DECLARE_LOCAL_page
	BYTE	i;
	WORD	HActive, HActiveOld, HStart;
	BYTE 	PolOld;
	volatile BYTE	Pol;

	ReadTW88Page(page);

	if(delay)
		delay1ms(delay);

	dPrintf("\nWaitStableLLPLL: ");
	for(i=0; i < 128; i++) {	//max loop
		if(MeasStartMeasure()) {
			dPrintf("fail measure");
			WriteTW88Page(page );
			return ERR_FAIL;
		}
		HActive = MeasGetHActive( &HStart );	//h_active_start h_active_perios
		Pol = VAdcGetInputStatus();
		if(i==0) {
			//skip.
		}
		else if((HActive==HActiveOld) && (Pol == PolOld)) {
			dPrintf("%bd times, HS: %d, HActive: %d InputStatus:0x%bx", i, HStart, HActive, Pol);
			WriteTW88Page(page );
			return ERR_SUCCESS;
		}
		HActiveOld = HActive;
		PolOld = Pol;
	}
	dPrintf("fail max loop");
	WriteTW88Page(page );
	return ERR_FAIL;
}
#endif


#if defined(SUPPORT_PC)
//-----------------------------------------------------------------------------
/**
* find PC input mode
*
* @return
*	0: fail
*	else: success.
*		  index number of PC Mode Data Table.
*/
BYTE FindInputModePC(WORD *vt /*BYTE fDTV*/)
{
	WORD	vtotal;
	BYTE	vfreq, i;

	//
	// get a vertical frequency and  a vertical total scan lines.
	//
	//BKFYI. We donot have a PLL value yet that depend on the mode.
	//so, we are using 27MHz register.
	vfreq = MeasGetVFreq();
	vfreq = MeasRoundDownVFreqValue(vfreq);

	vtotal = MeasGetVPeriod();	//Vertical Period Registers
	*vt = vtotal;

	//
	//Search PC mode.
	//
	for ( i=1; i<(sizeof_PCMDATA() / sizeof(struct _PCMODEDATA)); i++ ) {
		if ( PCMDATA[i].support == 0 ) continue;
		if ( PCMDATA[i].support==3 /* && fDTV==0 */) continue;
		if ( PCMDATA[i].vfreq == vfreq ) {			//check vfreq
			//dPrintf("\ni=%bd", i);
			if(( PCMDATA[i].vtotal == vtotal )		//check vtotal 
			|| ( PCMDATA[i].vtotal == (vtotal+1) ) 
			|| ( PCMDATA[i].vtotal == (vtotal-1) )
			|| ( PCMDATA[i].vtotal == (vtotal+2) ) 
			|| ( PCMDATA[i].vtotal == (vtotal-2) ) ){
				//dPrintf(" ==>FOUND vtotal:%d, %dx%d@%bdHz", vtotal, PCMDATA[i].han, PCMDATA[i].van, vfreq);
				dPrintf("\nFindInputModePC FOUND mode=%bd vtotal:%d, %dx%d@%bdHz", 
					i, vtotal, PCMDATA[i].han, PCMDATA[i].van, vfreq);
				//dPrintf("\nFindInputModePC return mode:%bd",i);
				return (i);
			}
		}
	}

	ePuts( "\nCurrent Input resolution IS Not Supported." );
	ePrintf(" V total: %d, V freq: %bd", vtotal, vfreq );
	return (0);							// not support
}
#endif

//-----------------------------------------------------------------------------
// component video table
//-----------------------------------------------------------------------------

							//       1      2       3      4      5      6      7      8      9     10
							//   	480i,  576i,   480p, 576p,1080i50,1080i60,720p50,720p60,1080p5,1080p6
//scaled
code	WORD	YUVDividerPLL[] = { 858,   864,   858,   864,   2460,  2200,  1980,  1650,  2640,  2200 };		//total horizontal pixels
code	WORD	YUVVtotal[]     = { 262,   312,   525,   625,   562,   562,   750,   750,   1124,  1124 };		//total vertical scan line
#if 1 //scaled
code	BYTE	YUVClampPos[]   = { 128,   128,   64,    58,    40,    32,    38,    38,    14,    14 };		//clamp position offset. R1D7. 
#else
code	BYTE	YUVClampPos[]   = { 140,   140,   52,    58,    24,    32,    38,    38,    14,    14 };		//clamp position offset. R1D7.
#endif

code	WORD	YUVCropH[]      = { 720,   720,   720,   720,   1920,  1920,  1280,  1280,  1920,  1920 };		// horizontal resolution
code	WORD	YUVCropV[]      = { 240,   288,   480,   576,   540,   540,   720,   720,   1080,  1080 };		// vertical resolution
#if 1
code	WORD	YUVDisplayH[]   = { 700,   700,   700,   700,   1880,  1880,  1260,  1260,  1880,  1880 };		// reduced. R042[3:0]R046[7:0] for overscan
code	WORD	YUVDisplayV[]   = { 230,   278,   460,   556,   520,   520,   696,   696,   1040,  1040 };		// reduced R042[6:4]R044[7:0] for overscan
#else
//same as YUVCropH and YUVCropV
code	WORD	YUVDisplayH[]   = { 720,   720,   720,   720,   1920,  1920,  1280,  1280,  1920,  1920 };		// R042[3:0]R046[7:0] for overscan
code	WORD	YUVDisplayV[]   = { 240,   288,   480,   576,   540,   540,   720,   720,   1080,  1080 };		// R042[6:4]R044[7:0] for overscan
#endif

#if 1
code	WORD	YUVStartH[]     = { 112,   126,   114,   123,   230,   233,   293,   293,   233,   233 };		// 0x040[7:6],0x045 InputCrop
code	WORD	YUVStartV[]     = { 1,     1,     2,     2,     2,     2,     2,     2,     2,     2 };			// 0x043 InputCrop
#else
code	WORD	YUVStartH[]     = { 121-16,131-16,121-16,131-16,235-16,235-16,299-16,299-16,   235-16,235-16 };		// 0x040[7:6],0x045 InputCrop
code	WORD	YUVStartV[]     = { 1,     1,     2,     2,     2,     2,     2,     2,     2,     2 };			// 0x043 InputCrop
#endif

#if 1
code	BYTE	YUVOffsetH[]    = { 5,     4,     10,    6,     40,    40,    20,    20,    30,    30 };
code	BYTE	YUVOffsetV[]    = { 48,    48,    48,    48,    28,    26,    24,    25,    26,    26 };		// use as V-DE 0x215	
#else
code	BYTE	YUVOffsetH[]    = { 5,     4,     10,    6,     40,    40,    20,    20,    30,    30 };
code	BYTE	YUVOffsetV[]    = { 42,    40,    40,    38,    20,    20,    18,    18,    10,    10 };		// use as V-DE 0x215	
#endif
code	BYTE	YUVScaleVoff[]  = { 128,   128,   0,     0,     128,   128,   0,     0,     0,     0 };

#if 1
code	WORD	MYStartH[]      = { 121,   131,   121,   131,235-44, 235-44,299-40,	299-40,	235-44,235-44 };		// 0x040[7:6],0x045 InputCrop
code	WORD	MYStartV[]      = { 19,   21,   	38,   44, 	20,		20,   25, 	25,   	41, 	41 };	
#else

code	BYTE  YUV_VDE_NOSCALE[] = { 21,    24,    40,    46,    22,    22,    27,    27,    22,    22 };		// use as V-DE 0x215	


code	WORD	MYStartH[]      = { 121,   131,   121,   131,235-44,   235-44,	299-40,  299-40, 	235-44,	235-44 };		// 0x040[7:6],0x045 InputCrop
code	WORD	MYStartV[]      = { 19,   21,   38,   44, 	20,	20,  25, 25,   	41, 41 };	
#endif



#if defined(SUPPORT_COMPONENT)
//-----------------------------------------------------------------------------
/**
* find component input mode
*
* @return
*	0: fail.
*	other:success. component mode+1 value.
* @todo
*	Current code can not have a return value 0.
*/
BYTE FindInputModeCOMP( void )
{
	DWORD	vperiod;
	WORD	vtotal;
	BYTE	vfreq, i;

	//
	// get a vertical frequency and  a vertical total scan lines.
	//
	//BKFYI. We donot have a PLL value yet that depend on the mode.
	//so, we are using 27MHz register.
	vfreq = MeasGetVFreq();

	if ( vfreq < 55 ) vfreq = 50;
	else  vfreq = 60;

	vtotal = MeasGetVPeriod();	//Vertical Period Registers
	i = 0;

	dPrintf( "\nYUV: VPeriod:%ld, VFreq: %bdHz, VTotal: %d", vperiod, vfreq, vtotal );
	if ( vfreq == 50 ) {
		if ( vtotal < 320 )			i = 2;	// 576i	 = 625 for 2, 312.5
		else if ( vtotal < 590 )	i = 5;	// 1080i50A
		else if ( vtotal < 630 )	i = 4;	// 576P=625	or 1080i50B = sync=5	  // vblank length different 576P=45, 1080i=21
											// can check with even/odd measure
		else if ( vtotal < 800 )	i = 7;	// 720P = 750
		else 						i = 9;	// 1080P = 1250 total from set-top box
	}
	else {
		if ( vtotal < 300 )			i = 1;	// 480i = 525 for 2, 262.5
		else if ( vtotal < 540 )	i = 3;	// 480P	= 525
		else if ( vtotal < 600 )	i = 6;	// 1080i
		else if ( vtotal < 800 )	i = 8;	// 720P = 750
		else 						i = 10;	// 1080P
	}

	return (i);							// not support
}
#endif



#ifdef SUPPORT_PC
//-----------------------------------------------------------------------------
/**
* set inputcrop for PC
*
* output
*	RGB_HSTART
*/
static void PCSetInputCrop( BYTE mode )
{
	BYTE	offset, HPol, VPol;
	WORD	hstart, vstart;
	WORD	Meas_HPulse,Meas_VPulse;

	//r041[0] r040[4]
	WriteTW88Page(PAGE0_INPUT );
	offset = ReadTW88(REG041) & 1;  //?RGB
	offset *= 2;
	offset += 2;
	offset += ((ReadTW88(REG040)&0x10) >> 4 );	//implicit DE

	HPol = (ReadTW88(REG041) >> 2) & 1;
	VPol = (ReadTW88(REG041) >> 3) & 1;

	dPrintf("\nPCSetInputCrop offset:%bd, HPol: %bd, VPol: %bd", offset, HPol, VPol );

	//read sync width
	Meas_HPulse = MeasGetHSyncRiseToFallWidth();
	Meas_VPulse = MeasGetVSyncRiseToFallWidth();
	dPrintf("\n\tHPulse: %d, VPulse: %d", Meas_HPulse, Meas_VPulse );

	hstart = MeasHStart + offset - (Meas_HPulse*HPol);
	vstart = MeasVStart - (Meas_VPulse*VPol);
	RGB_HSTART = hstart;

	//adjust EEPROM. 0..100. base 50. reversed value.
	hstart += 50;
	hstart -= GetHActiveEE(mode); //PcBasePosH;

	dPrintf("\n\tModified HS: %d, VS: %d", hstart, vstart );
	dPrintf("\n\tHLen: %d, VLen: %d", MeasHLen, MeasVLen );

	//InputSetCrop(hstart, vstart, MeasHLen, MeasVLen);
	InputSetCrop(hstart, 1, MeasHLen, 0x7FE);
}
#endif //#ifdef SUPPORT_PC

#if defined(SUPPORT_COMPONENT)
//-----------------------------------------------------------------------------
/**
* set component output value
*
*/
static void YUVSetOutput(BYTE mode)
{
	BYTE HDE;
	WORD temp16;

	//ScalerWriteOutputWidth(PANEL_H+1);
	//ScalerWriteOutputHeight(PANEL_V);

	ScalerSetHScale(YUVDisplayH[mode]);
	ScalerSetVScale(YUVDisplayV[mode]);

	HDE = ScalerReadLineBufferDelay() + 32;
	dPrintf("\nH-DE start = %bd", HDE);
	ScalerWriteHDEReg(HDE);
	ScalerWriteVDEReg(YUVOffsetV[mode]);

	//===== Free Run settings ==========
	temp16=ScalerCalcFreerunHtotal();
	ScalerWriteFreerunHtotal(temp16);

	temp16 = ScalerCalcFreerunVtotal();
	ScalerWriteFreerunVtotal(temp16);

	ScalerSetFreerunAutoManual(ON,OFF);
	ScalerSetMuteAutoManual(ON,OFF);
}
#endif

#ifdef SUPPORT_PC
//-----------------------------------------------------------------------------
/**
* set PC output value
*
* extern
*	MeasHLen
*	MeasVLen
*/
static void PCSetOutput( BYTE mode )
{
	BYTE HDE;
	WORD temp16;

	dPuts("\nPCSetOutput");

	ScalerSetHScale(MeasHLen);
	ScalerSetVScale(MeasVLen);

	//=============HDE=====================
	HDE = ScalerCalcHDE();
	dPrintf("\n\tH-DE start = %bd", HDE);
	ScalerWriteHDEReg(HDE);	//BKFYI. Scaler ouput width : 801

	//=============VDE=====================
	temp16 = ScalerCalcVDE();
	dPrintf("\n\tV-DE start = %d", temp16);
	RGB_VDE = temp16;
	//use EEPROM
	temp16 += GetVBackPorchEE(mode);
	temp16 -= 50;
	dPrintf("=> %d", temp16);
	ScalerWriteVDEReg((BYTE)temp16);

	//================= H Free Run settings ===================================
	temp16=ScalerCalcFreerunHtotal();
	dPrintf("\n\tFree Run Htotal: 0x%x", temp16);
	ScalerWriteFreerunHtotal(temp16);

	//================= V Free Run settings ===================================
	temp16=ScalerCalcFreerunVtotal();
	dPrintf("\n\tFree Run Vtotal: 0x%x", temp16);
	ScalerWriteFreerunVtotal(temp16);

	//================= FreerunAutoManual, MuteAutoManual =====================
	ScalerSetFreerunAutoManual(ON,OFF);
	ScalerSetMuteAutoManual(ON,0x02);	//use skip on Manual.
}
#endif

#ifdef SUPPORT_PC
//-----------------------------------------------------------------------------
/** 
* check PC mode
*
* @return
*	0:fail
*	other: pc mode value
*/
static BYTE PCCheckMode(void)
{
	BYTE i, mode;
	WORD VTotal;	//dummy

	for(i=0; i < 10; i++) {
		if(MeasStartMeasure())
			return 0;

 		mode = FindInputModePC(&VTotal);	// find input mode from Vfreq and VPeriod
		if(mode) {
			dPrintf("\nPCCheckMode ret:%bd",mode);
			return mode;
		}
	}

	return 0;
}
#endif

#if defined(SUPPORT_PC) || defined(SUPPORT_DVI) || defined(SUPPORT_HDMI_EP9351)|| defined(SUPPORT_HDMI_SiIRX)
//-----------------------------------------------------------------------------
/** 
* adjust the pixel clock
*
* oldname: void	PCLKAdjust( BYTE mode )
*
* INPUT_PC
*	use mode.
*
* INPUT_DVI
*	skip divider & mode.
*
* INPUT_HDMI
*	use divider.
*/
void AdjustPixelClk(WORD digital_divider, BYTE mode )
{
	DWORD	PCLK, PCLK1, PCLK2;
	BYTE	i, PCLKO;
	WORD	HDown, HPeriod, Divider, VPN, VScale, HActive, H_DE;
	DWORD	VPeriod, VFreq;


	PCLK = SspllGetPPF(0);
	//	FPCLK1 calculation
	//	FREQ = REG(0x0f8[3:0],0x0f9[7:0],0x0fa[7:0])															   	
	//	POST = REG(0x0fd[7:6])
	//	Hperiod = REG(0x524[7:0],0x525[7:0])
	//	Divider = REG(0x1c3[3:0],0x1c4[7:0]) ;;InFreq = (Divider+1) * (27000000 * FREQ / ((2^15)*(2^POST))) / Hperiod
	//	Hdown = REG(0x20a[3:0],0x209[7:0])
	//	PCLKO = REG(0x20d[1:0]) {1,1,2,3}
	//	PCLKx = REG(0x20d[1:0]) {1,2,3,4}
	//	result = ((Divider+1) * (27000000 * FREQ / ((2^15)*(2^POST))) / Hperiod) * (1024 / Hdown) * (PCLKx / PCLKO)
	//	result = ((Divider+1) * FPCLK / Hperiod) * (1024 / Hdown) * (PCLKx / PCLKO)

	HDown=ScalerReadXDownReg();
	HPeriod = MeasGetHPeriod();
	VPeriod = MeasGetVPeriod27();
	VFreq = 27000000L / VPeriod;
#if defined(SUPPORT_PC)
	if(InputMain==INPUT_PC /*|| InputMain==INPUT_COMP*/) {
		Divider = VAdcLLPLLGetDivider() + 1;
		//Divider = PCMDATA[ mode ].htotal - 1 +1;
	}
	else 
#endif
	{
		//DTV input(DVI,HDMI)
		//if DVI ??. No Component
		if(InputMain==INPUT_DVI) {
			Divider = MeasGetDviDivider();
		}
		else {
			//HDMI
			Divider = digital_divider; //DVI_Divider;
		}
	}

	VPN = MeasGetVPeriod();
	VScale = ScalerReadVScaleReg();

	H_DE = ScalerReadHDEReg();
	HActive = ScalerReadOutputWidth();
	//	FPCLK2 calculation
	//	PCLKx = REG(0x20d[1:0]) {1,2,3,4}
	//	VPN    = REG(0x522[7:0],0x523[7:0])
	//	Vscale = REG(0x206[7:0],0x205[7:0]) ;;Vtotal = VPN / (Vscale / 8192)
	//	H_DE   = REG(0x210[7:0])
	//	Hactive= REG(0x212[3:0],0x211[7:0]) ;;Htotal = H_DE + Hactive + 10
	//	Vperiod = REG(0x543[7:0],0x544[7:0],0x545[7:0]) ;;Vfreq = 27000000 / Vperiod
	//	result = (H_DE + Hactive + 1) * (VPN / (Vscale / 8192)) * (27000000 / Vperiod) * PCLKx

	dPrintf("\nPCLK:%ld, Divider: %d, HPeriod: %d, HDown: %d", PCLK, Divider, HPeriod, HDown);
	if(InputMain==INPUT_PC) {
		for ( i=2; i<=4; i++ ) {
			//PCLK1 = (DWORD)(((Divider+1) * PCLK / HPeriod) * (1024 / HDown) * i ) / (i-1);
			/*
			PCLK1 = PCLK / HPeriod;
			dPrintf("\n PCLK1 = PCLK / HPeriod :: %ld", PCLK1 );
			PCLK1 *= (Divider+1);
			dPrintf("\n PCLK1 *= (Divider+1) :: %ld", PCLK1 );
			PCLK1 /= HDown;
			dPrintf("\n PCLK1 /= HDown :: %ld", PCLK1 );
			PCLK1 *= 1024;
			dPrintf("\n PCLK1 *= 1024 :: %ld", PCLK1 );
			PCLK1 = (PCLK1 * i) / (i-1);
			*/
			PCLK1 = ((((Divider+1) * VFreq * VPN ) / HDown) * 1024 * i ) / (i-1);
			PCLK2 = (DWORD)( H_DE + HActive + 1 ) * ( VPN * 8192L* VFreq * i / VScale ) ;
			dPrintf("\n[%bd] - PCLK1: %ld, PCLK2: %ld", i, PCLK1, PCLK2);
			if ( i == 2 ) {
				PCLKO = 2;
				if ( PCLK1 > PCLK2 ) {
					PCLK = PCLK1;
				}
				else {
					PCLK = PCLK2;
				}
			}
			else {
				if ( PCLK1 > PCLK2 ) {
					if ( PCLK > PCLK1 )	{
						PCLK = PCLK1;
						PCLKO = i;
					}
				}
				else {
					if ( PCLK > PCLK2 )	{
						PCLK = PCLK2;
						PCLKO = i;
					}
				}
			}
		}
		PclkoSetDiv(PCLKO-1);
#ifdef	CHIP_MANUAL_TEST
			PclkSetPolarity(0);	//normal
#else
		if(mode>=5 && mode <= 8)	//640x480@60 
			PclkSetPolarity(0);	//normal
		else
			PclkSetPolarity(1);	//invert
#endif


		dPrintf("\nMinimum PCLK is %ld at PCLKO: %bd", PCLK, PCLKO );
		PCLK = PCLK + 4000000L;
		dPrintf("\nAdd 2MHz to PCLK is %ld", PCLK );
	
		i = SspllGetPost(0);
		PCLK = SspllFREQ2FPLL(PCLK, i);

	}
	else {
		//DVI & HDMI
		i = 3;
		{
			PCLK1 = ((((Divider+1) * VFreq * VPN ) / HDown) * 1024 * i ) / (i-1);
			PCLK2 = (DWORD)( H_DE + HActive + 1 ) * ( VPN * 8192L* VFreq * i / VScale ) ;
			dPrintf("\n[%bd] - PCLK1: %ld, PCLK2: %ld", i, PCLK1, PCLK2);
			if ( PCLK1 > PCLK2 ) {
				PCLK = PCLK1;
			}
			else {
				PCLK = PCLK2;
			}
			PCLK += 5000000L;
			if ( PCLK < 108000000L )	
				PCLK = 108000000L;
			else if ( PCLK > 120000000L )
				PCLK = 120000000L;
		}
		dPrintf("\nFound PCLK is %ld", PCLK, PCLKO );
		WriteTW88Page(0 );
		if ( PCLK == 108000000L )
			PCLK = 0x20000L;
		else {
			i = SspllGetPost(0);
			PCLK = SspllFREQ2FPLL(PCLK, i);
		}
	}


	SspllSetFreqReg(PCLK); 	

	//WriteTW88Page(page );
}
#endif


#ifdef SUPPORT_COMPONENT
//-----------------------------------------------------------------------------
/**
* convert the component mode to HW mode.
*
* SW and HW use a defferent mode value.
* ISR will check the HW mode value to check the SYNC change.
*/
static BYTE ConvertComponentMode2HW(BYTE mode)
{
	BYTE new_mode;
	switch(mode) {
	case 0: new_mode = mode;	break;	//480i
	case 1:	new_mode = mode;	break;	//576i
	case 2:	new_mode = mode;	break;	//480p
	case 3:	new_mode = mode;	break;	//576p
	case 4:	new_mode = 4;		break;	//1080i25->1080i
	case 5:	new_mode = 4;		break;	//1080i30->1080i
	case 6:	new_mode = 5;		break;	//720p50->720p
	case 7:	new_mode = 5;		break;	//720p60->720p
	case 8:	new_mode = 6;  		break;	//1080p50->1080p
	case 9:	new_mode = 6;		break;	//1080p60->1080p
	default: new_mode = 7;		break;	//UNKNOWN->non of above
	}
	return new_mode;
}
#endif

#define MEAS_YPBPR_MODE_480I		0
#define MEAS_YPBPR_MODE_576I		1
#define MEAS_YPBPR_MODE_480P		2
#define MEAS_YPBPR_MODE_576P		3
#define MEAS_YPBPR_MODE_1080I25		4
#define MEAS_YPBPR_MODE_1080I30		5
#define MEAS_YPBPR_MODE_720P50		6
#define MEAS_YPBPR_MODE_720P60		7
#define MEAS_YPBPR_MODE_1080P50		8
#define MEAS_YPBPR_MODE_1080P60		9



#ifdef SUPPORT_COMPONENT
//-----------------------------------------------------------------------------
/**
* check and set the componnent
*
* oldname: BYTE CheckAndSetYPBPR( void )
* @return
*	success	:ERR_SUCCESS
*	fail	:ERR_FAIL
*/
BYTE CheckAndSetComponent( void )
{
	BYTE	i,j;
	BYTE	mode, modeNew;
	BYTE ret;
	WORD temp16;
	DWORD temp32;
	DECLARE_LOCAL_page
	ReadTW88Page(page);

	InputVAdcMode = 0;		//BK111012

	for(i=0; i < 10; i++) {
		for(j=0; j < 10; j++) {
			if(MeasStartMeasure()) {
				WriteTW88Page(page );
				return ERR_FAIL;
			}
			// find input mode from Vfreq and VPeriod
			mode = FindInputModeCOMP();	
			if(mode)
				break;
		}	
		if(mode==0) {
			WriteTW88Page(page );
			return ERR_FAIL;
		}
#ifdef MODEL_TW8835FPGA
		SetExtVAdcI2C(0x98, mode + 4 -1);  //I did not adjust mode yet. So need a minus.
#endif

		VAdcSetLLPLLControl(0xF2);	// POST[7:6]= 3 -> div 1, VCO: 40~216, Charge Pump: 5uA
		ret = VAdcLLPLLUpdateDivider(YUVDividerPLL[ mode-1 ] - 1, 1, 0 );
		if(ret==ERR_FAIL) {
			WriteTW88Page(page );
			return ERR_FAIL;
		}		

		//BKFYI.VAdcLLPLLUpdateDivider(, 1,) has a MeasStartMeasure().

		// find input mode and compare it is same or not
		modeNew = FindInputModeCOMP();	
		if(mode==modeNew)
			break;
	}
	//if mode!=modeNew, just use mode..

	dPrintf("\nFind YUV mode: %bd", mode );
	//now adjust mode.
	mode--;
	InputVAdcMode = mode + EE_YUVDATA_START;
	InputSubMode = ConvertComponentMode2HW(mode);


	switch(mode) {
	case 0:	I2CDeviceInitialize(DataInit_Component_Init480i_step1, 0);	break;
	case 1:	I2CDeviceInitialize(DataInit_Component_Init576i_step1, 0);	break;
	case 2:	I2CDeviceInitialize(DataInit_Component_Init480p_step1, 0);	break;
	case 3:	I2CDeviceInitialize(DataInit_Component_Init576p_step1, 0);	break;
	case 4:	//we can't distinglish H28 and H31. Please select one.
			I2CDeviceInitialize(DataInit_Component_Init1080i25_H28_step1, 0);	break;
			//I2CDeviceInitialize(DataInit_Component_Init1080i25_H31_step1, 0);	break;
	case 5:	I2CDeviceInitialize(DataInit_Component_Init1080i30_step1, 0);		break;
	case 6:	I2CDeviceInitialize(DataInit_Component_Init720p50_step1, 0);	 	break;
	case 7:	I2CDeviceInitialize(DataInit_Component_Init720p60_step1, 0);		break;
	case 8:	I2CDeviceInitialize(DataInit_Component_Init1080p50_H56_step1, 0);	break;
	case 9:	I2CDeviceInitialize(DataInit_Component_Init1080p60_step1, 0);	 	break;
	default:
		break;
	}

	VAdcSetClampModeHSyncEdge(ON);					//R1D4[5]
	VAdcSetClampPosition(YUVClampPos[mode]);

	MeasSetErrTolerance(4);							//tolerance set to 32
	MeasEnableChangedDetection(ON);					// set EN. Changed Detection

	//check VPulse & adjust polarity
	temp16 = MeasGetVSyncRiseToFallWidth();
	if(temp16 > YUVDisplayV[mode]) {
		dPrintf("\nVSyncWidth:%d", temp16);
		WriteTW88Page(PAGE1_VADC );		
		WriteTW88(REG1CC, ReadTW88(REG1CC) | 0x02);	

		MeasStartMeasure();
		temp16 = MeasGetVSyncRiseToFallWidth();
		dPrintf("=>%d", temp16);		
	}

	//BKFYI: Component use a big value for Vertical Height.	0x700 + ((0xFF - YUVStartV[mode]))
	InputSetCrop(YUVStartH[mode], YUVStartV[mode], YUVDisplayH[mode], 0x700 + ((0xFF - YUVStartV[mode])));

	YUVSetOutput(mode);

	SspllSetFreqReg(0x20000);	//108MHz. Where is a POST value ?
	YUV_PrepareInfoString(mode);

	//
	//check HStart
	MeasStartMeasure();
	temp16 = MeasGetHActive2();
	dPrintf("\n**measure:%d",temp16);
	if(mode < 4) { //SDTV or EDTV
	}
	else {			//HDTV
		temp16 = MYStartH[mode] + temp16;
	}
	temp16 -=16;	//HWidth
	temp16 += 3;
	temp16 += ((YUVCropH[mode] - YUVDisplayH[mode]) / 2);
	dPrintf("\n**HStart:%d suggest:%d",YUVStartH[mode],temp16);	

	//
	//check VDE
	if(mode < 4) { //SDTV or EDTV
		temp16 = MeasGetVActive2();
		dPrintf("\n**measure:%d",temp16);
		//temp16 += 0.5;
		temp16 -= MeasGetVSyncRiseToFallWidth();  //if use faling.
		temp16 += ((YUVCropV[mode] - YUVDisplayV[mode]) / 2);
		temp32 = temp16;
		temp32 *= PANEL_V;
		temp32 /= YUVDisplayV[mode];
		dPrintf("\n**VDE:%bd suggest:%d",YUVOffsetV[mode],(WORD)temp32);
			
		temp16 = MeasGetVActive2();
		temp16 += 1;	//NOTE
		temp16 -= MeasGetVSyncRiseToFallWidth(); //if use faling.
		temp16 += ((YUVCropV[mode] - YUVDisplayV[mode]) / 2);
		temp32 = temp16;
		temp32 *= PANEL_V;
		temp32 /= YUVDisplayV[mode];
		dPrintf("~%d",(WORD)temp32);
	}
	else {			//HDTV
		temp16 = MYStartV[mode];
		//temp16 += 0.5;
		temp16 -= MeasGetVSyncRiseToFallWidth();
		temp16 += ((YUVCropV[mode] - YUVDisplayV[mode]) / 2);
		temp32 = temp16;
		temp32 *= PANEL_V;
		temp32 /= YUVDisplayV[mode];
		dPrintf("\n**VDE:%bd suggest:%d",YUVOffsetV[mode],(WORD)temp32);	
		temp16 = MYStartV[mode];
		temp16 += 1;   	//NOTE
		temp16 -= MeasGetVSyncRiseToFallWidth();
		temp16 += ((YUVCropV[mode] - YUVDisplayV[mode]) / 2);
		temp32 = temp16;
		temp32 *= PANEL_V;
		temp32 /= YUVDisplayV[mode];
		dPrintf("~%d",(WORD)temp32);	
	}
	

	if(mode==0 || mode==1) //480i,  576i
		InputSetFieldPolarity(0);
	else
		InputSetFieldPolarity(1);

	WriteTW88Page(page );
	return ERR_SUCCESS;
}
#endif


#undef CHECK_USEDTIME
#ifdef SUPPORT_PC
//-----------------------------------------------------------------------------
/**
* check and set the PC
*
* calls from ChangePC and Interrupt Handler
* @return
*	0:ERR_SUCCESS
*	1:ERR_FAIL
* @see ChangePC
* @see CheckAndSetInput
* @see NoSignalTask
*/
BYTE CheckAndSetPC(void)
{
	BYTE mode,new_mode;

#ifdef CHECK_USEDTIME
	DWORD UsedTime;
#endif
	BYTE value;
	BYTE value1;
	WORD new_VTotal;
	WORD wTemp;
	BYTE ret;
	DECLARE_LOCAL_page


	ReadTW88Page(page);
#ifdef CHECK_USEDTIME
	UsedTime = SystemClock;
#endif
	InputVAdcMode = 0;		//BK111012
	InputSubMode = InputVAdcMode;

	do {
		mode = PCCheckMode();
		if(mode==0) {
			WriteTW88Page(page );
			return ERR_FAIL;
		}

#ifdef MODEL_TW8835FPGA
		if(mode==5)						SetExtVAdcI2C(0x98, 0);		//VGA@60
		else if(mode==10)				SetExtVAdcI2C(0x98, 1);		//SVGA@60
		else if(mode==18)				SetExtVAdcI2C(0x98, 2);		//XGA@60
		else if(mode==43 || mode==44)	SetExtVAdcI2C(0x98, 3);		//SXGA 1400x1050@60
#endif

		//
		//set LLPLL	& wait
		//
		//BK110927
		//BK110928 assume LoopGain:2.
		//LLPLL divider:PCMDATA[ mode ].htotal - 1
		//ControlValue, 0xF2.  POST[7:6]= 3 -> div 1, VCO: 40~216, Charge Pump: 5uA
		//LLPLL init: ON
		//Wait delay for WaitStableLLPLL: 40ms
		//LLPLL Control.
		VAdcSetLLPLLControl(0xF2);	// POST[7:6]= 3 -> div 1, VCO: 40~216, Charge Pump: 5uA
		ret = VAdcLLPLLUpdateDivider(PCMDATA[ mode ].htotal - 1, 1, 40 );
		if(ret==ERR_FAIL) {
			WriteTW88Page(page );
			return ERR_FAIL;
		}

 		VAdcSetPolarity(0);

		//check Phase EEPROM.
		value = GetPhaseEE(mode);
		if(value!=0xFF) {
			dPrintf("\nuse Phase 0x%bx",value);
			value1=VAdcGetPhase();
			if(value != value1) {
				dPrintf("  update from 0x%bx",value1);
				VAdcSetPhase(value, 0);	//BKTODO? Why it does not have a init ?
				if(WaitStableLLPLL(0)) {
					WriteTW88Page(page );
					return ERR_FAIL;
				}
			}
			else {
				WaitStableLLPLL(0); //BK110830
				MeasCheckVPulse();
			}
		}
		else 
		{
			AutoTunePhase();
			value=VAdcGetPhase();
			dPrintf("\ncalculate Phase %bx",value);
			SavePhaseEE(mode,value);

			if(WaitStableLLPLL(0)) {
				WriteTW88Page(page );
				return ERR_FAIL;
			}
		}
		//adjust polarity again and update all measured value
		VAdcSetPolarity(0);
		MeasStartMeasure();

		//use measured value.  
		MeasVLen = MeasGetVActive( &MeasVStart );				//v_active_start v_active_perios
		MeasHLen = MeasGetHActive( &MeasHStart );				//h_active_start h_active_perios

		dPrintf("\nMeasure Value Start %dx%d Len %dx%d", MeasHStart,MeasVStart, MeasHLen,MeasVLen);

		if ( MeasVLen < PCMDATA[ mode ].van ) {			// use table
			MeasVStart = PCMDATA[mode].vstart;
			MeasVLen = PCMDATA[mode].van;
			dPrintf("->VS:%d VLen:%d",MeasVStart,MeasVLen);
		}
		if ( MeasHLen < PCMDATA[ mode ].han ) {			// use table
			MeasHStart = PCMDATA[mode].hstart;
			MeasHLen = PCMDATA[mode].han;
			dPrintf("->HS:%d HLen:%d",MeasHStart,MeasHLen);
		}

		PCSetInputCrop(mode);
		PCSetOutput(mode);

		new_mode = FindInputModePC(&new_VTotal);
	} while(mode != new_mode);

	InputVAdcMode = mode;
	InputSubMode = InputVAdcMode;

	//EE
	//PCLKAdjust();
	AdjustPixelClk(0, mode); //BK120117 need a divider value

	//adjust PCPixelClock here.
	//If R1C4[], measure block use a wrong value.
	wTemp = PCMDATA[ mode ].htotal - 1;
	dPrintf("\nPixelClk %d",wTemp);
	wTemp += GetPixelClkEE(mode);	//0..100. default:50
	wTemp -= 50;
	dPrintf("->%d EEPROM:%bd",wTemp,GetPixelClkEE(mode));

	VAdcLLPLLUpdateDivider(wTemp, OFF, 0);	//without init. Do you need VAdcSetLLPLLControl(0xF2) ?
	
	MeasSetErrTolerance(4);						//tolerance set to 32
	MeasEnableChangedDetection(ON);				//set EN. Changed Detection
	
	PC_PrepareInfoString(mode);

#ifdef CHECK_USEDTIME
	UsedTime = SystemClock - UsedTime;
	Printf("\nUsedTime:%ld.%ldsec", UsedTime/100, UsedTime%100 );
#endif
			
	WriteTW88Page(page );

	return ERR_SUCCESS;
}
#endif

//=============================================================================
//setup menu interface
//=============================================================================
#ifdef SUPPORT_PC
//-----------------------------------------------------------------------------
void PCRestoreH(void)
{
	WORD hstart;
	hstart = RGB_HSTART;

	if(InputVAdcMode==0) {
		//?Freerun mode
		return;
	}
	//adjust EEPROM. 0..100. base 50. reversed value.
	hstart += 50;
	hstart -= GetHActiveEE(InputVAdcMode); //PcBasePosH;
	InputSetHStart(hstart);
}
//-----------------------------------------------------------------------------
void PCRestoreV(void)
{
	WORD temp16;
	temp16 = RGB_VDE;
	dPrintf("\n\tV-DE start = %d", temp16);

	if(InputVAdcMode==0) {
		//?Freerun mode
		return;
	}

	temp16 += GetVBackPorchEE(InputVAdcMode);
	temp16 -= 50;
	dPrintf("=> %d", temp16);
	ScalerWriteVDEReg((BYTE)temp16);
}
//-----------------------------------------------------------------------------
void PCResetCurrEEPROMMode(void)
{
	BYTE temp;
	temp = GetPixelClkEE(InputVAdcMode);
	if(temp!=50)
		SavePixelClkEE(InputVAdcMode,50);
	temp = GetPhaseEE(InputVAdcMode);
	if(temp != 0xFF)	//NOTE. CheckAndSetPC1 will update it.
		SavePhaseEE(InputVAdcMode,0xFF);	
	temp = GetHActiveEE(InputVAdcMode);
	if(temp!=50)
		SaveHActiveEE(InputVAdcMode,50);
	temp = GetVBackPorchEE(InputVAdcMode);
	if(temp!=50)
		SaveVBackPorchEE(InputVAdcMode,50);	
}
#endif
