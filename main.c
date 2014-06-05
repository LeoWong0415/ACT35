/**
 * @file
 * main.c 
 * @author Harry Han
 * @author YoungHwan Bae
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	main file
 * @section DESCRIPTION
 *	- CPU : DP8051
 *	- Language: Keil C
 *  - See 'Release.txt' for firmware revision history 
 ******************************************************************************
 */

#include <intrins.h>
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

#if defined(MODEL_TW8835_EXTI2C)
#include "host.h"
#endif
#include "main.h"
#include "misc.h"
#include "Remo.h"
#include "TouchKey.h"
#include "eeprom.h"

#include "Settings.h"
#include "InputCtrl.h"
#include "decoder.h"
#include "Scaler.h"
#include "ImageCtrl.h"
#include "OutputCtrl.h"
#include "VAdc.h"
#include "DTV.h"
#include "measure.h"

#ifdef SUPPORT_HDMI_SiIRX
#include <hal_cp9223.h>
#include <SiI_config.h>
#include <amf.h>
#include <infofrm.h>
#include <registers.h>
#include <debug_cmd.h>
#include <CEC.h>
extern void PollPortSwitch(void);
extern void HdmiTask(void);
#endif
#ifdef SUPPORT_HDMI_EP9351
#include "hdmi_ep9351.h"
#endif

#include "SOsd.h"
#include "FOsd.h"
#include "SpiFlashMap.h"
#include "SOsdMenu.h"
#include "Demo.h"
#include "Debug.H"

//-----------------------------------------------------------------------------
/**
* "system no initialize mode" global variable.
*
* If P1_5 is connected at the PowerUpBoot, 
*   it is a system no init mode (SYS_MODE_NOINIT).
* If the system is a SYS_MODE_NOINIT, 
*  FW will skips the initialize routine, 
*  and supports only the Monitor function.
*  and SYS_MODE_NOINIT can not support a RCDMode and a PowerSaveMode.
* But, if the system bootup with normal, 
*  the P1_5 will be worked as a PowerSave ON/OFF switch.
*/
//-----------------------------------------------------------------------------
BYTE SysNoInitMode;

//-----------------------------------------------------------------------------
// Interrupt Handling Routine Variables			                                               
//-----------------------------------------------------------------------------
WORD main_VH_Loss_Changed;
BYTE main_INT_STATUS;
BYTE main_INT_STATUS2;
BYTE main_INT_STATUS3;	//for SW 7FF. ext4 intr
BYTE SW_Video_Status;
BYTE SW_INTR_cmd;
#define SW_INTR_VIDEO_CHANGED	1

//-----------------------------------------------------------------------------
// I2CCMD 
//-----------------------------------------------------------------------------
#if defined(SUPPORT_I2CCMD_SLAVE_V1)
bit F_i2ccmd_exec=0;				/*!< I2CCMD flag */
#define I2CCMD_CHECK	0x20
#define I2CCMD_EXEC		0x10
#endif


#if defined(SUPPORT_I2CCMD_TEST_SLAVE)
BYTE ext_i2c_cmd;
#endif

#if defined(MODEL_TW8835_SLAVE) && defined(SUPPORT_I2CCMD_TEST)
BYTE i2c_compare_page = 0xff;
BYTE i2c_compare_buff[256];
//-----------------------------------------------------------------------------
/**
* test routine.
*/
void test_set_i2c_slave_compare_page(BYTE test_page)
{
	WORD i;
	i2c_compare_page = test_page;
	for(i=0; i <= 255; i++) {
		i2c_compare_buff[i] = ReadTW88(test_page << 8 | i);
	}
	Printf("\nsave page %bd",test_page);
}
#endif


//=============================================================================
// Video TASK ROUTINES				                                               
//=============================================================================

//-----------------------------------------------------------------------------
// Task NoSignal
//-----------------------------------------------------------------------------
#define TASK_FOSD_WIN	0
#define NOSIGNAL_TIME_INTERVAL	(10*100)

void NoSignalTask( void );			//prototype
void NoSignalTaskOnWaitMode(void);
XDATA BYTE Task_NoSignal_cmd;		//DONE,WAIT_VIDEO,WAIT,RUN,RUN_FORCE
XDATA BYTE Task_NoSignal_count;		//for dPuts("\nTask NoSignal TASK_CMD_WAIT_VIDEO");

//-----------------------------------------------------------------------------
/**
* set NoSignalTask status
*		
* @param  cmd
*	- TASK_CMD_DONE
*	- TASK_CMD_WAIT_VIDEO
*	- TASK_CMD_WAIT_MODE
*	- TASK_CMD_RUN
*	- TASK_CMD_RUN_FORCE
*/
void TaskNoSignal_setCmd(BYTE cmd) 
{ 	
	if(cmd == TASK_CMD_WAIT_VIDEO && MenuGetLevel())	
		Task_NoSignal_cmd = TASK_CMD_DONE;	
	else
		Task_NoSignal_cmd = cmd;

	if(cmd == TASK_CMD_RUN_FORCE)
		tic_task = NOSIGNAL_TIME_INTERVAL;	//right now

	Task_NoSignal_count = 0;
}																				
//-----------------------------------------------------------------------------
/**
* get NoSignalTask status
*
* @return Task_NoSignal_cmd
*/
BYTE TaskNoSignal_getCmd(void) 
{ 	
	return Task_NoSignal_cmd;	
}

//=============================================================================
// MovingGrid TASK ROUTINES				                                               
//=============================================================================
extern void MovingGridTask( void );
XDATA BYTE Task_Grid_on;	   //Pls, use a friend function
XDATA BYTE Task_Grid_cmd;

//-----------------------------------------------------------------------------
/**
 * on/off Grid task
 *
 * @param onoff
*/
void TaskSetGrid(BYTE onoff)  {	Task_Grid_on = onoff;	}	
//-----------------------------------------------------------------------------
/**
 * get Grid task status
 *
 * @return Task_Grid_on
*/
BYTE TaskGetGrid(void)		  {	return Task_Grid_on;    }
//-----------------------------------------------------------------------------
/**
 * set Grid task command
 *
 * @param cmd
*/
void TaskSetGridCmd(BYTE cmd) { Task_Grid_cmd = cmd;	}	
//-----------------------------------------------------------------------------
/**
 * get Grid task command
 *
 * @return Task_Grid_cmd
*/
BYTE TaskGetGridCmd(void)	  { return Task_Grid_cmd;   } 


//=============================================================================
// CheckAndSet LINK ROUTINES				                                               
//=============================================================================

//-----------------------------------------------------------------------------
/**
 * function pointer for CheckAndSetInput
 *
*/
BYTE (*CheckAndSetInput)(void);

//-----------------------------------------------------------------------------
/**
 * dummy CheckAndSet function
 *
*/
BYTE CheckAndSetUnknown(void)
{
	return ERR_FAIL;
}

//-----------------------------------------------------------------------------
/**
 * link CheckAndSetInput Routine
 *
 * @see CheckAndSetDecoderScaler
 * @see CheckAndSetComponent
 * @see CheckAndSetPC
 * @see CheckAndSetDVI
 * @see CheckAndSetHDMI
 * @see CheckAndSetBT656
 * @see CheckAndSetUnknown
*/
void LinkCheckAndSetInput(void)
{
	switch(InputMain) {
#if defined(SUPPORT_CVBS) || defined(SUPPORT_SVIDEO)
	case INPUT_CVBS:
	case INPUT_SVIDEO:
		CheckAndSetInput = &CheckAndSetDecoderScaler;
		break;
#endif
#ifdef SUPPORT_COMPONENT
	case INPUT_COMP:
		CheckAndSetInput = &CheckAndSetComponent;
		break;
#endif
#ifdef SUPPORT_PC
	case INPUT_PC:
		CheckAndSetInput = &CheckAndSetPC;
		break;
#endif
#ifdef SUPPORT_DVI
	case INPUT_DVI:
		CheckAndSetInput = &CheckAndSetDVI;
		break;
#endif
#if defined(SUPPORT_HDMI_EP9351) || defined(SUPPORT_HDMI_SiIRX)
	case INPUT_HDMIPC:
	case INPUT_HDMITV:
		CheckAndSetInput = &CheckAndSetHDMI;
		break;
#endif
#ifdef SUPPORT_BT656
	case INPUT_BT656:
		CheckAndSetInput = &CheckAndSetBT656;
		break;
#endif
	default:
		CheckAndSetInput = &CheckAndSetUnknown;
		break;
	}
	SW_Video_Status = 0;					//clear
	FOsdWinEnable(TASK_FOSD_WIN,OFF);		//WIN0, Disable
	TaskNoSignal_setCmd(TASK_CMD_DONE);		//turn off NoSignal Task
}



#ifdef SUPPORT_RCD
//-----------------------------------------------------------------------------
/**
 * check RCD(Rear Camera Display) port
 *
 * @return 0:No, 1:Yes
*/
BYTE IsBackDrivePortOn(void)
{
	//BT656 uses P1_6. FW can not support RCD on BT656 mode.
	if(InputMain >= INPUT_DVI) 
		return 0;
	return (PORT_BACKDRIVE_MODE==0 ? 1 : 0);
}
#endif

//-----------------------------------------------------------------------------
/**
 * Update OSD Timer
 *
*/
void UpdateOsdTimerClock(void)
{
	DECLARE_LOCAL_page

	OsdTimerClock = (DWORD)OsdGetTime() *100;
	
	ReadTW88Page(page);

	//Turn On FontOSD.
	FOsdOnOff(ON, 0);	//with vdelay 0

	//BK120112
	if(Task_NoSignal_cmd != TASK_CMD_DONE) {
		if(getNoSignalLogoStatus()==0)
			InitLogo1();
	}

	WriteTW88Page(page);		
}

//-----------------------------------------------------------------------------
/**
 *	Check OSD Timer and clear OSD if timer is expired.
*/
void CheckAndClearOSD(void)
{
	DECLARE_LOCAL_page

	if(OsdGetTime()==0)
		return;

	if(OsdTimerClock==0) {
		ReadTW88Page(page);

		if(MenuGetLevel())	
			MenuEnd();	
		
		//Turn OFF Font OSD
		if(FOsdOnOff(OFF, 0))	//with vdelay 0
			dPuts("\nCheckAndClearOSD disable FOSD");
			
		if(getNoSignalLogoStatus())
			RemoveLogo();
					
		WriteTW88Page(page);
	}
}



//=============================================================================
// I2CCMD routines
//=============================================================================

#if defined(MODEL_TW8835_SLAVE)
//-----------------------------------------------------------------------------
/**
* stop internal MCU
*
* To resume, write REG0D4[1]=1 by I2C
*/
void StopCPU(void)
{
	Printf("\nGoto STOP mode");
	Printf("\nTo resume, write REG0D4[1]=1 by I2C");
	SFR_PCON |= 0x12;
	//----- need nop to clean up the 8051 pipeline.
	_nop_(); _nop_(); _nop_(); _nop_(); _nop_();
	_nop_(); _nop_(); _nop_(); _nop_(); _nop_();
	//----------------------------
	//to reboot, 
	//	write REG0D4[0]=1.
	//----------------------------		
}
#endif

#if defined(SUPPORT_I2CCMD_SLAVE_V1)
//-----------------------------------------------------------------------------
// I2CCMD proto type
//-----------------------------------------------------------------------------
BYTE I2CCMD_Read(void);
BYTE I2CCMD_Exec(void);
BYTE I2CCMD_Sfr(void);
BYTE I2CCmd_eeprom(void);
BYTE I2CCmd_key(void);

