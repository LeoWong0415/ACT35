/**
 * @file
 * HDMI_SIL9127.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	HDMI SIL9127 device driver.
 * 	Need a NDA for SiliconImage Code.
*/

#include "Config.h"

#ifdef SUPPORT_HDMI_SiIRX
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"
#include "cpu.h"
#include "HDMI_SIL9127.h"
#include "printf.h"
#include "i2c.h"
#include "main.h"
#include "dtv.h"

#include <SII_config.h>
#include <hal.h>
#include <debug_cmd.h>
#include <registers.h>
#include <amf.h>
#include <infofrm.h>

#include <CEC.h>

//===================================================================
// main loop area
//===================================================================

//------------------------------------------------------------------------------
// Global State Structure
//------------------------------------------------------------------------------
GlobalStatus_t CurrentStatus;



//===================================================================
// comes from hal_cp9223.c
//===================================================================
//I2C slave address
#define ADAC_SLAVE_ADDR     0x24	 //changed from 0x26 to avoid conflict with Simon

//register offsets
#define ADAC_CTRL1_ADDR     0x00
#define ADAC_CTRL2_ADDR     0x01
#define ADAC_SPEED_PD_ADDR  0x02
#define ADAC_CTRL3_ADDR     0x0A

//power down control bits
#define ADAC_POWER_DOWN        0x00
#define ADAC_NORMAL_OPERATION  0x4F

//dac operating modes
#define ADAC_PCM_MODE       0x00
#define ADAC_DSD_MODE       0x18

//------------------------------------------------------------------------------
// Function: HAL_HardwareReset
// Description: Give a hardware reset pulse to the chip
//------------------------------------------------------------------------------
void HAL_HardwareReset(void)
{
    GPIO_ClearPins(GPIO_PIN__HARDWARE_RESET);
    TIMER_Wait(TIMER__DELAY, 2);

    GPIO_SetPins(GPIO_PIN__HARDWARE_RESET);
    TIMER_Wait(TIMER__DELAY, 5);

}
#if (CONF__POLL_INTERRUPT_PIN == ENABLE)
//------------------------------------------------------------------------------
// Function: HAL_GetInterruptPinState
// Description: Get current state of interrupt pin input
//------------------------------------------------------------------------------
uint8_t HAL_GetInterruptPinState(void)
{
	BYTE page,value;
    uint8_t pinState = 0;

	Printf("\nGPIO_GetPins:RX_INTERRUPT");
	page = ReadTW88(0xff);
	WriteTW88Page(0);
	value = ReadTW88(0x98);
	WriteTW88Page(page);
	if(value & 0x02)
		pinState |= GPIO_PIN__RX_INTERRUPT;
	return pinState;
}
#endif



//------------------------------------------------------------------------------
// Function: HAL_PowerDownAudioDAC
// Description: Send power down command to audio DAC
//              Also mute the output by disableing output latch seperate from audio DAC
//------------------------------------------------------------------------------
void HAL_PowerDownAudioDAC(void)
{
    //+WriteI2CByte(ADAC_SLAVE_ADDR, ADAC_SPEED_PD_ADDR, ADAC_POWER_DOWN);    
    GPIO_SetPins(GPIO_PIN__AUDIO_DAC_MUTE);   //set mute pin (latch on 9135)
}


//------------------------------------------------------------------------------
// Function: HAL_WakeUpAudioDAC
// Description: Send wake up command to audio DAC
//              Also un-mute the output by enabling output latch seperate from audio DAC
//------------------------------------------------------------------------------
void HAL_WakeUpAudioDAC(void)
{
    //+WriteI2CByte(ADAC_SLAVE_ADDR, ADAC_SPEED_PD_ADDR, ADAC_NORMAL_OPERATION);  
    GPIO_ClearPins(GPIO_PIN__AUDIO_DAC_MUTE);  //clear mute pin (latch on 9135)
}


