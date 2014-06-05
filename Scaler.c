/**
 * @file
 * scaler.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	video scaler module 
 *
 * input => (scale down) => line buff => (scale up) =>  output	panel
 */

#include "Config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"
#include "Global.h"

//#include "main.h"
#include "Printf.h"
#include "Monitor.h"
#include "I2C.h"
#include "CPU.h"
#include "SOsd.h"
#include "FOsd.h"

#include "InputCtrl.h"
#include "VAdc.h"
#include "PC_modes.h"
#include "measure.h"
#include "settings.h"
#include "scaler.h"
#include "eeprom.h"
#include "util.h"

BYTE VideoAspect;

//=============================================================================
// Register Functions
//=============================================================================

//static function prototypes


//-----------------------------------------------------------------------------
//LNFIX		R201[2]		on/off
//			1 = Fix the scaler output line number defined by register LNTT.
//			0 = Output line number determined by scaling factor.
//LNTT		R20D[7:6]R219[7:0]	lines
//			It controls the scaler total output lines when LNFIX=1. It is used in special case. A 10-bit register.
//other name: Limit V Total
#ifdef UNCALLED_SEGMENT
void ScalerSetOutputFixedVline(BYTE onoff)
{
	WriteTW88Page(PAGE2_SCALER );
	if(onoff)	WriteTW88(REG201, ReadTW88(REG201) | 0x04);
	else		WriteTW88(REG201, ReadTW88(REG201) & 0xFB);
}
#endif

//register
//	R202[5:0]
#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
void ScalerSetFieldOffset(BYTE fieldOffset)
{
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG202, (ReadTW88(REG202) & 0xC0) | filedOffset);
}

#endif

//-------------------------------------
//Scaler scale ratio
//-------------------------------------

/*
XSCALE
*	register
*	R204[7:0]R203[7:0]	XSCALE UP
*	R20A[3:0]R209[7:0]	X-Down
*/

//-----------------------------------------------------------------------------
/**
* write Horizontal Up Scale register
*
* Up scaling ratio control in X-direction. 
* A 16-bit register. 
* The scaling ratio is defined as 2000h / XSCALE
*
*	register
*	R204[7:0]R203[7:0]	XSCALE UP
*/
void ScalerWriteXUpReg(WORD value)
{
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG204, (BYTE)(value>>8));		
	WriteTW88(REG203, (BYTE)value);
}
#if 0
//-----------------------------------------------------------------------------
WORD ScalerReadXUpReg(void)
{
	WORD wValue;
	WriteTW88Page(PAGE2_SCALER);
	Read2TW88(REG204,REG203,wValue);
	return wValue;
}
#endif

//-----------------------------------------------------------------------------
/**
* write Horizontal Down Scale register
*
* Down scaling ratio control in X-direction. 
* A 12-bit register. 
* The down scaling ratio is defined as 100h / HDSCALE
*	register
*	R204[7:0]R203[7:0]	XSCALE UP
*	R20A[3:0]R209[7:0]	X-Down
*/
void ScalerWriteXDownReg(WORD value)
{
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG20A, (ReadTW88(REG20A) & 0xF0) | (value >> 8));
	WriteTW88(REG209, (BYTE)value);
}

#if defined(SUPPORT_PC) || defined(SUPPORT_DVI) || defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
//-----------------------------------------------------------------------------
/**
* read Horizontal Down Scale register
*
* @see ScalerWriteXDownReg
*/
WORD ScalerReadXDownReg(void)
{
	WORD HDown;

	WriteTW88Page(PAGE2_SCALER );
	HDown = ReadTW88(REG20A ) & 0x0F;
	HDown <<= 8;
	HDown += ReadTW88(REG209 );
	return HDown;
}
#endif

#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
void ScalerSetHScaleReg(WORD down, WORD up)
{
	ScalerWriteXDownReg(down);
	ScalerWriteXUpReg(up);
}
#endif


