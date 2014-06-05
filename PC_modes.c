/**
 * @file
 * PC_modes.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	Video timimg table
*/

#include "Config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "global.h"
#include "printf.h"
#include "util.h"

#include "PC_modes.h"
#include "InputCtrl.h"
#include "SOsd.h"
#include "FOSD.h"

//BKTODO: 
//	Prepare WVGA panel.
//	Add DTV input mode


#if /*defined( SUPPORT_COMPONENT ) || */ defined( SUPPORT_PC ) || defined (SUPPORT_DVI)
//CONST struct _PCMODEDATA PCMDATA[] = {
code struct _PCMODEDATA PCMDATA[] = {
//===========================================================================================================
//                       PC mode Table for XGA Panel    17-July, 2002
//===========================================================================================================
// Support
//	0:NotSupport. 1:PC 2:Component 3:DTV
// han		horizontal addressable size, resolution
// van		vertical addressable size, resolution
// vfreq	vertical frequency
// htotal	horizontal total pixels. horizontal period use it to set PLL with (htotal-1)
// vtotal	vertical total lines. vertical period
// hsyncpol we don't need it.
// vsyncpol we don't need it.
// hstart	horizontal addressable start(not active start). It will be HsyncWidth+HBackPorch+HLeftBorder.
// vstart	vertical addressable start(not active start). It will be VsyncWidth+VBackPorch+VTopBorder.
// offseth	Offset from hstart for scaler hstart input
// offsetv  Offset from vstart for scaler vstart input
// dummy0 	- for VAN adjust. removed
// dummy1	- for VDE. removed
// IHF      - input HFreq based 100Hz
// IPF		- input pixel clock based 10KHz


// HS = HSYNC Polarity, VS = VSYNC Polarity, 0 = Negative, 1 = Positive
//
//																					Dummy1=VScaleOffset		
// 	                                                            Offset 		Dummy0 	Dummy1 	Dummy2 	Dummy3

// 	SUPPORT  HAN,VAN,VFREQ  	H&V TOTAL  HSP &VSP	Hst,Vst		OffsetH V 	Dummy0 	Dummy1 	Dummy2 	Dummy3
//  	             IVF     	CLOCK+1             HST,VST     				


/* 00*/	{  0,	0,	0,	0, 		0,	0,		0,0,	0,	0,		0,	0,		0,		0,		0,		0 },		// unknown

/* 01*/	{  0,	720,400,70,		900,0xbad,	0,0,	283,780,	10,	2,		0,		0,		0,		0 },		//  0: DOS mode
             
/* 02*/	{  0,	640,350,85,		832,445,	1,0,	96,	60,		0,	0,		0,		0,		0,		0 },		// 640x350x85
/* 03*/	{  0,	640,400,85,		832,445,	0,1,	96,	41,		0,	0,		0,		0,		0,		0 },		// 640x400x85
/* 04*/	{  0,	720,400,85,		936,446,	0,1,	108,42,		0,	0,		0,		0,		0,		0 },		// 720x400x85

#ifdef MODEL_TW8835FPGA
//		{  1,	640,480,60,		800,525,	0,0,	141,	36,		5,		1,		1,		2,		0,		0 },		// 640x480x60
/*05*/	{  1,	640,480,60,		800,525,	0,0,	144-1,	35+1,	5,		1,		1,		2,		315,	0 },		// 640x480x60
/*06*/	{  1,	640,480,72,		832,520,	0,0,	166,	32,		5,		1,		1,		2,		377,	0 },		// 640x480x72
#else
/*>05*/	{  1,	640,480,60,		800,525,	0,0,	40,25,		5,-3,		1,		2,		315,	0 },		// 640x480x60
/* 06*/	{  1,	640,480,72,		832,520,	0,0,	157,32,		5,1,		1,		2,		377,	0 },		// 640x480x72
#endif
/* 07*/	{  1,	640,480,75,		840,500,	0,0,	171,20,		5,-8,		4,		6,		375,	0 },		// 640x480x75
/* 08*/	{  0,	640,480,85,		832,509,	0,0,	127,29,		5,-6,		3,		4,		433,	0 },		// 640x480x85

/* 09*/	{  1,	800,600,56,		1024,625,	1,1,	128,22,		5,	-2,		1,		1,		351,	0 },		// 800x600x56
#ifdef MODEL_TW8835FPGA
/*10*/	{  1,	800,600,60,		1056,628,	1,1,	216-1, 	27+1,	4+1,	3-1,	1,		2,		379,	0 },		// 800x600x60
#else
/*>10*/	{  1,	800,600,60,		1056,628,	1,1,	88,	23,		5,	-4,		1,		2,		0,		0 },		// 800x600x60
#endif
/* 11*/	{  1,	800,600,70,		1040,625,	1,0,	111,25,		5,	-5,		1,		3,		0,		0 },		// 800x600x70
/* 12*/	{  1,	800,600,72,		1040,666,	1,1,	64,	23,		5,	-6,		1,		2,		481,	0 },		// 800x600x72
/* 13*/	{  1,	800,600,75,		1056,625,	1,1,	160,21,		5,	-3,		1,		1,		469,	0 },		// 800x600x75
/* 14*/	{  0,	800,600,85,		1048,631,	1,1,	152,27,		5,	-3,		1,		1,		537,	0 },		// 800x600x85
/* 15*/	{  0,	800,600,120,	960,636,	1,0,	80,	29,		0,	0,		0,		0,		0,		0 },		// 800x600x120	REDUCED

/* 16*/	{  1,	848,480,60,		1088,517,	1,1,	112,23,		0,	0,		0,		0,		0,		0 },		// 848x480x60

/* 17*/	{  1,	960,600,60,		1232,622,	1,0,	120,23,		5,	-6,		0,		0,		0,		0 },		// 800x600x60

#ifdef MODEL_TW8835FPGA
//		{  1,	1024,768,60,	1344,806,	0,0,	294, 	36,		5,		1,		2,		1,		0,		0 },		// 1024x768x60
/*18*/	{  1,	1024,768,60,	1344,806,	0,0,	296-2, 	35+1,	5,		1,		2,		1,		484,	0 },		// 1024x768x60
#else
/*>18*/	{  1,	1024,768,60,	1344,806,	0,0,	160,29,		4,	-6,		2,		1,		0,		0 },		// 1024x768x60
#endif
/*19*/	{  1,	1024,768,70,	1328,806,	0,0,	144,29,		5,	-9,		4,		3,		565,	0 },		// 1024x768x70
/*20*/	{  1,	1024,768,75,	1312,800,	1,1,	176,28,		5,	-7,		4,		3,		600,	0 },		// 1024x768x75
/*21*/	{  0,	1024,768,85,	1376,808,	0,0,	208,36,		0,	0,		0,		0,		683,	0 },		// 1024x768x85
/*22*/	{  0,	1024,768,120,	1184,813,	1,0,	80, 38,		0,	0,		0,		0,		0,		0 },		// 1024x768x120 REDUCED
                                             
/*23*/	{  1,	1152,864,75,	1600,900,	1,1,	256,32,		5,	-6,		0,		0,		0,		0 },		// 1152x864x75

/*24*/	{  1,	1280,720,60, 	1664,746,	1,0,	192,26,		5,	-7,  	0,		0,		0,		0 },		// 1280x720x60		 // 

/*25*/	{  1,	1280,768,60, 	1440,790,	1,0,	80, 12,		5,	-8,  	2,		2,		0,		0 },		// 1280x768x60	REDUCED
/*26*/	{  1,	1280,768,60, 	1664,798,	0,1,	192,20,		5,	-10,  	0,		0,		0,		0 },		// 1280x768x60		 // It cannot display !!!!
/*27*/	{  1,	1280,768,60, 	1688,802,	0,1,	232,34,		5,	-7,  	0,		0,		0,		0 },		// 1280x768x60		 // Windows7 mode
/*28*/	{  1,	1280,768,75, 	1696,805,	0,1,	208,27,		5,	-10, 	0,		0,		0,		0 },		// 1280x768x75
/*29*/	{  0,	1280,768,85, 	1712,809,	0,1,	216,31,		0,	0,  	0,		0,		0,		0 },		// 1280x768x85
/*30*/	{  0,	1280,768,120, 	1440,813,	1,0,	80, 35,		0,	0,  	0,		0,		0,		0 },		// 1280x768x120	REDUCED

/*31*/	{  1,	1280,800,60, 	1440,823,	1,0,	80, 14,		5,	-8,		0,		0,		0,		0 },		// 1280x800x60	REDUCED
/*32*/	{  1,	1280,800,60, 	1680,831,	0,1,	200,22,		5,	-7,		0,		0,		0,		0 },		// 1280x800x60
/*33*/	{  1,	1280,800,75, 	1696,838,	0,1,	208,29,		5,	-7,		0,		0,		0,		0 },		// 1280x800x75
/*34*/	{  0,	1280,800,85, 	1712,843,	0,1,	216,34,		0,	0,		0,		0,		0,		0 },		// 1280x800x85
/*35*/	{  0,	1280,800,120, 	1440,813,	1,0,	80, 38,		0,	0,		0,		0,		0,		0 },		// 1280x800x120	REDUCED

/*36*/	{  1,	1280,960,60, 	1800,1000,	1,1,	312,36,		5,	-7,		0,		0,		0,		0 },		// 1280x960x60
/*37*/	{  0,	1280,960,85, 	1728,1011,	1,1,	224,47,		0,	0,		0,		0,		0,		0 },		// 1280x960x85
/*38*/	{  0,	1280,960,120, 	1440,813,	1,0,	80, 50,		0,	0,		0,		0,		0,		0 },		// 1280x960x120	REDUCED

/*39*/	{  1,	1280,1024,60, 	1688,1066,	1,1,	248,38,		5,	-6,		0,		0,		640,	0 },		// 1280x1024x60
/*40*/	{  0,	1280,1024,75, 	1688,1066,	1,1,	248,38,		0,	0,		0,		0,		800,	0 },		// 1280x1024x75
/*41*/	{  0,	1280,1024,85, 	1728,1072,	1,1,	224,44,		0,	0,		0,		0,		911,	0 },		// 1280x1024x85

/*42*/	{  1,	1360,768,60, 	1792,795,	1,1,	256,18,		5,	-8,		0,		0,		0,		0 },		// 1360x768x60

/*43*/	{  1,	1400,1050,60, 	1560,1080,	1,0,	80, 23,		5,	-7,		0,		0,		0,		0 },		// 1400x1050x60	REDUCED
/*44*/	{  1,	1400,1050,60, 	1864,1089,	0,1,	232,32,		5,	-6,		0,		0,		0,		0 },		// 1400x1050x60			// It cannot display

/*45*/	{  1,	1440,900,60, 	1600,926,	1,0,	80, 17,		5,	-8,		0,		0,		0,		0 },		// 1400x900x60	REDUCED
/*46*/	{  1,	1440,900,60, 	1904,934,	0,1,	232,25,		5,	-8,		0,		0,		0,		0 },		// 1400x900x60
/*47*/	{  0,	1440,900,75, 	1936,942,	0,1,	248,33,		0,	0,		0,		0,		0,		0 },		// 1400x900x75

/*48*/	{  1,	1680,1050,60, 	1840,1080,	1,0,	80, 20,		0,	0,		0,		0,		0,		0 },		// 1680x1440x60	REDUCED
/*49*/	{  1,	1920,1080,60,	2200,1124,  0,0,	277,37,		30,	10,		0,		0,		0,		0 },		// EE_RGB_1080P
///============
	//if 0x1CC[0] is 0
// 	   SUPPORT  HAN,VAN,VFREQ  	H&V TOTAL  HSP &VSP	Hst,Vst		OffsetH,V	Dummy0 	Dummy1 	Dummy2 	Dummy3
/*49*/	{  3,	720,240+1,60,	858,262,	0,0,	112,17,		0,	0,		0,		0,		0,		0 },		// 34: EE_RGB_480I        	 0x359,0x359,   0x359,   120, 10,  123, 324, 21
/*50*/	{  3,	720,288,50,		864,312,	0,0,	109,22,		0,	0,		0,		0,		0,		0 },		// 35: EE_RGB_576I        	 0x35f,0x35f,   0x35f,   134, 10,  131, 324, 16
/*51*/	{  3,	720,480,60,		858,525,	0,0,	112,41,		0,	0,		0,		0,		0,		0 },		// 28: EE_RGB_480P        	 0x359,0x359,   0x359,   123, 28,  251, 378, 17
/*52*/	{  3,	720,576,50,		864,625,    0,0,	107,48,		0,	0,		0,		0,		0,		0 },		// 36: EE_RGB_576P			 0x35f,0x35f,   0x35f,   133, 36,  250, 324, 21
/*53*/	{  3,	1920,540,60,	2200,562,   0,0,	217,24,		0,	0,		0,		0,		0,		0 },		// 31: EE_RGB_1080I       	 0x897,0x897,   0x897,   300, 42,  740, 324, 32
/*54*/	{  3,	1280,720,60,	1650,750,   0,0,	277,30,		0,	0,		0,		0,		0,		0 },		// 29: EE_RGB_720P        	 0x671,0x671,   0x671,   282, 12,  742, 410, 3 
/*55*/	{  3,	1920,1080,60,	2200,1125,  0,0,	217,48,		0,	0,		0,		0,		0,		0 },		// EE_RGB_1080P
//	{  3,	1280,720,60,	1980,	    },		// 30: EE_RGB_720P50 ???  	 0x7bb,0x7bb,   0x7bb,   346, 25,  742, 378, 45
//	{  0,	1920,540-40,50,	2640,	    },		// 32: EE_RGB_1080I50A    	 0xa4f,0xa4f,   0xa4f,   300, 41,  740, 324, 32
//	{  0,	1920,540-40,60,	2200,	    },		// 33: EE_RGB_1080I50B ???	 0x897,0x897,   0x897,   300, 42,  740, 324, 32
/*54*/	{  3,	1280,720,50,	1000,100,   0,0,	277,30,		0,	0,		0,		0,		0,		0 },		// 720P@50
/*53*/	{  3,	1920,526,50,	1000,100,   0,0,	356,87,		0,	0,		0,		0,		0,		0 },		// 1080i@50
/*55*/	{  3,	1920,1080,50,	1000,100,   0,0,	217,48,		0,	0,		0,		0,		0,		0 },		// 1080P@50
//--------------
};
#endif  //.. defined( SUPPORT_PC ) || defined (SUPPORT_DVI)



