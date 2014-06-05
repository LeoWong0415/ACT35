/**
 * @file
 * OSDSPI.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	low level SpiOSD layer
*/
//*****************************************************************************
//
//								OSD.c
//
//*****************************************************************************
//
//
#include "config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"
#include "Global.h"

#include "CPU.h"
#include "printf.h"
#include "Util.h"

#include "I2C.h"
#include "spi.h"

#include "SOsd.h"
#include "FOsd.h"
#include "SpiFlashMap.h"
#include "Settings.h"

#ifdef SUPPORT_SPIOSD
#undef DEBUG_SPIFLASH_TEST
//==========================================
//win0: 0x420	animation
//win1: 0x440	background. low priority
//win2: 0x430
//..
//win8: 0x4B0	focused.    high priority
//----------------------------------------
XDATA BYTE SpiWinBuff[10*0x10];


//TW8835 have 9 windows
#ifdef SUPPORT_8BIT_CHIP_ACCESS
code BYTE	SpiOsdWinBase[9] = { SPI_WIN0_ST, 
		SPI_WIN1_ST, SPI_WIN2_ST, SPI_WIN3_ST, SPI_WIN4_ST,
		SPI_WIN5_ST, SPI_WIN6_ST, SPI_WIN7_ST, SPI_WIN8_ST
		};
#else
code WORD	SpiOsdWinBase[9] = { SPI_WIN0_ST, 
		SPI_WIN1_ST, SPI_WIN2_ST, SPI_WIN3_ST, SPI_WIN4_ST,
		SPI_WIN5_ST, SPI_WIN6_ST, SPI_WIN7_ST, SPI_WIN8_ST
		};
#endif

//=============================================================================
//		OSD Window Functions
//=============================================================================


//=============================================================================
//		OSD Window Setup
//=============================================================================
#define LUT_TYPE_FONT	0x00
#define LUT_TYPE_LUTS	0x80
#define LUT_TYPE_LUT	0xC0


#define DMA_TYPE_FONT	0	//x00
#define DMA_TYPE_CHIP	1	//x40
#define DMA_TYPE_SPIOSD	2	//x80
#define DMA_TYPE_MCU	3	//xC0


#ifdef UNCALLED_SEGMENT_CODE
#define DMA_LUTTYPE_ADDR	0x00	//slow..pls. replace your image to BYTE type
#define DMA_LUTTYPE_BYTE	0x80
//void SpiFlashDMA(BYTE DmaType, BYTE LutOffset, DWORD address, WORD size)
//{...}
#endif


//-----------------------------------------------------------------------------
// R40E[7:4]	OSD Linebuffer MSB
#ifdef MODEL_TW8836___TEST
void SOsdSetLineBuffSize(BYTE msb)
{
	WriteTW88(REG40E, (ReadTW88(REG40E) & 0x0F) | msb);
}

#endif



/**
* Set SpiOsd DE value
*
*	How to calculate DE value 
*	HDE = REG(0x210[7:0])
*	PCLKO = REG(0x20d[1:0]) {0,1,2,2}
*	PCLKO = REG(0x20d[1:0]) {1,1,1,1}  new
*	result = HDE + PCLKO - 17
*/
void SpiOsdSetDeValue(void)
{
	XDATA	WORD wTemp;
	BYTE HDE,PCLKO;

	WriteTW88Page(PAGE2_SCALER );
	HDE = ReadTW88(REG210 );				// HDE
	PCLKO = ReadTW88(REG20D) & 0x03;
	//if(PCLKO == 3)
	//	PCLKO = 2;
	PCLKO = 1;

	wTemp = (WORD)HDE + PCLKO - 17;

	WriteTW88Page(PAGE4_SOSD );
	WriteTW88(REG40E, (BYTE)(wTemp>>8) );		// write SPI OSD DE value(high nibble)
	WriteTW88(REG40F, (BYTE)wTemp );   		// write SPI OSD DE value(low byte)
	dPrintf("\nSpiOsdDe:%04x",wTemp);		
}


/**
* Enable SpiOsd. HW function
*
* MODEL_TW8835_EXTI2C
*	Do not toggle MCUSPI clock. Internal MCU can not make a synch.
*/
void SpiOsdEnable(BYTE en)
{
	BYTE dat;
	WriteTW88Page(PAGE4_SOSD );
	dat = ReadTW88(REG400);
	if( en ) {
#ifdef MODEL_TW8835_EXTI2C
		//-----------------------------------------
		// If System uses SPIOSD, SPIOSD do DMA to read the SPIOSD data.
		// If FW executes DMA on the vDE area, it can couurpt the DMA.
		// If DMA corruption is happened, FW can not recover it. 
		//-----------------------------------------
		WriteTW88(REG4C1, ReadTW88(REG4C1) |  0x01);		//DMA start at VBlank
//		WriteTW88(REG4E0, ReadTW88(REG4E0) & ~0x01);		//select PCLK
#endif

#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	//BK120615. for test clock source
#else
		//SPIOSD mode use PCLK or PLL108
		McuSpiClkSelect(MCUSPI_CLK_PCLK);					//select MCU/SPI Clock.
#endif
		WriteTW88Page(PAGE4_SOSD );
		WriteTW88(REG400, dat | 0x04);						//enable SpiOSD
	}
	else {
		WriteTW88(REG400, dat & ~0x04);						//disable SpiOSD

#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	//BK120615. for test clock source
#else
		//PLAYMODE_USE_27MHZ
		McuSpiClkSelect(MCUSPI_CLK_27M); 					//select MCU/SPI Clock.
	 	//BKFYI120112. If you want CKLPLL, select MCUSPI_CLK_PCLK
		//McuSpiClkSelect(MCUSPI_CLK_PCLK);	//select MCU/SPI Clock.
#endif

#ifdef MODEL_TW8835_EXTI2C
		WriteTW88Page(PAGE4_SOSD );
		WriteTW88(REG4C1, ReadTW88(REG4C1) & ~0x01);		//DMA start at immediately
//		WriteTW88(REG4E0, ReadTW88(REG4E0) |  0x01);		//select PLL108M
#endif

	}
}

#if 0 //#ifdef MODEL_TW8835_SLAVE
BYTE SpiOsdIsOn(void)
{
	BYTE dat;
	WriteTW88Page(PAGE4_SOSD );
	dat = ReadTW88(REG400);
	if(dat & 0x04)	return ON;
	else			return OFF;
}
#endif

//==============================
// Windows Finctions
//==============================

/**
* Enable SpiOsd Window. HW function.
*/
void SpiOsdWinHWEnable(BYTE winno, BYTE en)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	XDATA BYTE	index;
#else
	XDATA WORD index;
#endif
	XDATA	BYTE dat;

	index = SpiOsdWinBase[winno] + SPI_OSDWIN_ENABLE;

	WriteTW88Page(PAGE4_SOSD );
	dat = ReadTW88(index);
	if( en ) {
		WriteTW88(index, dat | 0x01);
	}
	else     WriteTW88(index, dat & 0xfe);
}

#ifdef MODEL_TW8836____TEST
//REG420[1] REG440[1] REG450[1],...
void SpiOsdWinHZoom(BYTE winno, BYTE en)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	XDATA BYTE	index;
#else
	XDATA WORD index;