//-----------------------------------------------------------------------------
//internal
/**
* set Horizontal Scaler value on FULL mode
*
* @see ScalerSetHScale
*/
void ScalerSetHScale_FULL(WORD Length)	
{
	DWORD	temp;

	ScalerWriteLineBufferDelay(1);
	ScalerPanoramaOnOff(OFF);

	WriteTW88Page(PAGE2_SCALER);
	if(PANEL_H >= Length) { 					
		//UP SCALE
		temp = Length * 0x2000L;
		//temp += (PANEL_H/2);			//roundup	
		temp /= PANEL_H;
		ScalerWriteXUpReg(temp);				//set up scale
		ScalerWriteXDownReg(0x0400);			//clear down scale
		dPrintf("\nScalerSetHScale(%d) DN:0x0400 UP:0x%04lx",Length, temp);
		if(InputMain==INPUT_COMP
		|| InputMain==INPUT_DVI
		|| InputMain==INPUT_HDMITV
		|| InputMain==INPUT_HDMIPC)
			ScalerSetLineBufferSize(Length);
		if(InputMain==INPUT_PC)
			ScalerSetLineBufferSize(PANEL_H);
	}
	else {										
		//DOWN SCALE
		if(InputMain==INPUT_PC
		|| InputMain==INPUT_COMP
		|| InputMain==INPUT_DVI
		|| InputMain==INPUT_HDMITV
		|| InputMain==INPUT_HDMIPC) {
			temp = Length * 0x0400L;						
			temp += 0x0200L;			//roundup ??	
		}
		else {
			Length++;		//BK110613
			temp = Length * 0x0400L;
		}						
		temp /= PANEL_H;
		ScalerWriteXUpReg(0x2000);			//clear up scale
		ScalerWriteXDownReg(temp);			//set down scale
		dPrintf("\nScalerSetHScale(%d) DN:0x%04lx UP:0x2000",Length, temp);
		if(InputMain==INPUT_COMP
		|| InputMain==INPUT_DVI
		|| InputMain==INPUT_PC
		|| InputMain==INPUT_HDMIPC
		|| InputMain==INPUT_HDMITV
		)
			ScalerSetLineBufferSize(PANEL_H );
	}
}

//-----------------------------------------------------------------------------
//internal
/**
* set Horizontal Scaler value on Panorama mode
*
* @see ScalerSetHScale
*/
void ScalerSetHScale_Panorama(WORD Length)	
{
	DWORD	temp;
	WORD	X1;
	WORD 	linebuff;

	X1 = Length;
	X1 += 32;

	WriteTW88Page(PAGE2_SCALER);
	if(PANEL_H >= X1) {
		//
		//UP SCALE
		//
		X1 = Length;
		X1 += 34;				//32+2
		linebuff = Length+1;

		dPrintf("\nScalerSetHScale(%d->%d) ",Length,X1); 

		temp = X1 * 0x2000L;					//8192
		temp /= PANEL_H;						//800
		ScalerWriteXUpReg(temp);				//set up scale
		ScalerWriteXDownReg(0x0400);			//clear down scale
		dPrintf("DN:0x0400 UP:0x%04lx lbuff:%d", temp,linebuff);
		ScalerSetLineBufferSize(linebuff);
	}
	else {
		//
		// DOWN SCALE
		//
		linebuff = PANEL_H - 34*2;				//(32+2)*2
		temp = Length * 0x0400L;				//1024						
		temp /= (linebuff - 1);					//target 800->731
		ScalerWriteXUpReg(0x2000);				//clear up scale
		ScalerWriteXDownReg(temp);				//set down scale
		dPrintf("DN:0x%04lx UP:0x2000 lbuff:%d", temp,linebuff);
		ScalerSetLineBufferSize(linebuff+1);
	}
	ScalerSetPanorama(0x400,0x20);
	ScalerPanoramaOnOff(ON);
}

//-----------------------------------------------------------------------------
/**
* set Horizontal Scaler value with ratio
*
*/
void ScalerSetHScaleWithRatio(WORD Length, WORD ratio)	
{
	DWORD temp;
	WORD new_Length;
	dPrintf("\nScalerSetHScaleWithRatio(%d,%d)",Length,ratio);
	ScalerWriteLineBufferDelay(1);
	ScalerPanoramaOnOff(OFF);

	WriteTW88Page(PAGE2_SCALER);

	temp = Length;
	temp *= ratio;
	temp /= 100;	//new length
	new_Length = temp;

	dPrintf("\nHLength %d->%d", Length, new_Length);
	if(ratio < 100) {
		//down scale
		ScalerWriteXUpReg(0x2000);			//clear up scale
		temp = 0x0400L;
		temp *= ratio;
		temp /= 100;
		ScalerWriteXDownReg(temp);
	}
	else {
		//upscale
		ScalerWriteXDownReg(0x0400);		// clear down scale
		temp = 0x2000L;
		temp *= ratio;
		temp /= 100;
		ScalerWriteXUpReg(temp);
	}
	if(new_Length < PANEL_H) {
		//adjust buffer output delay
		ScalerWriteLineBufferDelay((PANEL_H - new_Length) / 2 +1);
	}
}