//enum EE_PC_OFFSET {
//	EE_PC_UNKNOWN=0,
//	EE_PC_DOS,	
//	EE_PC_640x350_85,
//};
#if /*defined(SUPPORT_COMPONENT) || */ defined(SUPPORT_PC)
/**
* get sizeof PCMDATA table.(PC Mode Data Table)
*/
DWORD sizeof_PCMDATA(void)
{
	return sizeof(PCMDATA);
}
#endif
#if 0
DWORD sizeof_DVIMDATA(void)
{
	return sizeof(DVIMDATA);
}
#endif

#ifdef SUPPORT_PC
/**
* prepare Info String for PC
*/
void PC_PrepareInfoString(BYTE mode)
{
	BYTE itoa_buff[5];					

	//prepare info. ex: "PC 1024x768 60Hz"
	FOsdSetInputMainString2FOsdMsgBuff();	//GetInputMainString(FOsdMsgBuff);										 	
	TWstrcat(FOsdMsgBuff," ");
	TWitoa(PCMDATA[mode].han, itoa_buff);
	TWstrcat(FOsdMsgBuff,itoa_buff);
	TWstrcat(FOsdMsgBuff,"x");
	TWitoa(PCMDATA[mode].van, itoa_buff);
	TWstrcat(FOsdMsgBuff,itoa_buff);
	TWstrcat(FOsdMsgBuff," ");
	TWitoa(PCMDATA[mode].vfreq, itoa_buff);
	TWstrcat(FOsdMsgBuff,itoa_buff);
	TWstrcat(FOsdMsgBuff,"Hz");

//BK110811	FOsdCopyMsgBuff2Osdram(OFF);
}
#endif

