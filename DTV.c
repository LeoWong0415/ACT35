/**
 * @file
 * DTV.c 
 * @author Brian Kang
 * @version 1.1
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	DTV for DVI & HDMI
 *
 * history
 *  120803	add MeasStartMeasure before FW use a measured value.
 *			update Freerun Htotal,VTotal at VBlank.
 *			add checkroutine for HDMI detect flag.
*/
#include "Config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"
#include "Global.h"

#include "CPU.h"
#include "Printf.h"
#include "Monitor.h"

#include "I2C.h"

#include "Scaler.h"
#include "InputCtrl.h"
#include "eeprom.h"
#include "vadc.h"
#include "dtv.h"
#include "measure.h"
#include "PC_modes.h"	//for DVI_PrepareInfoString

#ifdef SUPPORT_HDMI_EP9351
#include "HDMI_EP9351.h"
#include "EP9x53RegDef.h"
#endif

#include "DebugMsg.h"


#if !defined(SUPPORT_DVI) && !defined(SUPPORT_HDMI_EP9351) && !defined(SUPPORT_HDMI_SiIRX)
//----------------------------
/**
* Trick for Bank Code Segment
*/
//----------------------------
CODE BYTE DUMMY_DTV_CODE;
void Dummy_DTV_func(void)
{
	BYTE temp;
	temp = DUMMY_DTV_CODE;
}
#endif

						

#if defined(SUPPORT_DVI) || defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
/**
* set DTV Polarity
*
*	register
*	R050[2]	HPol	1:AvtiveLow
*	R050[1]	VPol	1:ActiveLow
* @param	HPol	1:ActiveLow
* @param	VPol	1:ActiveLow
*/
void DtvSetPolarity(BYTE HPol, BYTE VPol)
{
	BYTE value;

	WriteTW88Page(PAGE0_DTV);
	value = ReadTW88(REG050) & 0xF9;
	if(HPol)	value |= 0x04;	//H Active Low		
	if(VPol)	value |= 0x02;	//V Active Low		
	WriteTW88(REG050, value);	
	
	//dPrintf("\nDTV Pol H:%bd V:%bd",HPol,VPol);	
}
#endif

#if defined(SUPPORT_DVI)  || defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
/**
* set DTV clock delay
*
*	register
*	R051[2:0]
*/
void DtvSetClockDelay(BYTE delay)
{
	WriteTW88Page(PAGE0_DTV);
	WriteTW88(REG051, (ReadTW88(REG051) & 0xF8) | delay);
}
#endif

#if defined(SUPPORT_DVI)  || defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX) || defined(SUPPORT_BT656)
/**
* set DTV DataRouting and InputFormat
*
*	register
*	R052[2:0]	DataRouting:100 Y(R):Pb(G):Pr(B)
*	R053[3:0]	InputFormat:1000=RGB565
*
* NOTE:R and B is reversed on FPGA.
* @param 	route:	Data bus routing selection for DTV
* @param	format
*/
void DtvSetRouteFormat(BYTE route, BYTE format)
{
	WriteTW88Page(PAGE0_DTV);
	WriteTW88(REG052, (ReadTW88(REG052) & 0xF8) | route);
	WriteTW88(REG053, (ReadTW88(REG053) & 0xF0) | format);
}
#endif

#if defined(SUPPORT_DVI) || defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
/**
* set DTV Field Detection Register
* param
*	register
*	R054[7:4]	End Location
*	R054[3:0]	Start Location
* example:
*	DtvSetFieldDetectionRegion(0x11)
*/
void DtvSetFieldDetectionRegion(BYTE fOn, BYTE r054)
{
	WriteTW88Page(PAGE0_DTV );
	if(fOn) {
		WriteTW88(REG050, ReadTW88(REG050) | 0x80 );	// set Det field by WIN
		WriteTW88(REG054, r054 );						// set window
	}
	else {
		WriteTW88(REG050, ReadTW88(REG050) & ~0x80 );	// use VSync/HSync Pulse
	}
}
#endif

#if defined(SUPPORT_DVI) || defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
/**
* set DTV VSyncDelay value
*/
void DtvSetVSyncDelay(BYTE value)
{
	WriteTW88Page(PAGE0_DTV );
	WriteTW88(REG056, value);
}
#endif


//=============================================================================
// DVI
//=============================================================================