//-----------------------------------------------------------------------------
/**
 * I2CCMD main
 *
 * @ingroup I2CCMD
 * @see	I2CCMD_Read
 * @see	I2CCMD_Exec
 * @see	I2CCMD_Sfr
 * @see	I2CCmd_eeprom
 * @see	I2CCmd_key
*/
BYTE I2CCMD_exec_main(void)
{
	BYTE ret;
	//I2CCMD_REG0~I2CCMD_REG4

	Printf("\nI2CCMD %bx:%bx:%bx:%bx:%bx",
		ReadTW88(I2CCMD_REG0),
		ReadTW88(I2CCMD_REG1),
		ReadTW88(I2CCMD_REG2),
		ReadTW88(I2CCMD_REG3),
		ReadTW88(I2CCMD_REG4));

	switch(ReadTW88(I2CCMD_REG0)) {
	case 0:		ret = I2CCMD_Read();	break;
	case 1:		ret = I2CCMD_Exec();	break;
	case 2:		ret = I2CCMD_Sfr();		break;
	case 3:		ret = I2CCmd_eeprom();	break;
	case 4:		ret = I2CCmd_key();		break;
	default:	ret = ERR_FAIL;
	}
	if(ret!=ERR_SUCCESS) {	
		WriteTW88(REG009, 0xE0);
	}
	return ret;
}
//-----------------------------------------------------------------------------
/**
 * I2CCMD read commad
 *
 * @ingroup I2CCMD
 * @see	I2CCMD_exec_main
*/
BYTE I2CCMD_Read(void)
{
	BYTE cmd;
	BYTE ret;

	ret = ERR_FAIL;

	cmd = ReadTW88(I2CCMD_REG1);
	if(cmd==0) {
		WriteTW88(I2CCMD_REG4,InputMain);
		ret = ERR_SUCCESS;
	}
	else if(cmd==1) {
		WriteTW88(I2CCMD_REG4,MenuGetLevel());
		ret = ERR_SUCCESS;
	}
	else if(cmd==2) {
		WriteTW88(I2CCMD_REG4,DebugLevel);
		ret = ERR_SUCCESS;
	}
	else if(cmd==3) {
		WriteTW88(I2CCMD_REG4,access);
		ret = ERR_SUCCESS;
	}
	else if(cmd==4) {
		WriteTW88(I2CCMD_REG4,access);
		if(SFR_WDCON & 0x02) WriteTW88(I2CCMD_REG4,0x01);	//On
		else                 WriteTW88(I2CCMD_REG4,0x00);	//Off
		ret = ERR_SUCCESS;
	}
	else if(cmd==5) {
		WriteTW88(I2CCMD_REG3,(WORD)SPI_Buffer>>8);
		WriteTW88(I2CCMD_REG4,(BYTE)SPI_Buffer);
		ret = ERR_SUCCESS;
	}
	else if(cmd==6) {
		WriteTW88(I2CCMD_REG3,SPI_BUFFER_SIZE>>8);
		WriteTW88(I2CCMD_REG4,(BYTE)SPI_BUFFER_SIZE);
		ret = ERR_SUCCESS;
	}

	if(ret==ERR_SUCCESS)
		WriteTW88(REG009,0xD0);	//Done
	else
		WriteTW88(REG009,0xF0);	//FAIL

	return ret;
}
//-----------------------------------------------------------------------------
/**
 * I2CCMD EXE commad
 *
 * @ingroup I2CCMD
 * @see	I2CCMD_exec_main
*/
BYTE I2CCMD_Exec(void)
{
	BYTE cmd;
	BYTE ret;

	ret = ERR_FAIL;

	cmd = ReadTW88(I2CCMD_REG1);
	if(cmd < 0x0F) {
		//if(MenuGetLevel())
		//	MenuEnd();
		//if(SpiOsdIsOn())
		//	SpiOsdEnable(OFF);
		if(getNoSignalLogoStatus())
			RemoveLogo();

		//Change input
		switch(cmd) {
		case INPUT_CVBS:	ChangeCVBS();		ret=ERR_SUCCESS;	break;
		case INPUT_SVIDEO:	ChangeSVIDEO();		ret=ERR_SUCCESS;	break;
#ifdef SUPPORT_COMPONENT
		case INPUT_COMP:	ChangeCOMPONENT();	ret=ERR_SUCCESS;	break;
#endif
#ifdef SUPPORT_PC
		case INPUT_PC:		ChangePC();			ret=ERR_SUCCESS;	break;
#endif
#ifdef SUPPORT_DVI
		case INPUT_DVI:		ChangeDVI();		ret=ERR_SUCCESS;	break;
#endif
		case INPUT_HDMIPC:	
		case INPUT_HDMITV:	ChangeHDMI();		ret=ERR_SUCCESS;	break;
#ifdef SUPPORT_BT656
		case INPUT_BT656:	ChangeBT656();		ret=ERR_SUCCESS;	break;
#endif
		default: break;
		}		
	}
	else if(cmd==0x0F) {
		//if(MenuGetLevel())
		//	MenuEnd();
		//if(SpiOsdIsOn())
		//	SpiOsdEnable(OFF);

		InputModeNext();
		ret=ERR_SUCCESS;
	}
	else if(cmd==0x10) {
		ret = CheckAndSetInput();	
	}
	//FYI: if master changes an access as 0, we can not support i2ccmd anymore.
	//else if(cmd==0x11) {
	//	access = ReadTW88(I2CCMD_REG2);	
	//	ret=ERR_SUCCESS;
	//}
	else if(cmd==0x12) {
		DebugLevel = ReadTW88(I2CCMD_REG2);	
		ret=ERR_SUCCESS;	
	}
	else if(cmd==0x80) {
		WriteTW88(REG0D4, ReadTW88(REG0D4) | 0x01);
	}
#ifdef USE_EXTMCU_ISP_I2CCMD
	else if(cmd==0x8A) {
		//Write Done flag frist. Because, I will stop myself.
		WriteTW88(REG009,0xD0);	
		StopCPU();
		return 0;
	}
#endif
	else if(cmd==0x90) {
		DisableWatchdog();
		ret=ERR_SUCCESS;
	}
	else if(cmd==0x91) {
		EnableWatchdog(0);
		ret=ERR_SUCCESS;
	}

	if(ret==ERR_SUCCESS)
		WriteTW88(REG009,0xD0);	//Done
	else
		WriteTW88(REG009,0xF0);	//FAIL

	return ret;
}

//-----------------------------------------------------------------------------
/**
 * I2CCMD SFR commad
 *
 * @ingroup I2CCMD
 * @see	I2CCMD_exec_main
*/
BYTE I2CCMD_Sfr(void)
{
	BYTE index;
	BYTE value;
	BYTE ret;

	index = ReadTW88(I2CCMD_REG2);

	if(ReadTW88(I2CCMD_REG1)==1) {
		//write
		value = ReadTW88(I2CCMD_REG3);
		ret = WriteSFR(index, value);
	}
	else {
		//read
		ret = ReadSFR(index);
		WriteTW88(I2CCMD_REG4,ret);	//result
	}
	WriteTW88(REG009,0xD0);	//Done

	return ERR_SUCCESS;
}
//-----------------------------------------------------------------------------
/**
 * I2CCMD eeprom commad
 *
 * @ingroup I2CCMD
 * @see	I2CCMD_exec_main
*/
BYTE I2CCmd_eeprom(void)
{
	WORD addr;
	BYTE value;

	addr = ReadTW88(I2CCMD_REG2);	addr <<=8;
	addr |= ReadTW88(I2CCMD_REG3);

	if(ReadTW88(I2CCMD_REG1)==1) {
		//write
		value = ReadTW88(I2CCMD_REG4);
		EE_Write(addr,value);
	}
	else {
		//read
		value = EE_Read(addr);
		WriteTW88(I2CCMD_REG4,value);
	}
	WriteTW88(REG009,0xD0);	//Done

	return ERR_SUCCESS;
}

//-----------------------------------------------------------------------------
/**
 * I2CCMD Key commad
 *
 * @ingroup I2CCMD
 * @see	I2CCMD_exec_main
*/
BYTE I2CCmd_key(void)
{
	BYTE key;
	BYTE ret;

	ret = ERR_FAIL;
	key = ReadTW88(I2CCMD_REG1);

	if(key <= NAVI_KEY_RIGHT) {
		if(MenuGetLevel()) {
			MenuKeyInput(key);
			ret = ERR_SUCCESS;
		}
	}
	else if(key==0x06) {
		if(MenuGetLevel()==0) {
			MenuStart();
			ret = ERR_SUCCESS;
		}
	}
	else if(key==0x07) {
		if(MenuGetLevel()) {
			MenuEnd();
			ret = ERR_SUCCESS;
		}
	}
	else 
		ret = ERR_FAIL;

	if(ret==ERR_SUCCESS)
		WriteTW88(REG009, 0xD0);	//done

	return ret;
}
#endif //..SUPPORT_I2CCMD_SLAVE_V1



//================================
// Power Save & Resume
//================================
BYTE Buf_r003;
BYTE Buf_0B0;	// Touch
BYTE Buf_106;	// ADC
BYTE Buf_1E6;	// AFE mode
BYTE Buf_1CB;	// LLPLL, SOG
BYTE Buf_1E1;	// LLPLL GPLL
BYTE Buf_4E1;	// Clock selection

//-----------------------------------------------------------------------------
/**
* Go into Power Save Mode
*
* System PowerSave procedure
* ==========================
*
*	set all GPIOs as input mode
*	switch MCU clock to 27MKz
*	Powerdown all analog blocks
*	Power up RC oscillator
*	Switch MCU/SPI clock to RC oscillator
*	Power	down crysital oscillator
*	Now, it is a PowerSave Mode
*
* @see WaitPowerOn
* @see SystemPowerResume
*/
void SystemPowerSave(void)
{
	BYTE i;

	ePrintf("\n----- SystemPowerSave -----");
	delay1ms(10);

	//set all GPIOs as input mode	
	WriteTW88Page(PAGE0_GPIO);
	WriteTW88(REG08C, 0x00);

	FP_BiasOnOff(OFF);

	WriteTW88Page(PAGE0_GENERAL);
	Buf_r003 = ReadTW88(REG003);
	Interrupt_enableVideoDetect(OFF);

	WriteTW88Page(PAGE0_OUTPUT); 		
	WriteTW88(REG008, ReadTW88(REG008) | 0x30);	// Tri-State All outputs & FPdata 

	FP_PWC_OnOff(OFF);

	//switch MCU clock to 27MKz
	WriteTW88Page(PAGE4_CLOCK);
	Buf_4E1 = ReadTW88(REG4E1);
	WriteTW88(REG4E1, 0x00);	 				// SPI clock Source=27MHz

	//----- Powerdown all analog blocks
	WriteTW88Page(PAGE0_LEDC );
	WriteTW88(REG0E0, 0xF2 );					// LEDC
	//WriteTW88Page(PAGE0_DCDC );
	WriteTW88(REG0E8, 0xFE );					// DCDC, VCOM-DC, VCOM-AMP

	SFR_ET1 = 0;								// Disable Touch Timer
	WriteTW88Page(PAGE0_TOUCH );
	Buf_0B0 = ReadTW88(REG0B0);
	WriteTW88(REG0B0, Buf_0B0 | 0x80 );			// TSC_ADC			  	*** 0.2uA

	WriteTW88Page(PAGE1_DECODER );
	Buf_106 = ReadTW88(REG106);
	WriteTW88(REG106, Buf_106 | 0x0F );			// ADC
	Buf_1E6 = ReadTW88(REG1E6);
	WriteTW88(REG1E6, 0x00 );					// AFE Mode=low speed	*** 0.6uA

	//WriteTW88Page(PAGE1_VADC );
	Buf_1CB = ReadTW88(REG1CB);
	WriteTW88(REG1CB, Buf_1CB & 0x1F );			// SOG, LLPLL
	Buf_1E1 = ReadTW88(REG1E1);
	WriteTW88(REG1E1, Buf_1E1 | 0x20 );			// LLPLL GPLL

	//----- SSPLL power down
	SSPLL_PowerUp(OFF);							// SSPLL

	//----- Switch MCU/SPI clock to RC oscillator
	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E1, 0x10);	 				// SPI clock Source=32KHz, ...

	//----- Power down crysital oscillator
	WriteTW88Page(PAGE0_LOPOR);
	WriteTW88(REG0D4, ReadTW88(REG0D4) | (0x80));	// Enable Xtal PD Control
	PORT_CRYSTAL_OSC = 0;						// Power down Xtal

	while( PORT_POWER_SAVE==1 );

	//----- Wait ~30msec to remove key bouncing
	for(i=0; i<100; i++);

	//
	//Now, it is a PowerSave Mode
	//
}