//------------------------------------------------------------------------------
// Function: HAL_SetAudioDACMode
// Description: Send mode change command to audio DAC to select DSD or standard PCM mode
//------------------------------------------------------------------------------
void HAL_SetAudioDACMode(uint8_t dacMode)
{
    //+WriteI2CByte(ADAC_SLAVE_ADDR, ADAC_SPEED_PD_ADDR, ADAC_POWER_DOWN);

    if (dacMode)
    {
        //+WriteI2CByte(ADAC_SLAVE_ADDR, ADAC_CTRL3_ADDR, ADAC_DSD_MODE);
        DEBUG_PRINT(("dsd dac\n"));
    }
    else {
        //+WriteI2CByte(ADAC_SLAVE_ADDR, ADAC_CTRL3_ADDR, ADAC_PCM_MODE);
        DEBUG_PRINT(("PCM dac\n"));
    }
}

//------------------------------------------------------------------------------
// Function: HAL_VccEnable
// Description: enable or disable the main +5V power from system
//------------------------------------------------------------------------------
 void HAL_VccEnable(uint8_t qON)
 {
 	Printf("\nHAL_VccEnable:%bx",qON);
	if(qON)
		GPIO_SetPins(GPIO_PIN__VCCEN);	
	else
		GPIO_ClearPins(GPIO_PIN__VCCEN);
 }



//------------------------------------------------------------------------------
// Function: AutoVideoSetup
// Description: Setup registers for Auto Video Mode
//
// Notes: Compile time configuration is done using CONF__* defines in config.h
//------------------------------------------------------------------------------
static void AutoVideoSetup(void)
{    
	const uint8_t unmuteTimeConf[] = {0xFF,0x00,0x00,0xFF,0x00,0x00};

	Puts("\nAutoVideoSetup");
	WriteI2C(I2CID_SIL9127_EDID, REG__WAIT_CYCLE, &unmuteTimeConf[0],6);	//video unmute wait 

    WriteI2CByte(I2CID_SIL9127_DEV0, REG__VID_CTRL,  (BIT__IVS   & CONF__VSYNC_INVERT) |  (BIT__IHS   & CONF__HSYNC_INVERT) );  //set HSYNC,VSNC polarity
    WriteI2CByte(I2CID_SIL9127_EDID, REG__RPI_AUTO_CONFIG, BIT__CHECKSUM_EN|BIT__V_UNMUTE_EN|BIT__HCDP_EN|BIT__TERM_EN);        //auto config
    WriteI2CByte(I2CID_SIL9127_DEV0, DEV1_REG__SRST,      BIT__SWRST_AUTO);            //enable auto sw reset
    WriteI2CByte(I2CID_SIL9127_DEV0, REG__VID_AOF,   CONF__OUTPUT_VIDEO_FORMAT);  //set output video format
    ModifyI2CByte(I2CID_SIL9127_DEV0, REG__AEC_CTRL,  BIT__AVC_EN, SET);                //enable auto video configuration

#if (CONF__ODCK_LIMITED == ENABLE)
	ModifyI2CByte(I2CID_SIL9127_DEV0, REG__SYS_PSTOP, MSK__PCLK_MAX, CONF__PCLK_MAX_CNT);
#endif

#if (PEBBLES_ES1_ZONE_WORKAROUND == ENABLE)	
	WriteI2CByte(I2CID_SIL9127_DEV0, REG__AVC_EN2, BIT__AUTO_DC_CONF);			   //mask out auto configure deep color clock
	WriteI2CByte(I2CID_SIL9127_DEV0, REG__VIDA_XPCNT_EN, BIT__VIDA_XPCNT_EN);	   //en read xpcnt
#endif	 

#if (PEBBLES_STARTER_NO_CLK_DIVIDER == ENABLE)
	ModifyI2CByte(I2CID_SIL9127_DEV0, REG__AVC_EN2, BIT__AUTO_CLK_DIVIDER,SET);	  //msk out auto clk divider
#endif
}