#if defined(SUPPORT_DVI)
//-----------------------------------------------------------------------------
//		void 	DVISetInputCrop( void )
//-----------------------------------------------------------------------------
/**
* set InputCrop for DVI
*
* extern
*	MeasHPulse ->Removed
*	MeasVPulse ->Removed
*	MeasVStart
*/
static void DVISetInputCrop( void )
{
	BYTE	offset, VPulse, HPulse;
	WORD	hstart, vstart, vtotal, hActive;
	BYTE HPol, VPol;
	WORD Meas_HPulse,Meas_VPulse;

	Meas_HPulse = MeasGetHSyncRiseToFallWidth();
	Meas_VPulse = MeasGetVSyncRiseToFallWidth();
	vtotal = MeasGetVPeriod();
	hActive = MeasGetHActive( &hstart );

#ifdef DEBUG_DTV
	dPuts("\nMeas");
	dPrintf("\n\tH           Pulse:%4d BPorch:%3d Active:%4d hAvtive:%d",Meas_HPulse,hstart,MeasHLen,hActive);
	dPrintf("\n\tV Total:%4d Pulse:%4d BPorch:%3d Active:%4d",vtotal,Meas_VPulse,MeasVStart,MeasVLen);
#endif

	offset = 5;  //meas delay value:4
	//hstart = MeasHStart + offset;
	hstart += offset;
	if ( Meas_HPulse > (hActive/2) ) {
		if(hActive > Meas_HPulse)
			HPulse = hActive - Meas_HPulse;
		else
			HPulse = Meas_HPulse - hActive;
		HPol = 0;	
	}
	else  {
		HPulse = Meas_HPulse;
		HPol = 1;
		hstart -= HPulse;	// correct position
	}

	if ( Meas_VPulse > (vtotal/2) ) {
		VPulse = vtotal - Meas_VPulse;
		VPol = 0;
	}
	else  {
		VPulse = Meas_VPulse;
		VPol = 1;
	}
	vstart = MeasVStart + VPulse;

	DtvSetPolarity(HPol,VPol);

#ifdef DEBUG_DTV
	dPuts("\nmodified");
	dPrintf("\n\tH           Pulse:%2bd BPorch:%3d Active:%4d Pol:%bd hActive:%4d ",HPulse,hstart,MeasHLen,HPol, hActive);
	dPrintf("\n\tV Total:%4d Pulse:%2bd BPorch:%3d Active:%4d Pol:%bd",vtotal,VPulse,vstart,MeasVLen,VPol);
#endif
	//BKFYI. The calculated method have to use "InputSetCrop(hstart, vstart, MeasHLen, MeasVLen);"
	//		 But, we using a big VLen value to extend the vblank area.
	InputSetCrop(hstart, 1, MeasHLen, 0x7fe);
}

/**
* set Output for DVI
*/
static void DVISetOutput( void )
{
	BYTE	HDE;
	WORD temp16;

	ScalerSetHScale(MeasHLen);
	ScalerSetVScale(MeasVLen);

	//=============HDE=====================
	HDE = ScalerCalcHDE();
#ifdef DEBUG_DTV
	dPrintf("\n\tH-DE start = %bd", HDE);
#endif
	ScalerWriteHDEReg(HDE);


	//=============VDE=====================
	// 	MeasVStart ??R536:R537
	//	MeasVPulse ??R52A,R52B
	temp16 = ScalerCalcVDE();
#ifdef DEBUG_DTV
	dPrintf("\n\tV-DE start = %d", temp16);
#endif
	ScalerWriteVDEReg((BYTE)temp16);

	//=================  Free Run settings ===================================
	temp16=ScalerCalcFreerunHtotal();
#ifdef DEBUG_DTV
	dPrintf("\n\tFree Run Htotal: 0x%x", temp16);
#endif
	ScalerWriteFreerunHtotal(temp16);

	//================= V Free Run settings ===================================
	temp16=ScalerCalcFreerunVtotal();
#ifdef DEBUG_DTV
	dPrintf("\n\tFree Run Vtotal: 0x%x", temp16);
#endif
	ScalerWriteFreerunVtotal(temp16);

	//================= FreerunAutoManual, MuteAutoManual =====================
	ScalerSetFreerunAutoManual(ON,OFF);
	ScalerSetMuteAutoManual(ON,OFF);
}

