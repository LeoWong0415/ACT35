/*
 *  main.c - main for  Master
 *
 *  Copyright (C) 2011~2012 Intersil Corporation
 *
 */
/*****************************************************************************/
/* CPU        : DP8051                                                       */
/* LANGUAGE   : Keil C                                                       */
/* PROGRAMMER : Harry Han                                                    */
/*              YoungHwan Bae                                                */
/*              Brian Kang                                                   */
/*****************************************************************************/
/* See 'Release.txt' for firmware revision history                           */

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

#include "main.h"
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

#include "SOsd.h"
#include "FOsd.h"
#include "SpiFlashMap.h"
#include "SOsdMenu.h"
#include "Demo.h"

#if !defined(MODEL_TW8835_MASTER)
BUG. It is a MODEL_TW8835_MASTER code
#endif

BYTE slave_model;			//0, 35:TW8835,36:TW8836
BYTE i2ccmd_reg_start;

//=============================================================================
// INIT HARDWARE for Master				                                               
//=============================================================================
void SSPLL_Master_PowerUp(BYTE fOn)
{
	WriteTW88Page(PAGE0_SSPLL);
	if(fOn)	WriteTW88(REG0FC, ReadTW88(REG0FC) & ~0x80);
	else	WriteTW88(REG0FC, ReadTW88(REG0FC) |  0x80);
}

void FP_Master_GpioDefault(void)
{
	WriteTW88Page(PAGE0_GPIO);
	WriteTW88(REG084, 0x00);	//FP_BiasOnOff(OFF);
	WriteTW88(REG08C, 0x00);	//FP_PWC_OnOff(OFF);
	WriteTW88(REG094, 0x00);	//output
}

void InitMasterWithNTSC(void)
{
	//---------------------------
	//clock
	//---------------------------
	WriteTW88Page(PAGE4_CLOCK);
	if(SpiFlashVendor==SFLASH_VENDOR_MX)	// MCUSPI Clock : Select 27MHz with
		WriteTW88(REG4E1, 0x02);			//if Macronix SPI Flash, SPI_CK_DIV[2:0]=2
	else
		WriteTW88(REG4E1, 0x01);			//SPI_CK_SEL[5:4]=2. PCLK, SPI_CK_DIV[2:0]=1
	WriteTW88(REG4E0, 0x01);				// PCLK_SEL[0]=1

	WriteTW88Page(PAGE0_GENERAL);
											//SSPLL FREQ R0F8[3:0]R0F9[7:0]R0FA[7:0]
	WriteTW88(REG0F8, 0x02);				//108MHz
	WriteTW88(REG0F9, 0x00);	
	WriteTW88(REG0FA, 0x00);	
	WriteTW88(REG0F6, 0x00); 				//PCLK_div_by_1 SPICLK_div_by_1
	WriteTW88(REG0FD, 0x34);				//SSPLL Analog Control
											//		POST[7:6]=0 VCO[5:4]=3 IPMP[2:0]=4
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG20D, 0x02);				//pclko div 3. 	108/3=36MHz

#if 0	//if you want CLKPLL 72MHz or 54MHz
	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG0FC, ReadTW88(REG0FC) & ~0x80);	//Turn off SSPLL PowerDown first.
	WriteTW88(REG0F6, ReadTW88(REG0F6) | 0x20); 	//SPICLK_div_by_3

	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E1, ReadTW88(REG4E1) | 0x20);		//SPI_CK_SEL[5:4]=2. CLKPLL