//------------------------------------------------------------------------------
// Function: AutoAudioSetup
// Description: Setup registers for Auto Audio Mode
//------------------------------------------------------------------------------
static void AutoAudioSetup(void)
{
    uint8_t abAecEnables[3];

	Puts("\nAutoAudioSetup");
    ModifyI2CByte(I2CID_SIL9127_DEV1, REG__ACR_CTRL3, MSK__CTS_THRESH, VAL__CTS_THRESH( CONF__CTS_THRESH_VALUE ));

    abAecEnables[0] = (BIT__SYNC_DETECT        |
                       BIT__CKDT_DETECT        |
                       BIT__CABLE_UNPLUG       );
    abAecEnables[1] = (BIT__HDMI_MODE_CHANGED  |
                       BIT__CTS_REUSED         |
                       BIT__AUDIO_FIFO_UNDERUN |
                       BIT__AUDIO_FIFO_OVERRUN |
                       BIT__FS_CHANGED         |
                       BIT__H_RES_CHANGED      );
#if (CONF__VSYNC_OVERFLOW != ENABLE)   
    abAecEnables[2] = (BIT__V_RES_CHANGED      );
#endif
    WriteI2C(I2CID_SIL9127_DEV0, REG__AEC_EN1, abAecEnables, 3);
	ModifyI2CByte(I2CID_SIL9127_DEV0, REG__AEC_CTRL, BIT__CTRL_ACR_EN, SET);
}


//------------------------------------------------------------------------------
// Function: SystemDataReset
// Description: Re-initialize receiver state
//------------------------------------------------------------------------------
void SystemDataReset(void)
{
	Printf("\nSystemDataReset CurrentStatus.PortSelection:%bx",CurrentStatus.PortSelection);
    TurnAudioMute(ON);
    TurnVideoMute(ON);

	CurrentStatus.ResolutionChangeCount = 0;
    CurrentStatus.ColorDepth = 0;
	CurrentStatus.AudioMode = AUDIO_MODE__NOT_INIT;
    ModifyI2CByte(I2CID_SIL9127_DEV0, REG__TMDS_CCTRL2, MSK__DC_CTL, VAL__DC_CTL_8BPP_1X);

	//ConfigureSelectedPort
	Printf("\nConfigureSelectedPort #1");
    ModifyI2CByte(I2CID_SIL9127_DEV0, DEV1_REG__PORT_SWTCH2, MSK__PORT_EN,VAL__PORT1_EN);     //select port 1
    WriteI2CByte(I2CID_SIL9127_DEV0, DEV1_REG__PORT_SWTCH, BIT__DDC1_EN);     //select DDC 1
    HAL_VccEnable(ON);
}