#endif
	XDATA	BYTE dat;

	index = SpiOsdWinBase[winno] + SPI_OSDWIN_HZOOM;

	WriteTW88Page(PAGE4_SOSD );
	dat = ReadTW88(index);
	if( en ) WriteTW88(index, dat | 0x02);
	else     WriteTW88(index, dat & ~0x02);
}
#endif


#if 0
BYTE SpiOsdWinIsOn(BYTE winno)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	XDATA	BYTE index;
#else
	XDATA WORD index;
#endif

	index = SpiOsdWinBase[winno] + SPI_OSDWIN_ENABLE;

	WriteTW88Page(PAGE4_SOSD );
	if( ReadTW88(index) & 0x01 ) return ON;
	else						 return OFF;
}
#endif

/**
* update SpiOsdWinBuff[]
*/
void SpiOsdWinBuffEnable(BYTE winno, BYTE en)
{
	DATA BYTE XDATA *data_p;

	if(winno) winno++;
	data_p = &SpiWinBuff[winno << 4];

	if(en) *data_p |= 0x01;
	else   *data_p &= 0xfe;
}


//--------------------------
/**
* clear all SpiWinBuff[]
*/
void SOsdWinBuffClean(BYTE hw)
{
	BYTE i,j;

	if(hw) {
		SpiOsdWinHWOffAll(1);
	 	SpiOsdRLC(0,0,0);	//disable RLE
	}

	for(i=0; i < 10; i++) {
		for(j=0; j < 0x0E; j++)
			SpiWinBuff[i*16+j]=0;
	}
}
//--------------------------
/**
* write SpiWinBuff to HW registers
*
*	start address for ecah window
*	WIN		0	1	2	3	4	5	6	7	8
*	addr	420 440 450 460 470 480 490 4A0 4B0
* @param  start: start window. between 0 to 8
* @param  end:   end window. between 0 to 8
*/
#pragma SAVE
#pragma OPTIMIZE(8,SPEED)
void SOsdWinBuffWrite2Hw(BYTE start, BYTE end)
{
	DATA BYTE i;
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	DATA BYTE reg_i;
#else
	DATA WORD reg_i;
#endif
	DATA BYTE XDATA *data_p;

#ifdef DEBUG_OSD
	dPrintf("\nSOsdWinBuffWrite2Hw(%bd,%bd)",start,end);
#endif

	if(start)	start++;
	if(end)		end++;

	//WaitVBlank(1);
	WriteTW88Page(PAGE4_SOSD );
	data_p = &SpiWinBuff[start << 4];

	for(i=start; i <= end; i++) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
		reg_i = (i << 4) + 0x20;
#else
		reg_i = 0x400 | (i << 4) + 0x20;
#endif
		//if(*data_p & 0x01) {
		//	dPrintf(" %bd:%bx", i-1, *data_p);
		//}
		//dPrintf("\nR:%bx ",reg_i);	
		//for(j=0; j < 16; j++) {
		//	//WriteTW88(reg_i++, *data_p++);
		//	dPrintf("%bx ",SpiWinBuff[i*16+j]);	
		//}
		//dPrintf("::%bx",*data_p);	

		WriteTW88(reg_i++, *data_p++);	//0
		WriteTW88(reg_i++, *data_p++);	//1
		WriteTW88(reg_i++, *data_p++);	//2
		WriteTW88(reg_i++, *data_p++);	//3
		WriteTW88(reg_i++, *data_p++);	//4
		WriteTW88(reg_i++, *data_p++);	//5
		WriteTW88(reg_i++, *data_p++);	//6
		WriteTW88(reg_i++, *data_p++);	//7
		WriteTW88(reg_i++, *data_p++);	//8
		WriteTW88(reg_i++, *data_p++);	//9
		WriteTW88(reg_i++, *data_p++);	//A
		WriteTW88(reg_i++, *data_p++);	//B
		WriteTW88(reg_i++, *data_p++);	//C
		WriteTW88(reg_i++, *data_p++);	//D
		WriteTW88(reg_i++, *data_p++);	//E
		if(i) {
			data_p++;						//F
		}
		else {
			WriteTW88(reg_i++, *data_p++);	//0F
			i++;
			WriteTW88(reg_i++, *data_p++);	//10
			WriteTW88(reg_i++, *data_p++);	//11
			WriteTW88(reg_i++, *data_p++);	//12
			WriteTW88(reg_i++, *data_p++);	//13
			WriteTW88(reg_i++, *data_p++);	//14
			WriteTW88(reg_i++, *data_p++);	//15
			WriteTW88(reg_i++, *data_p++);	//16
			reg_i+=9;
			data_p+=9;
		}
	}
}
#pragma RESTORE


#if 0
//desc
//	check win buff, if HW is enabled and buff is not, disable HW
void SpiOsdWinBuffSynchEnable(void)
{
	BYTE winno;
	BYTE buff;
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	BYTE	index;
#else
	WORD index;
#endif
	BYTE dat;

	WriteTW88Page(PAGE4_SOSD );

	dPrintf("\nSpiOsdWinBuffSynchEnable ");
	for(winno=0; winno <= 8; winno++) {
		if(winno) 	buff = SpiWinBuff[(winno+1) << 4];
		else		buff = SpiWinBuff[winno << 4];

		index = SpiOsdWinBase[winno] + SPI_OSDWIN_ENABLE;
		dat = ReadTW88(index);

		if(buff != dat) {
			dPrintf("win%02bx %02bx->%02bx ",winno, dat, buff);
			WriteTW88(index, dat & 0xfe);
		}
	}
	//RLE will be synch on menu, not here
}
#endif

#ifdef UNCALLED_CODE
void SpiOsdWinBuffOffAll(void)
{...}
#endif

/**
* turn off all SpiOsd Window.
*
* @see SpiOsdWinHWEnable
*/
void SpiOsdWinHWOffAll(BYTE wait)
{
	BYTE i;
	if(wait)
		WaitVBlank(wait);
	//SpiOsdEnableRLC(OFF);		//disable RLE
	SpiOsdDisableRLC(1);
	for(i=0; i<= 8; i++)
		SpiOsdWinHWEnable(i, 0);
}
#ifdef UNCALLED_CODE
void SpiOsdWinHWOff(BYTE start, BYTE end)
{...}
#endif

/**
* set image location
*/
//WINx Image Location on SpiFlash	 
void SpiOsdWinImageLoc(BYTE winno, DWORD start)
{	
	DATA BYTE XDATA *data_p;

	if(winno) winno++;
	data_p = &SpiWinBuff[winno << 4];

	data_p += SPI_OSDWIN_BUFFERSTART;

	*data_p++ =  (BYTE)(start>>16);				//+0x07
	*data_p++ =  (BYTE)(start>>8);				//+0x08
	*data_p++ =  (BYTE)start;					//+0x09
}

//win0 win1       win2 
//N/A  0x44A[7:6] 0x45A[7:6].,,,
/**
* set image bit location
*/
void SpiOsdWinImageLocBit(BYTE winno, BYTE start)
{
	DATA BYTE XDATA *data_p;

	if(winno) winno++;
	data_p = &SpiWinBuff[winno << 4];

	data_p += SPI_OSDWIN_BUFFERSTART_BIT;
	*data_p &= 0x3F;
	*data_p |= (start << 6);
}


