/**
 * @file
 * DECODER.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	Internal Decoder module 
 ******************************************************************************
 */
#include "Config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"
#include "Global.h"

#include "main.h"
#include "Printf.h"
#include "Monitor.h"
#include "I2C.h"
#include "CPU.h"
#include "Scaler.h"
#include "InputCtrl.h"

//-----------------------------------------------------------------------------
/**
* check Video Loss
*
* register
*	R101[0]
*
* oldname: CheckDecoderVDLOSS().
*
* @param n: wait counter
* @return
*	0:Video detected
*	1:Video not present. Video Loss
*/ 
BYTE DecoderCheckVDLOSS( BYTE n )
{
	volatile BYTE	mode;
	BYTE start;

	dPrintf("\nDecoderCheckVDLOSS(%d) start",(WORD)n);
	start = n;

	WriteTW88Page(PAGE1_DECODER );
	while (n--) {
		mode = ReadTW88(REG101);		//read Chip Status
		if (( mode & 0x80 ) == 0 ) {
			dPrintf("->end%bd",start - n);
			return ( 0 );				//check video detect flag
		}
		delay1ms(10);
	}
	ePrintf("\nDecoderCheckVDLOSS->fail");
	return ( 1 );						//fail. We loss the Video
}

#ifdef SUPPORT_FOSD_MENU
//-----------------------------------------------------------------------------
/**
* Is it a video Loss State
*
* @return
*	- 1:If no Input
*	- 0:Found Input
*/
BYTE DecoderIsNoInput(void)
{
	DECLARE_LOCAL_page
	BYTE ret;
	
	ReadTW88Page(page);
	WriteTW88Page(PAGE1_DECODER);
	ret = TW8835_R101;	
	WriteTW88Page(page);
	
	if(ret & 0x80)
		return 1;	//No Input
	return 0;		//found Input
}
#endif

//-----------------------------------------------------------------------------
/**
* set input mux format
*
* register
*	R102 - input format.
*	R105.
*	R106.
* @param InputMode
*/
void InMuxSetInput(BYTE InputMode)
{
	BYTE r102, r105, r106;
	WriteTW88Page(PAGE1_DECODER );

	r105 = ReadTW88(REG105) & 0xF0;
	r106 = ReadTW88(REG106) & ~0x03;	//Do not change Y.

	switch(InputMode) {
	case INPUT_CVBS:
		r102 = 0x40;		// 0x40 - FC27:27MHz, IFSEL:Composite, YSEL:YIN0 
		r105 |= 0x0F;		//decoder mode
		r106 |= 0x03;		// C,V adc in Power Down.
		break;
	case INPUT_SVIDEO:
		r102 = 0x54;		// 0x54	- FC27:27MHz, IFSEL:S-Video, YSEL:YIN1, CSEL:CIN0 
		r105 |= 0x0F;		//decoder mode
		r106 |= 0x01;		// V in PowerDown
		break;
	case INPUT_COMP:	//target r102:4A,r105:04 r016:00
						//     ->     4A      00      00		  

		r102 = 0x4A ;		// 0x4A - 	FC27:27MHz, 
							//		  	IFSEL:Composite, We are using aRGB. So composite is a correct value 
							//			YSEL:YIN2, CSEL:CIN1, VSEL:VIN0
		//r105 |= 0x04;		//??? ? someone overwrite as 00. R105[2]=0b is a correct
		//r106 				//C & V adc in normal(not Power Down)

		break;
	case INPUT_PC:	//target r102:4A r105:04 r106:00
		r102 = 0x4A;		// 0x4A - 	FC27:27MHz, 
							//		  	IFSEL:Composite, We are using aRGB. So composite is a correct value 
							//			YSEL:YIN2, CSEL:CIN1, VSEL:VIN0

		//r105 = 			//RGB mode
							//?? I think R105[2] have to be 0. not 1b.
		//r106 				//C & V adc in normal(not Power Down)
		break;
	case INPUT_DVI:			//target ? don't care
	case INPUT_HDMIPC:
	case INPUT_HDMITV:
	case INPUT_BT656:
		//digital. don't care.
		r102 = 0x00;
		break;
	}
	if(r102) {	//need update?
		WriteTW88(REG102, r102 );
		WriteTW88(REG105, r105 );
		WriteTW88(REG106, r106 );
	}
}