/**
* Check and Set DVI
*
* extern
*	MeasVStart,MeasVLen,MeasHLen	
* @return 
*	-0:ERR_SUCCESS
*	-1:ERR_FAIL
*/
BYTE CheckAndSetDVI( void )
{
	DECLARE_LOCAL_page
	WORD	Meas_HStart;
	WORD	MeasVLenDebug, MeasHLenDebug;
	WORD    MeasVStartDebug, MeasHStartDebug;

	ReadTW88Page(page);
	//DtvSetPolarity(0,0);

	do {														
		if(MeasStartMeasure()) {
			WriteTW88Page(page );
			return ERR_FAIL;
		}
		MeasVLen = MeasGetVActive( &MeasVStart );				//v_active_start v_active_perios
		MeasHLen = MeasGetHActive( &Meas_HStart );				//h_active_start h_active_perios
#ifdef DEBUG_DTV
		dPrintf("\nDVI Measure Value: %dx%d HS:%d VS:%d",MeasHLen,MeasVLen, Meas_HStart, MeasVStart);
		dPrintf("==>Htotal:%d",  MeasGetVsyncRisePos());
#endif

#ifdef MODEL_TW8835FPGA___________NEED_VERIFY
	if(mode==5)						SetExtVAdcI2C(0x9A, 0);		//VGA@60
	else if(mode==10)				SetExtVAdcI2C(0x9A, 1);		//SVGA@60
	else if(mode==18)				SetExtVAdcI2C(0x9A, 2);		//XGA@60
	else if(mode==43 || mode==44)	SetExtVAdcI2C(0x9A, 3);		//SXGA+ 1400x1050@60
	else if(mode==51)				SetExtVAdcI2C(0x9A, 6);		//480P
	else if(mode==52)				SetExtVAdcI2C(0x9A, 7);		//576P
	else if(mode==53)				SetExtVAdcI2C(0x9A, 8);		//1080I
	else if(mode==54)				SetExtVAdcI2C(0x9A, 9);		//720P
	else if(mode==55)				SetExtVAdcI2C(0x9A, 10);	//1080P
#endif


		DVISetInputCrop();
		DVISetOutput();

		MeasVLenDebug = MeasGetVActive( &MeasVStartDebug );		//v_active_start v_active_perios
		MeasHLenDebug = MeasGetHActive( &MeasHStartDebug );		//h_active_start h_active_perios

	} while (( MeasVLenDebug != MeasVLen ) || ( MeasHLenDebug != MeasHLen )) ;

	
	AdjustPixelClk(0, 0);	//NOTE:it uses DVI_Divider.

	//prepare info
	DVI_PrepareInfoString(MeasHLen,MeasVLen,MeasGetVFreq() /*0*/ /*freq*/);

	WriteTW88Page(page );

	//for debug. check the measure value again
	CheckMeasure();

	return ERR_SUCCESS;
}
#endif	//..SUPPORT_DVI

//=============================================================================
// HDMI
//=============================================================================