//WINBUFF
/**
* set image width
*/
void SpiOsdWinImageSizeW(BYTE winno, WORD w)
{
	DATA BYTE XDATA *data_p;
	BYTE value;

	data_p = &SpiWinBuff[(winno+1) << 4];	//No WIN0
	data_p += SPI_OSDWIN_DISPSIZE;

 	value = *data_p & 0xC0;

	*data_p++ = (BYTE)(w>>8 | value);		//+0x0A
	*data_p++ = (BYTE)w;					//+0x0B
}

//WINx buff size
/**
* set image width and height
*/
void SpiOsdWinImageSizeWH (BYTE winno, WORD w, WORD h)
{
	DATA BYTE XDATA *data_p;
	BYTE value;

	//WIN1to8 need only Width.
	if(winno) {
		SpiOsdWinImageSizeW(winno,w);
		return; 
	}

	//now only for WIN0
	data_p = SpiWinBuff;				   //Only WIN0
	data_p += SPI_OSDWIN_DISPSIZE;

	value = (BYTE)(h >> 8);
	value <<= 4;
	value |= (BYTE)( w>>8 );
	*data_p++ = value; 		//42A
	*data_p++ = (BYTE)w;	//42B
	*data_p++ = (BYTE)h;	//42C
}


//WINx Screen(win) Pos & Size
/**
* set window position and size
*/
void SpiOsdWinScreen(BYTE winno, WORD x, WORD y, WORD w, WORD h)
{
	DATA BYTE XDATA *data_p;
	BYTE value;

	if(winno) winno++;
	data_p = &SpiWinBuff[winno << 4];

	data_p += SPI_OSDWIN_SCREEN;
	value = (y >> 8);
	value <<= 4;
	value |= (x >> 8);
	*data_p++ = value;		//421	441...
	*data_p++ = (BYTE)x;	//422	442... 	
	*data_p++ = (BYTE)y;	//423	443...
	
	value = (h >> 8);
	value <<= 4;
	value |= (w >> 8);
	*data_p++ = value;		//424	444...
	*data_p++ = (BYTE)w;	//425	445...	 	
	*data_p++ = (BYTE)h;	//426	446...	 
}


//=============================================================================
//		Load LUT
//=============================================================================
//LUT offset use 5bit & 16 unit
/**
* set Lut Offset
*/
void SpiOsdWinLutOffset( BYTE winno, WORD table_offset )
{
	DATA BYTE XDATA *data_p;

	if(winno) winno++;
	data_p = &SpiWinBuff[winno << 4];
	data_p += SPI_OSDWIN_LUT_PTR;
	if(!winno) data_p += 4;
	
	//LUT offset use 5bit & 16 unit
	*data_p = table_offset >> 4;
}

		


//=============================================================================
//		Pixel Width
//=============================================================================
//bpp
//	0:4bit, 1:6bit others:8bit
//
/**
* set pixel width
*/
void SpiOsdWinPixelWidth(BYTE winno, BYTE bpp)
{
	DATA BYTE XDATA *data_p;
	BYTE mode;

	if(bpp==4)	mode=0;
	else if(bpp==6) mode=1;
	else mode=2;	//7 and 8 use mode 2

	if(winno) 	winno++;
	data_p = &SpiWinBuff[winno << 4];

	*data_p &= 0x3f;
	*data_p |= (mode <<6);
}
//=============================================================================
//		SpiOsdWinFillColor( BYTE winno, BYTE color )
//=============================================================================
//color will be an offset from the LUT location that Window have. 
//If window start LUT from 80, the color value means color+80 indexed color.
/**
* fill color
*/
void	SpiOsdWinFillColor( BYTE winno, BYTE color )
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	BYTE	index;
#else
	WORD index;
#endif

	index = SpiOsdWinBase[winno];
	WriteTW88Page(PAGE4_SOSD );

	if ( color ) {
		WriteTW88(index, (ReadTW88(index ) | 0x04));				// en Alpha & Global
	}
	else {
		WriteTW88(index, (ReadTW88(index ) & 0xFB ) );				// dis Alpha & Global
	}
	index = SpiOsdWinBase[winno] + SPI_OSDWIN_FILLCOLOR;
	if(!winno)	index += 8;
	WriteTW88(index, color );
}

//=============================================================================
//		SpiOsdWinGlobalAlpha( BYTE winno, BYTE alpha )
//=============================================================================
//alpha: 0 to 7F
/**
* set global alpha
*/
void SpiOsdWinGlobalAlpha( BYTE winno, BYTE alpha )
{
	DATA BYTE XDATA *data_p;

	if(winno) winno++;
	data_p = &SpiWinBuff[winno << 4];

	*data_p &= 0xCF;
	if(alpha) *data_p |= 0x10;

	data_p += SPI_OSDWIN_ALPHA;
	if(!winno)	data_p += 4;
	*data_p = alpha;
}

//=============================================================================
//		SpiOsdWinGlobalAlpha( BYTE winno, BYTE alpha )
//=============================================================================
/**
* set Pixel alpha
*/
void	SpiOsdWinPixelAlpha( BYTE winno, BYTE alpha )
{
	DATA BYTE XDATA *data_p;

	if(winno) winno++;
	data_p = &SpiWinBuff[winno << 4];

	if(alpha)	*data_p |= 0x30;
	else		*data_p &= 0xCF;

	data_p += SPI_OSDWIN_ALPHA;
	if(!winno)	data_p += 4;
	*data_p = alpha;
}



/**
* adjust Win0 priority
*
* NOTE:Only fow Win0
*/
void SpiOsdWin0SetPriority(BYTE high)
{
#if 0
	XDATA	BYTE dat;
	WriteTW88Page(PAGE4_SOSD );
	dat = ReadTW88(REG420);
	if( high ) WriteTW88(REG420, dat | 0x02);
	else       WriteTW88(REG420, dat & 0xfd);
#else
	DATA BYTE XDATA *data_p;

	data_p = &SpiWinBuff[0];

	if(high) *data_p |= 0x02;
	else   	*data_p &= 0xfd;
#endif
}

//=============================================================================
//		Animation
//=============================================================================
/**
* set Animation
* @param mode	
*	-0:display one time of the loop and then disappear
*	-1:display one time of the loop and then stay at the last frame
*	-2:Enable looping 
*	-3:static. Show the frame pointed by (0x431 and 0x432)
* @param Duration duration time of each frame (in unit of VSync)
*	- 0: infinite
*	- 1: One VSync period
*	- max 0xFF: 255 VSync period		
*/
void SpiOsdWin0Animation(BYTE mode, BYTE FrameH, BYTE FrameV, BYTE Duration)
{
	DATA BYTE XDATA *data_p;

	data_p = SpiWinBuff;	  			//Only WIN0
	data_p += SPI_OSDWIN_ANIMATION;

	*data_p++ = FrameH;
	*data_p++ = FrameV;
	*data_p++ = Duration;

	*data_p &= 0x3f;
	*data_p |= (mode << 6);
}

//WINx buff offset
/**
* set Win0 X,Y
*/
void SpiOsdWin0ImageOffsetXY (WORD x, WORD y)
{
	BYTE value;
	DATA BYTE XDATA *data_p;

	data_p = SpiWinBuff;			//Only WIN0
	data_p += SPI_OSDWIN_DISPOFFSET;

	value  = (BYTE)(y >> 8);
	value <<=4;
	value |= (BYTE)(x >> 8);
	*data_p++ = value;
	*data_p++ = (BYTE)x;
	*data_p++ = (BYTE)y;
}