//-----------------------------------------------------------------------------
/**
* Resume from Power Save Mode				                                               
*
* System Resume procedure
* ========================
*
*	Power up crystal oscillator
*	wait until crystal oscillator stable
*	switch MCU/SPI clock to 27MHz
*	Power up all analog blocks
*	Set MCU clock mode back
*	Set GPIO mode back
*	Now, Normal Operation mode
*
* @see WaitPowerOn
* @see SystemPowerSave
*/
void SystemPowerResume(void)
{
	BYTE i;

	SFR_EA = 0;

	//----- Power up Xtal Oscillator
	WriteTW88Page(PAGE0_LOPOR);
	PORT_CRYSTAL_OSC = 1;							// Power up Xtal
	WriteTW88(REG0D4, ReadTW88(REG0D4) & ~(0x80)); 	// Disable Xtal PD Control

	//----- Wait until Xtal stable (~30msec)
	for(i=0; i<100; i++);

	//----- switch MCU/SPI clock to 27MHz
	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E1, 0x00);	 					// SPI clock Source=27MHz, ...

	//----- Power up SSPLL
	SSPLL_PowerUp(ON);								// SSPLL
	//DCDC data out needs 200ms.
	//GlobalBootTime = SystemClock;
	//PrintSystemClockMsg("SSPLL_PowerUp");

	//----- Wait until SSPLL stable (~100usec)
	for(i=0; i<200; i++);
	for(i=0; i<200; i++);

	//----- Power up all analog blocks
	WriteTW88Page(PAGE1_VADC );
	WriteTW88(REG1E1, Buf_1E1);					// LLPLL GPLL
	WriteTW88(REG1CB, Buf_1CB);					// LLPLL, SOG
	
	WriteTW88Page(PAGE1_DECODER );
	WriteTW88(REG106, Buf_106);					// ADC
	WriteTW88(REG1E6, Buf_1E6);					// AFE mode
	
	WriteTW88Page(PAGE0_TOUCH );
	WriteTW88(REG0B0, Buf_0B0);					// Touch

	//----- Set MCU clock mode back
	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E1, Buf_4E1);	 				// Clock selection

	//----- Set GPIO mode back
	WriteTW88Page(PAGE0_GPIO);
	WriteTW88(REG08C, 0x0C);

	SFR_EA = 1;

	//
	// In case aRGB input, set Filter=0 and wait until stable and Filter=7
	//


	SFR_ET1 = 1;									// Enable Touch Timer

	//
	//Now, Normal Operation mode
	//

	while( PORT_POWER_SAVE==1 );				// Wait untill button is released
	delay1ms(100);								// To remove key bouncing

	//----- Power up Panel, Backlight
	//SSPLL_PowerUp needs 100ms before FW turns on the DataOut
	DCDC_StartUP();								// DCDC. it has WaitVBlank.
	LedPowerUp();								// LEDC


	ePuts("\n----- SystemPowerResume -------");
	Interrupt_enableVideoDetect(ON);
	WriteTW88Page(PAGE0_GPIO);
	WriteTW88(REG003, Buf_r003);	//recover ISR mask
	if(DebugLevel)
		Prompt();
}

//-----------------------------------------------------------------------------
/**
* wait powerup condition on the power save state
*
* @return 1:by button, 2:by Touch
* @see SystemPowerSave
* @see SystemPowerResume
*/
BYTE WaitPowerOn(void)
{
	BYTE i;
	while(1) {
		//----- Check Power Button
		if(PORT_POWER_SAVE==1) {
			for(i=0; i < 100; i++);
			if(PORT_POWER_SAVE==1) return 1;
		}

		//----- Check Touch
		if( P2_4==0 ) return 2;

		//----- Check Remote Control
		//if( P1_2==0 ) return 3;	// Need to confirm if it is by Power Button
	}
}


#ifndef MODEL_TW8835_EXTI2C
#ifdef SUPPORT_BT656
//-----------------------------------------------------------------------------
/*
* init BT656 encoder
*
* EVB2.1 and EVB3.1 use ADV7390.
* EVB2.0 and EVB3.0 use BU9969.
*/
void InitBT656_Encoder(void)
{
#if defined(EVB_21) || defined(EVB_31)
	//enable ADV739 BT656 output
	WriteI2CByte( I2CID_ADV7390, 0x17, 0x02 );
	WriteI2CByte( I2CID_ADV7390, 0x00, 0x1C );
	WriteI2CByte( I2CID_ADV7390, 0x01, 0x00 );
	WriteI2CByte( I2CID_ADV7390, 0x80, 0x10 );
	WriteI2CByte( I2CID_ADV7390, 0x82, 0xCB );

	WriteI2CByte( I2CID_ADV7391, 0x17, 0x02 );
	WriteI2CByte( I2CID_ADV7391, 0x00, 0x1C );
	WriteI2CByte( I2CID_ADV7391, 0x01, 0x00 );
	WriteI2CByte( I2CID_ADV7391, 0x80, 0x10 );
	WriteI2CByte( I2CID_ADV7391, 0x82, 0xCB );
#elif defined(EVB_20) || defined(EVB_30)
	//enable BU9969 BT656 output
	WriteI2CByte( I2CID_BU9969, 0, 1 );
	WriteI2CByte( I2CID_BU9969, 1, 7 );
	WriteI2CByte( I2CID_BU9969, 6, 6 );
	//delay1ms(1);
	WriteI2CByte( I2CID_BU9969, 2, 0 );
	WriteI2CByte( I2CID_BU9969, 4, 0 );
	WriteI2CByte( I2CID_BU9969, 5, 0 );
	WriteI2CByte( I2CID_BU9969, 7, 0 );
	WriteI2CByte( I2CID_BU9969, 0x10, 0 );
	WriteI2CByte( I2CID_BU9969, 0x11, 0 );
#endif
}
#endif
#endif


//=============================================================================
//	PICO qHD		                                               
//=============================================================================
#ifdef PICO_GENERIC
//-----------------------------------------------------------------------------
/**
* power on FLCOS
*
*	VDD(1.8V) -> VCC(3.3V) -> AVCC(5V)
*	GPIO64       GPIO63       GPIO65
*	Note:
*	It needs a GPIO61 ON before FW calls FLCOS_PowerOn()
*/
void FLCOS_PowerOn(void)
{
	WriteTW88Page(0);

	//VDD (1.8V)ON
	WriteTW88(REG096, ReadTW88(REG096) | 0x10);		//GPIO64
	//VCC(3.3V) ON
	WriteTW88(REG096, ReadTW88(REG096) | 0x08);		//GPIO63
	delay1ms(30);	//min. 23ms
	//AVCC(5.0V) ON
	WriteTW88(REG096, ReadTW88(REG096) | 0x20);		//GPIO65
	delay1ms(10);  //min. 1ms
}
//-----------------------------------------------------------------------------
/**
* init FLCOS
*/
void FLCOS_init(void)
{
    dPuts("\nFLCOS_init");
	WriteI2CByte(I2CID_E330_FLCOS, 0x01, 0x00);	 //BK111201 Sleeve1 use 0x20.
	WriteI2CByte(I2CID_E330_FLCOS, 0x02, 0x00);	 //BK111202 Sleeve1 use 0xC0

	WriteI2CByte(I2CID_E330_FLCOS, 0xd0, 0x01);	//HFlip.
	WriteI2CByte(I2CID_E330_FLCOS, 0x06, 0x02);
	WriteI2CByte(I2CID_E330_FLCOS, 0x07, 0x40);
	WriteI2CByte(I2CID_E330_FLCOS, 0x08, 0x09);
	WriteI2CByte(I2CID_E330_FLCOS, 0x09, 0x02);
	WriteI2CByte(I2CID_E330_FLCOS, 0x0a, 0x67);
	WriteI2CByte(I2CID_E330_FLCOS, 0x0b, 0x06);
	WriteI2CByte(I2CID_E330_FLCOS, 0x0c, 0x00);
	WriteI2CByte(I2CID_E330_FLCOS, 0x0d, 0x00);
	WriteI2CByte(I2CID_E330_FLCOS, 0x0e, 0x00);
	WriteI2CByte(I2CID_E330_FLCOS, 0x55, 0x11);		//set nSleep bit.(wakeup)
    //dPuts("\nwrite Panel x55=0x11");
//	WriteI2CByte(I2CID_E330_FLCOS, 0x55, 0x00);
}
//-----------------------------------------------------------------------------
/**
* toggle FLCOS
*
* FLCOS keeps try to use a previous video timming.
* toggle the TCON pin output to restart FLCOS.
*/
void FLCOS_toggle(void)
{
#if 1
	BYTE Status;
	WriteTW88Page(0);
	Status = ReadTW88(REG007);
	WriteTW88(REG007, Status & ~0x07);	//clear
	delay1ms(500);
	WriteTW88(REG007, Status);			//restore
#else
	WriteI2CByte(I2CID_E330_FLCOS, 0x55, 0x00);	
	delay1ms(300);	//100ms:NG,  PWRDN time was 1500us. 
	WriteI2CByte(I2CID_E330_FLCOS, 0x55, 0x11);	
#endif
}

//-----------------------------------------------------------------------------
/**
* enable LED Driver
*/
void LEDDriverEnable(BYTE fOn)
{
	WriteTW88Page(0);
	if(fOn) WriteTW88(REG094, ReadTW88(REG094) | 0x20);		//GPIO_45=1
	else	WriteTW88(REG094, ReadTW88(REG094) & ~0x20);	//GPIO_45=0
	//delay1ms(100);
}
//-----------------------------------------------------------------------------
/**
* init LED Driver
*/
void LEDDriverInit(void)
{
	WriteI2CByte(I2CID_ISL97901, 0x02, 0x04);	
	WriteI2CByte(I2CID_ISL97901, 0x06, 0x02);	
#if 0
	WriteI2CByte(I2CID_ISL97901, 0x13, 0xc0);	//red
	WriteI2CByte(I2CID_ISL97901, 0x14, 0xc0);	
	WriteI2CByte(I2CID_ISL97901, 0x15, 0xc0);	
	WriteI2CByte(I2CID_ISL97901, 0x16, 0x80);	
#else	
	//Dang suggest 120117. Need confirm
	WriteI2CByte(I2CID_ISL97901, 0x13, 0x00);	//red
	WriteI2CByte(I2CID_ISL97901, 0x14, 0x00);	
	WriteI2CByte(I2CID_ISL97901, 0x15, 0x00);	
	WriteI2CByte(I2CID_ISL97901, 0x16, 0x00);	
	WriteI2CByte(I2CID_ISL97901, 0x17, 0x55);	
#endif
}