//---------------------------------------------
//description
//	input data format selection
//	if input is PC(aRGB),DVI,HDMI, you have to set.
//parameter
//	0:YCbCr 1:RGB
//
//CVBS:0x40
//SVIDEO:0x54. IFSET:SVIDEO, YSEL:YIN1
#if 0
void DecoderSetPath(BYTE path)
{
	WriteTW88Page(PAGE1_DECODER );	
	WriteTW88(REG102, path );   		
}
#endif

//R104 HSYNC Delay Control


//
//parameter
//	input_mode	0:RGB mode, 1:decoder mode
//register
//	R105
#if 0
void DecoderSetAFE(BYTE input_mode)
{
	WriteTW88Page(PAGE1_DECODER );	
	if(input_mode==0) {
		WriteTW88(REG105, (ReadTW88(REG105) & 0xF0) | 0x04);	//? C is for decoder, not RGB	
	}
	else {
		WriteTW88(REG105, (ReadTW88(REG105) | 0x0F));	
	}
}
#endif

#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
//register
//	R106[2]
//	R106[1]
//	R106[0]
void DecoderPowerDown(BYTE fOn)
{
	WriteTW88Page(PAGE1_DECODER)
	if(fOn) WriteTW88(REG106, ReadTW88(REG106) | 0x07);
	else    WriteTW88(REG106, ReadTW88(REG106) & ~0x07);
}
#endif

//-----------------------------------------------------------------------------
//desc: set/get vertical delay
//@param	
//output
//
//register
//	R107[7:6]R108[7:0]
#ifdef UNCALLED_SEGMENT
void DecoderSetVDelay(WORD delay)
{
	WriteTW88Page(PAGE1_DECODER );		// get VDelay from Decoder
	WriteTW88(REG107, (ReadTW88(REG107 ) & 0x3F) | ( (delay & 0x0300) >> 2)); 
	WriteTW88(REG108, (BYTE)delay );
}
#endif
//-----------------------------------------------------------------------------
/**
* get decoder vertical delay value
*/
WORD DecoderGetVDelay(void)
{
	WORD vDelay;

	WriteTW88Page(PAGE1_DECODER );		// get VDelay from Decoder
	vDelay = ReadTW88(REG107 ) & 0xC0; 
	vDelay <<= 2;
	vDelay |= ReadTW88(REG108 );

	return vDelay;
}

//-----------------------------------------------------------------------------
/**
* set decoder vertical active length
*
*	register
*	R107[5:4]R109[7:0]
*/
void DecoderSetVActive(WORD length)
{
	WriteTW88Page(PAGE1_DECODER );		
	WriteTW88(REG107, (ReadTW88(REG107) & 0xCF) | ( (length & 0x0300) >> 4)); 
	WriteTW88(REG109, (BYTE)length );
}
#ifdef UNCALLED_SEGMENT
//-----------------------------------------------------------------------------
WORD DecoderGetVActive(void)
{
	WORD vActive;

	WriteTW88Page(PAGE1_DECODER );
	vActive = ReadTW88(REG107 ) & 0x30; 
	vActive <<= 4;
	vActive |= ReadTW88(REG109 );

	return vActive;
}
#endif

//-----------------------------------------------------------------------------
//desc:set/get Horizontal delay
//register
//	R107[3:2]R10A[7:0]
#ifdef UNCALLED_SEGMENT
void DecoderSetHDelay(WORD delay)
{
	WriteTW88Page(PAGE1_DECODER );		// get VDelay from Decoder
	WriteTW88(REG107, (ReadTW88(REG107 ) & 0xF3) | ( (delay & 0x0300) >> 6)); 
	WriteTW88(REG10A, (BYTE)delay );
}
WORD DecoderGetHDelay(void)
{
	WORD hDelay;

	WriteTW88Page(PAGE1_DECODER );		// get VDelay from Decoder
	hDelay = ReadTW88(REG107 ) & 0x0C; 
	hDelay <<= 6;
	hDelay |= ReadTW88(REG10A );

	return hDelay;
}
#endif