#ifdef FUNCTION_VERIFICATION
//no DMA
//@param
//	type:	
//		1:Byte pointer - LUTS type
//		0:Address pointer - LUT type
void SpiOsdIoLoadLUT_ARRAY(BYTE type, WORD LutOffset, WORD size, BYTE *pData)
{
}
#endif

//BKTODO:Remove. You can share a Data segment bank
BYTE temp_SPI_Buffer[64];	//only for SpiOsdIoLoadLUT.
/**
* download LUT by IO (without DMA.)
*
* LutOffset: 0~511(0x00~0x1FF)
*
* NOTE:Only for TW8832 image
* @param type:	
*	- 1:Byte pointer - LUTS type
*	- 0:Address pointer - LUT type
*/
void SpiOsdIoLoadLUT(BYTE type, WORD LutOffset, WORD size, DWORD spiaddr)
{
	BYTE i,j,k;
	BYTE R410_data;
#ifdef DEBUG_OSD
	dPrintf("\nSpiOsdIoLoadLUT%s LutLoc:%d size:%d 0x%06lx", type ? "S":" ", LutOffset, size, spiaddr);
#endif

	WriteTW88Page(PAGE4_SOSD );

	//--- SPI-OSD config
	if(type==0)	R410_data = 0xC0;			// LUT Write Mode, En & address ptr inc.
	else		R410_data = 0xA0;			// LUT Write Mode, En & byte ptr inc.
	if(LutOffset >> 8)
		R410_data |= 0x04;
	
	if(type==0) {
		//
		//ignore size. it is always 0x400.(256*4)
		//		
		for(i=0; i < 4; i++) {	 
			WriteTW88(REG410, R410_data | i );	//assign byte ptr	
			WriteTW88(REG411, (BYTE)LutOffset);	//reset address ptr.
			for(j=0; j<(256/64);j++) {
				SpiFlashDmaRead2XMem(temp_SPI_Buffer,spiaddr + i*256 + j*64,64);	 //BUGBUG120606 BANK issue
				for(k=0; k < 64; k++) {
					WriteTW88(REG412, temp_SPI_Buffer[k]);		//write data
				}
			}
		}
	}
	else {
		WriteTW88(REG410, R410_data);			//assign byte ptr. always start from 0.
		WriteTW88(REG411, (BYTE)LutOffset);	//reset address ptr.

		for(i=0; i < (size / 64); i++ ) {	//min size is a 64(16*4)
			SpiFlashDmaRead2XMem(temp_SPI_Buffer,spiaddr + i*64,64);
			for(k=0; k < 64; k++) {
				WriteTW88(REG412, temp_SPI_Buffer[k]);		//write data
			}
		}
	}
}

#define SPILUTBUFF_WIN

#ifdef SPILUTBUFF_WIN
//------------------------
//0: use flag
//1: size high
//2: size low
//3: lut offset	(LutOffset >> 6)
//4: lut offset	(LutOffset << 2)
//5: (address>>16) 
//6: (address>>8)
//7: (address)
//------------------------
BYTE SOsdHwBuff_win[9*8];
WORD SOsdHwBuff_alpha;
BYTE SOsdHwBuff_rle_win;
BYTE SOsdHwBuff_rle_bpp;
BYTE SOsdHwBuff_rle_count;
#endif

/**
* clear HwBuff.
*/
void SOsdHwBuffClean(void)
{
	BYTE i;

	SOsdHwBuff_alpha=0xffff;
	SOsdHwBuff_rle_win=0;

	for(i=0; i<=8; i++) {
		//clear use flag
		SOsdHwBuff_win[i*8]=0;		
	}
}

/**
* set LUT info to HwBuff. 
*/
void SOsdHwBuffSetLut(BYTE win, /*BYTE type,*/  WORD LutOffset, WORD size, DWORD address)
{
   	SOsdHwBuff_win[win*8+0] = 1;
	SOsdHwBuff_win[win*8+1] = (BYTE)(size >> 8);
	SOsdHwBuff_win[win*8+2] = (BYTE)size;

	SOsdHwBuff_win[win*8+3] = (BYTE)(LutOffset >> 6);
	SOsdHwBuff_win[win*8+4] = (BYTE)(LutOffset << 2);

	SOsdHwBuff_win[win*8+5] = (BYTE)(address>>16);
	SOsdHwBuff_win[win*8+6] = (BYTE)(address>>8) ;
	SOsdHwBuff_win[win*8+7] = (BYTE)(address) ;
}
/**
* set RLE info to HwBuff
*/
void SOsdHwBuffSetRle(BYTE win, BYTE bpp, BYTE count)
{
	SOsdHwBuff_rle_win = win;
	SOsdHwBuff_rle_bpp = bpp;
	SOsdHwBuff_rle_count = count;
}
/**
* set Alpha to HwBuff
*/
void SOsdHwBuffSetAlpha(WORD alpha_index)
{
	SOsdHwBuff_alpha = alpha_index;
}

/*
example: volatile & memory register access
volatile BYTE XDATA mm_dev_R1CD	_at_ 0xC1CD;	//use 1 XDATA BYTE
//#define TW8835_R1CD	(*((unsigned char volatile xdata *) (0xc000+0x1CD)))
#define TW8835_R1CD	(*((unsigned char volatile xdata *) (REG_START_ADDRESS+0x1CD) ))
void Dummy_Volatile_memory_register_test(void)
{
	volatile BYTE mode;
	volatile BYTE XDATA *p; // = (BYTE XDATA *)0xC1CD;

	mode = *(volatile BYTE XDATA*)(0xC1CD);

	p = (BYTE XDATA *)0xC1CD;
	mode = *p;

	mode = mm_dev_R1CD;

	mode = TW8835_R1CD;
}
*/