//-----------------------------------------------------------------------------
/**
* set Horizontal Scaler value
*
* @see ScalerSetHScale_FULL
* @see ScalerSetHScale_Panorama
*/
void ScalerSetHScale(WORD Length)	
{
	if(InputMain==INPUT_PC)
		VideoAspect = GetAspectModeEE();	//BK1100914


	if((InputMain==INPUT_CVBS || InputMain==INPUT_SVIDEO)
	&& VideoAspect == VIDEO_ASPECT_NORMAL) {

		ScalerWriteLineBufferDelay(1);	//BK110916 test. Normal needs it
		ScalerPanoramaOnOff(OFF);

		WriteTW88Page(PAGE2_SCALER);

		//only at CVBS
		ScalerWriteXUpReg(0x2000);			//clear up scale
		ScalerWriteXDownReg(0x0400);			// clear down scale
		//adjust buffer output delay
		ScalerWriteLineBufferDelay((PANEL_H - Length) / 2 +1);

		ScalerSetLineBufferSize(Length); //BK120111
	}
	else if(VideoAspect == VIDEO_ASPECT_PANO)
		ScalerSetHScale_Panorama(Length);
	else 
		//	 VideoAspect == VIDEO_ASPECT_FULL
		//of VideoAspect == VIDEO_ASPECT_ZOOM
		ScalerSetHScale_FULL(Length);
}


//YSCALE
//-----------------------------------------------------------------------------
/**
* Up / down scaling ratio control in Y-direction. 
*
* The scaling ratio is defined as 2000h / YSCALE.
* A 16-bit register. 
*
*	register
*	R206[7:0]R205[7:0]	YSCALE	
*/
void ScalerWriteVScaleReg(WORD value)
{
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG206, (BYTE)(value>>8));
	WriteTW88(REG205, (BYTE)value);
}

//-----------------------------------------------------------------------------
/**
* read Vertical Scale register value
*
* @see ScalerWriteVScaleReg
*/
WORD ScalerReadVScaleReg(void)
{
	WORD VScale;
	WriteTW88Page(PAGE2_SCALER);
	Read2TW88(REG206,REG205, VScale);
	return VScale;
}

//-----------------------------------------------------------------------------
/**
* set Vertical Scale with Ratio
*
*/
void ScalerSetVScaleWithRatio(WORD Length, WORD ratio)
{
	DWORD temp;
	WORD new_Length;

	dPrintf("\nScalerSetVScaleWithRatio(%d,%d)",Length,ratio);
	temp = Length;
	temp *= ratio;
	temp /= 100;	//new length
	new_Length = temp - Length;			//offset.
	new_Length = Length - new_Length;	//final 
	dPrintf("\nVLength %d->%d", Length, new_Length);

	temp = new_Length * 0x2000L;
	temp /= PANEL_V;
	ScalerWriteVScaleReg(temp);
}

//-----------------------------------------------------------------------------
/**
* set Vertical Scale
*
*/
void ScalerSetVScale(WORD Length)
{
	DWORD	temp;

	WriteTW88Page(PAGE2_SCALER);

    if((InputMain==INPUT_CVBS || InputMain==INPUT_SVIDEO) 
	&& VideoAspect == VIDEO_ASPECT_ZOOM) {
		//rate 720->800
		//
		dPrintf("\nLength:%d",Length);
		temp = Length;	
		temp = temp * 800 / 720;
		temp = temp - Length;	
		Length = Length - temp;
		dPrintf("=>%d",Length);
	}
	//else 
	{
		temp = Length * 0x2000L;
		if(InputMain==INPUT_PC
		|| InputMain ==INPUT_COMP
		|| InputMain ==INPUT_DVI) {
			if ( Length > PANEL_V ) {		// down scaling //BK110916??
				temp += 0x1000L;			// round up.
			}
		}
		else {
			//temp += (PANEL_V / 2);	//roundup
		}
		temp /= PANEL_V;
	
		dPrintf("\nScalerSetVScale(%d) 0x%04lx",Length, temp);
	
		ScalerWriteVScaleReg(temp);
	}
}