#if defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
/**
* Set Output for HDMI
*/
static void	HDMISetOutput(WORD HActive, WORD VActive, BYTE	vde )
{
	DECLARE_LOCAL_page
	BYTE	HDE;
	WORD HTotal, VTotal;
	WORD VScale;
	DWORD dTemp;

	ReadTW88Page(page);

	ScalerSetHScale(HActive);
	ScalerSetVScale(VActive);

	//=============HDE=====================
	HDE = ScalerCalcHDE();
#ifdef DEBUG_DTV
	dPrintf("\n\tH-DE start = %bd", HDE);
#endif


	//=============VDE=====================
	// 	MeasVStart ??R536:R537
	//	MeasVPulse ??R52A,R52B
	
//	temp16 = ScalerCalcVDE();
//	dPrintf("\n\tV-DE start = %d", temp16);

	VScale = ScalerReadVScaleReg();

#ifdef DEBUG_DTV
	dPrintf("\n\tV-DE start = %bd", vde);
#endif
	dTemp = vde;
	dTemp = (dTemp * 8192L) / VScale;
	dTemp++;
	vde = dTemp;
#ifdef DEBUG_DTV
	dPrintf("=> %bd", vde);
#endif


	//-------------------------------
	//BK120803. Did you run the measure ?
	//If you did not, you can not get the correct calculated VTotal value.

	//=================  Free Run settings ===================================
	HTotal=ScalerCalcFreerunHtotal();
#ifdef DEBUG_DTV
	dPrintf("\n\tFree Run Htotal: 0x%x", HTotal);
#endif

	//================= V Free Run settings ===================================
	VTotal=ScalerCalcFreerunVtotal();
#ifdef DEBUG_DTV
	dPrintf("\n\tFree Run Vtotal: 0x%x", VTotal);
#endif

	//---------------------UPDATE-----------------------------
//delay1s(1,__LINE__);
	//BK120803
	// If FW update HDE Start and VDE Start, we can see the flick on the logo image.
	// Remove logo here and draw an video image will remove a logo blink.
	WaitVBlank(1);
	WriteTW88Page(PAGE2_SCALER );							//Trick.
	WriteTW88(REG400, ReadTW88(REG400) & ~0x04);			//disable SpiOSD
	//ScalerWriteHDEReg(HDE);
	//ScalerWriteVDEReg(vde);
	//ScalerWriteFreerunHtotal(HTotal);
	//ScalerWriteFreerunVtotal(VTotal);
	WriteTW88Page(PAGE2_SCALER );
	WriteTW88(REG210, HDE);
	WriteTW88(REG215, vde);
	WriteTW88(REG21C, (ReadTW88(REG21C)&0x0F)|(HTotal>>4)&0xF0);
	WriteTW88(REG21D, (BYTE)HTotal );
	WriteTW88(REG20D, (ReadTW88(REG20D)&0x3F)|(VTotal>>2)&0xC0);
	WriteTW88(REG219, (BYTE)VTotal );


	//================= FreerunAutoManual, MuteAutoManual =====================
	ScalerSetFreerunAutoManual(ON,OFF);
	ScalerSetMuteAutoManual(ON,OFF);

	WriteTW88Page(page);
}

#ifdef SUPPORT_HDMI_EP9351
/**
* check AVI InfoFrame
* 
* call when $3D[7:6] == 11b
*           $3C[4] == 1
* check $29[0] first.
*/
BYTE CheckAviInfoFrame(void)
{
	BYTE TempByte[15];
	BYTE bTemp;
	BYTE result;
	//------------------------------------
	//check AVI InfoFrame

	bTemp = ReadI2CByte(I2CID_EP9351,EP9351_HDMI_INT);			//$29

#ifdef DEBUG_DTV
	Printf("\nCheckAviInfoFrame $29:%02bx", bTemp);
#endif
	if((bTemp & 0x01) == 0) {
#ifdef DEBUG_DTV
		Puts(" FAIL");
#endif
		return ERR_FAIL;
	}

	//---------------------
	// found AVI InfoFrame.
	//---------------------
	
	//read AVI InfoFrame from $2A
	ReadI2C(I2CID_EP9351, EP9351_AVI_InfoFrame, TempByte, 15);
	DBG_PrintAviInfoFrame();

	//---------------------
	//color convert to RGB
	//---------------------
	//	 Y [2][6:5]	   Input HDMI format
	//			0:RGB
	//			1:YUV(422)
	//			2:YUV(444)
	//			3:Unused
	bTemp = (TempByte[2] & 0x60) >> 5;
//#ifdef DEBUG_DTV
//	Puts("\nInput HDMI format ");
//	if(bTemp == 0) 		Puts("RGB");
//	else if (bTemp==1)  Puts("YUV(422)");
//	else if (bTemp==2)  Puts("YUV(444)");
//	else				Puts("unknown");
//#endif
	result = 0;
	if (bTemp==1)  		result = 0x50;
	else if (bTemp==2)  result = 0x10;

	// TempByte3[7:6] Colorimetry
	bTemp = (TempByte[3] & 0xc0)>>6;
	if(bTemp==2) result |= 0x04;	//BT.709
	//else if(bTemp==3) {
	//	//Extended Colorimetry Info
	//	i = TempByte[4]&0x70)>>4;
	//	...
	//}

	//	TempByte6[3:0] Pixel Repetition Factor
	//bTemp = TempByte[6] & 0x0F;
	//if(bTemp > 3) i = 0x03;	//EP9351 supports only 2 bits.
	//result |= bTemp;

//BK120731 test
//		TempBit03 =	(!TempBit04 && !(pEP9351C_Registers->Video_Status[0] & EP907M_Video_Status_0__VIN_FMT_Full_Range) || // Input RGB LR
//			 		 !TempBit05 && !(pEP9351C_Registers->Output_Format_Control & EP907M_Output_Format_Control__VOUT_FMT_Full_Range) );	// or Output RGB LR
//	result |= 0x08; //full range

	WriteI2CByte(I2CID_EP9351, EP9351_General_Control_2, result);

	return ERR_SUCCESS;
}
#endif