/**
* write H2Buff to real HW
*/
#ifdef MODEL_TW8835_EXTI2C
void SOsdHwBuffWrite2Hw(void)
{
	BYTE win;
	DATA BYTE XDATA *data_p;
	DATA BYTE i,j;
	BYTE reg4c1;
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	DATA BYTE reg_i;
#else
	DATA WORD reg_i;
#endif
	WORD LutOffset;

#ifdef DEBUG_OSD
	dPuts("\nSOsdHwBuffWrite2Hw.....");
#endif


	//-----------------------------
	// time cirtical section start
	//-----------------------------

	//----------------------------
	//update LUT
	WriteTW88Page(PAGE4_SOSD );
	WriteTW88(REG410, 0xa0 );    			// LUT Write Mode, En & byte ptr inc. DMA needs it.
	WriteTW88(REG411, 0); 					// LUT addr. set 0 on DMA


	for(win=0; win <= 8; win++) {
		//check Use flag.
		if(SOsdHwBuff_win[win*8] == 0) 
			continue;
		data_p = &SOsdHwBuff_win[win*8+1];

		//Spi Flash DMA
#ifdef FAST_SPIFLASH
		WriteTW88(REG4C0_SPIBASE+0x04, 0x00 );	// DMA stop

		WriteTW88(REG4C0_SPIBASE+0x03, 0x80 | SPICMD_x_BYTES ); //LUT,Increase, 0x0B with 5 commands, 0xeb with 7 commands	           
		WriteTW88(REG4C0_SPIBASE+0x0a, SPICMD_x_READ ); 			// SPI Command=R
#else
		SpiFlashDmaStop();	
		SpiFlashCmdRead(DMA_DEST_SOSD_LUT);		
#endif

		//WriteTW88(REG4C0_SPIBASE+0x1a, 0x00 ); // DMA size
		WriteTW88(REG4C0_SPIBASE+0x08, *data_p++ );	//size0
		WriteTW88(REG4C0_SPIBASE+0x09, *data_p++ );	//size1
		LutOffset = *data_p;
		LutOffset <<= 6;

		WriteTW88(REG4C0_SPIBASE+0x06, *data_p++ ); 	//LutOffset[8:6] -> R4C6[2:0]
		LutOffset |= (*data_p & 0x3F);
		WriteTW88(REG4C0_SPIBASE+0x07, *data_p++ );		//LutOffset[5:0] -> R4C7[7:2] 
		WriteTW88(REG4C0_SPIBASE+0x0b, *data_p++); 		//address0
		WriteTW88(REG4C0_SPIBASE+0x0c, *data_p++ );		//address1
		WriteTW88(REG4C0_SPIBASE+0x0d, *data_p++ ); 	//address2

		//==========================================
		// time critical area start
		//==========================================
		reg4c1 = ReadI2CByte(TW88I2CAddress,REG4C1);
		WriteI2CByte(TW88I2CAddress,REG4C1, reg4c1 & ~1);

		shadow_r4e0 = ReadI2CByte(TW88I2CAddress,REG4E0);
		shadow_r4e1 = ReadI2CByte(TW88I2CAddress,REG4E1);
		WriteI2CByte(TW88I2CAddress, 0xFF, PAGE0_GENERAL);
		WriteI2CByte(TW88I2CAddress, REG002, 0xff );			//clear	
		while((ReadI2CByte(TW88I2CAddress,REG002) & 0x40) ==0);	//wait vblank  I2C_WAIT_VBLANK
	 	WriteI2CByte(TW88I2CAddress, 0xFF, PAGE4_CLOCK);

		SPI_Buffer[0] = shadow_r4e0 & 0xFE;	 					//select PCLK
		SPI_Buffer[1] =	0x20 | 2; 								//divider 2
		WriteI2C(TW88I2CAddress,REG4E0,SPI_Buffer,2); 			//same:McuSpiClkToPclk(CLKPLL_DIV_2P0)
	
		WriteI2CByte(TW88I2CAddress,REG4C4, 0x01); 				//DMA READ start
		SPI_Buffer[0] = shadow_r4e0;	 						//save the restore value
		SPI_Buffer[1] =	shadow_r4e1; 							//save the restore value
		while(ReadI2CByte(TW88I2CAddress,REG4C4) & 0x01);		//SpiFlashDmaWait
		WriteI2C(TW88I2CAddress,REG4E0,SPI_Buffer,2);			//restore clk. same:McuSpiClkRestore 

		WriteI2CByte(TW88I2CAddress,REG4C1, reg4c1);
		//assume page4
		//==========================================
		// time critical area end
		//==========================================
	}


	//----------------------------
	//update RLE & pixel alpha

	WriteTW88Page(PAGE4_SOSD );
	if(SOsdHwBuff_rle_win) {
		WriteTW88(REG404, ReadTW88(REG404) | 0x01);
		WriteTW88(REG405, ((SOsdHwBuff_rle_bpp==7?8:SOsdHwBuff_rle_bpp) << 4) | (SOsdHwBuff_rle_count));
		WriteTW88(REG406, SOsdHwBuff_rle_win);
	}
	else {
		WriteTW88(REG404, ReadTW88(REG404) & 0xFE);
		WriteTW88(REG405, 0);
		WriteTW88(REG406, 0);
	}

	if(SOsdHwBuff_alpha != 0xFFFF) {
		WriteTW88(REG410, 0xc3 );    		// LUT Write Mode, En & byte ptr inc.
		if(SOsdHwBuff_alpha >> 8)	WriteTW88(REG410, ReadTW88(REG410) | 0x08);	//support 512 palette
		else            			WriteTW88(REG410, ReadTW88(REG410) & 0xF7);
		WriteTW88(REG411, (BYTE)SOsdHwBuff_alpha ); 	// alpha index
		WriteTW88(REG412, 0x7F/*value*/ ); 			// alpha value
	}

	//----------------------------
	//update WIN buffer

	//note: I update only win1 to win8, not win0.
	//      Pls. do not use win0 here.
	//start = 1+1;
	//end = 8+1;
	data_p = &SpiWinBuff[ 2 /*start*/ << 4];
	for(i=2/*start*/; i <= 9/*end*/; i++) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
		reg_i = (i << 4) + 0x20;
#else
		reg_i = 0x400 | (i << 4) + 0x20;
#endif
		for(j=0; j < 16; j++) {
			WriteTW88(reg_i++, *data_p++);	
		}
	}
	//-----------------------------
	// time cirtical section end
	//-----------------------------
#ifndef MODEL_TW8835_EXTI2C
//SFR_EA = 1;
#endif
}
#endif //..MODEL_TW8835_EXTI2C