//-------------------------------------
//Scaler Panorama
//-------------------------------------

//-----------------------------------------------------------------------------
/**
* set Panorama mode
*
*	register
*	R201[6]	Enable Panorama/waterglass display
*/
void ScalerPanoramaOnOff(BYTE fOn)
{
	WriteTW88Page(PAGE2_SCALER);
	if(fOn)	WriteTW88(REG201, ReadTW88(REG201) | 0x40);
	else    WriteTW88(REG201, ReadTW88(REG201) & ~0x40);
}

//-----------------------------------------------------------------------------
/**
* set the panorama parameters
*
*	register
*	R207[7:0]R212[7:4]	PXSCALE
*	R208[7:0]			PXINC
*/
void ScalerSetPanorama(WORD px_scale, BYTE px_inc)
{
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG207, px_scale >> 4);
	WriteTW88(REG212, (ReadTW88(REG212) & 0x0F) | (BYTE)(px_scale & 0x0F));
	WriteTW88(REG208, px_inc);
}


//-------------------------------------
//	Scaler LineBuffer
//-------------------------------------
//Output Buffer Delay R20B[7;0]
//Output Buffer Length R20E[6:4]R20C[7:0]
//See the Horizontal Timing on "AN-TW8832,33 Scaler & TCON".
//set the "Output Delay" and "Output Length" of the Line Buffer Output on Horizontal Timming Flow.

//-----------------------------------------------------------------------------
/**
* Write scaler LineBuffer output delay
*
* HDE is related with this delay value.
*
*	register
*	R20B
*	R20B[7:0]			HDELAY2	
*		Scaler buffer data output delay in number of pixels in relation to the H sync.
*/
void ScalerWriteLineBufferDelay(BYTE delay)
{
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG20B, delay);
}
//-----------------------------------------------------------------------------
/**
* Read scaler LineBuffer output delay
*
* @see ScalerWriteLineBufferDelay
*/
BYTE ScalerReadLineBufferDelay(void)
{
	WriteTW88Page(PAGE2_SCALER );
	return ReadTW88(REG20B);
}

//-----------------------------------------------------------------------------
/**
* set Scaler OutputLength that is related with the line buffer size. 
*
* max 1024.
* 
*	register
*	R20E[6:4]:R20C[7:0]		HACTIVE
*		Scaler data output length in number of pixels. 
*		A 10-bit register.==>11
*/
void ScalerSetLineBufferSize(WORD len)
{
	if(len>PANEL_H)
		len=PANEL_H;

	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG20E, (ReadTW88(REG20E) & 0x8F) | ((len & 0x700) >> 4));
	WriteTW88(REG20C, (BYTE)len);
}

#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
void ScalerSetLineBuffer(BYTE delay, WORD len)
{
	ScalerWriteLineBufferDelay(delay);
	ScalerSetLineBufferSize(len);
}
#endif

//only for LCOS
#if 0
//-----------------------------------------------------------------------------
void ScalerSetFPHSOutputPolarity(BYTE fInvert)
{
	WriteTW88Page(PAGE2_SCALER);
	if(fInvert) WriteTW88(REG20D, ReadTW88(REG20D) |  0x04);
	else		WriteTW88(REG20D, ReadTW88(REG20D) & ~0x04);
}
#endif

//HPADJ		R20E[3:0]R20F[7:0]
//			Blanking H period adjustment. A 12-bit 2's complement register
#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
void ScalerWriteOutputHBlank(WORD length)
{
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG20E, (ReadTW88(REG20E) & 0xF0) | (length >> 8) );
	WriteTW88(REG20F, (BYTE)length);
}
#endif