//------------------------------------------------------------------------------
// Function: System_Init
// Description: One time initialization at statup
//------------------------------------------------------------------------------
void HDMI_SystemInit(void)
{
	const uint8_t EQTable[] = {0x8A,0xAA,0x1A,0x2A};
	BYTE i,j;

	Puts("\n==>HDMI_SystemInit");

	Puts("\nAdd HW RESET");
	HAL_HardwareReset();

	//check BOOT_DONE flag
	for(i=0; i < 100; i++) {
		for(j=0; j < 100; j++) {
			if((ReadI2CByte(I2CID_SIL9127_EDID, REG__BSM_STAT)& BIT__BOOT_DONE ))
				break;
			delay1ms(1);
		}
	}
	if(i==100 && j==100) {
		Printf("\nHDMI_SystemInit FAIL!!!");
		return;
	}
    if((ReadI2CByte(I2CID_SIL9127_EDID, REG__BSM_STAT)& BIT__BOOT_ERROR)!=0)
        DEBUG_PRINT(("First Boot error! \n"));

	Printf("\nHDMI_SystemInit:%d HW RESET D60R06:%02bx", __LINE__, ReadI2CByte(0x60,0x06));

	ModifyI2CByte(I2CID_SIL9127_EDID, REG__HPD_HW_CTRL,MSK__INVALIDATE_ALL, SET);	//disable auto HPD conf at RESET
	TurnAudioMute(ON);
	TurnVideoMute(ON);


#if(PEBBLES_ES1_STARTER_CONF==ENABLE)
    WriteI2CByte(I2CID_SIL9127_DEV0, REG__TERMCTRL2, VAL__45OHM);	//1.term default value	

    WriteI2CByte(I2CID_SIL9127_COLOR, REG__FACTORY_A87,0x43);		//2.Set PLL mode to internal and set selcalrefg to F
    WriteI2CByte(I2CID_SIL9127_COLOR, REG__FACTORY_A81,0x18);		//  Set PLL zone to auto and set Div20 to 1

    WriteI2CByte(I2CID_SIL9127_EDID, REG__DRIVE_CNTL,0x64);			//3.change output strength,  

    WriteI2CByte(I2CID_SIL9127_COLOR, REG__FACTORY_ABB,0x04);		//4.desable DEC_CON

    WriteI2C(I2CID_SIL9127_COLOR, REG__FACTORY_A92,&EQTable[0],4);	//5.Repgrogram EQ table
    WriteI2CByte(I2CID_SIL9127_COLOR, REG__FACTORY_AB5,0x40);		//  EnableEQ

    WriteI2CByte(I2CID_SIL9127_EDID, REG__FACTORY_9E5, 0x02);		//6. DLL by pass
	WriteI2CByte(I2CID_SIL9127_COLOR, REG__FACTORY_A89,0x00);		//7. configure the PLLbias 	
	WriteI2CByte(I2CID_SIL9127_DEV0, REG__FACTORY_00E,0x40);		// for ES1.1 conf only
#endif
				
    //+CEC_Init();					  
	ProgramEDID();

    //set recommended values
    WriteI2CByte(I2CID_SIL9127_DEV0, REG__AACR_CFG1, CONF__AACR_CFG1_VALUE);	//pll config #1
    WriteI2CByte(I2CID_SIL9127_EDID, REG__CBUS_PAD_SC, VAL__SC_CONF);			//CBUS slew rate 
    WriteI2CByte(I2CID_SIL9127_DEV0, DEV1_REG__SRST,  BIT__SWRST_AUTO);			//enable auto sw reset
	WriteI2CByte(I2CID_SIL9127_DEV0, REG__INFM_CLR,BIT__CLR_GBD|BIT__CLR_ACP);	//clr GBD & ACP

    WriteI2CByte(I2CID_SIL9127_DEV0, REG__ECC_HDCP_THRES, CONF__HDCPTHRESH & 0xff);			//HDCP threshold low uint8_t
    WriteI2CByte(I2CID_SIL9127_DEV0, REG__ECC_HDCP_THRES+1, (CONF__HDCPTHRESH>>8) & 0xff);	//HDCP threshold high uint8_t
    AutoVideoSetup();
    AutoAudioSetup();
	    
    SetupInterruptMasks();

	Puts("\nInitializePortSwitch=>skip. always 0x06");
    CurrentStatus.PortSelection = 0x06; //0x06 means port#1.
	SystemDataReset();

    TurnPowerDown(OFF);	//supply 3.3V and 1.2V	 						   	

	ModifyI2CByte(I2CID_SIL9127_EDID, REG__HPD_HW_CTRL,MSK__INVALIDATE_ALL, CLEAR); //CLEAR disable auto HPD conf 

	/* Inti Hdmi Info frame related chip registers and data */
	HdmiInitIf ();
}

//------------------------------------------------------------------------------
// Function: HdmiTask
// Description: One time initialization at startup
//------------------------------------------------------------------------------
//static void HdmiTask(void)
void HdmiTask(void)
{
    if ((CurrentStatus.AudioState == STATE_AUDIO__REQUEST_AUDIO) ||
        (CurrentStatus.AudioState == STATE_AUDIO__AUDIO_READY)) {
        if (TIMER_Expired(TIMER__AUDIO)) {
            AudioUnmuteHandler();
        }
    }

    if (CurrentStatus.VideoState == STATE_VIDEO__UNMUTE) {
        if (TIMER_Expired(TIMER__VIDEO)) {
			DEBUG_PRINT(("v time out\n"));
			ResetVideoControl();
        }
    }
#if (PEBBLES_VIDEO_STATUS_2ND_CHECK==ENABLE)
	if (CurrentStatus.VideoState == STATE_VIDEO__ON) {
        if (TIMER_Expired(TIMER__VIDEO)) {
			PclkCheck();
        	TIMER_Set(TIMER__VIDEO, VIDEO_STABLITY_CHECK_INTERVAL);  // start the video timer 
			CurrentStatus.VideoStabilityCheckCount++;

        }
		if (CurrentStatus.VideoStabilityCheckCount == VIDEO_STABLITY_CHECK_TOTAL/VIDEO_STABLITY_CHECK_INTERVAL)	{
			DEBUG_PRINT(("V CHECK %d times\n",(int)CurrentStatus.VideoStabilityCheckCount));
			CurrentStatus.VideoState = STATE_VIDEO__CHECKED;
			CurrentStatus.VideoStabilityCheckCount = 0;
		}					  										
	}
#endif //#if (PEBBLES_VIDEO_STATUS_2ND_CHECK==ENABLE)
}