//-----------------------------------------------------------------------------
/**
* default GPIO for qHD
*/
void qHD_GpioDefault(void)
{
	WriteTW88Page(PAGE0_GPIO);
														//GPIO_0x
														//00:SCDT
														//01:HDMI_INT
														//7  6  5  4  3  2  1  0
	WriteTW88(REG088, ReadTW88(REG088) & ~0x03);		//                  I  I
	WriteTW88(REG080, ReadTW88(REG080) | 0x03);			//					G  G


														//GPIO_4x
														//44: RSOP76238 IN : Remocon
														//45: LED_DR_EN
														//7  6  5  4  3  2  1  0
	WriteTW88(REG094, ReadTW88(REG094) & ~0x20);		//x  x  0  0  x  x  x  x
	WriteTW88(REG08C, ReadTW88(REG08C) | 0x20);			//      O  X         
	WriteTW88(REG084, ReadTW88(REG084) | 0x20);			//      G  X         

														//GPIO_5x
														//57:EN_SW1
//														//7  6  5  4  3  2  1  0
//	WriteTW88(REG095, ReadTW88(REG095) | 0x80);			//1
//	WriteTW88(REG08D, ReadTW88(REG08D) | 0x80);			//O
//	WriteTW88(REG085, ReadTW88(REG085) | 0x80);			//G

														//GPIO_6x
														//60: HDMI_RST#
														//61: EN_3V3
														//62: EN_1V2
														//63: EN_VCC
														//64: EN_VDD
														//65: EN_AVCC
														//7  6  5  4  3  2  1  0
	WriteTW88(REG096,(ReadTW88(REG094) & ~0x3E) & 0x01);//      0  0  0	 0  0  1
	WriteTW88(REG08E, ReadTW88(REG08E) | 0x3F);			//      O  O  O	 O  O  O
	WriteTW88(REG086, ReadTW88(REG086) | 0x3F);			//		G  G  G	 G	G  G
}
#endif //..PICO_GENERIC



//=============================================================================
// RearCameraDisplayMode				                                               
//=============================================================================
#ifdef SUPPORT_RCD
//-----------------------------------------------------------------------------
/**
*	Turn On/Off Back drive grid SPIOSD image
*/
static void BackDriveGrid(BYTE on)
{
	if(on) {
		//draw parkgrid
		SOsdWinBuffClean(0);

		//init DE
		SpiOsdSetDeValue();

		//init SOSD
		WaitVBlank(1);
		SpiOsdEnable(ON);
		SpiOsdResetRLC(1,0);
		SpiOsdWinImageLocBit(1,0);
		SpiOsdWinLutOffset( 1, 0 /*SOSD_WIN_BG,  WINBG_LUTLOC*/ );  //old: SpiOsdLoadLUT_ptr
		SpiOsdWinFillColor( 1, 0 );

		MovingGridInit();
		//MovingGridDemo(0 /*Task_Grid_n*/);
		MovingGridTask_init();
		MovingGridLUT(3);	//I like it.

	}
	else {
		SpiOsdWinHWOffAll(0);	//without wait
		StartVideoInput();
	}
}

//-----------------------------------------------------------------------------
/**
* init RCD mode
*
* goto RCDMode (RearCameraDisplay Mode)
* and, prepare ParkingGrid.
* RCDMode does not support a video ISR.
* @return
*	0:success
*	other:error code
*/
BYTE InitRCDMode(BYTE fPowerUpBoot)
{
	BYTE ret;

	Printf("\nInitRCDMode(%bd)",fPowerUpBoot);
	if(fPowerUpBoot==0) {
		if(MenuGetLevel()) {
			MenuQuitMenu();
			SpiOsdWinHWOffAll(1);	//with WaitVBlank
		}
		//FYI. I don't care demo page.
	}

	//skip CheckEEPROM() and manually assign DevegLevel
	DebugLevel = 1;

	//set default setting.
	InitWithNTSC();

	FP_GpioDefault();

	SSPLL_PowerUp(ON);
	//PrintSystemClockMsg("SSPLL_PowerUp");
	//DCDC needs 100ms, but we have enough delay on...


	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG040, ReadTW88(REG040) & ~0x10);

	DCDC_StartUP();

	PrintSystemClockMsg("before DecoderCheck");

#if 1
	//We add a check routine because the customer wants a stable video..
	//Current code only check the NTSC. 
	//If you want to PAL, change REG11D value.
	//If we assign only NTSC, it uses a 300ms.
	//If we add all standard, it uses a 500ms.
	WriteTW88Page(1);
	WriteTW88(REG11D, 0x01);

	//wait until we have a stable signal
	ret=DecoderCheckVDLOSS(100);
	if(ret) {
		ePuts("\nCheckAndSetDecoderScaler VDLOSS");
	}
	else {
		//get standard
		ret = DecoderCheckSTD(100);
		if ( ret == 0x80 ) {
		    ePrintf("\nCheckAndSetDecoderScaler NoSTD");
			//return( 2 );
		}
		else {
			ret >>= 4;
			//InputSubMode = mode;
			ePrintf("\nMode:%bx",ret);
		}
	}
	PrintSystemClockMsg("after DecoderCheck");
#endif

	//disable interrupt.
	WriteTW88Page(PAGE0_GENERAL );
	WriteTW88(REG003, 0xFE );	// enable only SW interrupt

	LedBackLight(ON);
	ScalerSetMuteManual(OFF);

	//draw parkgrid
	BackDriveGrid(ON);

	LedPowerUp();

	return ret;
}
#endif

#ifdef SUPPORT_RCD
#define RET_RCDMODE_UNKNOWN	0
#define RET_RCDMODE_OFF		1
#define RET_RCDMODE_PSM		2
//-----------------------------------------------------------------------------
/* RCDMode main_loop
*
* RearCameraDisplayMode LOOP
* ==========================
*/
BYTE RCDMode_loop(void)
{
	//---------------------------------------------------------------
	//			             RearCameraDisplayMode Loop 
	//---------------------------------------------------------------
	while(1) {
		//-------------- Check Serial Port ---------------------
		Monitor();				// for new monitor functions

		if(IsBackDrivePortOn()==0) {
			//let's move to the normal mode(playback mode)
			return 1;
		}
		if(PORT_POWER_SAVE==1 && SysNoInitMode==SYS_MODE_NORMAL) {
			return RET_RCDMODE_PSM;
		}
		if(access==0) 	
			continue;

		if(Task_Grid_on)
			MovingGridTask();
	} //..while(1)
	return 0;
}
#endif


//-----------------------------------------------------------------------------
/**
* print model, version, compile date
*
* example:
*	********************************************************
*	 TW8835 Evaluation Board 3.1 - 18:52:43 (May 14 2012) SLAVE
*	********************************************************
*/
#ifdef MODEL_TW8835_EXTI2C
static void PrintModelVersionInfo(BYTE fSlave)
#else
static void PrintModelVersionInfo(void)
#endif
{
	//-------------------------------------
	Printf("\n********************************************************");
#if defined(MODEL_TW8830)
	Puts("\n TW8830 ");
#elif defined(MODEL_TW8835)
	Puts("\n TW8835 ");
#elif defined(MODEL_TW8836)
	Puts("\n TW8836 ");
#else
	Puts("\n TW88XX ");
#endif
#if defined(DP80390)
	Puts("DP80390 ");
#endif
#if defined(SUPPORT_8BIT_CHIP_ACCESS)
	Puts("8BIT ");
#endif
#if defined(CHIP_MANUAL_TEST) 
	Puts(" CHIPTEST ");
#endif
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	Puts("FPGA Verification - ");
#elif defined(MODEL_TW8835RTL) || defined(MODEL_TW8836RTL)
	Puts("RTL Verification - ");
#else
	Puts("Evaluation Board ");
	#ifdef EVB_10
	Puts("1.0 - ");
	#elif defined(EVB_21)
	Puts("2.1 - ");
	#elif defined(EVB_20)
	Puts("2.0 - ");
	#elif defined(EVB_31)
	Puts("3.1 - ");
	#elif defined(EVB_30)
	Puts("3.0 - ");
	#else
	Puts("0.0 - ");
	#endif
#endif
	Printf("%s (%s)", __TIME__, __DATE__);
#ifdef MODEL_TW8835_EXTI2C
	if(fSlave)	Puts(" SLAVE");
	else		Puts(" HOST");
#endif
#ifdef MODEL_TW8835_SLAVE
	Puts(" SLAVE");
#endif
#ifdef MODEL_TW8835_MASTER
BUG..Use main-master.c file
#endif

	Printf("\n********************************************************");
}




//=============================================================================
// INIT ROUTINES
//=============================================================================
//-----------------------------------------------------------------------------
extern BYTE	 OsdTime;
/**
* init global variables
*/
void InitVariables(void)
{
	DebugLevel=0;
	access = 1;
	SW_key = 0;

	//--task variables
	Task_Grid_on = 0;
	Task_Grid_cmd = 0;
	Task_NoSignal_cmd = TASK_CMD_DONE;
	SW_INTR_cmd = 0;

	SpiFlashVendor = 0;	//see spi.h
#ifdef DEBUG_REMO_NEC
	DebugRemoStep = 0;  //only for test BK110328
#endif
//	FirstInitDone = 0;
#if defined(SUPPORT_I2CCMD_TEST_SLAVE)
	ext_i2c_cmd=0;
	ext_i2c_timer=0;
#endif

	OsdTime	=	0;
}

//=============================================================================
// Init QuadIO SPI Flash			                                               
//=============================================================================

//-----------------------------------------------------------------------------
/**
* init core
*
* prepare SPIFLASH QuadIO
* enable chip interrupt
* enable remocon
*
*/
void InitCore(BYTE fPowerUpBoot)
{
	if(fPowerUpBoot) {
		//check port 1.5. if high, it is a skip(NoInit) mode.
		SysNoInitMode = SYS_MODE_NORMAL;
//BK120423
#ifdef MODEL_TW8836FPGA
		if(1)	
#else
		if(PORT_NOINIT_MODE == 1)	
#endif
		{
			SysNoInitMode = SYS_MODE_NOINIT;
			//turn on the SKIP_MODE.
			access = 0;
					
			McuSpiClkSelect(MCUSPI_CLK_27M);
			return;
		}
	}

	Puts("\nInitCore");	
	//----- Set SPI mode
	SpiFlashVendor = SPI_QUADInit();
	SPI_SetReadModeByRegister(SPI_READ_MODE);		// Match DMA READ mode with SPI-read

	//----- Enable Chip Interrupt

	WriteTW88Page(PAGE0_GENERAL );
	WriteTW88(REG002, 0xFF );	// Clear Pending Interrupts
	WriteTW88(REG003, 0xFE );	// enable SW. disable all other interrupts

#ifndef MODEL_TW8835_EXTI2C
	//enable remocon interrupt.
	EnableRemoInt();
#endif
}

