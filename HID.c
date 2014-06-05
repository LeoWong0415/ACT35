/**
 * @file
 * HID.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	Human Interface Device
 *	support Keypad, touch, remocon
*/
#include "config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "global.h"
#include "CPU.h"
#include "printf.h"

#include "I2C.h"

#include "main.h"
#include "Remo.h"
#include "TouchKey.h"
#include "HID.h"

#include "SOsd.h"
#include "FOsd.h"
#include "SOsdMenu.h" //NAVI_KEY_
#include "eeprom.h"
#include "InputCtrl.h"

#ifdef SUPPORT_FOSD_MENU
#include "FOsdString.h"
#include "FOsdMenu.h"
#include "FOsdDisplay.h"
extern    bit     OnChangingValue;
#endif

BYTE	InputMode = 0;
bit		OnPIP = 0;
bit		CaptureMode =0 ;
BYTE	SavePIP = 2;		// PIP_CVBS - start mode
BYTE	SaveInput = 0;	// 0(CVBS) or 1(SVGA60) for demo
BYTE	SaveNum = 0;
BYTE	LoadNum = 0;
#define CAPTURE_MAX		10

/**
* action for remocon
*/
BYTE ActionRemo(BYTE _RemoDataCode, BYTE AutoKey)
{
	DECLARE_LOCAL_page
	BYTE value;

	UpdateOsdTimerClock();

	//BKTODO: It need on Keypad also....Let's merge remo & keypad & monitor.
	if(TaskGetGrid()) {
		if(_RemoDataCode == REMO_CHNUP) {
			TaskSetGridCmd(NAVI_KEY_UP);
			return 0;
		}
		else if(_RemoDataCode == REMO_CHNDN) {
			TaskSetGridCmd(NAVI_KEY_DOWN);
			return 0;
		}
		else if(_RemoDataCode == REMO_VOLDN) {
			TaskSetGridCmd(NAVI_KEY_LEFT);
			return 0;
		}
		else if(_RemoDataCode == REMO_VOLUP) {
			TaskSetGridCmd(NAVI_KEY_RIGHT);
			return 0;
		}
		else if(_RemoDataCode == REMO_MENU || _RemoDataCode == REMO_EXIT) {
			TaskSetGridCmd(NAVI_KEY_ENTER);
			return 0;
		}
	}


	switch(_RemoDataCode) {

	case REMO_STANDBY:				// power
		if( AutoKey )				// need repeat key. 
			//return 1;
			return REQUEST_POWER_OFF;
		return 0;					// power off

	case REMO_MUTE:
//#ifdef SUPPORT_FOSD_MENU
//		if( AutoKey ) return 1;
//		ToggleAudioMute();
//		if( IsAudioMuteOn() )		DisplayMuteInfo();
//		else{						
//			ClearMuteInfo();
//			if( DisplayInputHold ) FOsdDisplayInput();
//		}
//#endif
		break;

	case REMO_INPUT:
//#ifdef SUPPORT_FOSD_MENU
//		InputModeNext();
//#else
		if(MenuGetLevel()) {
			//ignore
			break;	
		}
		//increase input mode..
		InputModeNext();
//#endif
		break;
	case REMO_INFO:
		if(MenuGetLevel()==0) {
			//toggle current video information.
			if(TaskNoSignal_getCmd()==TASK_CMD_DONE) {
//#ifdef SUPPORT_FOSD_MENU
//				FOsdDisplayInput();	
//#else
				ReadTW88Page(page);
				WriteTW88Page(PAGE3_FOSD);
				if((ReadTW88(REG310) & 0x80)==0) {
					FOsdCopyMsgBuff2Osdram(OFF);
				}
				WriteTW88Page(page);
				FOsdWinToggleEnable(0);	//win0
//#endif
			}				
		}
		return 1;
	
	case REMO_NUM0:
	case REMO_NUM1:
	case REMO_NUM2:
	case REMO_NUM3:
	case REMO_NUM4:
	case REMO_NUM5:
	case REMO_NUM6:
	case REMO_NUM7:
	case REMO_NUM8:
	case REMO_NUM9:
		break;

	
	case REMO_SELECT:
//#ifdef SUPPORT_FOSD_MENU
//		if( AutoKey ) return 1;
//		if(  DisplayedOSD & FOSD_MENU  )
//			FOsdMenuProcSelectKey();
//#else
		if(MenuGetLevel())
			MenuKeyInput(NAVI_KEY_ENTER);	
//#endif
		break;

	case REMO_CHNUP:
//#ifdef SUPPORT_FOSD_MENU
//		if(DisplayedOSD & FOSD_MENU)
//			FOsdMenuMoveCursor(FOSD_UP);
//#else
  		dPuts("\nREMO_CHNUP pressed!!!");
		if(MenuGetLevel())
			MenuKeyInput(NAVI_KEY_UP);
//#endif
		break;
	case REMO_CHNDN:
//#ifdef SUPPORT_FOSD_MENU
//		if(DisplayedOSD & FOSD_MENU)
//			FOsdMenuMoveCursor(FOSD_DN);
//#else
  		dPuts("\nREMO_CHNDN pressed!!!");
		if(MenuGetLevel())
			MenuKeyInput(NAVI_KEY_DOWN);
//#endif
		break;
	case REMO_VOLUP:
//#ifdef SUPPORT_FOSD_MENU
//		if(  DisplayedOSD & FOSD_MENU  ) {
//			if( OnChangingValue ) 	FOsdMenuValueUpDn(FOSD_UP );
//			else					FOsdMenuProcSelectKey();			
//		}
//		else 
//		{
//			ChangeVol(1);
//			DisplayVol();
//		}
//#else
  		dPuts("\nREMO_VOLUP pressed!!!");
		if(MenuGetLevel())
			MenuKeyInput(NAVI_KEY_RIGHT);
//#endif
		break;
	case REMO_VOLDN:
//#ifdef SUPPORT_FOSD_MENU
//		if(  DisplayedOSD & FOSD_MENU  ) {
//			if( OnChangingValue ) FOsdMenuValueUpDn(FOSD_DN );
//			else FOsdHighMenu();	
//		}
//		else 
//		{
//			ChangeVol(-1);
//			DisplayVol();
//		}
//#else
  		dPuts("\nREMO_VOLDN pressed!!!");
		if(MenuGetLevel())
			MenuKeyInput(NAVI_KEY_LEFT);
//#endif
		break;
	case REMO_MENU:
  		dPuts("\nREMO_MENU pressed!!!");
//#ifdef SUPPORT_FOSD_MENU
//		if(DisplayedOSD & FOSD_MENU)
//			FOsdHighMenu();
//		else {
//			if(GetOSDLangEE()==OSDLANG_KR)
//				InitFontRamByNum(FONT_NUM_PLUS_RAMFONT, 0);
//			else 
//				InitFontRamByNum(FONT_NUM_DEF12X18, 0);	//FOsdDownloadFont(0);	//12x18 default font
//			FOsdRamSetFifo(ON, 1);
//			FOsdMenuOpen();		
//		}
//#else
		if(MenuGetLevel()==0)
			MenuStart();	
		else
			MenuKeyInput(NAVI_KEY_ENTER);	
//#endif
		break;


	case REMO_EXIT:
  		dPuts("\nREMO_EXIT pressed!!!");
//#ifdef SUPPORT_FOSD_MENU
//		if(  DisplayedOSD & FOSD_MENU  )		
//			FOsdMenuDoAction(EXITMENU);
//#else
		if(MenuGetLevel() == 1) {
			MenuEnd();
		}
		else if(MenuGetLevel() > 1) {
			proc_menu_list_return();	//only one level...
		}
//#endif
		break;

	case REMO_PIPON:
		dPuts("\nPIP Display mode change");
		break;
				 
	case REMO_STILL:
		break;
	case REMO_SWAP:
		//Printf("\n=====SWAP InputMode:%bd",InputMain);
		if(InputMain==INPUT_BT656) {
			ReadTW88Page(page);
			WriteTW88Page(PAGE0_GENERAL);
			value = ReadTW88(REG006);
			if(value & 0x40) 	WriteTW88(REG006, value & ~0x40);
			else				WriteTW88(REG006, value | 0x40);
			WriteTW88Page(page);
		}			
		break;

	case REMO_PIPINPUT:
		dPuts("\nPIP input mode change");
		break;

	case REMO_TTXRED:
		break;


	case REMO_TTXYELLOW:
		break;

	case REMO_TTXGREEN:	
		break;
	
	case REMO_TTXCYAN:		
		break;

	case REMO_AUTO:
		break;

	case REMO_ASPECT:
		break;

	}
	return 1;
}