//===================================================================
// CheckAndSet Area
//===================================================================



void HdmiCheckDeviceId(void)
{
	BYTE Hi,Lo;
	Hi = ReadI2CByte(I2CID_SIL9127_DEV0,0x03);
	Lo = ReadI2CByte(I2CID_SIL9127_DEV0,0x02);
	Printf("\nSIL9XXXA ID:%02bx%02bx",Hi,Lo);
	if(Hi==0x91) {
		if(Lo==0x27)	Puts(" OK");
		else            Puts(" FAIL");	
	}
	else if(Hi==0x92) {
		if(Lo==0x23 || Lo==0x33)	Puts(" OK");
		else            			Puts(" FAIL");	
	}
	else {
		Puts(" FAIL");
	}
}

void HdmiInitReceiverChip(void)
{
	WriteI2CByte(I2CID_SIL9127_DEV0,0x08, ReadI2CByte(I2CID_SIL9127_DEV0,0x08) | 0x01);	//Off PowerDown

	WriteI2CByte(I2CID_SIL9127_DEV0,0x49, ReadI2CByte(I2CID_SIL9127_DEV0,0x49) | 0x04);	//Convert YCbCr to RGB
	//WriteI2CByte(I2CID_SIL9127_DEV0,0x48, ReadI2CByte(I2CID_SIL9127_DEV0,0x48) | 0x04);	//BT.709
	//WriteI2CByte(I2CID_SIL9127_DEV0,0x54, ReadI2CByte(I2CID_SIL9127_DEV0,0x54) | 0x01);	//Field 2 Back Porch Mode Register
}

//qHD GPIO
//------------------------------------------------------------------------------
// Function: GPIO_GetPins
// Description: Get current state of the GPIO pins specified in the mask parameter
//------------------------------------------------------------------------------
#if 0
uint8_t GPIO_GetPins(uint8_t pinMask)
{
    uint8_t pinState = 0;
	BYTE page,value;

//    if ((pinMask & GPIO_PIN__PORT_SELECT_SWITCH_0) && 0  /*BK111125 && (PinPortSelectSwitch0)*/)  pinState |= GPIO_PIN__PORT_SELECT_SWITCH_0;
//    if ((pinMask & GPIO_PIN__PORT_SELECT_SWITCH_1) && 1  /*BK111125 && (PinPortSelectSwitch1)*/)  pinState |= GPIO_PIN__PORT_SELECT_SWITCH_1;
//    if ((pinMask & GPIO_PIN__PORT_SELECT_SWITCH_2) && 1  /*BK111125 && (PinPortSelectSwitch2)*/)  pinState |= GPIO_PIN__PORT_SELECT_SWITCH_2;

    if ((pinMask & GPIO_PIN__AUDIO_DAC_MUTE)       /*&& (PinAudioDacMute)*/)       pinState |= GPIO_PIN__AUDIO_DAC_MUTE;
    if ((pinMask & GPIO_PIN__AUDIO_DAC_RESET)      /*&& (PinAudioDacReset)*/)      pinState |= GPIO_PIN__AUDIO_DAC_RESET;
#if 0 //BK111125
    if ((pinMask & GPIO_PIN__HARDWARE_RESET)       && (PinHardwareReset))      pinState |= GPIO_PIN__HARDWARE_RESET;
    if ((pinMask & GPIO_PIN__RX_INTERRUPT)         && (PinRxInterrupt))        pinState |= GPIO_PIN__RX_INTERRUPT;
#else
	if (pinMask & GPIO_PIN__HARDWARE_RESET) {
		Printf("\nGPIO_GetPins:HARDWARE_RESET");
		page = ReadTW88(0xff);
		WriteTW88Page(0);
		value = ReadTW88(0x9E);	 //GPIO_60
		WriteTW88Page(page);
		if(value & 0x01)
			pinState |= GPIO_PIN__HARDWARE_RESET;
	}
    if (pinMask & GPIO_PIN__RX_INTERRUPT) {
		Printf("\nGPIO_GetPins:RX_INTERRUPT");
		page = ReadTW88(0xff);
		WriteTW88Page(0);
		value = ReadTW88(0x98);
		WriteTW88Page(page);
		if(value & 0x02)
			pinState |= GPIO_PIN__RX_INTERRUPT;
	}
#endif
    return (pinState);
}
#endif