//-----------------------------------------------------------------------------
//desc: set/get Horizontal active
//register
//	R107[1:0]R10B[7:0]
#ifdef UNCALLED_SEGMENT
void DecoderSetHActive(WORD length)
{
	WriteTW88Page(PAGE1_DECODER );		// get VDelay from Decoder
	WriteTW88(REG107, (ReadTW88(REG107 ) & 0xFC) | ( (length & 0x0300) >> 8)); 
	WriteTW88(REG10B, (BYTE)length );
}
WORD DecoderGetHActive(void)
{
	WORD hActive;

	WriteTW88Page(PAGE1_DECODER );		// get VDelay from Decoder
	hActive = ReadTW88(REG107 ) & 0x03; 
	hActive <<= 8;
	hActive |= ReadTW88(REG10B );

	return hActive;
}
#endif

//-----------------------------------------------------------------------------
/**
* read detected decoder mode
*
*	register
*	R11C[7]		0:idle, 1:detection in progress
*	R11C[6:4]	000: NTSC
*				001: PAL
*				...
*			111:N/A
*/
BYTE DecoderReadDetectedMode(void)
{
	BYTE mode;
	WriteTW88Page(PAGE1_DECODER);
	mode = ReadTW88(REG11C);
	mode >>= 4;
	return mode;
}


#ifdef SUPPORT_FOSD_MENU
//-----------------------------------------------------------------------------
/**
* read video input standard
*
* BKTODO120201 Pls, remove this
*/
BYTE DecoderReadVInputSTD(void)
{
	DECLARE_LOCAL_page
	BYTE std, ret;

	ReadTW88Page(page);
	
	if( DecoderIsNoInput() ) ret = 1; // Noinput!!	BUGBUG


	std = DecoderReadDetectedMode();
	if(std & 0x08) 
		ret = 0xff;	// Detection in progress..
	else
		ret = std + 1;

	WriteTW88Page(page );
	return (ret);
}
#endif

//-----------------------------------------------------------------------------
/**
* check detected decoder video input standard
*
*	To get a stable the correct REG11C[6:4] value,
*		read REG101[6] and REG130[7:5] also.
*	I saw the following values(BK110303)
* 		E7 E7 67 67 87 87 87 87 ..... 87 87 87 87 87 87 87 87 87 07 07 07 .... 
* 		B7 B7 B7 37 37 87 87 87 ..... 87 87 87 87 87 87 87 87 87 07 07 07 07 07 07 07
*
* oldname: CheckDecoderSTD
*
* register
*	R11C[6:4].
* 	R101[6].
*	R130[7:5].
* @return
*	0x80: filed.
*	other: detected standard value.
*/
BYTE DecoderCheckSTD( BYTE n )
{
	volatile BYTE	r11c,r101,r130;
	BYTE start=n;
	BYTE count;
	ePrintf("\nDecoderCheckSTD(%d) start",(WORD)n);
	WriteTW88Page(PAGE1_DECODER );		// set Decoder page
	
	count=0;
	while (n--) {
		r11c = ReadTW88(REG11C);
		if (( r11c & 0x80 ) == 0 ) {
			r101 = ReadTW88(REG101);
			r130 = ReadTW88(REG130);
			dPrintf("\n%02bx:%02bx-%02bx-%02bx ",start-n, r11c, r101,r130);
			if((r101 & 0x40) && ((r130 & 0xE0)==0)) {
				ePrintf("->success:%d",(WORD)start-n);
				if(count > 4)
					return (r11c);
				count++;
			}
 		}
		delay1ms(5);
	}
	ePrintf("->fail");
	return ( 0x80 );
}

//-----------------------------------------------------------------------------
/**
* set decoder freerun mode
*
* example
*   DecoderFreerun(DECODER_FREERUN_60HZ);
*
* R133[7:6]
* @param
*	mode	0:AutoMode
*			1:AutoMode
*			2:60Hz
*			3:50Hz
*/
void DecoderFreerun(BYTE mode)
{
	WriteTW88Page(PAGE1_DECODER );
	WriteTW88(REG133, (ReadTW88(REG133) & 0x3F) | (mode<<6));
}