#ifdef SUPPORT_TOUCH
//extern BYTE	TouchStatus;
extern WORD PosX,PosY;
/**
* action for Touch
*/
BYTE ActionTouch(void)
{
	BYTE TscStatus;
	BYTE ret;
	WORD	xpos, ypos;

	UpdateOsdTimerClock();

	TscStatus = TouchStatus;
	xpos = PosX;
	ypos = PosY;

	if(MenuGetLevel()==0) {
 
		if(TaskGetGrid()) {
			if(TscStatus == TOUCHCLICK || TscStatus == TOUCHDOUBLECLICK)
			{
				TaskSetGridCmd(NAVI_KEY_ENTER);
				return 0;
			}
		}
		if(TscStatus == TOUCHDOUBLECLICK) {
			MenuStart();
			SetTouchStatus(TOUCHEND); //BK111108
		}
		else if(TscStatus == TOUCHPRESS || TscStatus >= TOUCHMOVE) {
		}
		else if(TscStatus == TOUCHMOVED) {
			SetTouchStatus(TOUCHEND); //END
		}
		else if(TscStatus == TOUCHCLICK) {
			SetTouchStatus(TOUCHEND); //END
		}

		return 0;	
	}

	//in the menu
	if(TscStatus == TOUCHPRESS || (TscStatus >= TOUCHMOVE)) {
		//serial input mode.
		ret = MenuIsTouchCalibMode();
		if(ret) {
			CalibTouch(ret-1);

			MenuKeyInput(NAVI_KEY_ENTER); //goto next "+"
			//wait until TOUCH_END
			WaitTouchButtonUp();
			SetTouchStatus(TOUCHEND);
		}
//		else if(MenuIsSlideMode()) {
//			//need MOVE for slider bar
//			MenuCheckTouchInput(TscStatus, xpos, ypos);
//			//note: do not call SetTouchStatus(0);
//		}
		else {
			//
			//update focus.
			//
			MenuCheckTouchInput(TscStatus, xpos, ypos);
			//note: do not call SetTouchStatus(0);
		}
		return 0;
	}
	//else if(TscStatus == TOUCHMOVE) {
	//	//if lost focus, do something.
	//}
	else if(TscStatus == TOUCHCLICK || TscStatus == TOUCHDOUBLECLICK || TscStatus == TOUCHLONGCLICK || TscStatus == TOUCHMOVED) {
		//action mode

		if(TscStatus == TOUCHLONGCLICK) {
			if(MenuGetLevel()==1 || MenuIsSystemPage()) {
				//special.
				//use default value.
				MenuTouchCalibStart();
				SetTouchStatus(TOUCHEND);
				return 0;
			}
		}

		if(TscStatus == TOUCHMOVED)
			TscStatus = TOUCHCLICK;

		MenuCheckTouchInput(TscStatus,xpos, ypos);
		SetTouchStatus(TOUCHEND);  //BK110601
		return 0;
	}
	ePrintf("\nUnknown TscStatus :%bd", TscStatus);

	return 1;
}
#endif