//-----------------------------------------------------------------------------
/**
* set Horizontal DE position(DEstart) & length(active, DEwidth).
*
*	R210[7:0]	HA_POS
*				Output DE position control relative to the internal reference in number of output clock
* set the "DEstart" and "DEwidth" on Horizontal Timming Flow.
*/
void ScalerWriteHDEReg(BYTE pos)
{
	//no not add debugmsg, it will makes a blink.
	//dPrintf("\nScalerWriteHDEReg pos:%bd",pos);
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG210, pos);
}
//-----------------------------------------------------------------------------
/**
* read HDE register
*
* @see ScalerWriteHDEReg
*/
BYTE ScalerReadHDEReg(void)
{
	WriteTW88Page(PAGE2_SCALER );
	return ReadTW88(REG210);
}

//-----------------------------------------------------------------------------
/**
* Calculate HDE value.
*
*	method
*	Buffer_Delay = REG(0x20b[7:0])
*	result = Buffer_Delay + 32
*/
WORD ScalerCalcHDE(void)
{
	WORD wTemp;
	wTemp = ScalerReadLineBufferDelay();
	return wTemp+32;
}


//-----------------------------------------------------------------------------
//register
//	R212[3:0]R211[7:0]	HA_LEN
//						Output DE length control in number of the output clocks. A 12-bit register
//						output height. normally PANEL_V
/**
* Read output witdh
*/
WORD ScalerReadOutputWidth(void)
{
	WORD HActive;
	WriteTW88Page(PAGE2_SCALER);
	HActive = ReadTW88(REG212) & 0x0F;
	HActive <<= 8;
	HActive |= ReadTW88(REG211);
	return HActive;
}
//set Scaler.Output.Width
#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
void ScalerWriteOutputWidth(WORD width)
{
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG212, (ReadTW88(REG212) & 0xF0) | (BYTE)(width>>8));
	WriteTW88(REG211, (BYTE)width);
}
#endif

//HS_POS	R213[7:0]	HSynch pos
//			Output H sync position relative to internal reference in number of output clocks.
//HS_LEN	R214[3:0]	HSynch width
//			Output H sync length in number of output clocks
//HSstart 
//HSwidth
//set the "HSstart" and "HSwidth" on Horizontal Timming Flow.
// Scaler set output HSynch
#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
void ScalerSetHSyncPosLen(BYTE pos, BYTE len)
{
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG213, pos);
	WriteTW88(REG214, (ReadTW88(REG214) & 0xF0) | len);
}
#endif

//VA_POS	R215[7:0]		VDE pos 
//			Output DE position control relative to the internal reference in number of output lines
//VA_LEN	R217[3:0]R216[7:0]	  width
//			Output DE control in number of the output lines. A 12-bit register

//-----------------------------------------------------------------------------
/**
* Read Vertical DE register
*/
BYTE ScalerReadVDEReg(void)
{
	WriteTW88Page(PAGE2_SCALER );
	return ReadTW88(REG215);
}
//-----------------------------------------------------------------------------
/**
* Write Vertical DE register
*/
void ScalerWriteVDEReg(BYTE pos)
{
	//dPrintf("\nScalerSetVDEAndWidth pos:%bd len:%d",pos,len);
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG215, pos);
}

//-----------------------------------------------------------------------------
//register
//	R217[3:0]R216[7:0]	output width. normally PANLE_H+1
#ifdef UNCALLED_SEGMENT
void ScalerWriteOutputHeight(WORD height)
{
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG217, (ReadTW88(REG217) & 0xF0) | (BYTE)(height>>8));
	WriteTW88(REG216, (BYTE)height);
}
#endif
//-----------------------------------------------------------------------------
/**
* Read Outout Height register
*/
WORD ScalerReadOutputHeight(void)
{
 	WORD height;
	WriteTW88Page(PAGE2_SCALER );
	height = ReadTW88(REG217) & 0x0F;
	height <<= 8;
	height |= ReadTW88(REG216);	//V Width
	return height;
}