//=============================================================================
//			                                               
//=============================================================================

//-----------------------------------------------------------------------------
/**
* start video with a saved input
*
* @see ChangeInput
*/
void StartVideoInput(void)
{
	BYTE InputMainEE;
				
#if defined(PICO_GENERIC)
	//"pico generic" only have a HDMI input.
	if((InputMainEE != INPUT_HDMIPC) && (InputMainEE != INPUT_HDMITV)) {
		if(GetHdmiModeEE())  InputMainEE = INPUT_HDMITV;
		else 				 InputMainEE = INPUT_HDMIPC;
		SaveInputEE(InputMainEE);
	}
#endif


	ePrintf("\nStart with Saved Input: ");
	InputMainEE = GetInputEE();
	PrintfInput(InputMainEE,1);

	InputMain = 0xff;			// start with saved input						
	ChangeInput( InputMainEE );	
}



//--------------------------------------------------
// Description  : Video initialize
// Input Value  : None
// Output Value : None
//--------------------------------------------------

//-----------------------------------------------------------------------------
/**
* initialize TW8835 System
*
*	CheckEEPROM
*	InitWithNTSC
*	Set default GPIO
*	SSPLL_PowerUp
*	Startup DCDC
*	download Font
*	Start Video input
*	InitLogo1
*	Powerup LED
*	Remove Logo
*	Init Touch
*
* @param bool fPowerUpBoot
* 	if fPowerUpBoot is true, 
*		download default value,
*		turn on Panel
* @return 0:success
* @see InitWithNTSC
* @see FP_GpioDefault
* @see SSPLL_PowerUp
* @see DCDC_StartUP
* @see FontOsdInit
* @see StartVideoInput
* @see InitLogo1
* @see LedPowerUp
* @see RemoveLogoWithWait
* @see OsdSetTime
* @see OsdSetTransRate
* @see BackLightSetRate
* @see MeasSetErrTolerance
* @see InitTouch
* @see UpdateOsdTimerClock
*/
BYTE InitSystem(BYTE fPowerUpBoot)
{
#ifndef MODEL_TW8835_EXTI2C
	BYTE ee_mode;
#endif
	BYTE value;
	BYTE FirstInitDone;
#ifdef SW_I2C_SLAVE
	BYTE i;
#endif

	if(access==0) {
		//do nothing.
		return 0;
	}

	//check EEPROM
#ifdef MODEL_TW8835_EXTI2C
	// do nothing. InitHostSystem() will setup ee_mode and DebugLevel
#else
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
#endif

	ePrintf("\nInitSystem(%bd)",fPowerUpBoot);


	//
	//set default setting.
	//
	if(fPowerUpBoot) {

		//Init HW with default
		InitWithNTSC();

		//------------------
		//first GPIO position
#if defined(PICO_GENERIC)
		qHD_GpioDefault();
#else
		FP_GpioDefault();
#endif

		SSPLL_PowerUp(ON);
		//DCDC data out needs 200ms.
		PrintSystemClockMsg("SSPLL_PowerUp");

#if 0 //BKFYI120112. If you want to use CKLPLL, select MCUSPI_CLK_PCLK 
		McuSpiClkSelect(MCUSPI_CLK_PCLK);
#endif

#ifdef SUPPORT_I2C2
		//I2C2 test position.
		//If you are using I2C2, you can add your code from here.
		WriteI2C2Byte(0x8C,0x00,0xAB);
		Printf("\nI2C2 test value %bx",ReadI2C2Byte(0x8C,0x00));
#endif
	}

#ifdef SUPPORT_HDMI_SiIRX
	//it will supply +3V3 and +1V2.
	HDMI_SystemInit();
#endif
#ifdef SUPPORT_HDMI_EP9351
	//it will download EDID & HDCP
	Hdmi_SystemInit_EP9351();
#endif

	InputMain = GetInputEE();
	FirstInitDone = 0;

	if(fPowerUpBoot) {
		//---------------------
		// turn on DCDC
		//---------------------
		DCDC_StartUP();
	}
	//---------------
	//FontOSD first message
	FontOsdInit();
	FOsdSetDeValue();
#if 1 //BKFYI. first FontOSD Message.
	FOsdIndexMsgPrint(FOSD_STR1_TW8835);
	FOsdWinEnable(0, OFF);	//win0, disable..
#endif


#if defined(PICO_GENERIC)
	//--------------------
	//
	//--------------------
	FLCOS_PowerOn();
	LEDDriverEnable(ON); //it needs 100ms delay before LEDDriverInit()

	FLCOS_init();
    delay1ms(100);

	LEDDriverInit();  //    NSLEDDriverinit();
	//WriteI2CByte(I2CID_E330_FLCOS, 0x55, 0x11);	//wakeup
	PrintSystemClockMsg("After Init FLCOS & LED");
#endif
	
	DumpClock(0);
	//------------------------
	//start with saved input
	//------------------------
	StartVideoInput();
	PrintSystemClockMsg("StartVideoInput");

	//
	//Logo and LedPowetUp
	//
	if(FirstInitDone ==0) {
		InitLogo1();
		FirstInitDone =1;
	}
	LedPowerUp();

#ifndef MODEL_TW8835_EXTI2C
#ifdef SUPPORT_BT656
	//enable BT656 output encoder
	if(fPowerUpBoot)
		InitBT656_Encoder();
#endif
#endif
	//
	//remove InitLogo
	//
	if(FirstInitDone ==1) {
		FirstInitDone = 2;
#ifdef NOSIGNAL_LOGO
		if(Task_NoSignal_cmd == TASK_CMD_DONE) {
			RemoveLogoWithWait(fPowerUpBoot);
			FOsdWinEnable(0, OFF);	//win0, disable..
		}
#else
		RemoveLogoWithWait(1);
		if(Task_NoSignal_cmd == TASK_CMD_DONE)
			FOsdWinEnable(0, OFF);	//win0, disable..
#endif
	}	
#ifdef NOSIGNAL_LOGO 
	//BK120803. If VDLoss, set FreerunManual.
	else {
		WriteTW88Page(PAGE0_GENERAL);
		if(ReadTW88(REG004) & 0x01)
			ScalerSetFreerunManual(ON);
	}
#endif

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


#ifdef USE_SFLASH_EEPROM
	//to cleanup E3PROM
	//call EE_CleanBlocks();
#endif


#ifdef SUPPORT_TOUCH
	//read CalibDataX[] and CalibDataY[] from EEPROM.
	ReadCalibDataFromEE();
	InitTouch();	

	SetTouchStatus(TOUCHEND);
	SetLastTouchStatus(TOUCHEND);
#endif

	UpdateOsdTimerClock();
	//dPrintf("\nOsdTimerClock:%ld",OsdTimerClock);

#ifdef SW_I2C_SLAVE
	dbg_sw_i2c_sda_count = 0;
	dbg_sw_i2c_scl_count = 0;
	sw_i2c_regidx = 0;
	for(i=0; i < 4; i++) {
		dbg_sw_i2c_index[i] = 0;
		dbg_sw_i2c_devid[i] = 0;
		dbg_sw_i2c_regidx[i] = 0;
		dbg_sw_i2c_data[i] = 0; 
	}

	sw_i2c_index = 0x00;
	E2IE |=  0x80;
#endif

#if defined(MODEL_TW8835_SLAVE)
	//
	// check PORT
	//
	Puts("\nI2C with GPIO(P1_3/INT10) ");
	if(PORT_I2CCMD_GPIO_SLAVE) {
		Printf("Ready!!"); 

		SFR_E2IE  |= 0x08; //enable INT10 interrupt
	}
	else
		Printf("FAIL!!"); 
#if defined(SUPPORT_EXTMCU_ISP) && defined(USE_EXTMCU_ISP_GPIO)
	Puts("\nI2C with ISP(P3_2) ");
	if(PORT_EXTMCU_ISP) Printf("Ready!!");
	else {
		Printf("FAIL!!. access become 0");
		access = 0;
	}
#endif
#endif

	// re calculate FOSD DE
	FOsdSetDeValue();

#ifdef SUPPORT_WATCHDOG
	//if(access)
		EnableWatchdog(0);
		F_watch = 1; //for first  wdt_last value
#endif

	//dPuts("\nInitSystem-END");
	return 0;
}

//=============================================================================
// Video Signal Task				                                               
//=============================================================================

//-----------------------------------------------------------------------------
/**
* do Video Signal Task Routine
*
*
* @see Interrupt_enableVideoDetect
* @see CheckAndSetInput
* @see VInput_enableOutput
*/
void NoSignalTask(void)
{	
	DECLARE_LOCAL_page
	BYTE ret;
	BYTE r004;

	if(Task_Grid_on)
		// MovingGridTask uses tic_task. It can not coexist with NoSignalTask.
		return;

	if(Task_NoSignal_cmd==TASK_CMD_DONE)
		return;

	if(tic_task < NOSIGNAL_TIME_INTERVAL) 
		return;

	if(Task_NoSignal_cmd==TASK_CMD_WAIT_VIDEO) {

		ReadTW88Page(page);

		FOsdWinToggleEnable(TASK_FOSD_WIN); //WIN0-toggle
		if(Task_NoSignal_count < 3) {
			dPuts("\nTask NoSignal TASK_CMD_WAIT_VIDEO");
			Task_NoSignal_count++;
		}
		tic_task = 0;

		WriteTW88Page(page);
		return;
	}
	if(Task_NoSignal_cmd==TASK_CMD_WAIT_MODE)
		return;
 
	//--------------------------------------------
	//
	//--------------------------------------------

	dPuts("\n***Task NoSignal TASK_CMD_RUN");
	if(Task_NoSignal_cmd == TASK_CMD_RUN_FORCE)
		dPuts("_FORCE");

	ReadTW88Page(page);
 	WriteTW88Page(PAGE0_GENERAL);
	r004 = ReadTW88(REG004);
	if(r004 & 0x01) {						
		ePrintf("..Wait...Video");

		tic_task = 0;
		WriteTW88Page(page);
		return;
	}

	//turn off Interrupt.
	Interrupt_enableVideoDetect(OFF);

	//start negotition
	ret = CheckAndSetInput();

	//turn on Interrupt. 
	//if success, VInput_enableOutput() will be executed.
	Interrupt_enableVideoDetect(ON);

	if(ret==ERR_SUCCESS) {
		dPuts("\n***Task NoSignal***SUCCESS");
		VInput_enableOutput(VH_Loss_Changed);
		FOsdWinEnable(TASK_FOSD_WIN,OFF); 	//WIN0, Disable


#ifdef NOSIGNAL_LOGO
		if(getNoSignalLogoStatus()) {
			ScalerSetFreerunManual(OFF);	//BK120803
			RemoveLogo();
		}
#endif
#ifdef PICO_GENERIC
		FLCOS_toggle();
#endif

	}
#ifdef SUPPORT_PC
	else {
		//fail 
		if(InputMain==INPUT_PC) {
			WriteTW88Page(PAGE0_GENERAL);
			if(ReadTW88(REG004) & 0x01)
				FOsdIndexMsgPrint(FOSD_STR2_NOSIGNAL);	//over write
			else
				FOsdIndexMsgPrint(FOSD_STR3_OUTRANGE);	//replace 
		}
	}
#endif


	//update tic_task.
	tic_task = 0;

	WriteTW88Page(page);
}
//-----------------------------------------------------------------------------
/**
*  Check each input status
*
*  recover routine for unstable video input status.
*  only need it when user connect/disconnect the connector 
*  or, the QA toggles the video mode on the pattern generator.
*/
void NoSignalTaskOnWaitMode(void)
{
	BYTE ret;
	DECLARE_LOCAL_page
	if((Task_NoSignal_cmd != TASK_CMD_WAIT_MODE))
		return;
	
	ReadTW88Page(page); 
	if(InputMain==INPUT_CVBS || InputMain==INPUT_SVIDEO) {
		ret=DecoderReadDetectedMode();
		//only consider NTSC & PAL with an idle mode.
		if(ret == 0 || ret == 1) {
			if(InputSubMode != ret) {
				ScalerSetMuteManual( ON );

				SW_INTR_cmd = SW_INTR_VIDEO_CHANGED;
				dPrintf("\nRequest SW Interrupt cmd:%bd InputSubMode:%bd->%bd",SW_INTR_cmd, InputSubMode,ret);
				InputSubMode = ret;
				WriteTW88Page(PAGE0_GENERAL);
				WriteTW88(REG00F, SW_INTR_VIDEO);	//SW interrupt.		
			} 
		}
	}
#ifdef SUPPORT_COMPONENT
	else if(InputMain==INPUT_COMP) {
		ret = VAdcGetInputStatus();	//detected input.
		if(ret & 0x08) {			//check the compoiste detect status first.
			ret &= 0x07;
			if( (ret!=7) && (InputSubMode != ret) ) {
				ScalerSetMuteManual( ON );

				SW_INTR_cmd = SW_INTR_VIDEO_CHANGED;
				dPrintf("\nRequest SW Interrupt cmd:%bd InputSubMode:%bd->%bd",SW_INTR_cmd, InputSubMode,ret);
				InputSubMode = ret;
				WriteTW88Page(PAGE0_GENERAL);
				WriteTW88(REG00F, SW_INTR_VIDEO);	//SW interrupt.		
			} 
		}
	}
#endif
	WriteTW88Page(page);
}