//related with YUVCropH[] array.
#ifdef SUPPORT_COMPONENT
/**
* prepare Info String for Component
*/
void YUV_PrepareInfoString(BYTE mode)
{
	//prepare info. ex: "Component 1080p 60Hz"
	FOsdSetInputMainString2FOsdMsgBuff();	//GetInputMainString(FOsdMsgBuff);										 	
	TWstrcat(FOsdMsgBuff," ");
	switch(mode) {
	case 0:	TWstrcat(FOsdMsgBuff,"480i");	break;
	case 1:	TWstrcat(FOsdMsgBuff,"576i");	break;
	case 2:	TWstrcat(FOsdMsgBuff,"480p");	break;
	case 3:	TWstrcat(FOsdMsgBuff,"576p");	break;
	case 4:	TWstrcat(FOsdMsgBuff,"1080i 50Hz");	break;
	case 5:	TWstrcat(FOsdMsgBuff,"1080i 60Hz");	break;
	case 6:	TWstrcat(FOsdMsgBuff,"720p 50Hz");	break;
	case 7:	TWstrcat(FOsdMsgBuff,"720p 60Hz");	break;
	case 8:	TWstrcat(FOsdMsgBuff,"1080p 50Hz");	break;
	case 9:	TWstrcat(FOsdMsgBuff,"1080p 60Hz");	break;
	default:TWstrcat(FOsdMsgBuff,"Unknown");	break;
	}
//BK110811	FOsdCopyMsgBuff2Osdram(OFF);
}
#endif

#ifdef SUPPORT_DVI
/**
* prepare Info String for DVI
*/
void DVI_PrepareInfoString(WORD han, WORD van, BYTE vfreq)
{
	BYTE itoa_buff[5];					

	//prepare info. ex: "DVI 1024x768 60Hz"
	FOsdSetInputMainString2FOsdMsgBuff();	//GetInputMainString(FOsdMsgBuff);										 	
	TWstrcat(FOsdMsgBuff," ");
	TWitoa(han, itoa_buff);
	TWstrcat(FOsdMsgBuff,itoa_buff);
	TWstrcat(FOsdMsgBuff,"x");
	TWitoa(van, itoa_buff);
	TWstrcat(FOsdMsgBuff,itoa_buff);
	if(vfreq) {
		TWstrcat(FOsdMsgBuff," ");
		TWitoa(vfreq, itoa_buff);
		TWstrcat(FOsdMsgBuff,itoa_buff);
		TWstrcat(FOsdMsgBuff,"Hz");
	}
//BK110811	FOsdCopyMsgBuff2Osdram(OFF);
}
#endif