#if defined(SUPPORT_COMPONENT) || defined(SUPPORT_PC) || defined(SUPPORT_DVI)
//-----------------------------------------------------------------------------
/**
* Calculate VDE value.
*
*	method
*	VStart = REG(0x536[7:0],0x537[7:0])
*	VPulse = REG(0x52a[7:0],0x52b[7:0])
*	VPol = REG(0x041[3:3])
*	VScale = REG(0x206[7:0],0x205[7:0])
*	result = ((VStart - (VPulse * VPol)) * 8192 / VScale) + 1
*/
WORD ScalerCalcVDE(void)
{
	BYTE VPol;
	WORD VStart,VPulse,VScale;
	DWORD dTemp;;

	WriteTW88Page(PAGE5_MEAS);
	Read2TW88(REG536,REG537, VStart);
	Read2TW88(REG52A,REG52B, VPulse);

	if(InputMain==INPUT_DVI) {
		WriteTW88Page(PAGE0_DTV);
		VPol = ReadTW88(REG050) & 0x02 ? 1: 0;
	}
	else {
		WriteTW88Page(PAGE0_INPUT);
		VPol = ReadTW88(REG041) & 0x08 ? 1: 0;
	}
	VScale = ScalerReadVScaleReg();

	dTemp = VStart;
	if(VPol) {
		if(dTemp < VPulse) {
			ePrintf("\nBugBug: dTemp:%ld < VPulse:%d",dTemp,VPulse);
			WriteTW88Page(PAGE5_MEAS);
			Read2TW88(REG538,REG539, VStart);
			dTemp = VStart;
		}

		dTemp -= VPulse;
	}
	dTemp = (dTemp * 8192L) / VScale;
	dTemp++;
	
	return (WORD)dTemp;
}
#endif

//-----------------------------------------------------------------------------
//set the "VDEstart" and "VDEwidth" on Vertical Timming Flow.
#if 0
void ScalerSetVDEPosHeight(BYTE pos, WORD len)
{
	dPrintf("\nScalerSetVDEPosHeight pos:%bd len:%d",pos,len);
	ScalerWriteVDEReg(pos);
	ScalerWriteOutputHeight(len);
}
#endif


//-----------------------------------------------------------------------------
//register
//	R212[3:0]R211[7:0]	HA_LEN
//						Output DE length control in number of the output clocks. A 12-bit register
//						output height. normally PANEL_V
//	R217[3:0]R216[7:0]	output width. normally PANLE_H+1
#ifdef UNCALLED_SEGMENT
void ScalerSetOutputWidthAndHeight(WORD width, WORD height)
{
	ScalerWriteOutputWidth(width);
	ScalerWriteOutputHeight(height);
}
#endif


//-----------------------------------------------------------------------------
//VS_LEN		R218[7:6]	VSyhch width
//VS_POS		R218[5:0]	VSyhch pos
//set the "VSstart" and "VSwidth" on Vertical Timming Flow.
//Scaler set output VSynch
#ifdef UNCALLED_SEGMENT
void ScalerSetVSyncPosLen(BYTE pos, BYTE len)
{
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG218, len << 6 | pos);
}
#endif


//-----------------------------------------------------------------------------
/**
* Write Freerun VTotal value
*
*	R20D[7:6]:R219[7:0]
*/
void ScalerWriteFreerunVtotal(WORD value)
{
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG20D, (ReadTW88(REG20D)&0x3F)|(value>>2)&0xC0);
	WriteTW88(REG219, (BYTE)value );
}
//-----------------------------------------------------------------------------
/**
* Read Freerun VTotal value
*
* @see ScalerWriteFreerunVtotal
*/
WORD ScalerReadFreerunVtotal(void)
{
	WORD value;

	WriteTW88Page(PAGE2_SCALER );
	value = ReadTW88(REG20D) & 0xC0;
	value <<= 2;
	value |= ReadTW88(REG219);
	return value;
}
//-----------------------------------------------------------------------------
/**
* calcualte Freerun VTotal value
*
*	method
*	VPN    = REG(0x522[7:0],0x523[7:0])
*	Vscale = REG(0x206[7:0],0x205[7:0])
*	result = VPN / (Vscale / 8192)
*
* NOTE: It needs a MeasStart.
*/
WORD ScalerCalcFreerunVtotal(void)
{
	WORD VScale;
	DWORD temp32;
	
	VScale = ScalerReadVScaleReg();
	
	temp32 = MeasGetVPeriod();
	temp32 *= 8192L;
	temp32 /= VScale;
	
	return (WORD)temp32;
}