//=============================================================================
// Interrupt Handling Routine			                                               
//=============================================================================
//-----------------------------------------------------------------------------
/**
* enable VideoDetect interrupt
*
* Turn off the SYNC Change(R003[2]) mask,
*               the Video Loss(R003[1]) mask,
*               the WirteReg0x00F(R003[0] mask.
* 
* Turn On  the Video Loss(R003[1]) mask,
*               the WirteReg0x00F(R003[0] mask. 
*
* I do not turn on the SYNC Change.
* if you want to turn on SYNC, You have to call Interrupt_enableSyncDetect(ON).
*
* @param bool fOn
* @see Interrupt_enableSyncDetect
*/
void Interrupt_enableVideoDetect(BYTE fOn)
{
#ifdef DEBUG_ISR
	WORD temp_VH_Loss_Changed;
	BYTE temp_INT_STATUS, temp_INT_STATUS2;
#endif
	DECLARE_LOCAL_page

	ReadTW88Page(page);
	WriteTW88Page(PAGE0_GENERAL);
	if(fOn) {
		WriteTW88(REG002, 0xFF);	//clear
		WriteTW88(REG004, 0xFF);	//clear
		WriteTW88(REG003, 0xFC);	//release Video, but still block SYNC
	}
	else {
		WriteTW88(REG003, 0xFE);	//block.
		WriteTW88(REG002, 0xFF);	//clear
		WriteTW88(REG004, 0xFF);	//clear

#ifdef DEBUG_ISR
		//copy
		temp_INT_STATUS = INT_STATUS;
		temp_VH_Loss_Changed = VH_Loss_Changed;
		temp_INT_STATUS2 = INT_STATUS2;
#endif
		//clear
		INT_STATUS = 0;
		VH_Loss_Changed = 0;
		INT_STATUS2 = 0;
#ifdef DEBUG_ISR
		if(temp_INT_STATUS+temp_VH_Loss_Changed+temp_INT_STATUS2)
			dPrintf("\nclear INT_STATUS:%bx INT_STATUS2:%bx VH_Loss_Changed:%d",temp_INT_STATUS,temp_INT_STATUS2,temp_VH_Loss_Changed);
#endif
	}
	WriteTW88Page(page);
}

//-----------------------------------------------------------------------------
/**
* Turn off/on SYNC Interrupt mask.
*
* @see Interrupt_enableVideoDetect
*/
void Interrupt_enableSyncDetect(BYTE fOn)
{
	DECLARE_LOCAL_page
#ifdef DEBUG_ISR
	BYTE temp_INT_STATUS, temp_INT_STATUS2;
#endif

	ReadTW88Page(page);
	WriteTW88Page(PAGE0_GENERAL);
	if(fOn) {
		WriteTW88(REG002, 0x04);	//clear
		WriteTW88(REG004, 0x06);	//clear
		WriteTW88(REG003, ReadTW88(REG003) & ~0x04);	//release

		SW_Video_Status = 1;
		//dPrintf("\nSW_Video_Status:%bd",SW_Video_Status);
	}
	else {
		WriteTW88(REG003, ReadTW88(REG003) | 0x04);	//block
		WriteTW88(REG002, 0x04);	//clear
		WriteTW88(REG004, 0x06);	//clear

#ifdef DEBUG_ISR
		//copy
		temp_INT_STATUS = INT_STATUS;
		temp_INT_STATUS2 = INT_STATUS2;
#endif
		//clear
		INT_STATUS &= ~0x04;
		INT_STATUS2 &= ~0x06;
#ifdef DEBUG_ISR
		if( (temp_INT_STATUS != INT_STATUS) || (temp_INT_STATUS2 != INT_STATUS2))
			dPrintf("\nclear SYNC at INT_STATUS:%bx INT_STATUS2:%bx",temp_INT_STATUS,temp_INT_STATUS2);
#endif
	}
	WriteTW88Page(page);
}

//-----------------------------------------------------------------------------
/**
* do interrupt polling
*
* Read interrupt global value that changed on interrupt service routine,
* and print status.
*
* @see ext0_int
* @see InterruptHandlerRoutine
*/
void InterruptPollingRoutine(void)
{
	DECLARE_LOCAL_page
	BYTE temp_INT_STATUS_ACC, temp_INT_STATUS2_ACC;
	BYTE r003;
	BYTE i, tmp;

	//------------ Chip Interrupt --------------	
	SFR_EX0 = 0; 	//disable INT0
	//copy
	main_INT_STATUS = INT_STATUS;         
	main_INT_STATUS2 = INT_STATUS2;
	main_VH_Loss_Changed = VH_Loss_Changed;
	temp_INT_STATUS_ACC = INT_STATUS_ACC;
	temp_INT_STATUS2_ACC = INT_STATUS2_ACC;
	//clear
	INT_STATUS = 0;			//can be removed
	INT_STATUS2 = 0;		//can be removed
	VH_Loss_Changed = 0;
	INT_STATUS_ACC = 0;
	INT_STATUS2_ACC = 0;
	SFR_EX0 = 1;	//enable INT0

	ReadTW88Page(page);
	WriteTW88Page(PAGE0_GENERAL);
	r003 = ReadTW88(REG003);
	WriteTW88Page(page);

	//mask
	main_INT_STATUS &= ~r003;

	//
	// print INT debug message
	//
	if(main_INT_STATUS & 0x07) {
		ePrintf("\nInterrupt !!! [%02bx] ", main_INT_STATUS);
		tmp = main_INT_STATUS;
		for(i=0; i<8; i++) {
			if(tmp & 0x80) ePrintf("1"); else ePrintf("0");
			tmp <<= 1;
		}
		//adjust from _ACC
		if(main_INT_STATUS != temp_INT_STATUS_ACC) {
			temp_INT_STATUS_ACC	&= ~r003;
			if(main_INT_STATUS != temp_INT_STATUS_ACC) {
				ePrintf(" [ACC:%02bx]", temp_INT_STATUS_ACC);
				if(temp_INT_STATUS_ACC & 0x01)
					main_INT_STATUS |= 0x01;				//NOTE
			}
		}
		ePrintf(" [%02bx] ", main_INT_STATUS2);
		tmp = main_INT_STATUS2;
		for(i=0; i<3; i++) {
			if(tmp & 0x04) ePrintf("1"); else ePrintf("0");
			tmp <<= 1;
		}
		//adjust from _ACC
		if(main_INT_STATUS2 != temp_INT_STATUS2_ACC) {
			ePrintf(" [ACC2:%02bx]", temp_INT_STATUS2_ACC);
			if((r003 & 0x04) == 0) {
				main_INT_STATUS2 |= (temp_INT_STATUS2_ACC & 0x06);	//NOTE
				ePrintf("->[%02bx]",main_INT_STATUS2);	
			}
		}

		ePrintf(" mask:%bx",r003);
	}
	//if( main_INT_STATUS & 0x80 ) ePrintf("\n   - SPI-DMA completion ");
	//if( main_INT_STATUS & 0x40 ) ePrintf("\n   - V display end ");
	//if( main_INT_STATUS & 0x20 ) ePrintf("\n   - Measurement Ready ");
	//if( main_INT_STATUS & 0x08 ) ePrintf("\n   - VSync leading edge ");

	//if( main_INT_STATUS & 0x04 ) {
	//	ePrintf("\n   - Sync Changed ");
	//	if(main_INT_STATUS2 & 0x02) ePrintf(" - HSync changed ");
	//	if(main_INT_STATUS2 & 0x04) ePrintf(" - VSync changed ");
	//}

	if( (main_INT_STATUS & 0x04 ) && (main_INT_STATUS2 & 0x04)) {
		ePrintf("\n   - Sync Changed ");
		ePrintf(" - VSync changed ");
	}			

	if(main_VH_Loss_Changed) {		//INT_STATUS[1] use accumulated VH_Loss_Changed value.
		//Video change happen.
		ePrintf("\n   - V/H Loss Changed:%d ", main_VH_Loss_Changed);
		if(main_INT_STATUS2 & 0x01)	ePrintf(" - Video Loss ");
		else						ePrintf(" - Video found ");
	}
	if(main_INT_STATUS & 0x01) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
		ePrintf("\n   - Write register 0x00F ");
#else		
		//Printf("\nR00F[%02bx]",ReadTW88(REG00F));
#endif
		if(SW_INTR_cmd == SW_INTR_VIDEO_CHANGED)
			dPrintf("\n*****SW_INTR_VIDEO_CHANGED");
#if defined(SUPPORT_I2CCMD_SLAVE_V1)
		else {
			BYTE cmd;
			cmd = ReadTW88(REG00F);
			if(cmd & SW_INTR_EXTERN) {
				if(cmd & I2CCMD_CHECK) {
					WriteTW88(REG009, 0xA1);
					return;	
				}
				if(cmd & I2CCMD_EXEC) {
					F_i2ccmd_exec = 1;	//request loop routine
					WriteTW88(REG009, 0xA1);
					return;						
				}
			}	
		}		
#endif
	}

	//----------------------------------------
	// now, We uses 
	//	main_INT_STATUS
	//  main_INT_STATUS2
	//  main_VH_Loss_Changed
}