/**
* Check and Set HDMI
*
* Hot Boot: Reset only TW8835.
*		Hot boot needs a EP9351 Software Reset, but FW does not support it anymore.
*		Please, use a Reset button or Power Switch.
*/
BYTE CheckAndSetHDMI(void)
{
	DECLARE_LOCAL_page
	WORD HActiveStart;
	WORD VDEStart;
	BYTE ret;

	WORD HSync,HBPorch,HActive,HFPorch;
	WORD VActive, VFPorch;
#ifdef SUPPORT_HDMI_SiIRX
	BYTE VSyncWidthBPorch;
	BYTE Interlaced;
	WORD meas_active;
	WORD temp16;		
#else
	WORD VSync,VBPorch;

	//WORD ii;
	BYTE i;
	//volatile BYTE vTemp;
	BYTE TempByte[15];
	volatile BYTE bTemp;
#endif

	BYTE Status;
	BYTE HPol,VPol;		
	WORD HTotal,VTotal;


	dPuts("\nCheckAndSetHDMI START");
	ReadTW88Page(page);

#ifdef SUPPORT_HDMI_EP9351
	//FW turns off the power down mode. So check $3D[7:6] first.
	bTemp = ReadI2CByte(I2CID_EP9351, EP9351_Status_Register_1 );
	dPrintf("\n$3D:%bx",bTemp);
	if((bTemp & 0xC0) != 0xC0) {
		Printf(" => NoSignal");
		WriteTW88Page(page);
		return ERR_FAIL;
	}
	//BKTODO120803. We need a delay. TW8835 too fast.
	for(i=0; i < 10; i++) {
		bTemp = ReadI2CByte(I2CID_EP9351, EP9351_Status_Register_0 );
		delay1ms(10);
		if(bTemp & 0x10)
			break;
	}
	dPrintf(" $3C:%bx @%bx",bTemp, i);


	if(bTemp & 0x10) {
		Puts(" HDMI mode");
		ret = CheckAviInfoFrame();
	}
	else {
		Puts(" DVI mode");
		WriteI2CByte(I2CID_EP9351, EP9351_General_Control_2, 0x00 ); 	//clear
		//Previous FW uses $49=0x09. now FW uses 0xB1
		//!//bTemp = ReadI2CByte(I2CID_EP9351,EP9351_General_Control_3);
		//!//WriteI2CByte(I2CID_EP9351,EP9351_General_Control_3, bTemp | 0x04);
		//!WriteI2CByte(I2CID_EP9351,EP9351_General_Control_9, 0x09);
		//BT709 && Full Range
	}

	//BK120730
	//do not update $41[7:6] the polarity value. FW needs a correct polarity value.
	//Old FW updated $41[7:6] and tred to use a Positive value.
	//bTemp = ReadI2CByte(I2CID_EP9351,EP9351_General_Control_1);
	//Printf("\n$41:%02bx",bTemp);
#endif

	//-----------------------------
	// read timing register value
	//-----------------------------
#if 0 //check point
#ifdef SUPPORT_HDMI_EP9351
	//FW checked $3D[7:6], so we don't need this routine anymore.
	//FW checked the HActive(HDE width) and VActive(VDE width) value only when it is a DVI.

	//CheckAviInfoFrame uses 1 sec, I will use 3.5 sec. I saw it needs 2.97sec
	for(ii=0; ii < 350; ii++) {
		ReadI2C(I2CID_EP9351, EP9351_Timing_Registers, TempByte, 13);
		HActive = TempByte[1]; 	HActive <<= 8;		HActive += TempByte[0];
		VActive = TempByte[9]; 	VActive <<= 8;		VActive += TempByte[8];
		if((HActive > 700) && (VActive > 200)) {
			Printf("\ntime_reg check success @:%d",ii);
			break;
		}
		delay1ms(10);
	}
	if(ii==350)
		Printf("\ntime_reg check FAIL");
#endif
#endif

#ifdef SUPPORT_HDMI_EP9351
	//-----------------------------
	// read timing register value, $3B
	ReadI2C(I2CID_EP9351, EP9351_Timing_Registers, TempByte, 13);
	Status = ReadI2CByte(I2CID_EP9351,EP9351_General_Control_1);  /* NOTE: $41 */

	HActive = TempByte[1]; 	HActive <<= 8;		HActive += TempByte[0];
	HFPorch = TempByte[3]; 	HFPorch <<= 8;		HFPorch += TempByte[2];
	HBPorch = TempByte[5]; 	HBPorch <<= 8;		HBPorch += TempByte[4];
	HSync   = TempByte[7]; 	HSync <<= 8;		HSync   += TempByte[6];
	VActive = TempByte[9]; 	VActive <<= 8;		VActive += TempByte[8];
	VFPorch =  TempByte[10];
	VBPorch =  TempByte[11];
	VSync =  TempByte[12]&0x7F;

	HTotal = HSync + HBPorch + HActive + HFPorch;
	VTotal = VSync + VBPorch + VActive + VFPorch;

	HPol = Status & 0x40 ? 1:0;	//ActiveLow
	VPol = Status & 0x80 ? 1:0;	//ActiveLow


#ifdef DEBUG_DTV
	DBG_PrintTimingRegister();	//HDMI_DumpTimingRegister(TempByte);
	//dPrintf("\nTimeReg $41:%02bx",Status);
	//dPrintf("\n\tH Total:%4d Pulse:%4d BPorch:%4d Active:%4d FPorch:%3d Pol:%bd",
	//	HTotal,HSync,HBPorch,HActive,HFPorch,HPol);
	//dPrintf("\n\tV Total:%4d Pulse:%4d BPorch:%4d Active:%4d FPorch:%3d Pol:%bd",
	//	VTotal,VSync,VBPorch,VActive,VFPorch,VPol);
	//dPrintf("\n\t%s", TempByte[12] & 0x80 ? "Interlace" : "Progressive");
#endif

#if 0 //check point
	if(VFPorch > VActive) {
		//If the connector was not good, sometimes it has a wrong value.
		//I have been read the time register after 1sec, but it was not solve this issue.
		//I saw Sony DVD player makes a garbage VSync.
		//          
		Puts(" WRONG!!");
	}
	if((HActive < 700) || (VActive < 200)) {
		Puts(" WRONG!!");
	}
#endif

	ret = ERR_SUCCESS;
#endif

#ifdef SUPPORT_HDMI_SiIRX
	HTotal   = ReadI2CByte(I2CID_SIL9127_DEV0,0x3B) & 0x1F; HTotal  <<= 8;  HTotal  |= ReadI2CByte(I2CID_SIL9127_DEV0,0x3A);	//Video H Resoultion 	0x3B[4:0]0x3A[7:0]
	VTotal   = ReadI2CByte(I2CID_SIL9127_DEV0,0x3D) & 0x07; VTotal  <<= 8;  VTotal  |= ReadI2CByte(I2CID_SIL9127_DEV0,0x3C);	//Video V Refresh   	0x3D[2:0]0x3C[7:0]
	HActive  = ReadI2CByte(I2CID_SIL9127_DEV0,0x4F) & 0x0F; HActive <<= 8;  HActive |= ReadI2CByte(I2CID_SIL9127_DEV0,0x4E);	//Video DE Pixel 		0x4F[3:0]0x4E[7:0]
	VActive  = ReadI2CByte(I2CID_SIL9127_DEV0,0x51) & 0x07; VActive <<= 8;  VActive |= ReadI2CByte(I2CID_SIL9127_DEV0,0x50);	//Video DE Line 		0x51[2:0]0x50[7:0]
	VSyncWidthBPorch  = ReadI2CByte(I2CID_SIL9127_DEV0,0x52) & 0x3F;                                    //Video VSYNC to Active Video Lines		0x52[5:0]
	VFPorch  = ReadI2CByte(I2CID_SIL9127_DEV0,0x53) & 0x3F;												//Video Vertical Front Porch  0x53[5:0]
	Status   = ReadI2CByte(I2CID_SIL9127_DEV0,0x55) & 0x07;		//Video Status [2]:Interlace [1]:VPol. Positive. [0]:HPol.Positive
	HFPorch  = ReadI2CByte(I2CID_SIL9127_DEV0,0x5A) & 0x03; HFPorch <<= 8;  HFPorch |= ReadI2CByte(I2CID_SIL9127_DEV0,0x59);	//Video Horizontal Front Porch 		0x5A[1:0]0x59[7:0]
	HSync    = ReadI2CByte(I2CID_SIL9127_DEV0,0x5C) & 0x03; HSync   <<= 8;    HSync |= ReadI2CByte(I2CID_SIL9127_DEV0,0x5B);	//Video Horizontal Front Porch 		0x5C[1:0]0x5B[7:0]											

	HPol = Status & 0x01;			//ActiveHigh
	VPol = Status & 0x02 ? 1:0;		//ActiveHigh
	Interlaced = Status & 0x04 ? 1 : 0;

	HBPorch = HTotal - HActive - HFPorch - HSync; //base EndOfHSync

#ifdef DEBUG_DTV
	dPrintf("\nTimeReg: Interlaced:%bd",Interlaced);
	dPrintf("\n\tH Total:%4d Pulse:%4d BPorch:%3d Active:%4d FPorch:%02bd Pol:%bd",
		HTotal,HSync,HBPorch,HActive,HFPorch,HPol);
	dPrintf("\n\tV Total:%4d Pulse+BPorch:%3bd Active:%4d FPorch:%d Pol:%bd",
		VTotal,VSyncWidthBPorch,VActive,VFPorch,VFPorch,VPol);
#endif

	//VBPorch = VTotal = VActive - HSyncWidth 		   
	//It can not calculate VBPorch because it does not have VSyncWidth.
	//We have to use VSyncWidthBPorch value..
	if((HTotal == 0) || (VTotal == 0)) {
		WriteTW88Page(page);
		Puts("\nCheckAndSetHDMI FAIL");
		return ERR_FAIL;
	}
#ifdef DEBUG_DTV
	Printf("\nCheckAndSetHDMI FOUND %dx%d",HActive,	VActive);
	PrintSystemClockMsg("CheckAndSetHDMI");
#endif

	ret = ERR_SUCCESS;
#endif

	//-----------------------------------
	//set DTV polarity, HActiveStart, VDE
	//-----------------------------------
	//TW8835 prefer HPorch & VPorch. 
	// Use reversed value(if input ActiveHigh, use ActiveLow to remove SynchWidth)
	//-----------------------------------		
#ifdef SUPPORT_HDMI_SiIRX
	//But, HDMI_9XXXA donot have VSyncWidth, we have to use VSyncWidth+VBackPorch value.
	//H:reverse,V:Normal
	DtvSetPolarity(HPol ? 1:0, VPol ? 0:1);	//base ActiveHigh		
	HActiveStart = HBPorch + 1;
	VDEStart = 	VSyncWidthBPorch +2;
#else
	DtvSetPolarity(HPol ? 0:1, VPol ? 0:1);	//base ActiveLow		
	HActiveStart = HBPorch + 1;
	//------------------------------------
	//Do not update $41[6] register, we can not know it's real polarity.
	//If someone updates $41[6], we have to add HSync width here.
	//DTV perfer a short value.
	//------------------------------------
	//if(HPol)
	//	HActiveStart += HSync;
	//------------------------------------
	VDEStart = 	VBPorch +2;
#endif
#ifdef DEBUG_DTV
	dPrintf("\nBASE HActiveStart:%d VDEStart:%d",HActiveStart,VDEStart);
#endif

	//BK120803. HDMISetOutput() needs a MeasStartMeasure().
	MeasStartMeasure();


#ifdef SUPPORT_HDMI_SiIRX
	//------------------------<<?PixelRepeat
	//if source is 1440x240, SiI9127 reports it as 720x240.
	//we have to measure it and correct it.
	if(HActive==720 && VActive==240) {
		//if(MeasStartMeasure()==0) 
		{
			//temp16 = MeasGetVActive2();
			meas_active = MeasGetHActive(&temp16);
			if(meas_active != HActive) {
				Printf("\nMeas update HActive %d->%d",HActive,meas_active);
				HActive = meas_active;
			}	
		}
		//or. if not interlaced, use 1440x240.
	}
#endif
	InputSetCrop(HActiveStart, 1, HActive, 0x7fe);

	HDMISetOutput( HActive,VActive,  VDEStart );

	AdjustPixelClk(HTotal, 0);	//NOTE:it uses DVI_Divider.

	WriteTW88Page(page);

	return ret ; //ERR_SUCCESS;
}
#endif //..defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)


