/**
 * @file
 * misc.c 
 * @author Harry Han
 * @author YoungHwan Bae
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	main file
 * @section DESCRIPTION
 *	developer code
 ******************************************************************************
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
#include "SPI.h"

#include "main.h"
#include "misc.h"

#include "SOsd.h"
#include "FOsd.h"
#include "Scaler.h"
#include "InputCtrl.h"
#include "EEPROM.h"
#include "ImageCtrl.h"
#include "decoder.h"
#include "InputCtrl.h"
#include "OutputCtrl.h"
#include "Settings.h"
#include "measure.h"

#include "SOsdMenu.h"

//=============================================================================
//	CHIP_MANUAL_TEST		                                               
//=============================================================================

#ifdef CHIP_MANUAL_TEST
extern void TestUpper256Char(void);

void ChipManualTest_InitPart(void)
{	
	DCDC_StartUP();

	FontOsdInit();
	FOsdSetDeValue();


	TestUpper256Char();

	InitLogo1();
	LedPowerUp();
	RemoveLogoWithWait(1);

	//recover FOSD
	FontOsdInit();
	FOsdSetDeValue();
	FOsdWinEnable(0, OFF);	//win0, disable..
}

#ifdef SUPPORT_BT656
extern void InitBT656_Encoder(void);
#endif
extern XDATA BYTE Task_NoSignal_cmd;		//DONE,WAIT_VIDEO,WAIT,RUN,RUN_FORCE

BYTE InitSystemForChipTest(BYTE fPowerUpBoot)
{
	BYTE ee_mode;
	BYTE value;
	BYTE FirstInitDone;

	if(access==0) {
		//do nothing.
		return 0;
	}

	//check EEPROM
	ee_mode = CheckEEPROM();
	if(ee_mode==1) {
		//---------- if FW version is not matched, initialize EEPROM data -----------
		InitWithNTSC();
		
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

	ePrintf("\nInitSystem(%bd)",fPowerUpBoot);



	//set default setting.
	if(fPowerUpBoot) {

		//Init HW with default
		InitWithNTSC();

		//------------------
		//first GPIO position
		FP_GpioDefault();

		SSPLL_PowerUp(ON);
		//DCDC data out needs 200ms.
		PrintSystemClockMsg("SSPLL_PowerUp");
	}

	ChipManualTest_InitPart();
	FirstInitDone = 2;	

	DumpClock(0);
	//------------------------
	//start with saved input
	//------------------------
	//++ StartVideoInput();
	//start from CVBS
	InputMain=INPUT_CVBS;
	LinkCheckAndSetInput();		//link CheckAndSetInput
	
	InputMain = 0xff;			// start with saved input
	ChangeCVBS();

	PrintSystemClockMsg("StartVideoInput");

	//
	//Logo and LedPowetUp
	//
	if(FirstInitDone ==0) {
		InitLogo1();
		FirstInitDone =1;
	}

	LedPowerUp();

#ifdef SUPPORT_BT656
	//enable BT656 output encoder
	if(fPowerUpBoot)
		InitBT656_Encoder();
#endif

	//remove InitLogo
	if(FirstInitDone ==1) {
		FirstInitDone = 2;
		RemoveLogoWithWait(1);
		if(Task_NoSignal_cmd == TASK_CMD_DONE)
			FOsdWinEnable(0, OFF);	//win0, disable..
	}	

	//------------------------
	// setup eeprom effect
	//------------------------
	SetAspectHW(GetAspectModeEE());
	value = EE_Read(EEP_FLIP);	//mirror
	if(value) {
		WriteTW88Page(PAGE2_SCALER);
	    WriteTW88(REG201, ReadTW88(REG201) | 0x80);
	}
	OsdSetTime(EE_Read(EEP_OSD_TIMEOUT));


	OsdSetTransRate(EE_Read(EEP_OSD_TRANSPARENCY));
	BackLightSetRate(EE_Read(EEP_BACKLIGHT));
	//set the Error Tolerance value for "En Changed Detection"
	MeasSetErrTolerance(0x04);		//tolerance set to 32

	UpdateOsdTimerClock();
	//dPrintf("\nOsdTimerClock:%ld",OsdTimerClock);

	//dPuts("\nInitSystem-END");

	FOsdSetDeValue();	//??

	return 0;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void Chip_Manual_Test(void)
{
	BYTE	ch;

	Puts("\nChip Manual Test with CVBS, DTV, PC");
	ChangeCVBS();

	Puts("\nCVBS displayed...Press ANY key for Digital Input");
	while ( !RS_ready() ) ;
	ch = RS_rx();
#ifdef SUPPORT_DVI
	ChangeDVI();
	Puts("\nDigital Input displayed...Press ANY key for PC(aRGB)");
	while ( !RS_ready() ) ;
	ch = RS_rx();
#endif
	ChangePC();

#if defined(EVB_20) || defined(EVB_21) || defined(EVB_30) || defined(EVB_31)
	Puts("\nPC (RGB)displayed...Press ANY key for BT656");
	while ( !RS_ready() ) ;
	ch = RS_rx();
	ChangeBT656();
#endif
	Puts("\nInput Test Finished");

//??	WriteTW88Page(PAGE0_GENERAL);
//??	WriteTW88(REG0D6, ReadTW88(REG0D6) & ~0x70);	//clear TCLK output dely value
}

#endif	//..CHIP_MANUAL_TEST