#endif

	//---------------------------
	WriteTW88Page(PAGE1_DECODER);
	WriteTW88(REG1C0, 0x01);				//LLPLL input def 0x00
	WriteTW88(REG105, 0x2f);				//Reserved def 0x0E


	//---------------------------
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG21E, 0x03);				//BLANK def 0x00.	--enable screen blanking with AUTO

	//---------------------------
	WriteTW88Page(PAGE0_LEDC);
	WriteTW88(REG0E0, 0xF2);				//LEDC. default. 
											//	R0E0[1]=1	Analog block power down
											//	R0E0[0]=0	LEDC digital block disable
											//DCDC.	default. HW:0xF2
											//	R0E8[1]=1	Sense block power down
											//	R0E8[0]=0	DC converter digital block disable.
	WriteTW88(REG0E8, 0x70);
	WriteTW88(REG0EA, 0x3F);

	//=====================
	//All Register setting
	//=====================

	//-------------------
	// PAGE 0
	//-------------------
	WriteTW88Page(PAGE0_GENERAL);

	WriteTW88(REG007, 0x00);	// TCON
	WriteTW88(REG008, 0xA9);	//<<<======	

	WriteTW88(REG040, 0x10);		

	WriteTW88(REG041, 0x0C);

	WriteTW88(REG042, 0x02);	
	WriteTW88(REG043, 0x10);	
	WriteTW88(REG044, 0xF0);	
	WriteTW88(REG045, 0x82);	
	WriteTW88(REG046, 0xD0);	

	WriteTW88(REG047, 0x80);	
	WriteTW88(REG049, 0x41);	//def 0x00

	WriteTW88(REG0ED, 0x40);	//def:0x80
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88(REG0F1, 0xC0);	//[7]FP_PWC [6]FP_BIAS		
#endif

	//-------------------
	// PAGE 1
	//-------------------
	WriteTW88Page(PAGE1_DECODER);
	WriteTW88(REG11C, 0x0F);	//*** Disable Shadow
	WriteTW88(REG102, 0x40);
	WriteTW88(REG106, 0x03);	//ACNTL def 0x00
	WriteTW88(REG107, 0x02);	//Cropping Register High(CROP_HI) def 0x12
	WriteTW88(REG109, 0xF0);	//Vertical Active Register,Low(VACTIVE_LO) def 0x20
	WriteTW88(REG10A, 0x0B);	//Horizontal Delay,Low(HDELAY_LO) def 0x10
	WriteTW88(REG111, 0x5C);	//CONTRAST def 0x64
	WriteTW88(REG117, 0x80);	//Vertical Peaking Control I	def 0x30
	WriteTW88(REG11E, 0x00);	//Component video format: def 0x08
	WriteTW88(REG121, 0x22);	//Individual AGC Gain def 0x48
	WriteTW88(REG127, 0x38);	//Clamp Position(PCLAMP)	def 0x2A
	WriteTW88(REG128, 0x00);
	WriteTW88(REG12B, 0x44);	//Comb Filter	def -100-100b
	WriteTW88(REG130, 0x00);
	WriteTW88(REG134, 0x1A);
	WriteTW88(REG135, 0x00);
								//ADCLLPLL
	WriteTW88(REG1C6, 0x20); //WriteTW88(REG1C6, 0x27);	//WriteTW88(REG1C6, 0x20);  Use VAdcSetFilterBandwidth()
	WriteTW88(REG1CB, 0x3A);	//WriteTW88(REG1CB, 0x30);

	//-------------------
	// PAGE 2
	//-------------------
	WriteTW88Page(PAGE2_SCALER);
	//WriteTW88(REG201, 0x00);
	//WriteTW88(REG202, 0x20);
	WriteTW88(REG203, 0xCC);		//XSCALE: 0x1C00. def:0x2000
	WriteTW88(REG204, 0x1C);		//XSCALE:
	WriteTW88(REG205, 0x8A);		//YSCALE: 0x0F8A. def:0x2000
	WriteTW88(REG206, 0x0F);		//YSCALE:
	WriteTW88(REG207, 0x40);		//PXSCALE[11:4]	def 0x800
	WriteTW88(REG208, 0x20);		//PXINC	def 0x10
	WriteTW88(REG209, 0x00);		//HDSCALE:0x0400 def:0x0100
	WriteTW88(REG20A, 0x04);		//HDSCALE
	WriteTW88(REG20B, 0x08);		//HDELAY2 def:0x30
	//WriteTW88(REG20C, 0xD0);	//HACTIVE2_LO
	WriteTW88(REG20D, 0x90 | (ReadTW88(REG20D) & 0x03));  //LNTT_HI def 0x80. 
	WriteTW88(REG20E, 0x20);		//HPADJ def 0x0000.  20E[6:4] HACTIVE2_HI
	WriteTW88(REG20F, 0x00);
	WriteTW88(REG210, 0x21);		//HA_POS	def 0x10
	
	//HA_LEN		Output DE length control in number of the output clocks.
	//	R212[3:0]R211[7:0] = PANEL_H+1
	WriteTW88(REG211, 0x21);		//HA_LEN	def 0x0300
	WriteTW88(REG212, 0x03);		//PXSCALE[3:0]	def:0x800 HALEN_H[3:0] def:0x03
	WriteTW88(REG213, 0x00);		//HS_POS	def 0x10
	WriteTW88(REG214, 0x20);		//HS_LEN
	WriteTW88(REG215, 0x2E);		//VA_POS	def 0x20
	//VA_LEN	Output DE control in number of the output lines.
	//	R217[7:0]R216[7:0] = PANEL_V
	WriteTW88(REG216, 0xE0);		//VA_LEN_LO	def 0x0300
	WriteTW88(REG217, 0x01);		//VA_LEN_HI
	
	WriteTW88(REG21C, 0x42);	//PANEL_FRUN. def 0x40
	WriteTW88(REG21E, 0x03);		//BLANK def 0x00.	--enable screen blanking. SW have to remove 21E[0]
	WriteTW88(REG280, 0x20);		//Image Adjustment register
	WriteTW88(REG28B, 0x44);
	WriteTW88(REG2E4, 0x21);		//--dither

	//-------------
	//Enable TCON
	//TW8835					TW8832
	WriteTW88Page(PAGE2_TCON);
	WriteTW88(REG240, 0x10);		//WriteTW88(REG240, 0x11);
	WriteTW88(REG241, 0x00);		//WriteTW88(REG241, 0x0A);
	WriteTW88(REG242, 0x05);		//WriteTW88(REG242, 0x05);
	WriteTW88(REG243, 0x01);		//WriteTW88(REG243, 0x01);
	WriteTW88(REG244, 0x64);		//WriteTW88(REG244, 0x64);
	WriteTW88(REG245, 0xF4);		//WriteTW88(REG245, 0xF4);
	WriteTW88(REG246, 0x00);		//WriteTW88(REG246, 0x00);
	WriteTW88(REG247, 0x0A);		//WriteTW88(REG247, 0x0A);
	WriteTW88(REG248, 0x36);		//WriteTW88(REG248, 0x36);
	WriteTW88(REG249, 0x10);		//WriteTW88(REG249, 0x10);
	WriteTW88(REG24A, 0x00);		//WriteTW88(REG24A, 0x00);
	WriteTW88(REG24B, 0x00);		//WriteTW88(REG24B, 0x00);
	WriteTW88(REG24C, 0x00);		//WriteTW88(REG24C, 0x00);
	WriteTW88(REG24D, 0x44);		//WriteTW88(REG24D, 0x44);
	WriteTW88(REG24E, 0x04);		//WriteTW88(REG24E, 0x04);

	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG006, 0x06);	//display direction. TSCP,TRSP
	WriteTW88(REG007, 0x00);	// TCON
}	
 