/**
* check Key input
*
* keypad: LEFT, RIGHT, DOWN, UP, MENU, MODE
*/
void CheckKeyIn( void ) 
{
	BYTE	key;

//#ifdef SUPPORT_TOUCH
//	//if we support the touch, we need to init TscAdc again
//	InitAUX(3);
//#endif

	//key = ReadKeyPad();
	key = GetKey(1);
	if(key == 0) {
		//sw key
		if(SW_key) {
			key = SW_key;
			SW_key=0;
		}
		if( key == 0)
			return;
	}

	UpdateOsdTimerClock();

#ifdef DEBUG_KEYREMO
	dPrintf("\nGetKey(1):%02bx",key);
#endif
	if(TaskGetGrid()) {
		switch(key) {
	  	case KEY_RIGHT:
			TaskSetGridCmd(NAVI_KEY_RIGHT);
			break;
		case KEY_UP:
			TaskSetGridCmd(NAVI_KEY_UP);
			break;
		case KEY_LEFT:
			TaskSetGridCmd(NAVI_KEY_LEFT);
			break;
		case KEY_DOWN:
			TaskSetGridCmd(NAVI_KEY_DOWN);
			break;
		case KEY_INPUT:
		case KEY_MENU:
			TaskSetGridCmd(NAVI_KEY_ENTER);
			break;
		}
		return ;
	}


	switch ( key ) {
	  case 	KEY_RIGHT:
#ifdef DEBUG_KEYREMO
  		dPuts("\nRIGHT pressed!!!");
#endif
#ifdef SUPPORT_FOSD_MENU
		if(  DisplayedOSD & FOSD_MENU  ) {
			if( OnChangingValue ) 	FOsdMenuValueUpDn(FOSD_UP );
			else					FOsdMenuProcSelectKey();			
		}
		else 
		{
			ChangeVol(1);
			DisplayVol();
		}
#else
		if(MenuGetLevel())
			MenuKeyInput(NAVI_KEY_RIGHT);
#endif
    	break;

	  case 	KEY_UP:
#ifdef DEBUG_KEYREMO
  		dPuts("\nUP pressed!!!");
#endif
#ifdef SUPPORT_FOSD_MENU
		if(DisplayedOSD & FOSD_MENU)
			FOsdMenuMoveCursor(FOSD_UP);
#else
		if(MenuGetLevel())
			MenuKeyInput(NAVI_KEY_UP);
#endif
    	break;

	  case 	KEY_MENU:
#ifdef DEBUG_KEYREMO
  		dPuts("\nMENU pressed!!!");
#endif
#ifdef SUPPORT_FOSD_MENU
		if(DisplayedOSD & FOSD_MENU)
			FOsdHighMenu();
		else {
			if(GetOSDLangEE()==OSDLANG_KR)
				InitFontRamByNum(FONT_NUM_PLUS_RAMFONT, 0);
			else 
				InitFontRamByNum(FONT_NUM_DEF12X18, 0);	//FOsdDownloadFont(0);	//12x18 default font
			FOsdRamSetFifo(ON, 1);
			FOsdMenuOpen();		
		}
#else
		if(MenuGetLevel()==0)
			MenuStart();	
		else
			MenuKeyInput(NAVI_KEY_ENTER);	
#endif
    	break;

	  case 	KEY_LEFT:
#ifdef DEBUG_KEYREMO
  		dPuts("\nLEFT pressed!!!");
#endif
#ifdef SUPPORT_FOSD_MENU
		if(  DisplayedOSD & FOSD_MENU  ) {
			if( OnChangingValue ) FOsdMenuValueUpDn(FOSD_DN );
			else FOsdHighMenu();	
		}
		else 
		{
			ChangeVol(-1);
			DisplayVol();
		}
#else
		if(MenuGetLevel())
			MenuKeyInput(NAVI_KEY_LEFT);
#endif
    	break;

	  case 	KEY_DOWN:
#ifdef DEBUG_KEYREMO
  		dPuts("\nDOWN pressed!!!");
#endif
#ifdef SUPPORT_FOSD_MENU
		if(DisplayedOSD & FOSD_MENU)
			FOsdMenuMoveCursor(FOSD_DN);
#else
		if(MenuGetLevel())
			MenuKeyInput(NAVI_KEY_DOWN);
#endif
    	break;

	  case 	KEY_INPUT:
#ifdef DEBUG_KEYREMO
  		dPuts("\nINPUT pressed!!!");
#endif
		if(MenuGetLevel())
			MenuEnd();
		else
			InputModeNext();
    	break;
	}
}