#ifndef MODEL_TW8835_EXTI2C
void SOsdHwBuffWrite2Hw(void)
{
	BYTE win;
	DATA BYTE XDATA *data_p;
	DATA BYTE i,j;
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	DATA BYTE reg_i;
#else
	DATA WORD reg_i;
#endif
#ifdef DEBUG_SPIFLASH_TEST
	BYTE B0, B;	 //bugrate random. => Pls. use PCLK. 
#endif
	WORD LutOffset;

#ifdef DEBUG_OSD
	dPuts("\nSOsdHwBuffWrite2Hw.....");
#endif

	WaitVBlank(1);	

	//-----------------------------
	// time cirtical section start
	//-----------------------------
#ifndef MODEL_TW8835_EXTI2C
//SFR_EA = 0;
#endif
	//save clock mode & select PCLK
#if defined(MODEL_TW8836FPGA) || defined(MODEL_TW8835FPGA)
	//BK120615. for test clock source
#else
	McuSpiClkToPclk(CLKPLL_DIV_2P0);	//with divider 1=1.5(72MHz)	try 2
#endif

#ifdef DEBUG_SPIFLASH_TEST
	//to fix LUT0 B0 debug.
	WriteTW88Page(PAGE4_SOSD );
	WriteTW88(REG410,0xa0);		//read B
	WriteTW88(REG411,0);		//addr 0

	//read twice
	B0=ReadTW88(REG412);		
	B0=ReadTW88(REG412);
#endif

	//----------------------------
	//update LUT
	WriteTW88Page(PAGE4_SOSD );
	WriteTW88(REG410, 0xa0 );    			// LUT Write Mode, En & byte ptr inc. DMA needs it.
	WriteTW88(REG411, 0); 					// LUT addr. set 0 on DMA


	for(win=0; win <= 8; win++) {
		//check Use flag.
		if(SOsdHwBuff_win[win*8] == 0) 
			continue;
		data_p = &SOsdHwBuff_win[win*8+1];

		//Spi Flash DMA
#ifdef FAST_SPIFLASH
		WriteTW88(REG4C0_SPIBASE+0x04, 0x00 );	// DMA stop

		WriteTW88(REG4C0_SPIBASE+0x03, 0x80 | SPICMD_x_BYTES ); //LUT,Increase, 0x0B with 5 commands, 0xeb with 7 commands	           
		WriteTW88(REG4C0_SPIBASE+0x0a, SPICMD_x_READ ); 			// SPI Command=R
#else
		SpiFlashDmaStop();	
		SpiFlashCmdRead(DMA_DEST_SOSD_LUT);		
#endif

		//WriteTW88(REG4C0_SPIBASE+0x1a, 0x00 ); // DMA size
		WriteTW88(REG4C0_SPIBASE+0x08, *data_p++ );	//size0
		WriteTW88(REG4C0_SPIBASE+0x09, *data_p++ );	//size1
		LutOffset = *data_p;
		LutOffset <<= 6;

		WriteTW88(REG4C0_SPIBASE+0x06, *data_p++ ); 	//LutOffset[8:6] -> R4C6[2:0]
		LutOffset |= (*data_p & 0x3F);
		WriteTW88(REG4C0_SPIBASE+0x07, *data_p++ );		//LutOffset[5:0] -> R4C7[7:2] 
		WriteTW88(REG4C0_SPIBASE+0x0b, *data_p++); 		//address0
		WriteTW88(REG4C0_SPIBASE+0x0c, *data_p++ );		//address1
		WriteTW88(REG4C0_SPIBASE+0x0d, *data_p++ ); 	//address2

#ifdef MODEL_TW8835_EXTI2C
		SpiFlashDmaStart(SPIDMA_READ, 0 /*SPIDMA_BUSYCHECK*/,__LINE__);
#else
		WriteTW88(REG4C0_SPIBASE+0x04, 0x01 ); 			// DMA Start
		//while(ReadTW88Page() != PAGE4_SPI);			//trick. check DONE. BusyWait
#endif

#ifdef DEBUG_SPIFLASH_TEST
		//readback LUT0 B0 value
		WriteTW88(REG410,0xa0);	//read B
		WriteTW88(REG411,0);		//addr 0
		//read twice
		B=ReadTW88(REG412);		
		B=ReadTW88(REG412);		
		if(LutOffset) {
			if(B0 != B) {
				//WriteTW88(REG410, 0x80 );	//LUTADDR[8] will be same
				WriteTW88(REG411, 0);			//addr 0
//BK110809		WriteTW88(REG412, B0);	//overwrite
//SFR_EA = 1;
#ifdef DEBUG_OSD
				ePrintf("\n***BUGBUG*** win%bd %bx->%bx",win, B,B0); //--pls, use without EA
#endif
//SFR_EA = 0;
			}
		}
		else {
			B0 = B;
		}
#endif
	}

#if defined(MODEL_TW8836FPGA) || defined(MODEL_TW8835FPGA)
	//BK120615. for test clock source
#else
	//restore clock mode
	McuSpiClkRestore();
#endif

	//----------------------------
	//update RLE & pixel alpha

	WriteTW88Page(PAGE4_SOSD );
	if(SOsdHwBuff_rle_win) {
		WriteTW88(REG404, ReadTW88(REG404) | 0x01);
		WriteTW88(REG405, ((SOsdHwBuff_rle_bpp==7?8:SOsdHwBuff_rle_bpp) << 4) | (SOsdHwBuff_rle_count));
		WriteTW88(REG406, SOsdHwBuff_rle_win);
	}
	else {
		WriteTW88(REG404, ReadTW88(REG404) & 0xFE);
		WriteTW88(REG405, 0);
		WriteTW88(REG406, 0);
	}

	if(SOsdHwBuff_alpha != 0xFFFF) {
		WriteTW88(REG410, 0xc3 );    		// LUT Write Mode, En & byte ptr inc.
		if(SOsdHwBuff_alpha >> 8)	WriteTW88(REG410, ReadTW88(REG410) | 0x08);	//support 512 palette
		else            			WriteTW88(REG410, ReadTW88(REG410) & 0xF7);
		WriteTW88(REG411, (BYTE)SOsdHwBuff_alpha ); 	// alpha index
		WriteTW88(REG412, 0x7F/*value*/ ); 			// alpha value
	}

	//----------------------------
	//update WIN buffer

	//note: I update only win1 to win8, not win0.
	//      Pls. do not use win0 here.
	//start = 1+1;
	//end = 8+1;
	data_p = &SpiWinBuff[ 2 /*start*/ << 4];
	for(i=2/*start*/; i <= 9/*end*/; i++) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
		reg_i = (i << 4) + 0x20;
#else
		reg_i = 0x400 | (i << 4) + 0x20;
#endif
		for(j=0; j < 16; j++) {
			WriteTW88(reg_i++, *data_p++);	
		}
	}
	//-----------------------------
	// time cirtical section end
	//-----------------------------
#ifndef MODEL_TW8835_EXTI2C
//SFR_EA = 1;
#endif
}
#endif //..MODEL_TW8835_EXTI2C