//=============================================================================
//				                                               
//=============================================================================


//=============================================================================
// Interrupt Handling Routine			                                               
//=============================================================================


//=============================================================================
// MAIN LOOP				                                               
//=============================================================================
//external
//	INT_STATUS
//	EX0
//	P1_3
//extern BYTE	TouchStatus;
//return
//	0:
//	1:PowerSaveMode by Remo
//	2:PowerSaveMode by Port
//	3:RCDMode by Port
//
BYTE main_loop(void)
{
	BYTE ret;

	//---------------------------------------------------------------
	//			             Main Loop 
	//---------------------------------------------------------------
	while(1) {
		//-------------- Check TW8835 Interrupt ------------------------
		//-------------- ext_i2c_timer ------------------------
		//-------------- Check Watchdog ------------------------
		
		//-------------- Check Serial Port ---------------------
		Monitor();				// for new monitor functions

		//-------------- SW I2C SLAVE ---------------------

		//-------------- block access routines -----------------
		if ( access == 0 ) continue;		

		//-------------- Check Key in --------------------------
		//-------------- Check backdrive_mode ---------------
 		//-------------- Check Remote Controller ---------------
		//-------------- Check Touch ---------------			
		//-------------- Check OSD timer -----------------------
		//-------------- Check OFF timer -----------------------
		//============== HDMI Section ==========================
		//============== Task Section ==========================
		
	} //..while(1)

	return ret;
}



