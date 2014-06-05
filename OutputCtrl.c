/**
 * @file
 * OutputCtrl.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	video output module 
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
//#include "main.h"
#include "OutputCtrl.h"
#include "SOsd.h"
#include "FOsd.h"



//LEDC
//R0E0
 //==============================================================================
// void	LedBackLight( BYTE on )
//==============================================================================
/**
* control LEDC digital block
*/
void LedBackLight( BYTE on )
{
	WaitVBlank(1);
	WriteTW88Page(PAGE0_LEDC);
	if ( on ) {
		WriteTW88(REG0E0, ReadTW88(REG0E0 ) | 1 );
	}
	else {
		WriteTW88(REG0E0, ReadTW88(REG0E0 ) & ~0x01 );
	}
}


//=============================================================================
//	void	BlackScrren( BYTE on )
//=============================================================================
#ifdef UNCALLED_SEGMENT_CODE
void	BlackScreen( BYTE on )
{...}
#endif


//-----------------------------------------------------------------------------
/**
* LEDOn step
*/
BYTE LEDCOn(BYTE step)
{
	BYTE i;

	WriteTW88Page(PAGE0_LEDC);

	switch(step)
	{
	case 0:
		WriteTW88(REG0E0, 0x72);	//default. & disable OverVoltage
		WriteTW88(REG0E5, 0x80);	//LEDC digital output enable.
		WriteTW88(REG0E0, 0x12);	//Off OverCurrent. Disable Protection
		WriteTW88(REG0E0, 0x13);	//LEDC digital block enable
		break;
	case 1:
		WriteTW88(REG0E0, 0x11);	//Analog block powerup
		break;
	case 2:
		WriteTW88(REG0E0, 0x71);	//enable OverCurrent, enable Protection control
		break;
	//default:
	//	ePuts("\nBUG");
	//	return;
	}
	for(i=0; i < 10; i++) {
		if((ReadTW88(REG0E2) & 0x30)==0x30) {	//wait normal
			//dPrintf("\nLEDC(%bd):%bd",step,i);
			return ERR_SUCCESS;	//break;
		}
		delay1ms(2);
	}
	dPrintf("\nLEDC(%bd) FAIL",step);
	return ERR_FAIL;
}

#if defined(EVB_10)
//-----------------------------------------------------------------------------
void LEDCGpioOn(void)
{
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
#else
	WriteTW88Page(PAGE0_GPIO);
	WriteTW88(REG084, 0x0C);	//enable
	WriteTW88(REG08C, 0x0C);	//output enable
	delay1ms(2);
#endif
}
#endif

//-----------------------------------------------------------------------------
/**
* power up LED
*/
void LedPowerUp(void)
{
#ifdef EVB_10
	//EVB_20 does not need LEDCGpioOn(). But I am not sure on EVB_10.
	LEDCGpioOn();
#endif

	LEDCOn(0);
	LEDCOn(1);
	LEDCOn(2);

	//WaitVBlank(1);
}

//-----------------------------------------------------------------------------
/**
* enable BT656 output
*/
void BT656OutputEnable(BYTE fOn, BYTE clear_port)
{
	DECLARE_LOCAL_page

	ReadTW88Page(page);
	WriteTW88Page(PAGE0_INPUT);
	if(fOn) {
		WriteTW88(REG007, ReadTW88(REG007) | 0x08);	 
	}
	else {
		WriteTW88(REG007, ReadTW88(REG007) & ~0x08);	//DataInitNTSC clear it. 
		//clear port
		if(clear_port) {
			if(P1_6 == 0)
				P1_6 = 1;
		}
	}
	WriteTW88Page(page);
}


//-----------------------------------------------------------------------------
/**
* enable Output pin
*
* DataOut need EnableOutputPin(ON,ON)
* target R008 = 0x89
*/
void OutputEnablePin(BYTE fFPDataPin, BYTE fOutputPin)
{
	BYTE value;

	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG008, 0x80 | (ReadTW88(REG008) & 0x0F));	//Output enable......BUGBUG
	value = ReadTW88(REG008) & ~0x30;
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	if(fFPDataPin==0)
		Puts("\nskip fFPDataPin=0");
#else
	if(fFPDataPin==0) 		value |= 0x20;
#endif
	if(fOutputPin==0)		value |= 0x10;
	WriteTW88(REG008,  value);
}