//-----------------------------------------------------------------------------
/**
* Interrupt Handler Routine 
*
* use InterruptPollingRoutine first
* @see ext0_int
* @see InterruptPollingRoutine
*/
void InterruptHandlerRoutine(void)
{
	DECLARE_LOCAL_page

	BYTE ret;
	BYTE r004;
	BYTE not_detected;

	ReadTW88Page(page);
	if(main_INT_STATUS & 0x01) {
		if(SW_INTR_cmd == SW_INTR_VIDEO_CHANGED) {
			SW_INTR_cmd = 0;

			LedBackLight(OFF);
			ScalerSetMuteManual( ON );

			//start negotiation right now
			TaskNoSignal_setCmd(TASK_CMD_RUN_FORCE);		
		}
		else {
			//assume external MCU requests interrupts.
			//read DMA buffer registers that the external MCU write the commmand.
			//we need a pre-defined format
			//execute

		}
		//NOTE:TASK_CMD_RUN2 can be replaced on the following condition. LedBackLight(OFF) can make a problem.
	}
	//CHECK SYNCH first and then check VDLoss. VDLoss will have a high priority.
	if( main_INT_STATUS & 0x04 ) {
		//check only VSync.	I have too many HSync.
		//service SYNC only when we have a video.
		if(( (main_INT_STATUS2 & 0x05) == 0x04 ) && (Task_NoSignal_cmd==TASK_CMD_DONE || Task_NoSignal_cmd==TASK_CMD_WAIT_MODE)) {
			if(InputMain==INPUT_CVBS || InputMain==INPUT_SVIDEO) {
				dPrintf("\n*****SYNC CHANGED");

				ret=DecoderReadDetectedMode();
				not_detected = ret & 0x08 ? 1 : 0;	//if not_detected is 1, not yet detected(in progress).
				ret &= 0x07;
				dPrintf(" InputSubMode %bd->%bd",InputSubMode,ret);
				if(not_detected || (ret == 7)) {
					dPrintf(" WAIT");
					TaskNoSignal_setCmd(TASK_CMD_WAIT_MODE);
				}
				else if(InputSubMode != ret) {
					dPrintf(" NEGO");
					LedBackLight(OFF);
					ScalerSetMuteManual( ON );
	
					//start negotiation	right now
					TaskNoSignal_setCmd(TASK_CMD_RUN_FORCE);
				}
				else
					dPrintf(" SKIP");
			}
#ifdef SUPPORT_COMPONENT
			else if(InputMain==INPUT_COMP) {
				dPrintf("\n*****SYNC CHANGED");
	
				ret = VAdcGetInputStatus();	//detected input.
				not_detected = ret & 0x08 ? 0:1;	 //if not_detected is 1, not yet detected.
				ret &= 0x07;
				dPrintf(" InputSubMode %bd->%bd",InputSubMode,ret);
				if(not_detected || (ret == 7)) {
					dPrintf(" WAIT");
					TaskNoSignal_setCmd(TASK_CMD_WAIT_MODE);
				}
				else if(InputSubMode != ret) {
					dPrintf(" NEGO");
					LedBackLight(OFF);
					ScalerSetMuteManual( ON );
	
					//start negotiation right now
					TaskNoSignal_setCmd(TASK_CMD_RUN_FORCE);
				}
				else
					dPrintf(" SKIP");
			}
#endif
#ifdef SUPPORT_PC
			else if(InputMain==INPUT_PC) {
				//Need to verify.
	
				//-------------------------------
				// Video Signal is already changed. I can not use a FreeRun with FOSD message.
				//WaitVBlank(1);
				LedBackLight(OFF);
				ScalerSetMuteManual( ON );
	
				dPrintf("\n*****SYNC CHANGED");
				dPrintf(" NEGO");

				//start negotiation	right now
				TaskNoSignal_setCmd(TASK_CMD_RUN_FORCE);
			}
#endif
			else {
				//Need to verify.

				//-------------------------------
				// Video Signal is already changed. I can not use a FreeRun with FOSD message.
				//WaitVBlank(1);
				LedBackLight(OFF);
				ScalerSetMuteManual( ON );

				dPrintf("\n*****SYNC CHANGED");
				dPrintf(" NEGO");

				//start negotiation	right now
				TaskNoSignal_setCmd(TASK_CMD_RUN_FORCE);
			}
		}
	}


	if(main_VH_Loss_Changed) {		//INT_STATUS[1] use accumulated VH_Loss_Changed value.
		//Video change happen.
		main_VH_Loss_Changed = 0;
		if(InputMain==INPUT_PC) {
			;
		}
		else {
			//------------------------
			//read INT_STATUS2 value from HW.
			//ReadTW88Page(page);
			WriteTW88Page(PAGE0_GENERAL);
			r004 = ReadTW88(REG004);
			//WriteTW88Page(page);
			if(((main_INT_STATUS2 ^ r004) & 0x01) == 0x01) {						
				ePrintf("\nWarning SW replace Video Loss");
			//	main_INT_STATUS2 ^= 0x01;
			}
		}
		//--OK, what is a current status
		if(main_INT_STATUS2 & 0x01) {
			//Video Loss Happen
			if(SW_Video_Status) {
				dPuts("\nVideo Loss Happen");
				//turn off SYNC							
				SW_Video_Status = 0;
				//dPrintf("\nSW_Video_Status:%bd",SW_Video_Status);

				Interrupt_enableSyncDetect(OFF);
#ifdef SUPPORT_COMPONENT
				if(InputMain == INPUT_COMP) {
					//Change to 0 for fast recover.
					VAdcSetFilterBandwidth(0, 0);		
				}
#endif
				//free run
				ScalerCheckPanelFreerunValue();										
				ScalerSetFreerunManual(ON);	// turn on Free Run Manual
				ScalerSetMuteManual( ON );	// turn on Mute Manual
			
				//start "No Signal" blinking
				if(MenuGetLevel()==0) {
					FOsdIndexMsgPrint(FOSD_STR2_NOSIGNAL);
#ifdef NOSIGNAL_LOGO
					if(getNoSignalLogoStatus() == 0)
						InitLogo1();						
#endif
				}

				tic_task = 0;
				TaskNoSignal_setCmd(TASK_CMD_WAIT_VIDEO);	//block the negotiation until you have a Video Signal
			}
			else {
				//tic_task = 0;
				TaskNoSignal_setCmd(TASK_CMD_WAIT_VIDEO);	//block the negotiation until you have a Video Signal
			}
		}
		else {
			//Video Found Happen
			if(SW_Video_Status==0) {
				dPuts("\nVideo found Happen");
				SW_Video_Status = 1;
				//dPrintf("\nSW_Video_Status:%bd",SW_Video_Status);

				//turn ON SYNC
				//Interrupt_enableSyncDetect(ON);  not yet. turn on it after it decide the video mode.
			}						

			if(Task_NoSignal_cmd==TASK_CMD_DONE) {
				dPrintf("\n********RECHECK");
				tic_task = NOSIGNAL_TIME_INTERVAL;			//do it right now..	
			}
			else {
				tic_task=NOSIGNAL_TIME_INTERVAL - 500;		//wait 500ms. 100ms is too short.
			}
			//start negotiation
			TaskNoSignal_setCmd(TASK_CMD_RUN);	
		}
	}
	WriteTW88Page(page);
}




//=============================================================================
// MAIN LOOP				                                               
//=============================================================================
//external
//	INT_STATUS
//	EX0
//	P1_3
//extern BYTE	TouchStatus;
//
#define RET_MAIN_LOOP_PSM_BY_REMO	1
#define RET_MAIN_LOOP_PSM_BY_PORT	2
#define RET_MAIN_LOOP_RCDMODE		3
/**
 * main_loop
 *
 * main_loop
 * =========
 *
 * @param void
 * @return 
 *	0: 
 *	1:PowerSaveMode by Remo
 *	2:PowerSaveMode by Port
 *	3:RCDMode by Port
 * @see ext0_int
 * @see	InterruptPollingRoutine
 * @see	InterruptHandlerRoutine
 * @see	I2CCMD_exec_main
 * @see Monitor
 * @see	CheckKeyIn
 * @see	CheckRemo
 * @see	GetTouch2
 * @see	ActionTouch
 * @see CheckAndClearOSD
 * @see	NoSignalTask
 * @see NoSignalTaskOnWaitMode
*/
BYTE main_loop(void)
{
	BYTE ret;
#ifdef SUPPORT_HDMI_SiIRX
	uint16_t wOldTickCnt = 0;
	uint16_t wNewTickCnt = 0;
	uint16_t wTickDiff;
#endif

	//---------------------------------------------------------------
	//			             Main Loop 
	//---------------------------------------------------------------
	while(1) {
		//-------------- Check TW8835 Interrupt ------------------------
		if(access) {
#ifdef MODEL_TW8835_EXTI2C
			if(eint10_intr_count) {
				Printf("\n$$eint10_intr_count:%bd",eint10_intr_count);
				eint10_intr_count = 0;
				//execute Pseudo ext0_int
				Ext0PseudoISR();
			}
#endif		
			if(INT_STATUS || VH_Loss_Changed ) {
				InterruptPollingRoutine();
				InterruptHandlerRoutine();
			}
		}
#ifdef MODEL_TW8836
		if(INT_STATUS3) {
			extern DWORD IntCount;
			Printf("\nINT_STATUS3:%bx  count: %ld",INT_STATUS3, IntCount );
			INT_STATUS3 = 0;
		}
#endif

		//-------------- I2CCMD -------------------------------
#if defined(SUPPORT_I2CCMD_SLAVE_V1)
		if(F_i2ccmd_exec) {
			F_i2ccmd_exec = 0;
			I2CCMD_exec_main();
		}
#endif

		//-------------- ext_i2c_timer ------------------------
#if defined(SUPPORT_I2CCMD_TEST_SLAVE)
		if(ext_i2c_cmd) {
			if(ext_i2c_timer)
				continue;

			//if timeover
			WriteTW88(REG009,0);	// ? clear
			
			ext_i2c_cmd = 0;		//clear
			InitISR(0);	
			Printf("\next_i2c_timer expire");
		}
#endif

		//-------------- Check Watchdog ------------------------
#ifndef MODEL_TW8835_EXTI2C
#ifdef DEBUG_WATCHDOG
		if( F_watch ) {
			static DWORD wdt_last=0;
			DWORD wdt_diff;

			F_watch = 0;

			//RestartWatchdog
			RestartWatchdog();

			wdt_diff = SystemClock - wdt_last;
			wdt_last = SystemClock;
			ePrintf("\nWatchdog Interrupt !!! %02bx  %ld.%ldsec", SFR_WDCON, SystemClock/100, SystemClock%100);
			ePrintf(" diff:%ld.%02ldsec",wdt_diff/100, wdt_diff%100);

			RestartWatchdog();
		}
#elif defined(SUPPORT_WATCHDOG)
		//RestartWatchdog
		RestartWatchdog();
#endif
#endif		
	
		//-------------- Check Serial Port ---------------------
		Monitor();				// for new monitor functions
#ifdef SUPPORT_UART1
		Monitor1();				// for UART1
#endif


		//-------------- SW I2C SLAVE ---------------------
#ifdef SW_I2C_SLAVE
		if(dbg_sw_i2c_sda_count && sw_i2c_index==0 /*(E2IE & 0x40)==0*/) {
			BYTE i;
			//only print out when you have a READ command.
			//the previous WRITE command also print out when you have a READ command.
			//or
			//print out when sw_i2c_index has a value 0.
			Printf("\nSW_I2C_SLAVE %bd",dbg_sw_i2c_sda_count);
			for(i=0; i < dbg_sw_i2c_sda_count; i++) {
				if(dbg_sw_i2c_index[i]) {
					Printf("\n%bd::index:%bx devid:%02bx",
						i,dbg_sw_i2c_index[i],dbg_sw_i2c_devid[i] );
	
					if(dbg_sw_i2c_devid[i] & 0x01) {
						//read 
						Printf(" ACK:%02bx",dbg_sw_i2c_data[i]);
					}
					else {
						//write
						Printf(" regidx:%02bx",dbg_sw_i2c_regidx[i]);
						Printf(" data:%02bx",dbg_sw_i2c_data[i]);
					}	
				}
			}
			dbg_sw_i2c_sda_count=0;
		}
#endif

		//-------------- block access routines -----------------
		if ( access == 0 ) continue;		
		/* NOTE: If you donot have an access, You can not pass */

 		//-------------- Check Keypad input --------------------
		CheckKeyIn();

 		//-------------- Check Remote Controller ---------------
		ret = CheckRemo();
		if(ret == REQUEST_POWER_OFF && SysNoInitMode==SYS_MODE_NORMAL) {
			ePrintf("\n===POWER SAVE===by Remo");
			ret = 1;
			break;
		}

 		//-------------- Check special port ---------------
#ifdef SUPPORT_RCD
		if(IsBackDrivePortOn() && SysNoInitMode==SYS_MODE_NORMAL) {
			ePrintf("\n===RCDMode requested");
			ret = 3;
			break;
		}			
#endif		
		if(PORT_POWER_SAVE==1 && SysNoInitMode==SYS_MODE_NORMAL) {
			ePrintf("\n===POWER SAVE===by PORT_POWER_SAVE");
			ret = 2;
			break;
		}
#if defined(MODEL_TW8835_SLAVE)
#if defined(SUPPORT_EXTMCU_ISP) && defined(USE_EXTMCU_ISP_GPIO)
		if(PORT_EXTMCU_ISP==0 && SysNoInitMode==SYS_MODE_NORMAL)
			StopCPU();
#endif
#endif

		//-------------- Check Touch ---------------
#ifdef SUPPORT_TOUCH
#ifdef MODEL_TW8835_EXTI2C
		if(timer1_intr_count)
			TscPseudoISR();
#endif
		if ( TraceAuto ) TraceTouch();
		ret = GetTouch2();
		if(ret) {
#ifdef DEBUG_TOUCH_SW
			dPrintf("==>Tsc Action");
#endif
			ActionTouch();		
		}
#endif
			
		//-------------- Check OSD timer -----------------------
		CheckAndClearOSD();

		//============== HDMI Section ==========================
#ifdef SUPPORT_HDMI_SiIRX
        //if ( (!DEBUG_PAUSE_FIRMWARE) && (!GPIO_GetComMode())  )
        {
            if (TIMER_Expired(TIMER__POLLING)) {
                TIMER_Set(TIMER__POLLING, 20);       //poll every 20ms	   
                //+PollPortSwitch();  
                PollInterrupt();
            }
			//+CEC_Event_Handler();
			HdmiTask();

			wNewTickCnt = TIMER_GetTickCounter();
			if ( wNewTickCnt > wOldTickCnt ){
				wTickDiff = wNewTickCnt - wOldTickCnt;
			}
			else { /* counter wrapping */
				wTickDiff = ( 0xFFFF - wOldTickCnt ) + wNewTickCnt;
			}
			wTickDiff >>= 1; /* scaling ticks to ms */
			if ( wTickDiff > 36 ){
				wOldTickCnt = wNewTickCnt;
				HdmiProcIfTo( wTickDiff );
			}
        }
        DEBUG_POLL();
#endif

		//============== Task Section ==========================

		if(Task_Grid_on)
			MovingGridTask();
		//--------------
		
		NoSignalTask();
		NoSignalTaskOnWaitMode(); //Check each input status when WAIT_MODE
	} //..while(1)

	return ret;
}