//------------------------------------------------------------------------------
// Function: GPIO_SetPins
// Description: Set to 1 the current state of the GPIO pins specified in the mask parameter
//------------------------------------------------------------------------------
void GPIO_SetPins(uint8_t pinMask)
{
	DECLARE_LOCAL_page

	if (pinMask & GPIO_PIN__HARDWARE_RESET) {
		Printf("\nGPIO_SetPins:HARDWARE_RESET");
		ReadTW88Page(page);
		WriteTW88Page(0);
		WriteTW88(REG096, ReadTW88(REG096) | 0x01);	//GPIO60
		WriteTW88Page(page);
	}
    if (pinMask & GPIO_PIN__RX_INTERRUPT) {
		Printf("\nGPIO_SetPins:RX_INTERRUPT");
		ReadTW88Page(page);
		WriteTW88Page(0);
		WriteTW88(REG088, ReadTW88(REG088) | 0x02);	  //NOTE
		delay1ms(1);
		WriteTW88(REG090, ReadTW88(REG090) | 0x02);
		WriteTW88Page(page);
	}
    if (pinMask & GPIO_PIN__VCCEN) {
		Printf("\nGPIO_SetPins:VCCEN"); //PinVccEn   = 1;
		ReadTW88Page(page);
		WriteTW88Page(0);
		WriteTW88(REG096, ReadTW88(REG096) | 0x06);	 //TURN ON EN_3V3 & EN_1V2
		WriteTW88Page(page);
	}
}

#if(CONF__CEC_ENABLE == ENABLE)
void GPIO_ClearCecD(void)
{
#ifdef SUPPORT_HDMI_SiIRX
	Puts("\nGPIO_ClearCecD");
//   PinCecD = 0 ;
#else
   PinCecD = 0 ;
#endif
}

#endif //#if(CONF__CEC_ENABLE == ENABLE)

//------------------------------------------------------------------------------
// Function: GPIO_GetComMode
// Description: get com mode status, for simon use
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function: GPIO_ClearPins
// Description: Clear to 0 the current state of the GPIO pins specified in the mask parameter
//------------------------------------------------------------------------------
void GPIO_ClearPins(uint8_t pinMask)
{
	DECLARE_LOCAL_page

	if (pinMask & GPIO_PIN__HARDWARE_RESET) {
		Printf("\nGPIO_ClearPins:HARDWARE_RESET");
		ReadTW88Page(page);
		WriteTW88Page(0);
		WriteTW88(REG096, ReadTW88(REG096) & ~0x01);	//GPIO60
		WriteTW88Page(page);
	}
    if (pinMask & GPIO_PIN__RX_INTERRUPT) {
		Printf("\nGPIO_ClearPins:RX_INTERRUPT");
		ReadTW88Page(page);
		WriteTW88Page(0);
		WriteTW88(REG088, ReadTW88(REG088) | 0x02);	  //NOTE
		delay1ms(1);
		WriteTW88(REG090, ReadTW88(REG090) & ~0x02);
		WriteTW88Page(page);
	}
    if (pinMask & GPIO_PIN__VCCEN) {
		Printf("\nGPIO_ClearPins:VCCEN"); //PinVccEn   = 0;
		ReadTW88Page(page);
		WriteTW88Page(0);
		WriteTW88(REG096, ReadTW88(REG096) & ~0x06);	 //TURN OFF EN_3V3 & EN_1V2
		WriteTW88Page(page);
	}
 }






//----------------------------------------------------------------------------------------------------------------------
#endif //..#HDMI_SiIRX SUPPORT_HDMI_SIL9XXXA