/**
* download LUT
*
* NOTE BK110330:after we support 512 palette, we donot support the address method.
* We need a width and a height info. but RTL only supports a size info.
* So, if you want to use the address method, use a PIO method.
*
* NOTE Do not add WaitVBlank() here
*
* @param type	
*	- 1:Byte pointer - LUTS type
*	- 0:Address pointer - LUT type
*	if 0, use LutOffset:0, size:0x400.
* @see SpiOsdIoLoadLUT
* @see McuSpiClkToPclk
* @see McuSpiClkRestore
* @see SpiFlashDmaStart
*/
#ifdef MODEL_TW8835_EXTI2C
void SpiOsdLoadLUT(BYTE winno, BYTE type, WORD LutOffset, WORD size, DWORD address)
{
	BYTE reg;
	BYTE reg4c1;
	WORD ii;
	volatile BYTE vdata;

	reg = winno;	//skip warning

	if(type==0) {
		SpiOsdIoLoadLUT(type,0,0x400,address);
		return;
	}	
#ifdef DEBUG_SPI
	dPrintf("\nSpiOsdLoadLUT%s LutLoc:0x%x size:%d 0x%06lx", type ? "S":" ", LutOffset, size, address);
#endif

	//save clock mode & select PCLK	
#ifdef MODEL_TW8835_EXTI2C
//BK120619 test
#else
	McuSpiClkToPclk(CLKPLL_DIV_2P0);	//with divider 1=1.5(72MHz)	try 2
#endif

	WriteTW88Page(PAGE4_SOSD);

	//--- SPI-OSD config
	reg = 0;	//ReadTW88(REG410);
	if(LutOffset & 0x100) reg = 0x08;
	reg |= 0x80;									// LUT Write Mode.
	if(type==0)	reg |= 0x40;						// address ptr inc						
	else		reg |= 0x20;						// byte ptr inc.
	if(LutOffset > 0xff)   							
		reg |= 0x08;								
	WriteTW88(REG410, reg);
	WriteTW88(REG411, (BYTE)LutOffset ); 			// LUT addr. set 0 on DMA


	//Spi Flash DMA
//!#ifdef FAST_SPIFLASH
//??	WriteTW88(REG4C0_SPIBASE+0x04, 0x00 );	// DMA stop	

	WriteTW88(REG4C0_SPIBASE+0x03, 0x80 | 7/*SPICMD_x_BYTES*/ ); //LUT,Increase, 0x0B with 5 commands, 0xeb with 7 commands	           
	WriteTW88(REG4C0_SPIBASE+0x0a, 0xEB/*SPICMD_x_READ*/ ); 			// SPI Command=R

	WriteTW88(REG4C0_SPIBASE+0x0b, (BYTE)(address>>16) ); 	// SPI Addr
	WriteTW88(REG4C0_SPIBASE+0x0c, (BYTE)(address>>8) );
	WriteTW88(REG4C0_SPIBASE+0x0d, (BYTE)(address) ); 		//////00

	//d		h		addr	 addr  byte
	//0    0x00     0x000		0	0 	
	//128  0x80		0x200	   80   0
	//192  0xC0		0x300	   c0   0

	//if use byte ptr inc.
	WriteTW88(REG4C0_SPIBASE+0x06, (BYTE)(LutOffset >> 6) ); 	//LutOffset[8:6] -> R4C6[2:0]
	WriteTW88(REG4C0_SPIBASE+0x07, (BYTE)(LutOffset << 2) );	//LutOffset[5:0] -> R4C7[7:2] 
		                                                        	//					R4C7[1:0]  start of byteptr

	WriteTW88(REG4C0_SPIBASE+0x1a, 0x00 ); // DMA size
	WriteTW88(REG4C0_SPIBASE+0x08, (BYTE)(size >> 8) );
	WriteTW88(REG4C0_SPIBASE+0x09, (BYTE)size );


	//==========================================
	// time critical area start
	//==========================================
	reg4c1 = ReadI2CByte(TW88I2CAddress,REG4C1);
	WriteI2CByte(TW88I2CAddress,REG4C1, reg4c1 & ~1);

	shadow_r4e0 = ReadI2CByte(TW88I2CAddress,REG4E0);
	shadow_r4e1 = ReadI2CByte(TW88I2CAddress,REG4E1);
	WriteI2CByte(TW88I2CAddress, 0xFF, PAGE0_GENERAL);
Printf("\nSpiOsdLoadLUT %d",__LINE__);
	WriteI2CByte(TW88I2CAddress, REG002, 0xff );			//clear	
	while((ReadI2CByte(TW88I2CAddress,REG002) & 0x40) ==0);	//wait vblank  I2C_WAIT_VBLANK
PORT_DEBUG = 0;
 	WriteI2CByte(TW88I2CAddress, 0xFF, PAGE4_CLOCK);
															
	SPI_Buffer[0] = shadow_r4e0 & 0xFE;	 					//select PCLK
	SPI_Buffer[1] =	0x20 | 2; 								//divider 2
	WriteI2C(TW88I2CAddress,REG4E0,SPI_Buffer,2); 			//same:McuSpiClkToPclk(CLKPLL_DIV_2P0)

PORT_DEBUG = 1;
	WriteI2CByte(TW88I2CAddress,REG4C4, 0x01); 				//DMA READ start
PORT_DEBUG = 0;
	SPI_Buffer[0] = shadow_r4e0;	 						//save the restore value
	SPI_Buffer[1] =	shadow_r4e1; 							//save the restore value
	//while(ReadI2CByte(TW88I2CAddress,REG4C4) & 0x01);		//SpiFlashDmaWait
	for(ii=0; ii<500; ii++) {
		vdata = ReadI2CByte(TW88I2CAddress,REG4C4);
		if((vdata & 0x01)==0)
			break;
	}
	if(ii==500) 
		Printf("\n=====DMA FAIL!!!"); 
		
PORT_DEBUG = 1;
Printf("\nSpiOsdLoadLUT %d",__LINE__);
	WriteI2C(TW88I2CAddress,REG4E0,SPI_Buffer,2);			//restore clk. same:McuSpiClkRestore 

	WriteI2CByte(TW88I2CAddress,REG4C1, reg4c1);
	//assume page4
	//==========================================
	// time critical area end
	//==========================================

#ifdef DEBUG_SPI
	dPrintf("\nSpiOsdLoadLUT --END");
#endif

}
#endif //..MODEL_TW8835_EXTI2C


#ifndef MODEL_TW8835_EXTI2C
//=====================================
void SpiOsdLoadLUT(BYTE winno, BYTE type, WORD LutOffset, WORD size, DWORD address)
{
#ifdef DEBUG_SPIFLASH_TEST
	volatile BYTE B0,B;
#endif
	//BYTE win_lut_debug;
	BYTE reg;
//	BYTE SPICMD_x_READ = 0xEB;
//	BYTE SPICMD_x_BYTES = 0x07;


#ifndef MODEL_TW8836
	reg = winno;	//skip warning
#endif

	if(type==0) {
#ifdef DEBUG_SPI
		dPrintf("\nSpiOsdLoadLUT convert LutOffset:%d->0, LutSize:0x%03x->0x400",LutOffset,size);
#endif
		SpiOsdIoLoadLUT(type,0,0x400,address);
		return;
	}	
#ifdef DEBUG_SPI
	dPrintf("\nSpiOsdLoadLUT%s LutLoc:0x%x size:%d 0x%06lx", type ? "S":" ", LutOffset, size, address);
#endif

	//save clock mode & select PCLK	
#ifdef MODEL_TW8836FPGA
//BK120619 test
#else
	McuSpiClkToPclk(CLKPLL_DIV_2P0);	//with divider 1=1.5(72MHz)	try 2
#endif

#ifdef DEBUG_SPIFLASH_TEST
	//win_lut_debug = 0;
	if(LutOffset) {
		WriteTW88Page(PAGE4_SOSD );
		WriteTW88(REG410,0x80/*0xa0*/);	//read B
		WriteTW88(REG411,0);		//addr 0
		//read twice
		B0=ReadTW88(REG412);		
		B0=(volatile)ReadTW88(REG412);		
		//win_lut_debug = 1;
	}
//EA = 0;
//P1_3 =1;
//P1_4 = !P1_4;
#endif
	WriteTW88Page(PAGE4_SOSD);

	//--- SPI-OSD config
	reg = 0;	//ReadTW88(REG410);
	if(LutOffset & 0x100) reg = 0x08;
	reg |= 0x80;									// LUT Write Mode.
	if(type==0)	reg |= 0x40;						// address ptr inc						
	else		reg |= 0x20;						// byte ptr inc.
#ifdef MODEL_TW8836
	if(winno == 1 || winno == 2)					// if win1 or win2, 
		reg |= 0x04;								//	select group B LUT
#endif
	if(LutOffset > 0xff)   							
		reg |= 0x08;								
	WriteTW88(REG410, reg);
	WriteTW88(REG411, (BYTE)LutOffset ); 			// LUT addr. set 0 on DMA


	//Spi Flash DMA
#ifdef FAST_SPIFLASH
	WriteTW88(REG4C0_SPIBASE+0x04, 0x00 );	// DMA stop	

	WriteTW88(REG4C0_SPIBASE+0x03, 0x80 | SPICMD_x_BYTES ); //LUT,Increase, 0x0B with 5 commands, 0xeb with 7 commands	           
	WriteTW88(REG4C0_SPIBASE+0x0a, SPICMD_x_READ ); 			// SPI Command=R

	WriteTW88(REG4C0_SPIBASE+0x0b, (BYTE)(address>>16) ); 	// SPI Addr
	WriteTW88(REG4C0_SPIBASE+0x0c, (BYTE)(address>>8) );
	WriteTW88(REG4C0_SPIBASE+0x0d, (BYTE)(address) ); 		//////00

	//d		h		addr	 addr  byte
	//0    0x00     0x000		0	0 	
	//128  0x80		0x200	   80   0
	//192  0xC0		0x300	   c0   0
	if(type==0) {
		//if use addrss ptr inc.
		//addr_ptr = LutOffset;
		//byte_ptr  0;
		WriteTW88(REG4C0_SPIBASE+0x06, (BYTE)(LutOffset >> 8));	//LutOffset[8]  ->R4C6[0]
		WriteTW88(REG4C0_SPIBASE+0x07, (BYTE)LutOffset);			//LutOffset[7:0]->R4C7[7:0]
		
	}
	else {
		//if use byte ptr inc.
		WriteTW88(REG4C0_SPIBASE+0x06, (BYTE)(LutOffset >> 6) ); 	//LutOffset[8:6] -> R4C6[2:0]
		WriteTW88(REG4C0_SPIBASE+0x07, (BYTE)(LutOffset << 2) );	//LutOffset[5:0] -> R4C7[7:2] 
		                                                        	//					R4C7[1:0]  start of byteptr
	}

	WriteTW88(REG4C0_SPIBASE+0x1a, 0x00 ); // DMA size
	WriteTW88(REG4C0_SPIBASE+0x08, (BYTE)(size >> 8) );
	WriteTW88(REG4C0_SPIBASE+0x09, (BYTE)size );

#ifdef MODEL_TW8836
	if(winno==1 || winno==2) {
		WriteTW88(REG410, ReadTW88(REG410) | 0x04);	//indicate GROUP_B palette table. MAX size 256.
	}
#endif


#ifdef MODEL_TW8835_EXTI2C
	SpiFlashDmaStart(SPIDMA_READ, 0 /*SPIDMA_BUSYCHECK*/, __LINE__);
#else
	WriteTW88(REG4C0_SPIBASE+0x04, 0x01 ); // DMA Start
//P1_3 =0;
//P1_4 = !P1_4;
//	while(ReadTW88Page() != PAGE4_SPI);			//trick. check DONE. BusyWait
//EA = 1;
#endif
#else
	//================================================
	//
	//================================================
	SpiFlashDmaStop();
		
	SpiFlashCmdRead(DMA_DEST_SOSD_LUT);
	SpiFlashDmaFlashAddr(address);
	if(type==0)	SpiFlashDmaBuffAddr(LutOffset); 	//NOTE:Need 512x4 data
	else		SpiFlashDmaBuffAddr(LutOffset<<2);
	SpiFlashDmaReadLen(size);
//delay1ms(100);
	SpiFlashDmaStart(SPIDMA_READ,0/*SPIDMA_BUSYCHECK*/, __LINE__);	//use busycheck
//delay1ms(100);




#endif

	
#ifdef DEBUG_SPIFLASH_TEST
	if(LutOffset) {
		WriteTW88Page(PAGE4_SOSD );
		WriteTW88(REG410,0x80/*0xa0*/);	//read B
		WriteTW88(REG411,0);		//addr 0
		//read twice
		B=ReadTW88(REG412);		
		B=ReadTW88(REG412);	

		if(B0 != B) {
			WriteTW88(REG411, 0);			//addr 0
//BK110809			WriteTW88(REG412, B0);	//overwrite
#ifdef DEBUG_OSD
			ePrintf("\n***BUGBUG*** B0 %bx->%bx",B, B0); //--pls, use without EA
#endif
		}
	}
#endif


#ifdef DEBUG_SPI
	dPrintf("\nSpiOsdLoadLUT --END");
#endif

	//restore clock mode
#ifdef MODEL_TW8836FPGA
//BK120619 test
#else
	McuSpiClkRestore();
#endif
}
#endif //.. MODEL_TW8835_EXTI2C