//-----------------------------------------------------------------------------
//DM_TOP	R21A[7:0]	top 	 
//DM_BOT	R21B[7:0]	bottom 
//set number of data masked lines from the top of DE and the end of DE.
#ifdef UNCALLED_SEGMENT
void ScalerSetVDEMask(BYTE top, BYTE bottom)
{
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG21A, top);
	WriteTW88(REG21B, bottom);
}
#endif

//-----------------------------------------------------------------------------
/**
* Write Freerun Htotal value 
*
*	R21C[7:4]:R21D[7:0]
*/
void ScalerWriteFreerunHtotal(WORD value)
{
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG21C, (ReadTW88(REG21C)&0x0F)|(value>>4)&0xF0);
	WriteTW88(REG21D, (BYTE)value );
}
//-----------------------------------------------------------------------------
/**
* read Freerun HTotal value
*
* @see ScalerWriteFreerunHtotal
*/
WORD ScalerReadFreerunHtotal(void)
{
	WORD value;

	WriteTW88Page(PAGE2_SCALER );
	value = ReadTW88(REG21C) & 0xF0;
	value <<= 4;
	value |= ReadTW88(REG21D);
	return value;
}
//-----------------------------------------------------------------------------
/**
* calculate the Freerun HTotal value
*
*	method
*	HPN    = REG(0x524[7:0],0x525[7:0])
*	Vscale = REG(0x206[7:0],0x205[7:0])
*	PCLKO  = REG(0x20d[1:0])
*	result = (HPN * Vscale / 8192) / (PCLKO+1)
*/
WORD ScalerCalcFreerunHtotal(void)
{
	WORD VScale;
	BYTE PCLKO;
	DWORD temp32;

	VScale = ScalerReadVScaleReg();
	WriteTW88Page(PAGE2_SCALER);
	PCLKO = ReadTW88(REG20D)&0x03;


	temp32 = MeasGetHPeriod();
	temp32 *= VScale;
	temp32 /= 8192L;
	temp32 /= (PCLKO+1);

	Printf("\nScalerCalcFreerunHtotal temp:%ld",temp32);
	
	return (WORD)temp32;
}


//RRUN		R21C[2]
//			Panel free run control. 1 = free run with HTOTAL and LNTT.
//-----------------------------------------------------------------------------
/**
* Force scaler data output to all 0’s
*
* comes fromTW8823.
* free run control R21C[2] -Panel free run control. 1 = free run with HTOTAL(R21C[7:4]R21D[7:0] and LNTT(R20D[7:6]R219[7:0]).
* free run on the condition of input loss R21C[1].
*/
void ScalerSetFreerunManual( BYTE on )
{
	WriteTW88Page(PAGE2_SCALER );

	if(on)	WriteTW88(REG21C, (ReadTW88(REG21C) | 0x04) );			//on manual freerun
	else 	WriteTW88(REG21C, (ReadTW88(REG21C) & ~0x04) );		//off manual freerun
}
#ifdef UNCALLED_CODE
BYTE ScalerIsFreerunManual( void )
{
	BYTE value;

	WriteTW88Page(PAGE2_SCALER );
	value = ReadTW88(REG21C);
	if(value & 0x04) return 1;
	return 0;
}
#endif
//-----------------------------------------------------------------------------
/**
* set FreerunAuto and FreerunManual
*/
void ScalerSetFreerunAutoManual(BYTE fAuto, BYTE fManual)
{
	BYTE value;
	
	WriteTW88Page(PAGE2_SCALER );
	value = ReadTW88(REG21C);
	if(fAuto != 0x02) {
		if(fAuto)	value |= 0x02;		//on auto freerun
		else 		value &= ~0x02;		//off auto freerun		
	}
	if(fManual != 0x02) {
		if(fManual)	value |= 0x04;		//on manual freerun
		else 		value &= ~0x04;		//off manual freerun		
	}
	WriteTW88(REG21C, value);
}
//-----------------------------------------------------------------------------
/**
* set MuteAuto & MuteManual
*/
void ScalerSetMuteAutoManual(BYTE fAuto, BYTE fManual)
{
	BYTE value;
	
	WriteTW88Page(PAGE2_SCALER );
	value = ReadTW88(REG21E);
	if(fAuto != 0x02) {
		if(fAuto)	value |= 0x02;		//on auto mute
		else 		value &= ~0x02;		//off auto mute		
	}
	if(fManual != 0x02) {
		if(fManual)	value |= 0x01;		//on manual mute
		else 		value &= ~0x01;		//off manual mute		
	}
	WriteTW88(REG21E, value);
}