//-----------------------------------------------
//
//-----------------------------------------------
void InitVariables(void)
{
	DebugLevel=0;
	access = 1;

	//--task variables
//	Task_Grid_on = 0;
//	Task_Grid_cmd = 0;
//	Task_NoSignal_cmd = TASK_CMD_DONE;
//	SW_INTR_cmd = 0;

	SpiFlashVendor = 0;	//see spi.h
}

//=============================================================================
//			                                               
//=============================================================================
void InitMasterCore(BYTE fPowerUpBoot)
{
	if(fPowerUpBoot) {
	}

	Puts("\nInitMasterCore");	
	//----- Set SPI mode
	//SpiFlashVendor = SPIHost_QUADInit();
	//SPIHost_SetReadModeByRegister(SPI_READ_MODE);		// Match DMA READ mode with SPI-read
	SpiFlashVendor = SPI_QUADInit();
	SPI_SetReadModeByRegister(SPI_READ_MODE);		// Match DMA READ mode with SPI-read

	//----- Enable Chip Interrupt

	WriteTW88Page(PAGE0_GENERAL );
	WriteTW88(REG002, 0xFF );	// Clear Pending Interrupts
	WriteTW88(REG003, 0xFE );	// enable SW. disable all other interrupts

	//enable remocon interrupt.
	EnableRemoInt();
}


//=============================================================================
//			                                               
//=============================================================================
void InitMasterSystem(BYTE fPowerUpBoot)
{
	BYTE temp = fPowerUpBoot;
	DebugLevel=3;
	//Init HW with default
	InitMasterWithNTSC();
	FP_Master_GpioDefault();
	SSPLL_Master_PowerUp(ON);
	PrintSystemClockMsg("SSPLL_Master_PowerUp");

	WriteTW88Page(0);
	WriteTW88(REG008, 0x89);	//Output enable

	Printf("\nNow, enable the slave system....");
}


//-----------------------------------------------------------------------------
// MAIN
//-----------------------------------------------------------------------------
#define PSM_REQ_FROM_NORMAL		1
#define PSM_REQ_FROM_RCDMODE	2
void main(void)
{
	BYTE request_power_save_mode;
	BYTE ret;

	InitVariables();
	InitCPU();
	InitMasterCore(1);

	//-------------------------------------
	// PRINT MODEL VERSION
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
	Puts("8BIT-BUG ");
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
#ifdef MODEL_TW8835_MASTER
	Puts(" MASTER");
#endif

	Printf("\n********************************************************");
	if(access==0) {
		Puts("\n***SKIP_MODE_ON***");
		DebugLevel=3;
		//skip...do nothing
		Puts("\nneed **init core***ee find***init***");
	}

	SetMonAddress(TW88I2CAddress);
	Prompt(); //first prompt

	//==================================================
	// Init System
	//==================================================
	InitMasterSystem(1);

	//Check Slave
	ret=CheckI2C(TW88I2CAddress); 
	if(ret) {
		Printf("\nTW88xx slave fail. Please turn on the slave");
		while(ret) {
			ret=CheckI2C(TW88I2CAddress);
			delay1ms(100);
		}
	}

	WriteI2CByteToTW88(0xFF,0);
	ret = ReadI2CByteFromTW88(REG000);
	if((ret & 0xFC) == 0x74) {
		slave_model = 0x35;
		i2ccmd_reg_start = (BYTE)REG4DB;
		Printf("\nOK Slave is a TW8835");
	}
	else if((ret & 0xFC) == 0x78) {
		slave_model = 0x36;
		i2ccmd_reg_start = (BYTE)REG4FA;
		Printf("\nOK Slave is a TW8836");
	}
	else {
		slave_model = 0;
		Printf("\nSlave fail..Please reboot Slave & Master");
		return;
	}

	PrintSystemClockMsg("start loop");

	WriteTW88Page(PAGE0_GENERAL);
	Prompt(); //second prompt

	request_power_save_mode = 0;
	while(1) 
	{
		//==================================================
		// MAIN LOOP
		//==================================================
		ret=main_loop();		
		//exit when power off. or RCDMode. 
		//FYI:SkipMode can not exit the main_loop.
		ePrintf("\nmain_loop() ret %bd",ret);

	}	
	//you can not be here...
}
//==============MAIN.C=======END