//BKTODO: If you donot using alpha, disable alpha.
/**
* set alpha attribute
*/
void SpiOsdPixelAlphaAttr(WORD lutloc, BYTE value)
{
	WriteTW88Page(PAGE4_SOSD );

	//--- SPI-OSD config
	//WriteTW88(REG410, 0xc0 );    		// LUT Write Mode, En & address ptr inc.
	//WriteTW88(REG410, 0xa0 );    		// LUT Write Mode, En & byte ptr inc.
	WriteTW88(REG410, 0xc3 );    		// LUT Write Mode, En & byte ptr inc.
	if(lutloc >> 8)	WriteTW88(REG410, ReadTW88(REG410) | 0x08);	//support 512 palette
	else            WriteTW88(REG410, ReadTW88(REG410) & 0xF7);
	WriteTW88(REG411, (BYTE)lutloc ); // LUT addr
	WriteTW88(REG412, value ); // LUT addr

//	WriteTW88(REG411, 0 );    			// LUT addr
//	WriteTW88(REG4C0_SPIBASE+0x04, 0x01 ); // DMA Start
}



//-----------------------------------
// RLE functions
//
//-----------------------------------

/**
* set RLE register
* @param winno win number.
*		winno 0 means disable.
* @param dcnt Data BPP
*	- 4:4bit, 6:6bit, others:8bit
* @param ccnt counter value.
*	- 4:4bit,5:5bit,..15:16bit, others:16bit
*/
void SpiOsdRLC(BYTE winno,BYTE dcnt, BYTE ccnt)
{
	//7 means 8BPP with 128 color.
	if(dcnt==7)
		dcnt++;

	WriteTW88Page(PAGE4_SOSD );
#ifdef MODEL_TW8836
	if(winno==1 || winno==2) {
		WriteTW88(REG407, (dcnt << 4) | (ccnt));
		WriteTW88(REG406, (ReadTW88(REG406) & 0x0F) | (winno << 4));
	}
	else
	{
		WriteTW88(REG405, (dcnt << 4) | (ccnt));
		WriteTW88(REG404, (ReadTW88(REG404) & 0x0F) | (winno << 4));
	}
#else
	//TW8835
	WriteTW88(REG405, (dcnt << 4) | (ccnt));
	WriteTW88(REG406, winno);
#endif
}

/**
* reset RLC register
*/
void SpiOsdResetRLC(BYTE winno, BYTE reset)
{
#ifndef MODEL_TW8836
	BYTE temp = winno;
#endif

	WriteTW88Page(PAGE4_SOSD );
#ifdef MODEL_TW8836
	if(winno==1 || winno==2) {
		if(reset)	WriteTW88(REG406, ReadTW88(REG406) | 0x02);
		else		WriteTW88(REG406, ReadTW88(REG406) & 0xFD);
	}
	else 
#endif
	{
		if(reset)	WriteTW88(REG404, ReadTW88(REG404) | 0x02);
		else		WriteTW88(REG404, ReadTW88(REG404) & 0xFD);
	}
}

#if 0
//BKFYI:Removed on TW8835.
void SpiOsdEnableRLC(BYTE en)
{
	WriteTW88Page(PAGE4_SOSD );
	if(en)		WriteTW88(REG404, ReadTW88(REG404) | 0x01);
	else		WriteTW88(REG404, ReadTW88(REG404) & 0xFE);
}
#endif
/**
* disable RLC
* 
* win0 means disable
*/
void SpiOsdDisableRLC(BYTE winno)
{
#ifdef MODEL_TW8836
	WriteTW88Page(PAGE4_SOSD );
	if(winno==1 || winno==2) {
		WriteTW88(REG406, (ReadTW88(REG406) & 0x0F));
	}
	else
	{
		WriteTW88(REG404, (ReadTW88(REG404) & 0x0F));
	}
#else
	BYTE temp = winno;
	WriteTW88Page(PAGE4_SOSD );
	WriteTW88(REG406, 0);
#endif
}



#endif //..SUPPORT_SPIOSD