//=============================================================================
// MAIN
//=============================================================================
#define PSM_REQ_FROM_NORMAL		1
#define PSM_REQ_FROM_RCDMODE	2
/**
 * main
 *
 * main
 * ====
 *
 *	InitCPU
 *	InitCore
 *	InitSystem
 *	while(1)
 *		main_loop
 *
 * @param void
 * @return NONE
 * @see InitCPU
 * @see InitCore
 * @see InitSystem
 * @see main_loop
*/
void main(void)
{
	BYTE request_power_save_mode;
	BYTE ret;
#ifdef SUPPORT_RCD
	DWORD BootTime;
#endif

	InitVariables();
	InitCPU();
#if defined(MODEL_TW8835_EXTI2C)
	InitHostCore(1);
#else
	InitCore(1);
#endif

	//-------------------------------------
	// PRINT MODEL VERSION
#ifdef MODEL_TW8835_EXTI2C
	PrintModelVersionInfo(0);
#else
	PrintModelVersionInfo();
#endif

	if(access==0) {
		Puts("\n***SKIP_MODE_ON***");
		DebugLevel=3;
		//skip...do nothing
		Puts("\nneed **init core***ee find***init***");
	}

	SetMonAddress(TW88I2CAddress);
	Prompt(); //first prompt

#ifdef MODEL_TW8836RTL
	//-------------------------------------
	// RTL Verification
	//-------------------------------------
	ret=main_loop();
	//you can not be here...
#endif

	//==================================================
	// Init System
	//==================================================
#if defined(MODEL_TW8835_EXTI2C)
	InitHostSystem(1);

	//Check Slave
	WriteTW88Page(0);
	if((ReadTW88(REG000) & 0xFC) == 0x74)
		Printf("\nOK Slave is a TW8835");
	else {
		Printf("\nSlave fail..Please reboot Slave & Master");
		return;
	}
	//init slave core
	InitCore(1);
	//-------------------------------------
	// PRINT Slave VERSION
	//-------------------------------------
	PrintModelVersionInfo(1);

	SetMonAddress(TW88I2CAddress);
	Prompt(); //2nd prompt
#endif //..MODEL_TW8835_EXTI2C

	//-------------------
	// InitSystem
	//-------------------
#ifdef CHIP_MANUAL_TEST
	InitSystemForChipTest(1);
	Chip_Manual_Test();
#else
#ifdef SUPPORT_RCD
	if((PORT_BACKDRIVE_MODE==0) && (SysNoInitMode == SYS_MODE_NORMAL)) { //see IsBackDrivePortOn()
		InputMain = 0;	//dummy
		InitRCDMode(1);
	}
	else 
#endif
	InitSystem(1);
#endif

	PrintSystemClockMsg("start loop");
	//dump clock
	DumpClock(0);
#ifdef MODEL_TW8835_EXTI2C
	Prompt(); //third prompt
#else
	Prompt(); //second prompt
#endif


#if 0
//Printf("\nline:%d",__LINE__);
	WriteHostPage(0);
	Printf("\nHOST SSPLL:%bx:%bx:%bx", ReadHost(REG0F8), ReadHost(REG0F9), ReadHost(REG0FA));
	WriteHostPage(2);
	Printf("\nHOST 20D:%bx", ReadHost(REG20D));
	WriteHostPage(PAGE4_CLOCK);
	Printf("\nHOST 4E0:%bx 4E1:%bx", ReadHost(REG4E0), ReadHost(REG4E1));

	WriteHost(REG4E1, 0x02);
Printf("\nline:%d",__LINE__);
	WriteHost(REG4E0, 0x01);
Printf("\nline:%d",__LINE__);
//??	WriteHost(REG4E1, 0x22);
	WriteHostPage(0);
Printf("\nline:%d",__LINE__);
#endif

	WriteTW88Page(PAGE0_GENERAL);

#if 1
	WriteTW88(REG084, 0x0C);
	WriteTW88(REG08C, 0x0C);
	WriteTW88(REG0E3, 0x1C);
	WriteTW88(REG0E0, 0x61);
	WriteTW88(REG0E8, 0x61);
	WriteTW88(REG0EB, 0x29);

#endif


	request_power_save_mode = 0;
	while(1) 
	{
		//----------------------
		//check power save first
		//----------------------
		if((SysNoInitMode == SYS_MODE_NORMAL) && (request_power_save_mode)) {
			//move to PowerSaveMode
			SystemPowerSave();	
			WaitPowerOn();		//wait PowerOn keypad, PowerOn Remo. not Touch
			SystemPowerResume();

#ifdef SUPPORT_RCD
			if(request_power_save_mode == PSM_REQ_FROM_NORMAL) {
				if(IsBackDrivePortOn())	
					//Normal->RCDMode
					InitRCDMode(0);
				else {
					//Normal->Normal
					//InitSystem(0);		
				}
			}
			else if(request_power_save_mode == PSM_REQ_FROM_RCDMODE) {
				if(IsBackDrivePortOn()==0) {
					//RCDMode->Normal
					//turn off a parkgrid task first
					TaskSetGrid(OFF);
					SpiOsdWinHWEnable(0,OFF);
					SpiOsdWinHWEnable(1,OFF);
					SpiOsdEnable(OFF);
					InitSystem(0);	//InitSystem with skip ...
				}
				//else
				//	RCDMode->RCDMode
			}
#endif
			request_power_save_mode=0;
		}
#ifdef SUPPORT_RCD
		//----------------------
		//check RCD mode
		else if((SysNoInitMode == SYS_MODE_NORMAL) && IsBackDrivePortOn()) {
			//==================================================
			// RCDMODE LOOP
			//==================================================
			ret=RCDMode_loop();
			ePrintf("\nRCDMode_loop() ret %bd",ret);
			if(ret==RET_RCDMODE_PSM)
				//move to PSMode
				request_power_save_mode=PSM_REQ_FROM_RCDMODE;	//from RCDMODE	
			else if(ret==RET_RCDMODE_OFF) {
				BootTime = SystemClock;
				//move to Normal Mode
				//turn off a parkgrid task first
				TaskSetGrid(OFF);
				SpiOsdWinHWEnable(0,OFF);
				SpiOsdWinHWEnable(1,OFF);
				SpiOsdEnable(OFF);
				InitSystem(0);		//InitSystem with skip ...

				BootTime = SystemClock - BootTime;
				ePrintf("\nBootTime(RCD2Normal):%ld.%ldsec", BootTime/100, BootTime%100 );
			}
		}
#endif
		//----------------------
		//normal or NoInit mode
		else {
			//==================================================
			// MAIN LOOP
			//==================================================
			ret=main_loop();		
			//exit when power save. or RCDMode. 
			//FYI:SkipMode can not exit the main_loop.
			ePrintf("\nmain_loop() ret %bd",ret);

#ifdef SUPPORT_RCD
			if(ret==RET_MAIN_LOOP_RCDMODE) {	
				BootTime = SystemClock;
				FOsdOnOff(OFF, 0);	////with vdelay 1		

				//move to RCDMode
				InitRCDMode(0);

				BootTime = SystemClock - BootTime;
				ePrintf("\nBootTime(Normal2RCD):%ld.%ldsec", BootTime/100, BootTime%100 );
			}
			else
#endif			 
			if(ret==RET_MAIN_LOOP_PSM_BY_REMO || ret==RET_MAIN_LOOP_PSM_BY_PORT)
				//move to PowerSave Mode
				request_power_save_mode=PSM_REQ_FROM_NORMAL;
			//else 
			//	DO NOTHING
		}
	}	
	//you can not be here...
}
//==============MAIN.C=======END