//=============================================================================
//		BYTE CheckHumanInputs( void )
//=============================================================================
/**
* check Human Inputs. (Keypad, touch, remocon, UART0)
*
* Just Check, NO Action.
* extern BYTE TouchChanged;
* @return
*	-0: No Input
*	-other: input type
*/
BYTE	CheckHumanInputs( BYTE skip_tsc )
{
#ifndef DP80390
#ifdef SUPPORT_TOUCH
#else
	BYTE temp = skip_tsc;
#endif

	//
	//check remo input
	//
	//if(CheckRemo())
	//	return 1;
	if(RemoDataReady)
		return HINPUT_REMO;

	//
	//check keypad
	//
//#ifdef SUPPORT_ANALOG_SENSOR_NEED_VERIFY
//	//CheckKeyIn();
//	if ( ReadKeyPad() ) {
//		dPuts("\nGet Key data");
//		return ( HINPUT_KEY );		// input!!!
//	}
//#endif
	//
	//check touch
	//	
#ifdef SUPPORT_TOUCH
	if(skip_tsc==0) {
		if ( CpuTouchPressed || CpuTouchChanged ) {
			dPrintf("\nSenseTouch CpuTouchPressed:%bd CpuTouchChanged:%bd",CpuTouchPressed,CpuTouchChanged);
			return ( HINPUT_TSC );		// input!!!
		}
	}
#endif

	//
	//check RS232 monitor input
	//	
	if (RS_ready()) {
		dPuts("\nGet serial data");
		return ( HINPUT_SERIAL );		// input!!!
	}
#else
	BYTE temp = skip_tsc;
#endif //DP80390

	return (HINPUT_NO);
}