//-----------------------------------------------------------------------------
/**
* set MuteManual
*/
void ScalerSetMuteManual( BYTE on )
{
	Wait1VBlank();
	WriteTW88Page(PAGE2_SCALER );

	if(on)	WriteTW88(REG21E, (ReadTW88(REG21E) | 0x01) );			//on manual mute
	else 	WriteTW88(REG21E, (ReadTW88(REG21E) & ~0x01) );		//off manual mute
}


//PanelFreerun value
//
//
//      component					CVBS
//mode	Htotal	Vtotal XYScale		Htotal	Vtotal	XYScale
//----	------	------ -------		------	------	-------
//480i	1069	551	   1B5C	0F33	1069	551(553)1C00 0F33
//576i	1294	548	   1B5C	1244	1299	544		1C00 1255
//480p	1069	552
//576p	1292	548
//1080i	1122	525
//720p	1122	526
//1080p	????	???
//
//BKFYI
//	I saw the measure block can detect the Period value on CVBS & DVI.
//	And, CVBS(SVideo) use 27MKz fixed clock, so I don't need to consider InputMain.
//

//BKFYI:HW default HTOTAL:1024,VTOTAL:512. I don't need a force mode.
//-----------------------------------------------------------------------------
/**
* set Scaler Freerun value
*/
void ScalerSetFreerunValue(BYTE fForce)
{
	WORD HTotal;
	WORD VTotal;
	BYTE ret;

	if(fForce) {
		//scaled NTSC Freerun value
		HTotal = 1085;  
		VTotal = 553;
	}
	else {

		//Before measure, disable an En.Change Detection. and then start a measure.
		//MeasStartMeasure will capture a reference value for "En.Change detection".
		MeasEnableChangedDetection(OFF);

		//call measure once to update the value or use a table value
		ret=MeasStartMeasure();
		if(ret) {
			dPrintf("\nFreerunValue failed!!");
			HTotal = 1085;	//1100;	//1018;	//1107;
			VTotal = 553;	//553;	//542;
		}
		else {
			HTotal = ScalerCalcFreerunHtotal();
			VTotal = ScalerCalcFreerunVtotal();
		}
		//turn on the En.Changed Detection.
		MeasEnableChangedDetection(ON);
	}
	dPrintf("\nFreerunValue(%bd) HTotal:%d VTotal:%d",fForce,HTotal,VTotal);
	ScalerWriteFreerunHtotal(HTotal);
	ScalerWriteFreerunVtotal(VTotal);
}

//-----------------------------------------------------------------------------
/**
* check the freerun value before we go into the freerun mode.
*	
* Hmin = HDE+HWIDTH
*
* Vmin = VDE+VWIdth
*/
void ScalerCheckPanelFreerunValue(void)
{
	WORD Total, Min;
	BYTE changed;

	WriteTW88Page(PAGE2_SCALER );
	changed = 0;

	// Horizontal
	Total = ScalerReadFreerunHtotal();
	Min = ScalerReadOutputWidth();	//H Width
	Min += ScalerReadHDEReg();		//H-DE
	Min += 2;
	if(Total < Min) {
		dPrintf("\nScaler Freerun HTotal %d->%d",Total, Min);
		Total = Min;
		ScalerWriteFreerunHtotal(Total);
		changed++;
	}

	//Vertical
	Total = ScalerReadFreerunVtotal();
	Min = ScalerReadOutputHeight();	//V Width
	Min += ScalerReadVDEReg();		//V-DE
	Min += 2;
	if(Total < Min) {
		dPrintf("\nScaler Freerun VTotal %d->%d",Total, Min);
		Total = Min;
		ScalerWriteFreerunVtotal(Total);
		changed++;
	}

	if(changed) {
		SpiOsdSetDeValue();	//BK111013
		FOsdSetDeValue();
	}
}

