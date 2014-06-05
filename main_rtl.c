/*
 *  main_RTL.c - only for RTL verification 
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

#include "FOsd.h"
//#include "osd.h"
//#include "SPI_Map_B.h"
//#include "menu8835B.h"

//#include "Demo.h"
//void FOsdDownloadFontByDMA(WORD dest_loc, DWORD src_loc, WORD size);
extern void DumpFont(void);

//----------------------------------
// "system no initialize mode" global variable
// If P1_5 is connected at the PowerUpBoot, it is a system no init mode (SYS_MODE_NOINIT).
// If the system is a SYS_MODE_NOINIT, 
//  FW will skips the initialize routine. 
//  and supports only the Monitor function.
//  and SYS_MODE_NOINIT can not support a RCDMode and a PowerSaveMode.
// But, if the system bootup with normal, the P1_5 will be worked as a PowerSave ON/OFF switch.
//----------------------------------
#define SYS_MODE_NORMAL		0
#define SYS_MODE_NOINIT		1
BYTE SysNoInitMode;


#if 0
/*BKFYI. 
If you want to execute the code at the end of code area, add this dummy code.
It will add a big blank code at the front of code area.
*/
code BYTE dummy_code[1023*5] = {
};
#endif


DWORD request_sleep;





//=============================================================================
// Video TASK ROUTINES				                                               
//=============================================================================

//-----------------------
// Task NoSignal
//-----------------------
#define TASK_FOSD_WIN	0
#define NOSIGNAL_TIME_INTERVAL	(10*100)

void NoSignalTask( void );			//prototype
void NoSignalTaskOnWaitMode(void);
XDATA BYTE Task_NoSignal_cmd;		//DONE,WAIT_VIDEO,WAIT,RUN,RUN_FORCE
XDATA BYTE Task_NoSignal_count;		//for dPuts("\nTask NoSignal TASK_CMD_WAIT_VIDEO");

//desc
//	set NoSignalTask status
//desc
//	read NoSignalTask status.	

//-----------------------
// Task MovingGrid
//-----------------------

//-----------------------
// Task BackDriveGrid
//-----------------------
//void BackDriveGrid(BYTE on);
 

//=============================================================================
//				                                               
//=============================================================================


//=============================================================================
// Interrupt Handling Routine			                                               
//=============================================================================
WORD main_VH_Loss_Changed;
BYTE main_INT_STATUS;
BYTE main_INT_STATUS2;
BYTE SW_Video_Status;
BYTE SW_INTR_cmd;
#define SW_INTR_VIDEO_CHANGED	1

//desc: Turn off the SYNC Change(R003[2]) mask,
//               the Video Loss(R003[1]) mask,
//               the WirteReg0x00F(R003[0] mask. 
//		Turn On  the Video Loss(R003[1]) mask,
//               the WirteReg0x00F(R003[0] mask. 
//I do not turn on the SYNC Change.
//if you want to turn on SYNC, You have to call Interrupt_enableSyncDetect(ON).
//

//desc: Turn off/on SYNC Interrupt mask.

//interrupt polling
//desc
//	Read interrupt global value that changed on interrupt service routine
//  and print

//
//Interrupt Handler Routine 
//



//
//CheckAndSetInput pointer function
//
//BYTE (*CheckAndSetInput)(void);


//desc
//	link CheckAndSetInput Routine.


//
// Video Signal Task Routine
//

//============== Check each input status ===============

//desc
//	check RCD port

//description
//	Update OSD Timer



//-------------------------------------
//
//-------------------------------------

//desc
//	start video with saved input

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
#define RET_MAIN_LOOP_PSM_BY_REMO	1
#define RET_MAIN_LOOP_PSM_BY_PORT	2
#define RET_MAIN_LOOP_RCDMODE		3
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
		
		//-------------- Check Serial Port ---------------------
		Monitor();				// for new monitor functions


		//-------------- block access routines -----------------
		if ( access == 0 ) continue;		
		
	} //..while(1)

	return ret;
}

//=============================================================================
// RearCameraDisplayMode LOOP				                                               
//=============================================================================

//================================
// PowerSaveMode(PSMode)
//================================

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
	SW_INTR_cmd = 0;

	SpiFlashVendor = 0;	//see spi.h
#ifdef DEBUG_REMO_NEC
	DebugRemoStep = 0;  //only for test BK110328
#endif
//	FirstInitDone = 0;
	request_sleep=0;
}

//=============================================================================
//			                                               
//=============================================================================
void InitCore(BYTE fPowerUpBoot)
{
	if(fPowerUpBoot) {
		//check port 1.5. if high, it is a skip(NoInit) mode.
		SysNoInitMode = SYS_MODE_NORMAL;
#ifdef MODEL_TW8836FPGA
		if(1)	{
			SysNoInitMode = 1;
#else
		if(PORT_NOINIT_MODE == 1)	{
			SysNoInitMode = SYS_MODE_NOINIT;
#endif
			//turn on the SKIP_MODE.
			access = 0;
					
			//McuSpiClkSelect(MCUSPI_CLK_27M);
			WriteTW88Page(PAGE4_CLOCK);
			WriteTW88(REG4E1, ReadTW88(REG4E1) & 0x0F);
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

	//enable remocon interrupt.
}



//=============================================================================
//			                                               
//=============================================================================
//VDD(1.8V) -> VCC(3.3V) -> AVCC(5V)
//GPIO64       GPIO63       GPIO65
//Note:
//	It need a GPIO61 ON before FW calls FLCOS_PowerOn()


void InitWithNTSC_RTL(void)
{
	//---------------------------
	//clock
	//---------------------------
	WriteTW88Page(PAGE4_CLOCK);
//	if(SpiFlashVendor==SFLASH_VENDOR_MX)
//		WriteTW88(REG4E1, 0x02);	//if Macronix SPI Flash, SPI_CK_DIV[2:0]=2
//	else
		WriteTW88(REG4E1, 0x01);	//SPI_CK_SEL[5:4]=2. PCLK, SPI_CK_DIV[2:0]=1
	WriteTW88(REG4E0, 0x01);	// PCLK_SEL[0]=1

	WriteTW88Page(PAGE0_GENERAL);
							//SSPLL FREQ R0F8[3:0]R0F9[7:0]R0FA[7:0]
	WriteTW88(REG0F8, 0x02);	//108MHz
	WriteTW88(REG0F9, 0x00);	
	WriteTW88(REG0FA, 0x00);	
	WriteTW88(REG0F6, 0x00); 	//PCLK_div_by_1 SPICLK_div_by_1
	WriteTW88(REG0FD, 0x34);	//SSPLL Analog Control
							//		POST[7:6]=0 VCO[5:4]=3 IPMP[2:0]=4
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG20D, 0x01);  //pclko div 2. 	 

	//---------------------------
	WriteTW88Page(PAGE1_DECODER);
	WriteTW88(REG1C0, 0x01);		//LLPLL input def 0x00
	WriteTW88(REG105, 0x2f);		//Reserved def 0x0E


	//---------------------------
	WriteTW88Page(PAGE2_SCALER);
	WriteTW88(REG21E, 0x03);		//BLANK def 0x00.	--enable screen blanking with AUTO

	//---------------------------
	WriteTW88Page(PAGE0_LEDC);
	WriteTW88(REG0E0, 0xF2);		//LEDC. default. 
								//	R0E0[1]=1	Analog block power down
								//	R0E0[0]=0	LEDC digital block disable
								//DCDC.	default. HW:0xF2
								//	R0E8[1]=1	Sense block power down
								//	R0E8[0]=0	DC converter digital block disable.
	WriteTW88(REG0E8, 0x70);
	WriteTW88(REG0EA, 0x3F);		//Van. 110909

												//SSPLL_PowerDown
												//TWTerCmd:	"b 8a fc 77 ff"
	//WriteTW88Page(PAGE0_SSPLL);				//DocDefault:0x30. HWDefault:0xB0
	//WriteTW88(REG0FC, ReadTW88(REG0FC) | 0x80);	//	R0FC[7]=1	SSPLL power down

//#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	//set external decoder
	WriteI2CByte(I2CID_TW9910, 0x03, 0x82);
	WriteI2CByte(I2CID_TW9910, 0x1A, 0x02);
	WriteI2CByte(I2CID_TW9910, 0x1F, 0x01);
//#endif



	//=====================
	//All Register setting
	//=====================

	//-------------------
	// PAGE 0
	//-------------------
	WriteTW88Page(PAGE0_GENERAL);

//R000 74	Product ID Code Register

//R002 0A	IRQ
//R003 FE	IMASK
//R004 01	Status

//R006 00	SRST

//R007 00	Output Ctrl I
//	[7]	SWPPRT
//	[6]	SWPNIT
//	[5:4]	SHFT2
//	[3]		EN656OUT
//	[2:0]	TCONSEL						
	WriteTW88(REG007, 0x02);	// FP LSB
//R008 30	Output Ctrl II	
//	[7:6]	TCKDRV	=10	8mA	
//	[5]		TRI_FPD	=1	1:Tristate all FP data pins.
//	[4]		TRI_EN	=0	1:Tristate all output pins
//	[3:0]	GPOSEL	=9	sDE(Serial RGB DE)
//	SW use 0xA9. TRI_FPD will turn on at FP module.
//#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88(REG008, 0x89);		
//#else
//#endif

//R00F 00	INT0 WRITE PORT
//R01F 00	TEST
													
//R040 00	INPUT Control I
//	[7:6]	IPHDLY	Input H cropping position control in implicit DE mode. This is a 10-bit register.
//	[4]		CKINP	Scaler input clock polarity control
//					1 = Inversion 0 = no inversion
//	[3]		DTVDE_EN
//					1 = INT_4 pin is turn into dtvde pin 
//					0 = INT_4 pin is normal operation
//	[2]		DTVCK2_EN
//					1 = dtv_vs pin is turn into 2nd DTV clock pin 
//					0 = dtv_vs pin is normal operation
//	[1:0]	IPSEL	Input selection
//					0 = Internal decoder 1 = Analog RGB/YUV 2 = DTV
	WriteTW88(REG040, 0x10);		

//R041 00	INPUT CONTROL II
//	[5]		PROG	Field control for progressive input
//	[4]		IMPDE	1 = implicit DE mode. It is only available in DTV input mode
//	[3]		IPVDET	Input V sync detection edge control
//				1 = falling edge 0 = rising edge
//	[2]		IPHDET	Input H sync detection edge control
//				1 = falling edge 0 = rising edge
//	[1]		IPFD	Input field control
//				1 = inversion 0 = no inversion
//	[0]		RGBIN	Input data format selection
//				1 = RGB 0 = YCbCr
	WriteTW88(REG041, 0x0C);

//R042 02	INPUT CROP_HI
//	[6:4]	IPVACT_HI	
//				Input V cropping length control in number of input lines for 
//				use in implicit DE mode. This is an 11-bit register.
//	[3:0]	IPHACT_HI	
//				Input H cropping length control in number of input pixels for 
//				use in implicit DE mode. This is a 12-bit register.
//R043 20	INPUT V CROP Position
//	[7:0]	IPVDLY
//				Input V cropping starting position in number of lines relative to the V sync. 
//				This is used in implicit DE mode
//R044 F0	INPUT V CROP LENGTH LO
//	[7:0]	IPVACT_LO
//				Input V cropping length control in number of input lines for 
//				use in implicit DE mode. This is an 11-bit register.
//R045 20	IINPUT H CROP POSITION LO
//	[7:0]	IPHDLY_LO
//				Input H cropping position control relative to leading edge of 
//				H sync in implicit DE mode. This is a 10-bit register.
//R046 D0	INPUT H CROP LENGTH LO
//	[7:0]	IPHACT_LO
//				Input H cropping length control in number of input pixels for 
//				use in implicit DE mode. This is a 12-bit register.
//---------------------------
//	input h start 0x082	length 0x2D0 
//  input v start 0x10	length 0x0F0	
	WriteTW88(REG042, 0x02);	
	WriteTW88(REG043, 0x10);	
	WriteTW88(REG044, 0xF0);	
	WriteTW88(REG045, 0x82);	
	WriteTW88(REG046, 0xD0);	

//R047 00	BT656 DECODER CONTROL I
//	[7]		FRUN	BT656 input control
//				0 = External input 1 = Internal pattern generator
//	[6]		HZ50	Internal pattern generator field frequency control.
//				0 = 60 Hz 1 = 50 Hz
//	[5]		DTVCKP	BT656 input clock control
//				0 = no inversion 1 = Inversion
//	[4:0]	EVDELAY	BT656 input V delay control in number of lines.
//R048 00	BT646 DECODER CONTROL II
//	[7]		NONSTA	Non-standard BT656 signal decoding.
//	[5:0]	EHDELAY	BT656 input H delay control in number of pixels.
//R049 00	BT656 Status I
//	[7:0]	LNPF656[9:2]	BT656 Input line count per frame status.
//R04A 01	BT656 STATUS II
//	[2:1]	LNPF656[1:0]
//	[0]		VDET656		BT656 input video loss detection.
//				0 = video detected 1 = no video input
	WriteTW88(REG047, 0x80);	
	//WriteTW88(REG048, 0x00);
	WriteTW88(REG049, 0x41);	//def 0x00

//R050~R05F	DTV
	//WriteTW88(REG05F, 0x00);
	
//R080~R09E	GPIO

//R0A0~ MBIST

//R0B0~ TSC CONTROL

//R0D4

//R0E0 LEDC CONTROL
//	WriteTW88(REG0F7, 0x16); def
//	WriteTW88(REG0F8, 0x01); def
//									
//;DC/DC setting
//	WriteTW88(REG0e8, 0xf1);  DC sense powerup. DC digital enable. DCDC-on. DONOT change it here. 
//	WriteTW88(REG0e9, 0x06);  def
//	WriteTW88(REG0ea, 0x0a);	??
//	WriteTW88(REG0eb, 0x40);	def
//	WriteTW88(REG0ec, 0x20);	def
	WriteTW88(REG0ED, 0x40);	//def:0x80
//#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	WriteTW88(REG0F1, 0xC0);	//[7]FP_PWC [6]FP_BIAS		
//#endif
//
//							SSPLL FREQ R0F8[3:0]R0F9[7:0]R0FA[7:0]
//	WriteTW88(REG0F9, 0x50);	//70MHz NOT HERE
//	WriteTW88(REG0FA, 0x00);	//def
//	WriteTW88(REG0FB, 0x40);	//def
//;FPCLK_PD
//	WriteTW88(REG0FC, 0x23);	En.SSPLL
//	WriteTW88(REG0FD, 0x00);

	//-------------------
	// PAGE 1
	//-------------------
	WriteTW88Page(PAGE1_DECODER);
	WriteTW88(REG11C, 0x0F);	//*** Disable Shadow
	WriteTW88(REG102, 0x40);
	//WriteTW88(REG104, 0x00);
	//moveup		WriteTW88(REG105, 0x2f);	//Reserved def 0x0E
	WriteTW88(REG106, 0x03);	//ACNTL def 0x00
	WriteTW88(REG107, 0x02);	//Cropping Register High(CROP_HI) def 0x12
	//WriteTW88(REG108, 0x12);
	WriteTW88(REG109, 0xF0);	//Vertical Active Register,Low(VACTIVE_LO) def 0x20
	WriteTW88(REG10A, 0x0B);	//Horizontal Delay,Low(HDELAY_LO) def 0x10
	//WriteTW88(REG10B, 0xD0);
	//WriteTW88(REG10C, 0xCC);
	//WriteTW88(REG110, 0x00);
	WriteTW88(REG111, 0x5C);	//CONTRAST def 0x64
	//WriteTW88(REG112, 0x11);
	//WriteTW88(REG113, 0x80);
	//WriteTW88(REG114, 0x80);
	//WriteTW88(REG115, 0x00);
	WriteTW88(REG117, 0x80);	//Vertical Peaking Control I	def 0x30
	//WriteTW88(REG118, 0x44);
	//WriteTW88(REG11D, 0x7F);
	WriteTW88(REG11E, 0x00);	//Component video format: def 0x08
	//WriteTW88(REG120, 0x50);	
	WriteTW88(REG121, 0x22);	//Individual AGC Gain def 0x48
	//WriteTW88(REG122, 0xF0);
	//WriteTW88(REG123, 0xD8);
	//WriteTW88(REG124, 0xBC);
	//WriteTW88(REG125, 0xB8);
	//WriteTW88(REG126, 0x44);
	WriteTW88(REG127, 0x38);	//Clamp Position(PCLAMP)	def 0x2A
	WriteTW88(REG128, 0x00);
	//WriteTW88(REG129, 0x00);
	//WriteTW88(REG12A, 0x78);
	WriteTW88(REG12B, 0x44);	//Comb Filter	def -100-100b
	//WriteTW88(REG12C, 0x30);
	//WriteTW88(REG12D, 0x14);
	//WriteTW88(REG12E, 0xA5);
	//WriteTW88(REG12F, 0xE0);
	WriteTW88(REG130, 0x00);
	//WriteTW88(REG133, 0x05);	//CLMD
	WriteTW88(REG134, 0x1A);
	WriteTW88(REG135, 0x00);
								//ADCLLPLL
	//moveup	WriteTW88(REG1C0, 0x01);		//LLPLL input def 0x00
	//WriteTW88(REG1C2, 0x01);
	//WriteTW88(REG1C3, 0x03);
	//WriteTW88(REG1C4, 0x5A);
	//WriteTW88(REG1C5, 0x00);
	WriteTW88(REG1C6, 0x20); //WriteTW88(REG1C6, 0x27);	//WriteTW88(REG1C6, 0x20);  Use VAdcSetFilterBandwidth()
	//WriteTW88(REG1C7, 0x04);
	//WriteTW88(REG1C8, 0x00);
	//WriteTW88(REG1C9, 0x06);
	//WriteTW88(REG1CA, 0x06);
	WriteTW88(REG1CB, 0x3A);	//WriteTW88(REG1CB, 0x30);
	//WriteTW88(REG1CC, 0x00);
	//WriteTW88(REG1CD, 0x54);
	//WriteTW88(REG1D0, 0x00);
	//WriteTW88(REG1D1, 0xF0);
	//WriteTW88(REG1D2, 0xF0);
	//WriteTW88(REG1D3, 0xF0);
	//WriteTW88(REG1D4, 0x00);
	//WriteTW88(REG1D5, 0x00);
	//WriteTW88(REG1D6, 0x10);
	//WriteTW88(REG1D7, 0x70);
	//WriteTW88(REG1D8, 0x00);
	//WriteTW88(REG1D9, 0x04);
	//WriteTW88(REG1DA, 0x80);
	//WriteTW88(REG1DB, 0x80);
	//WriteTW88(REG1DC, 0x20);
	//WriteTW88(REG1E2, 0xD9);
	//WriteTW88(REG1E3, 0x07);
	//WriteTW88(REG1E4, 0x33);
	//WriteTW88(REG1E5, 0x31);
	//WriteTW88(REG1E6, 0x00);
	//WriteTW88(REG1E7, 0x2A);

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
	
	//WriteTW88(REG218, 0x00);	//VA_LEN_POS
	//WriteTW88(REG219, 0x00);
	//WriteTW88(REG21A, 0x00);
	//WriteTW88(REG21B, 0x00);
	WriteTW88(REG21C, 0x42);	//PANEL_FRUN. def 0x40
	//WriteTW88(REG21D, 0x00);
	WriteTW88(REG21E, 0x03);		//BLANK def 0x00.	--enable screen blanking. SW have to remove 21E[0]
	WriteTW88(REG280, 0x20);		//Image Adjustment register
	//WriteTW88(REG281, 0x80);
	//WriteTW88(REG282, 0x80);
	//WriteTW88(REG283, 0x80);
	//WriteTW88(REG284, 0x80);
	//WriteTW88(REG285, 0x80);
	//WriteTW88(REG286, 0x80);
	//WriteTW88(REG287, 0x80);
	//WriteTW88(REG288, 0x80);
	//WriteTW88(REG289, 0x80);
	//WriteTW88(REG28A, 0x80);
	WriteTW88(REG28B, 0x44);
	//WriteTW88(REG28C, 0x00);
	//WriteTW88(REG2B0, 0x10);	//Image Adjustment register	
	//WriteTW88(REG2B1, 0x40);	
	//WriteTW88(REG2B2, 0x40);
	//WriteTW88(REG2B6, 0x67);
	//WriteTW88(REG2B7, 0x94);
	//WriteTW88(REG2BF, 0x00);	//Test Pattern Generator Register
	//WriteTW88(REG2E0, 0x00);	//--disable gamma
	WriteTW88(REG2E4, 0x21);		//--dither
	//WriteTW88(REG2F8, 0x00);	//8bit Panel interface
	//WriteTW88(REG2F9, 0x00);	//8bit Panel interface	TW8832:0x80

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
	WriteTW88(REG007, 0x02);	// FP LSB
#endif 
}
//=============================================================================
//			                                               
//=============================================================================
#if 0
//test FOSD WIN1
void FOsdTest_Download_Win1(void)
{
	WriteTW88Page(0x00);
	WriteTW88(REG0E0,0x80);
	WriteTW88(REG0E0,0x07);
		 
	WriteTW88Page(0x03);
	WriteTW88(REG300,0x60);
	WriteTW88(REG3D0,0x09);
	WriteTW88(REG3D1,0x1b);
	WriteTW88(REG304,0x02);
	WriteTW88Page(0x00);
	WriteTW88(REG047,0x80);
	WriteTW88(REG040,0x02);
	WriteTW88Page(0x02);
	WriteTW88(REG20D,0x81);
					 
	WriteTW88Page(0x03);
	WriteTW88(REG305,0x0e);
					
	WriteTW88(REG304,0x01);
	WriteTW88(REG309,0xfb);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
					  
	WriteTW88(REG309,0xfc);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	
	WriteTW88(REG309,0xfd);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
					  
	WriteTW88(REG309,0xfe);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
					  
	WriteTW88(REG309,0xfa);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x31);
	WriteTW88(REG30A,0xff);
	WriteTW88(REG30A,0xc8);
	WriteTW88(REG30A,0x67);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x6e);
	WriteTW88(REG30A,0x76);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x13);
	WriteTW88(REG30A,0xff);
	WriteTW88(REG30A,0xc8);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x6e);
	WriteTW88(REG30A,0x76);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0xe6);
	WriteTW88(REG30A,0x13);
	WriteTW88(REG30A,0xff);
	WriteTW88(REG30A,0x8c);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
					  
	WriteTW88(REG304,0x00);
	 				  
	WriteTW88(REG30E,0x00);
	WriteTW88(REG311,0x43);
	WriteTW88(REG313,0x1c);
	WriteTW88(REG314,0x06);
	WriteTW88(REG316,0x03);
	WriteTW88(REG315,0x01);
	WriteTW88(REG318,0x84);
	WriteTW88(REG317,0x01);
	WriteTW88(REG31C,0x84);
	WriteTW88(REG31B,0x04);
	WriteTW88(REG319,0x01);
	WriteTW88(REG31A,0x01);
	WriteTW88(REG31D,0x00);
	WriteTW88(REG310,0xc0);
	WriteTW88(REG31F,0x00);
	WriteTW88(REG31E,0x32);
					
	WriteTW88(REG304,0x00);
	WriteTW88(REG306,0x00);
	WriteTW88(REG307,0xfb);
	WriteTW88(REG308,0x37);
	WriteTW88(REG306,0x01);
	WriteTW88(REG307,0xfb);
	WriteTW88(REG308,0x37);
	WriteTW88(REG306,0x02);
	WriteTW88(REG307,0xfb);
	WriteTW88(REG308,0x37);
	 	 			   
	WriteTW88Page(0x00); 	  	  
}

//test FOSD WIN5
void FOsdTest_Download_Win5(void)
{
						// page0
	WriteTW88Page(0x00);	 
	WriteTW88(REG0E0,0x80);
	WriteTW88(REG0E0,0x07);
						//; page3
	WriteTW88Page(0x03);
	WriteTW88(REG300,0x60);
	WriteTW88(REG3D0,0x09);
	WriteTW88(REG3D1,0x1b);
	WriteTW88(REG304,0x02);
						//; page0
	WriteTW88Page(0x00);
	WriteTW88(REG047,0x80);
	WriteTW88(REG040,0x02);
						//; page2
	WriteTW88Page(0x02);
	WriteTW88(REG20D,0x81);
						//; page3
	WriteTW88Page(0x03);
	WriteTW88(REG305,0x0e);
					 
	WriteTW88(REG304,0x01);
	WriteTW88(REG309,0xfb);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
				  
	WriteTW88(REG309,0xfc);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
				 
	WriteTW88(REG309,0xfd);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
				 
	WriteTW88(REG309,0xfe);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
	WriteTW88(REG30A,0x55);
	WriteTW88(REG30A,0xaa);
					 
	WriteTW88(REG309,0xfa);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x31);
	WriteTW88(REG30A,0xff);
	WriteTW88(REG30A,0xc8);
	WriteTW88(REG30A,0x67);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x6e);
	WriteTW88(REG30A,0x76);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x13);
	WriteTW88(REG30A,0xff);
	WriteTW88(REG30A,0xc8);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x6e);
	WriteTW88(REG30A,0x76);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0xe6);
	WriteTW88(REG30A,0x13);
	WriteTW88(REG30A,0xff);
	WriteTW88(REG30A,0x8c);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
	WriteTW88(REG30A,0x00);
				  
	WriteTW88(REG304,0x00);
				  
	WriteTW88(REG30E,0x00);
	WriteTW88(REG351,0x43);
	WriteTW88(REG353,0x1c);
	WriteTW88(REG354,0x06);
	WriteTW88(REG356,0x03);
	WriteTW88(REG355,0x01);
	WriteTW88(REG358,0x84);
	WriteTW88(REG357,0x01);
	WriteTW88(REG35C,0x84);
	WriteTW88(REG35B,0x04);
	WriteTW88(REG359,0x01);
	WriteTW88(REG35A,0x01);
	WriteTW88(REG35D,0x00);
	WriteTW88(REG350,0xc0);
	WriteTW88(REG35F,0x00);
	WriteTW88(REG35E,0x32);
					
	WriteTW88(REG304,0x00);
	WriteTW88(REG306,0x00);
	WriteTW88(REG307,0xfb);
	WriteTW88(REG308,0x37);
	WriteTW88(REG306,0x01);
	WriteTW88(REG307,0xfb);
	WriteTW88(REG308,0x37);
	WriteTW88(REG306,0x02);
	WriteTW88(REG307,0xfb);
	WriteTW88(REG308,0x37);
							//; page0   
	WriteTW88Page(0x00);
}
#endif

//=============================================================================
//			                                               
//=============================================================================
#if 0
void InitSystem(BYTE fPowerUpBoot)
{
}
#endif



//-----------------------------------------------------------------------------
// MAIN
//-----------------------------------------------------------------------------
void main(void)
{
	BYTE request_power_save_mode;
	BYTE ret;

	InitVariables();
	InitCPU();
	InitCore(1);

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
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	Puts("FPGA Verification - ");
#elif defined(MODEL_TW8835RTL) || defined(MODEL_TW8836RTL)
	Puts("RTL Verification - ");
#else
	Puts("Evaluation Board ");
	#ifdef EVB_10
	Puts("1.0 - ");
	#elif defined(EVB_20)
	Puts("2.0 - ");
	#elif defined(EVB_21)
	Puts("2.1 - ");
	#elif defined(EVB_30)
	Puts("3.0 - ");
	#elif defined(EVB_31)
	Puts("3.1 - ");
	#else
	Puts("0.0 - ");
	#endif
#endif
	Printf("%s (%s)", __TIME__, __DATE__);
	Printf("\n********************************************************");
	if(access==0) {
		Puts("\n***SKIP_MODE_ON***");
		DebugLevel=3;
		//skip...do nothing
		Puts("\nneed **init isr***init core***ee find***init ntsc***");
	}

	SetMonAddress(TW88I2CAddress);
	Prompt(); //first prompt

#ifdef MODEL_TW8836RTL
	InitWithNTSC_RTL();
	//===================================
	// RTL Verification
//	FOsdTest_Download_Win1();
//	FOsdTest_Download_Win5();
#endif


	//==================================================
	// Init System
	//==================================================
	//InitSystem(1);
	FOsdDownloadFontByDMA(0, 0x400000, 0x27F9);
	DumpFont();


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

//===================================================================================
// setting.c for RTL
//=====================================================================
BYTE shadow_r4e0;
BYTE shadow_r4e1;
BYTE McuSpiClkToPclk(BYTE divider)
{
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	BYTE temp;
	//do not change divider. I will use 65MHz PCLK with divider 1.
	temp = divider;			
	return 0;
#endif
#ifdef MODEL_TW8835_EXTI2C
	BYTE temp;
	temp = divider;			
	WriteTW88Page(PAGE4_CLOCK);
	shadow_r4e0 = ReadTW88(REG4E0);
	shadow_r4e1 = ReadTW88(REG4E1);

	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG002, 0xff );				
	while((ReadTW88(REG002) & 0x40) ==0);	//wait vblank  I2C_WAIT_VBLANK
PORT_DEBUG = 0;
 	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E0, shadow_r4e0 & 0xFE);	//select PCLK.
	WriteTW88(REG4E1, 0x20 | divider);
PORT_DEBUG = 1;
	return 0;
#else
	WriteTW88Page(PAGE4_CLOCK);
	shadow_r4e0 = ReadTW88(REG4E0);
	shadow_r4e1 = ReadTW88(REG4E1);

	WriteTW88(REG4E0, shadow_r4e0 & 0xFE);	//select PCLK.
	WriteTW88(REG4E1, 0x20 | divider);		//CLKPLL + divider.

	return 0;
#endif
}

//-----------------------------------------------------------------------------
/**
* restore MCUSPI clock
*
* @see McuSpiClkToPclk
*/

void McuSpiClkRestore(void)
{
#if defined(MODEL_TW8835FPGA) || defined(MODEL_TW8836FPGA)
	//do not change. I will use 65MHz PCLK with divider 1.
	return;
#endif
	//Printf("\nMcuSpiClkRestore REG4E0:%bx REG4E1:%bx",shadow_r4e0,shadow_r4e1);
#ifdef MODEL_TW8835_EXTI2C	//I2C_WAIT_VBLANK
	WriteTW88Page(PAGE0_GENERAL);
	WriteTW88(REG002, 0xff );				
	while((ReadTW88(REG002) & 0x40) ==0); //wait vblank
#endif
PORT_DEBUG = 0;
 	WriteTW88Page(PAGE4_CLOCK);
	WriteTW88(REG4E0, shadow_r4e0);
//#ifdef MODEL_TW8835_EXTI2C
//#else
	WriteTW88(REG4E1, shadow_r4e1);
//#endif
PORT_DEBUG = 1;
	//Printf("-Done");
}


//===================================================================================
// inputctrl.c for RTL
//=====================================================================
#ifdef MODEL_TW8835_EXTI2C
#define VBLANK_WAIT_VALUE	0x0100 
#else
#define VBLANK_WAIT_VALUE	0xFFFE 
#endif

void WaitVBlank(BYTE cnt)
{
	XDATA	BYTE i;
	WORD loop;
	DECLARE_LOCAL_page

	ReadTW88Page(page);
	WriteTW88Page(PAGE0_GENERAL );

	for ( i=0; i<cnt; i++ ) {
		WriteTW88(REG002, 0xff );
		loop = 0;
		while (!( ReadTW88(REG002 ) & 0x40 ) ) {
			// wait VBlank
			loop++;
			if(loop > VBLANK_WAIT_VALUE  ) {
				wPrintf("\nERR:WaitVBlank");
				break;
			}
		}		
	}
	WriteTW88Page(page);
}



//===================================================================================
// monitor.c for RTL
//=====================================================================


		BYTE 	DebugLevel = 0;
XDATA	BYTE	MonAddress = TW88I2CAddress;	
XDATA	BYTE	MonIndex;
XDATA	BYTE	MonRdata, MonWdata;
XDATA	BYTE	monstr[50];				// buffer for input string
XDATA	BYTE 	*argv[12];
XDATA	BYTE	argc=0;
		bit		echo=1;
		bit		access=1;

void Prompt(void)
{
#ifdef BANKING
	if ( MonAddress == TW88I2CAddress )
		Printf("\n[B%02bx]MCU_I2C[%02bx]>", BANKREG, MonAddress);
	else
#else
	if ( MonAddress == TW88I2CAddress )
		Printf("\nMCU_I2C[%02bx]>", MonAddress);
	else
#endif
		Printf("\nI2C[%02bx]>", MonAddress);
}

void Mon_tx(BYTE ch)
{
	RS_tx(ch);
}

void SetMonAddress(BYTE addr)
{
	MonAddress = addr;
}

void MonReadI2CByte(void)
{
	////////////////////////////////////
	BYTE len;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
			
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if( argc>=2 ) 
		for(len=0; len<10; len++) 
			if( argv[1][len]==0 ) 
				break;

	if( len>2 ) {
		MonWdata = a2h( argv[1] ) / 0x100;
		MonIndex = 0xff;

		Printf("\nWrite %02bxh:%02bxh ", MonIndex, MonWdata);
		if ( MonAddress == TW88I2CAddress ) {
			WriteTW88Byte(MonIndex, MonWdata);
			MonRdata = ReadTW88Byte(MonIndex);
		}
		else {
			WriteI2CByte(MonAddress, MonIndex, MonWdata);
			MonRdata = ReadI2CByte(MonAddress, MonIndex);
		}
   		Printf("==> Read %02bxh:%02bxh", MonIndex, MonRdata);
		if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
	}
#endif
	////////////////////////////////////

	if( argc>=2 ) {
#ifndef SUPPORT_8BIT_CHIP_ACCESS
		for(len=0; len<10; len++) 
			if( argv[1][len]==0 ) 
				break;
		if( len>2 ) {
			MonPage = a2h( argv[1] );	 	//page+index
			MonRdata = ReadTW88(MonPage);
			if( echo )
				Printf("\nRead %03xh:%02bxh", MonPage, MonRdata);
			return;	
		}
#endif	
		MonIndex = a2h( argv[1] );
	}
	else	{
		Printf("   --> Missing parameter !!!");
		return;
	}

#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if ( MonAddress == TW88I2CAddress )
		MonRdata = ReadTW88(MonIndex);
#else
	if ( MonAddress == TW88I2CAddress )	{
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
#endif
	//else if( MonAddress == TW88SalveI2CAddress )
	//	MonRdata = ReadI2CByte(MonAddress+2, MonIndex);
	else
		MonRdata = ReadI2CByte(MonAddress, MonIndex);
	if( echo )
		Printf("\nRead %02bxh:%02bxh", MonIndex, MonRdata);	
	
	MonWdata = MonRdata;
}

/**
*
* w addr data

*/
void MonWriteI2CByte(void) 
{
	////////////////////////////////////
	BYTE len;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif

#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if( argc>=2 ) 
		for(len=0; len<10; len++) 
			if( argv[1][len]==0 ) 
				break;

	if( len>2 ) {
		MonWdata = a2h( argv[1] ) / 0x100;
		MonIndex = 0xff;

		Printf("\nWrite %02bxh:%02bxh ", MonIndex, MonWdata);
		if ( MonAddress == TW88I2CAddress ) {
			if(MonIndex==0xff) 	{ WriteTW88BytePage(MonWdata); }
			else				WriteTW88Byte(MonIndex, MonWdata);
			MonRdata = ReadTW88Byte(MonIndex);
		}
		else {
			WriteI2CByte(MonAddress, MonIndex, MonWdata);
			MonRdata = ReadI2CByte(MonAddress, MonIndex);
		}
   		Printf("==> Read %02bxh:%02bxh", MonIndex, MonRdata);
		if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
	}
#endif
	////////////////////////////////////

	if( argc<3 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	MonIndex = a2h( argv[1] );
	MonWdata = a2h( argv[2] );

#ifndef SUPPORT_8BIT_CHIP_ACCESS
	for(len=0; len<10; len++) 
		if( argv[1][len]==0 ) 
			break;
	if( len>2 ) {
		MonPage = a2h( argv[1] );	 	//page+index
		WriteTW88(MonPage,MonWdata);
		if( echo ) {
			Printf("\nWrite %03xh:%02bxh", MonPage, MonWdata);
			MonRdata = ReadTW88(MonPage);			
	   		Printf("==> Read %03xh:%02bxh", MonPage, MonRdata);
			if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
		}
		return;	
	}
#endif	
	
	if( echo ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
		Printf("\nWrite %02bxh:%02bxh ", MonIndex, MonWdata);
		if ( MonAddress == TW88I2CAddress ) {
			WriteTW88(MonIndex, MonWdata);
			MonRdata = ReadTW88(MonIndex);
		}
#else
		MonPage = ReadTW88Byte(0xff) << 8;
		Printf("\nWrite %03xh:%02bxh ", MonPage | MonIndex, MonWdata);
		if ( MonAddress == TW88I2CAddress ) {
			WriteTW88(MonPage | MonIndex, MonWdata);

			MonPage = ReadTW88Byte(0xff) << 8;
			MonRdata = ReadTW88(MonPage | MonIndex);
		}
#endif
		//else if( MonAddress == TW88SalveI2CAddress ) {
		//	WriteI2CByte(MonAddress+2, MonIndex, MonWdata);
		//	MonRdata = ReadI2CByte(MonAddress+2, MonIndex);
		//}
		else {
			WriteI2CByte(MonAddress, MonIndex, MonWdata);
			MonRdata = ReadI2CByte(MonAddress, MonIndex);
		}
   		Printf("==> Read %02bxh:%02bxh", MonIndex, MonRdata);
		if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
	}
	else {
		if ( MonAddress == TW88I2CAddress ) {
			if(MonIndex==0xff) 	{ WriteTW88Page(MonWdata); }
			else				WriteTW88(MonIndex, MonWdata);
		}
		//else if( MonAddress == TW88SalveI2CAddress )
		//	WriteI2CByte(MonAddress+2, MonIndex, MonWdata);
		else {
			WriteI2CByte(MonAddress, MonIndex, MonWdata);
		}
	}
}

void MonIncDecI2C(BYTE inc)
{
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
	switch(inc){
		case 0:  MonWdata--;	break;
		case 1:  MonWdata++;	break;
		case 10: MonWdata-=0x10;	break;
		case 11: MonWdata+=0x10;	break;
	}

	if ( MonAddress == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
		WriteTW88(MonIndex, MonWdata);
		MonRdata = ReadTW88(MonIndex);
#else
		MonPage = ReadTW88Byte(0xff) << 8;
		WriteTW88(MonPage | MonIndex, MonWdata);
		MonRdata = ReadTW88(MonPage | MonIndex);
#endif
	}
	else {
		WriteI2CByte(MonAddress, MonIndex, MonWdata);
		MonRdata = ReadI2CByte(MonAddress, MonIndex);
	}

	if( echo ) {
		Printf("Write %02bxh:%02bxh ", MonIndex, MonWdata);
		Printf("==> Read %02bxh:%02bxh", MonIndex, MonRdata);
	}

	Prompt();
}

void MonDumpI2C(void)
{
	BYTE ToMonIndex;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
	int  cnt=7;

	if( argc>=2 ) MonIndex   = a2h(argv[1]);
	if( argc>=3 ) ToMonIndex = a2h(argv[2]);
	else          ToMonIndex = MonIndex+cnt;
	
	if ( ToMonIndex < MonIndex ) ToMonIndex = 0xFF;
	cnt = ToMonIndex - MonIndex + 1;

	if( echo ) {
		if ( MonAddress == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonIndex);
				Printf("\n==> Read %02bxh:%02bxh", MonIndex, MonRdata);
				MonIndex++;
			}
#else
			MonPage = ReadTW88Byte(0xff) << 8;
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonPage | MonIndex);
				Printf("\n==> Read %02bxh:%02bxh", MonIndex, MonRdata);
				MonIndex++;
			}
#endif
		}
		else {
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadI2CByte(MonAddress, MonIndex);
				Printf("\n==> Read %02bxh:%02bxh", MonIndex, MonRdata);
				//if( MonWdata != MonRdata ) Printf(" [%02bx]", MonWdata);
				MonIndex++;
			}
		}
	}
	else {
		if ( MonAddress == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonIndex);
				MonIndex++;
			}
#else
			MonPage = ReadTW88Byte(0xff) << 8;
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadTW88(MonPage | MonIndex);
				MonIndex++;
			}
#endif
		}
		else {
			for ( ; cnt > 0; cnt-- ) {
				MonRdata = ReadI2CByte(MonAddress, MonIndex);
				MonIndex++;
			}
		}
	}
}

//-----------------------------------------------------------------------------

void MonNewReadI2CByte(void)
{
	BYTE Slave;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif

	if( argc>=3 ) MonIndex = a2h( argv[2] );
	else	{
		Printf("   --> Missing parameter !!!");
		return;
	}
	Slave = a2h(argv[1]);

#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if ( Slave == TW88I2CAddress )
		MonRdata = ReadTW88(MonIndex);
#else
	if ( Slave == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
#endif	
	else
		MonRdata = ReadI2CByte(Slave, MonIndex);

	if( echo )
		Printf("\n<R>%02bx[%02bx]=%02bx", Slave, MonIndex, MonRdata);
	MonWdata = MonRdata;
}

void MonNewWriteI2CByte(void)
{
	BYTE Slave;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif

	if( argc<4 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	
	Slave    = a2h( argv[1] );
	MonIndex = a2h( argv[2] );
	MonWdata = a2h( argv[3] );
	
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if ( Slave == TW88I2CAddress ) {
		WriteTW88(MonIndex, MonWdata);
		MonRdata = ReadTW88(MonIndex);
	}
#else
	if ( Slave == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		WriteTW88(MonPage | MonIndex, MonWdata);
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
#endif
	else {
		WriteI2CByte(Slave, MonIndex, MonWdata);
		MonRdata = ReadI2CByte(Slave, MonIndex);
   	}
	if( echo ) {
		Printf("\n<R>%02bx[%02bx]=%02bx", Slave, MonIndex, MonRdata);
	}
}

void MonNewDumpI2C(void)
{
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
	BYTE 	ToMonIndex, Slave;
	WORD	i;
	
	if( argc>=2 ) MonIndex = a2h(argv[2]);
	if( argc>=3 ) ToMonIndex = a2h(argv[3]);
	Slave = a2h(argv[1]);

	if( echo ) {
		if ( Slave == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
			for(i=MonIndex; i<=ToMonIndex; i++) {
				MonRdata = ReadTW88(i);
        		Printf("\n<R>%02bx[%02x]=%02bx", Slave, i, MonRdata);
			}
#else
			MonPage = ReadTW88Byte(0xff) << 8;
			for(i=MonIndex; i<=ToMonIndex; i++) {
				MonRdata = ReadTW88(MonPage | i);
        		Printf("\n<R>%02bx[%02x]=%02bx", Slave, i, MonRdata);
			}
#endif
		}
		else {
			for(i=MonIndex; i<=ToMonIndex; i++) {
				MonRdata = ReadI2CByte(Slave, i);
        		Printf("\n<R>%02bx[%02x]=%02bx", Slave, i, MonRdata);
			}
		}
	}
	else {
		if ( Slave == TW88I2CAddress ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
			for(i=MonIndex; i<=ToMonIndex; i++)
				MonRdata = ReadTW88(i);
#else
			MonPage = ReadTW88Byte(0xff) << 8;
			for(i=MonIndex; i<=ToMonIndex; i++)
				MonRdata = ReadTW88(MonPage | i);
#endif
		}
		else {
			for(i=MonIndex; i<=ToMonIndex; i++) {
				MonRdata = ReadI2CByte(Slave, i);
			}
		}
	}
}

//format:
// 	b 88 index startbit|endbit data
void MonWriteBit(void)
{
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD MonPage;
#endif
	BYTE mask, i, FromBit, ToBit,  MonMask, val;
	BYTE Slave;

	if( argc<5 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	Slave = a2h(argv[1]);

	MonIndex = a2h( argv[2] );
	FromBit  =(a2h( argv[3] ) >> 4) & 0x0f;
	ToBit    = a2h( argv[3] )  & 0x0f;
	MonMask  = a2h( argv[4] );

	if( FromBit<ToBit || FromBit>7 || ToBit>7) {
		Printf("\n   --> Wrong range of bit operation !!!");
		return;
	}
	
	mask = 0xff; 
	val=0x7f;
	for(i=7; i>FromBit; i--) {
		mask &= val;
		val = val>>1;
	}

	val=0xfe;
	for(i=0; i<ToBit; i++) {
		mask &= val;
		val = val<<1;
	}

#ifdef SUPPORT_8BIT_CHIP_ACCESS
	if ( Slave == TW88I2CAddress ) {
		MonRdata = ReadTW88(MonIndex);
		MonWdata = (MonRdata & (~mask)) | (MonMask & mask);
				
		WriteTW88Byte(MonIndex, MonWdata);
		MonRdata = ReadTW88(MonIndex);
	}
#else
	if ( Slave == TW88I2CAddress ) {
		MonPage = ReadTW88Byte(0xff) << 8;
		MonRdata = ReadTW88(MonPage | MonIndex);
		MonWdata = (MonRdata & (~mask)) | (MonMask & mask);
				
		WriteTW88(MonPage | MonIndex, MonWdata);
		MonRdata = ReadTW88(MonPage | MonIndex);
	}
#endif
	else {
		MonRdata = ReadI2CByte(Slave, MonIndex);
		MonWdata = (MonRdata & (~mask)) | (MonMask & mask);
				
		WriteI2CByte(Slave, MonIndex, MonWdata);
		MonRdata = ReadI2CByte(Slave, MonIndex);
	}
	if( echo )
		//TW_TERMINAL need this syntax
		Printf("\n<R>%02bx[%02bx]=%02bx", Slave, MonIndex, MonRdata);
}
// wait reg mask result max_wait
void MonWait(void)
{
	WORD i,max;
	BYTE reg, mask, result;
	if( argc<5 ) {
		Printf("   --> Missing parameter !!!");
		return;
	}
	reg = a2h( argv[1] );
	mask = a2h( argv[2] );
	result = a2h( argv[3] );
	max = a2h( argv[4] );
	for(i=0; i < max; i++) {
		if((ReadTW88(reg) & mask)==result) {
			Printf("=>OK@%bd",i);
			break;
		}
		delay1ms(2);
	}
	if(i >= max)
		Printf("=>fail wait %bx %bx %bx %d->fail",reg,mask,result,max);
}

//=============================================================================
//			Help Message
//=============================================================================
void MonHelp(void)
{
	Puts("\n=======================================================");
	Puts("\n>>>     Welcome to Intersil Monitor  Rev 1.01       <<<");
	Puts("\n=======================================================");
	Puts("\n   R ii             ; Read data.(");
	Puts("\n   W ii dd          ; Write data.)");
	Puts("\n   D [ii] [cc]      ; Dump.&");
	Puts("\n   B AA II bb DD    ; Bit Operation. bb:StartEnd");
	Puts("\n   C aa             ; Change I2C address");
	Puts("\n   Echo On/Off      ; Terminal Echoing On/Off");
#ifdef EVB_30
	Puts("\n   HDMI ii nn       ; HDMI register ii read nn bytes");
	Puts("\n   HDINIT           ; HDMI initialize");
	Puts("\n   HDINFO           ; HDMI ouput timing");
#endif
	Puts("\n=======================================================");
	Puts("\n=== DEBUG ACCESS time init MCU SPI EE menu task [on] ====");
	Puts("\nM [CVBS|SVIDEO|COMP|PC|DVI|HDMI|BT656]      ; Change Input Mode");
	Puts("\nselect [CVBS|SVIDEO|COMP|PC|DVI|HDMI|BT656] ; select Input Mode");
	Puts("\ninit default                               : default for selected input");
	Puts("\nCheckAndSet                                ; CheckAndSet selected input");
	Puts("\n=======================================================");
#if defined(MODEL_TW8835_SLAVE) && defined(SUPPORT_I2CCMD_TEST)
	Puts("\ni2ctest mode duration");
	Puts("\n	mode:0	read");
	Puts("\n	mode:1	read with SFR_EA");
	Puts("\n	mode:2	read and write");
	Puts("\n	mode:3	write");
	Puts("\n	mode:4	write x 20 times");
	Puts("\n	mode:5	read R4DC");
	Puts("\n	mode:6	write R4DC=0xAB");
	Puts("\ni2ctestpage page           ; set compare page");
	Puts("\n=======================================================");
#endif
#if defined(SUPPORT_I2CCMD_TEST)
	Puts("\ni2ccmdtest mode duration");
	Puts("\n	mode:0	??");
	Puts("\n	mode:1	??");
	Puts("\n	mode:2	??");
#endif
	Puts("\n");

}
//=============================================================================
//
//=============================================================================
BYTE MonGetCommand(void)
{
	static BYTE comment=0;
	static BYTE incnt=0, last_argc=0;
	BYTE i, ch;
	BYTE ret=0;

	if( !RS_ready() ) return 0;
	ch = RS_rx();

	//----- if comment, echo back and ignore -----
	if( comment ) {
		if( ch=='\r' || ch==0x1b ) comment = 0;
		else { 
			Mon_tx(ch);
			return 0;
		}
	}
	else if( ch==';' ) {
		comment = 1;
		Mon_tx(ch);
		return 0;
	}

	//=====================================
	switch( ch ) {

	case 0x1b:
		argc = 0;
		incnt = 0;
		comment = 0;
		Prompt();
		return 0;

	//--- end of string
	case '\r':

		if( incnt==0 ) {
			Prompt();
			break;
		}

		monstr[incnt++] = '\0';
		argc=0;

		for(i=0; i<incnt; i++) if( monstr[i] > ' ' ) break;

		if( !monstr[i] ) {
			incnt = 0;
			comment = 0;
			Prompt();
			return 0;
		}
		argv[0] = &monstr[i];
		for(; i<incnt; i++) {
			if( monstr[i] <= ' ' ) {
				monstr[i]='\0';
				i++;
				while( monstr[i]==' ' ) i++;
				argc++;
				if( monstr[i] ){
     			 	argv[argc] = &monstr[i];
				}
			}
		}

		ret = 1;
		last_argc = argc;
		incnt = 0;
		
		break;

	//--- repeat command
	case '/':
		argc = last_argc;
		ret = 1;
		break;

	//--- repeat command without CR
	case '`':
	{
		BYTE i, j, ch;

		for(j=0; j<last_argc; j++) {
			for(i=0; i<100; i++) {
				ch = argv[j][i];
				if( ch==0 ) {
					if( j==last_argc-1 ) return 0;
					ch = ' ';
					RS_ungetch( ch );
					break;
				}
				else {
					RS_ungetch( ch );
				}
			}
		}
		break;
	}

	//--- back space
	case 0x08:
		if( incnt ) {
			incnt--;
			Mon_tx(ch);
			Mon_tx(' ');
			Mon_tx(ch);
		}
		break;

	//--- decreamental write
	case ',':
		if( incnt ) {
			Mon_tx(ch);
			monstr[incnt++] = ch;
		}
		else
			MonIncDecI2C(0);
		break;

	case '<':
		if( incnt ) {
			Mon_tx(ch);
			monstr[incnt++] = ch;
		}
		else
			MonIncDecI2C(10);
		break;

	//--- increamental write
	case '.':
		if( incnt ) {
			Mon_tx(ch);
			monstr[incnt++] = ch;
		}
		else
			MonIncDecI2C(1);
		break;

	case '>':
		if( incnt ) {
			Mon_tx(ch);
			monstr[incnt++] = ch;
		}
		else
			MonIncDecI2C(11);
		break;

	default:
		Mon_tx(ch);
		monstr[incnt++] = ch;
		break;
	}

	if( ret ) {
		comment = 0;
		last_argc = argc;
		return ret;
	}
	else {
		return ret;
	}
}
//*****************************************************************************
//				Monitoring Command
//*****************************************************************************

BYTE *MonString = 0;

void WaitVBlank1(void)
{
	//XDATA	BYTE i;
	WORD loop;
	volatile BYTE vdata;

	WriteTW88Page(0);	//WriteI2CByte(0x8a,0xff,0x00);

//	PORT_DEBUG = 0;
	WriteTW88(REG002, 0xFF);	//WriteI2CByte( 0x8a,0x02, 0xff ); //clear
	loop = 0;
	while(1) {
		vdata = ReadTW88(REG002);	//vdata = ReadI2CByte( 0x8a,0x02 );
		//Printf("\nREG002:%bx", vdata);		
		if(vdata & 0x40) 
			break;
		loop++;
		if(loop > 0xFFFE) {
			ePrintf("\nERR:WaitVBlank");
			break;
		}
	}
//	PORT_DEBUG = 1;
	//Printf("\nWaitVBlank1 loop:%d", loop);
}

void Monitor(void)
{
	WORD wValue;
#if defined(MODEL_TW8835_EXTI2C)
	DWORD dValue;
#endif

	if( MonString ) {																				  
		RS_ungetch( *MonString++ );
		if( *MonString==0 ) MonString = 0;
	}

	if( !MonGetCommand() ) return;

	//---------------- Write Register -------------------
	if( !stricmp( argv[0], "W" ) ) {
		MonWriteI2CByte();
	}
	else if( !stricmp( argv[0], ")" ) ) {
		MonNewWriteI2CByte();
	}
	//---------------- Read Register --------------------
	else if ( !stricmp( argv[0], "R" ) ) {
		MonReadI2CByte();
	}
	else if ( !stricmp( argv[0], "(" ) ) {
		MonNewReadI2CByte();
	}
	//---------------- Dump Register --------------------
	else if( !stricmp( argv[0], "D" ) ) {
		Puts("\ndump start");
		MonDumpI2C();
	}
	else if( !stricmp( argv[0], "&" ) ) {
		MonNewDumpI2C();
	}

	//---------------- Bit Operation --------------------
	else if( !stricmp( argv[0], "B" ) ) {// Write bits - B AA II bb DD
		MonWriteBit();
	}
	//---------------- Change I2C -----------------------
	else if( !stricmp( argv[0], "C" ) ) {
		MonAddress = a2h( argv[1] );
Printf("\nSetMonAddress:%d",__LINE__);
	}
	//---------------- wait -----------------------
	else if( !stricmp( argv[0], "wait" ) ) {
		MonWait();
	}
	//---------------- delay -----------------------
	else if( !stricmp( argv[0], "delay" ) ) {
		wValue = a2i( argv[1] );
		delay1ms(wValue);
	}
	//---------------- Help -----------------------------
	else if( !stricmp( argv[0], "H" ) || !stricmp( argv[0], "HELP" ) || !stricmp( argv[0], "?" ) ) {
		MonHelp();

	}
#if 0
	else if(!stricmp( argv[0], "wwww")) {
		WaitVBlank(1);
	}
#endif
	//---------------- HOST OSIOSD TEST  ------
#if 1 // OSPOSD Move test
#endif
#if 1
	else if(!stricmp( argv[0], "move1")) {
	}
	else if(!stricmp( argv[0], "move2")) {
	}
	else if(!stricmp( argv[0], "move3")) {
	}
#endif
#if 0
	//---------------- UART TEST  ------
	else if(!stricmp( argv[0], "UARTDUMP")) {
		BYTE	ch;

		do {
			if( !RS_ready() ) continue;
			ch = RS_rx();
			Printf("%02bx ", ch );
		} while ( ch != 'x' );
	}
#endif

//#if 1
//	else if( !stricmp( argv[0], "crc8" ) ) {
//		TestCrC8();
//	}
//#endif

	//---------------- Read/Write Register for slow I2C  ------
#if 0
//!	//==>to test SW I2C Slave
//!	else if ( !stricmp( argv[0], "RR" ) ) {
//!		MonReadSlowI2CByte();	
//!	}
//!	else if( !stricmp( argv[0], "WW" ) ) {
//!		MonWriteSlowI2CByte();	
//!	}
#endif
#if 0
//!	//---------------- Read/Write Register for internal register ------
//!	else if ( !stricmp( argv[0], "RR" ) ) {
//!		MonReadInternalReg();	//==>see "mcu Rx addr"
//!	}
//!	else if( !stricmp( argv[0], "WW" ) ) {
//!		MonWriteInternalReg();	//==>see "mcu Wx addr"
//!	}
#endif

#ifdef SW_I2C_SLAVE
#endif
	//---------------------------------------------------
	//----------------------------------------------------
	//---------------- Init ------------------------------
	//----------------------------------------------------
	else if(!stricmp( argv[0], "init" ) ) {
	}
	//-----input select------------------------------------------
	else if ( !stricmp( argv[0], "input" ) ) {
	}
	//---------------- Change Input Mode ---------------------
	// M [CVBS|SVIDEO|COMP|PC|DVI|HDMI|BT656]
	//--------------------------------------------------------      
	else if ( !stricmp( argv[0], "M" ) ) {
	}
	//---------------- CheckAndSet ---------------------
	else if ( !stricmp( argv[0], "CheckAndSet" ) ) {
	}
	//---------------- check -------------------------
	else if ( !stricmp( argv[0], "check" ) ) {
	}

	//---------------- SPI Debug -------------------------
	else if( !stricmp( argv[0], "SPI" ) ) {
	}
	else if( !stricmp( argv[0], "SPIC" ) ) {
	}
	//---------------- EEPROM Debug -------------------------
#ifdef USE_SFLASH_EEPROM
	else if( !stricmp( argv[0], "EE" ) ) {
	}
#endif
	//---------------- MENU Debug -------------------------
	else if( !stricmp( argv[0], "menu" ) ) {
	}
	//---------------- Font Osd Debug -------------------------
	else if( !stricmp( argv[0], "fosd" ) ) {
	}
	//---------------- SPI Osd Debug -------------------------
	else if( !stricmp( argv[0], "sosd" ) ) {
	}
	//---------------- Debug Level ---------------------
	else if ( !stricmp( argv[0], "DEBUG" ) ) {
		if( argc==2 ) {
			DebugLevel = a2h(argv[1]);
		}
		Printf("\nDebug Level = %2bx", DebugLevel);
	}
	//---------------- Echo back on/off -----------------
	else if ( !stricmp( argv[0], "echo" ) ) {
		if( !stricmp( argv[1], "off" ) ) {
			echo = 0;
			Printf("\necho off");
		}
		else {
			echo = 1;
			Printf("\necho on");
		}
	}
	//---------------- Echo back on/off -----------------
	else if ( !stricmp( argv[0], "ACCESS" ) ) {
		if( !stricmp( argv[1], "0" ) ) {
			access = 0;
			Printf("\nAccess off");
			//disable interrupt.
			WriteTW88Page(PAGE0_GENERAL );
			WriteTW88(REG003, 0xFE );	// enable only SW interrupt
		}
		else {
			access = 1;
			Printf("\nAccess on");
		}
	}
	//---------------- task on/off -----------------
	else if ( !stricmp( argv[0], "task" ) ) {
	}
	//---------------- System Clock Display -----------------
	else if ( !stricmp( argv[0], "time" ) ) {
			Printf("\nSystem Clock: %ld:%5bd", SystemClock, tic01);
	}
	//---------------- MCU Debug -------------------------
	else if( !stricmp( argv[0], "MCU" ) ) {
	//	MonitorMCU();
	}
	//---------------- HDMI test -------------------------
#ifdef SUPPORT_HDMI_EP9351
#endif
	
#if 0
	//---------------- pclk -------------------------
	//	pclk 1080 means 108MHz
	//	pclk 27 means 27MHz
	else if( !stricmp( argv[0], "pclk" ) ) {
		if(argc >= 2) {
			dValue = a2i( argv[1] );
			dValue *= 100000L;
		 	SspllSetFreqAndPll(dValue);
		}
		//print current pclk info
		Printf("\nsspll:%ld",SspllGetPPF(0));
	}
#endif
#if defined(MODEL_TW8835_EXTI2C)
#endif	
	//====================================================
	// OTHER TEST ROUTINES
	//====================================================
#if defined(MODEL_TW8835_SLAVE) && defined(SUPPORT_I2CCMD_TEST)
#endif
#if defined(SUPPORT_I2CCMD_TEST)
#endif

	else if( !stricmp( argv[0], "testfont" ) )	 {
	}	

	//==========================================
	// FontOSD Test
	//==========================================
#if 1
	else if ( !stricmp( argv[0], "FT0" )) {
	}
	else if ( !stricmp( argv[0], "FT1" )) {
	}
	else if ( !stricmp( argv[0], "FT2" )) {
	}
	else if ( !stricmp( argv[0], "FT3" )) {
	}
	else if ( !stricmp( argv[0], "FT4" )) {
	}
	else if ( !stricmp( argv[0], "FT5" )) {
	}
	else if ( !stricmp( argv[0], "FT6" )) {
	}
#endif
	//==========================================
	// SpiOSD Test
	//==========================================
#if 0
#endif

	//---------------- TOUCH Debug -------------------------
#ifdef SUPPORT_TOUCH
#endif
	//---------------- Delta RGB Panel Test -------------------------
#ifdef SUPPORT_DELTA_RGB
#endif
#ifdef EVB_30
	//---------------- HDMI -------------------
	//Read HDMI register

#endif

#if 1
	//----<<TEST customer PARKGRID>>------------------------------------
#endif
	//----------------------------------------------------
	//make compiler happy.
	//Please, DO NOT EXECUTE
	//----------------------------------------------------	
	else if(!stricmp( argv[0], "compiler" )) {
#ifndef MODEL_TW8835_MASTER
		CheckI2C(0x8A);
#endif
#if !defined(SUPPORT_HDMI_SiIRX)
		WriteI2C(0x00, 0x00, (BYTE *)&wValue, 1);
		ReadI2C(0x00, 0x00, (BYTE *)&wValue, 1);
#endif
		WriteSlowI2CByte(TW88I2CAddress, 0xFF,4);
		ReadSlowI2CByte(I2CID_SX1504, 0);
		I2CDeviceInitialize((BYTE *)&wValue, 0);
		delay1s(0,0);
		EnableWatchdog(0);
		DisableWatchdog();
		EnableInterrupt(0);
		DisableInterrupt(0);
		TWitoa(0, (BYTE *)&wValue);
		TWhtos(0, (BYTE *)&wValue);
		TWstrlen((BYTE *)&wValue);
		TWstrcpy((BYTE *)&wValue,(BYTE *)&wValue);
		TWstrcat((BYTE *)&wValue,(BYTE *)&wValue);
		IsDigit(0);
		TWabsShort(0);
		dPrintf("\nTest");
		wPrintf("\nTest");
		dPuts("\nTest");
		wPuts("\nTest");
		PrintSystemClockMsg("test");
		WaitVBlank1();

	}
	//----------------------------------------------------
	else {
		Printf("\nInvalid command...");
	}
	Prompt();
}

//=======================================================================
// SPI.C for RTL
//=======================================================================



XDATA BYTE SPI_Buffer[SPI_BUFFER_SIZE];

BYTE SPICMD_x_READ      	= 0x03;
BYTE SPICMD_x_BYTES			= 5;
BYTE SpiFlashVendor;

//-----------------------------------------------
//internal prototype
//-----------------------------------------------
void SpiFlashCmd2(BYTE cmd1, BYTE cmd2);
void SpiFlashCmd3(BYTE cmd1, BYTE cmd2, BYTE cmd3);

//-----------------------------------------------------------------------------
/**
* stop SpiFlashDMA
*
*/
#if 0
void SpiFlashDmaStop(void)
{
#ifdef MODEL_TW8835_EXTI2C
//skip
#else
	WriteTW88Page(PAGE4_SPI);
	//if(ReadTW88(REG4C4) & 0x01)
	//	Printf("\nLINE:%d DMA STOP at BUSY",__LINE__);

	WriteTW88(REG4C4, 0);
#endif
}
#endif

#if 0
void SpiFlashCmdRead(BYTE dest)
{
	WriteTW88(REG4C3, dest << 6 | SPICMD_x_BYTES);
	WriteTW88(REG4CA, SPICMD_x_READ);
}
#endif

void SpiFlashDmaDestType(BYTE dest, BYTE access_mode)
{
	BYTE dat;
	dat = ReadTW88(REG4C3) & 0x0F;
	dat |= (dest << 6);
	dat |= (access_mode << 4);
	WriteTW88(REG4C3, dat);
}

//-----------------------------------------------------------------------------
/**
* set DMA Length
*/
void SpiFlashSetCmdLength(BYTE len)
{
	WriteTW88(REG4C3, (ReadTW88(REG4C3) & 0xF0) | len);
}


//-----------------------------------------------------------------------------
/**
* wait until DMA start flag is cleared.
*
* SpiFlashDmaStart() call it.
* If DMA was a read, we can not use a busy flag.
* So, FW checks the start bit that is a self clear bit.
* 
* R4C4[0] : start command execution. Self cleared.
*/
static BYTE SpiFlashDmaWait(BYTE wait, BYTE delay, WORD call_line)
{
	BYTE i;
	volatile BYTE vdata;
	//------------------------
	//FYI:Assume it is a Page4
	//WriteTW88Page(4);
	//------------------------
	for(i=0; i < wait; i++) {
		vdata = ReadTW88(REG4C4);
		if((vdata & 0x01)==0)	//check a self clear bit
			break;
		if(delay)
			delay1ms(delay);
	}
	if(i==wait) {
		Printf("\nSpiFlashDmaWait DMA Busy. LINE:%d",call_line);
		return ERR_FAIL;
	}
	return ERR_SUCCESS;
}

//-----------------------------------------------------------------------------
/**
* start SpiFlashDMA
*
* use REG4C1[0]=1 on ExtI2C mode.
* see REG4C1[0]: At Vertical Blank
*
* @param fWrite
*	- 0:read, 1:write
* @param fBusy
*	busy check. see REG4D8 and REG4D9. So, only works with a write mode
* @param call_line for debug
*/
void SpiFlashDmaStart(BYTE fWrite, BYTE fBusy, WORD call_line)
{
	BYTE dat;

	dat = 0x01;					//start
	if(fWrite) 	dat |= 0x02;	//write
	if(fBusy)	dat |= 0x04;	//busy

	WriteTW88Page(4);
	WriteTW88(REG4C4, dat);
	SpiFlashDmaWait(200,1,call_line);
}


//-----------------------------------------------------------------------------
/**
* assign SpiFlashDMA buffer address
*/
void SpiFlashDmaBuffAddr(WORD addr)
{
	WriteTW88(REG4C6, (BYTE)(addr >> 8));	//page
	WriteTW88(REG4C7, (BYTE)addr);			//index
}
#if defined(MODEL_TW8836_MASTER)
void I2cSpiFlashDmaBuffAddr(WORD addr)
{
	WriteTW88(REG4F6, (BYTE)(addr >> 8));	//page
	WriteTW88(REG4F7, (BYTE)addr);			//index
}
#endif


//-----------------------------------------------------------------------------
/**
* assign the read length
*/
void SpiFlashDmaReadLen(DWORD len)
{
	WriteTW88(REG4DA, len>>16 );
	WriteTW88(REG4C8, len>>8 );
	WriteTW88(REG4C9, len );
}
//-----------------------------------------------------------------------------
/**
* assign the read length (only low byte)
*/
void SpiFlashDmaReadLenByte(BYTE len_l)
{
	WriteTW88(REG4C9, len_l );
}
#if defined(MODEL_TW8836_MASTER)
void I2cSpiFlashDmaReadLen(DWORD len)
{
	WriteTW88(REG4F5, len>>16 );
	WriteTW88(REG4F8, len>>8 );
	WriteTW88(REG4F9, len );
}
void I2cSpiFlashDmaReadLenByte(BYTE len_l)
{
	WriteTW88(REG4F9, len_l );
}
#endif


//-----------------------------------------------------------------------------
/**
* assign command and command length
*
*	register
*	R4CA - DMA Command buffer1
*	R4CB - DMA Command buffer2
*	R4CC - DMA Command buffer3
*	R4CD - DMA Command buffer4
*	R4CE - DMA Command buffer5
*	REG4C3[3:0] - Command write byte count

* @see SpiFlashSetCmdLength
*/
void SpiFlashCmd(BYTE cmd, BYTE cmd_len)
{
	WriteTW88(REG4CA, cmd);
	SpiFlashSetCmdLength(cmd_len);
}

//-----------------------------------------------------------------------------
/**
* assign two commands
*
* @see SpiFlashCmd
*/
static void SpiFlashCmd2(BYTE cmd1, BYTE cmd2)
{
	WriteTW88(REG4CA, cmd1);
	WriteTW88(REG4CB, cmd2);
	SpiFlashSetCmdLength(2);
}
//-----------------------------------------------------------------------------
/**
* assign three commands
*
* @see SpiFlashCmd
*/
static void SpiFlashCmd3(BYTE cmd1, BYTE cmd2, BYTE cmd3)
{
	WriteTW88(REG4CA, cmd1);
	WriteTW88(REG4CB, cmd2);
	WriteTW88(REG4CC, cmd3);
	SpiFlashSetCmdLength(3);
}

//-----------------------------------------------------------------------------
/**
* assign a flash address
*/
void SpiFlashDmaFlashAddr(DWORD addr)
{
	WriteTW88(REG4CB, (BYTE)(addr >> 16));
	WriteTW88(REG4CC, (BYTE)(addr >> 8));
	WriteTW88(REG4CD, (BYTE)(addr));
}

//-----------------------------------------------------------------------------
/**
* read SpiFlash
*
* @param dest_type
*	DMA_DEST_FONTRAM,DMA_DEST_CHIPREG,DMA_DEST_SOSD_LUT,DMA_DEST_MCU_XMEM
* @param dest_loc
*	destination location. WORD
* @param src_loc source location
* @param size	size
*/
void SpiFlashDmaRead(BYTE dest_type,WORD dest_loc, DWORD src_loc, WORD size)
{
//#ifdef MODEL_TW8835_EXTI2C
//	if(dest_type==DMA_DEST_MCU_XMEM) {
//		//use 8 registers and copy to XMem
//		SpiFlashDmaReadForXMem(dest_type, dest_loc, src_loc, size);
//		return; 
//	}
//#endif
	WriteTW88Page(PAGE4_SPI);
	SpiFlashDmaDestType(dest_type,0);
	SpiFlashCmd(SPICMD_x_READ, SPICMD_x_BYTES);
	SpiFlashDmaFlashAddr(src_loc);
	SpiFlashDmaBuffAddr(dest_loc);
	SpiFlashDmaReadLen(size);						
	SpiFlashDmaStart(SPIDMA_READ, SPIDMA_BUSYCHECK, __LINE__);
}
void SpiFlashDmaRead2XMem(BYTE *dest_loc, DWORD src_loc, WORD size)
{
#ifdef MODEL_TW8835_EXTI2C
	//use 8 registers and copy to XMem
	SpiFlashDmaReadForXMem(/*DMA_DEST_MCU_XMEM,*/ dest_loc, src_loc, size);
	return; 
#endif
	WriteTW88Page(PAGE4_SPI);
	SpiFlashDmaDestType(DMA_DEST_MCU_XMEM,0);
	SpiFlashCmd(SPICMD_x_READ, SPICMD_x_BYTES);
	SpiFlashDmaFlashAddr(src_loc);
	SpiFlashDmaBuffAddr((WORD)dest_loc);
	SpiFlashDmaReadLen(size);						
	SpiFlashDmaStart(SPIDMA_READ, SPIDMA_BUSYCHECK, __LINE__);
}

//=============================================================================
//
//=============================================================================
//-----------------------------------------------------------------------------
/**
* set SpiFlash ReadMode
*
* updata HW and, SPICMD_x_READ and SPICMD_x_BYTES.
*
* @param mode
*	- 0: slow	CMD:0x03	BYTE:4
*	- 1: fast	CMD:0x0B	BYTE:5
*	- 2: dual	CMD:0x3B	BYTE:5
*	- 3: quad	CMD:0x6B	BYTE:5
*	- 4: Dualo	CMD:0xBB	BYTE:5
*	- 5: QuadIo	CMD:0xEB	BYTE:7
*/
void SPI_SetReadModeByRegister( BYTE mode )
{
	WriteTW88Page(PAGE4_SPI);
	WriteTW88(REG4C0, (ReadTW88(REG4C0) & ~0x07) | mode);

	switch( mode ) {
		case 0:	//--- Slow
			SPICMD_x_READ	= 0x03;	
			SPICMD_x_BYTES	= 4;
			break;
		case 1:	//--- Fast
			SPICMD_x_READ	= 0x0b;	
			SPICMD_x_BYTES	= 5;
			break;
		case 2:	//--- Dual
			SPICMD_x_READ	= 0x3b;
			SPICMD_x_BYTES	= 5;
			break;
		case 3:	//--- Quad
			SPICMD_x_READ	= 0x6b;	
			SPICMD_x_BYTES	= 5;
			break;
		case 4:	//--- Dual-IO
			SPICMD_x_READ	= 0xbb;	
			SPICMD_x_BYTES	= 5;
			break;
		case 5:	//--- Quad-IO
			SPICMD_x_READ	= 0xeb;	 
			SPICMD_x_BYTES	= 7;
			break;
 	}
}


static void SPI_QuadInit_MICRON(void)
{
	BYTE temp;
	BYTE dat0;

	SpiFlashCmd(SPICMD_RDVREG, 1);	//cmd, read Volatile register
	SpiFlashDmaReadLenByte(1);
	SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
	temp = SPIDMA_READDATA(0);
	dPrintf("\nVolatile Register: %02bx", temp );
	if ( temp != 0x6B ) {
		SpiFlashCmd(SPICMD_WREN, 1);
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,0, __LINE__);

		SpiFlashCmd2(SPICMD_WDVREG, 0x6B);		// cmd, write Volatile. set 6 dummy cycles
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,1, __LINE__);
		dPuts("\nVolatile 6 dummy SET" );

		SpiFlashCmd(SPICMD_WRDI, 1);
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,0, __LINE__);
	}
	// set non-Volatile
	SpiFlashCmd(SPICMD_RDNVREG, 1);	//cmd, read Non-Volatile register
	SpiFlashDmaReadLenByte(2);
	SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
	dat0 = SPIDMA_READDATA(0);
	temp = SPIDMA_READDATA(1);
	dPrintf("\nNon-Volatile Register: %02bx, %02bx", dat0, temp );
	if ( temp != 0x6F ) {
		SpiFlashCmd(SPICMD_WREN, 1);
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,0, __LINE__);

		SpiFlashCmd3(SPICMD_WDNVREG, 0xFF,0x6F);	// cmd, write Non-Volatile. B7~B0, B15~B8, set 6 dummy cycles
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,1, __LINE__);
		dPuts("\nnon-Volatile 6 dummy SET" );

		SpiFlashCmd(SPICMD_WRDI, 1);
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,0, __LINE__);
	}
}

//=============================================================================
/**
* Init QuadIO mode
*
* @return
*	0: fail or MX. default
*	1: EON
*/	
BYTE SPI_QUADInit(void)
{
	BYTE dat0;
	//BYTE dat1;
	BYTE vid;
	BYTE cid;
	BYTE ret;
	BYTE temp;
							 
	WriteTW88Page(4);						 
	SpiFlashDmaDestType(DMA_DEST_CHIPREG, 0);
	SpiFlashDmaBuffAddr(DMA_BUFF_REG_ADDR);
	SpiFlashDmaReadLen(0);	//clear high & middle bytes 
	SpiFlashCmd(SPICMD_RDID, 1);	
	SpiFlashDmaReadLenByte(3);
	SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
	vid  = ReadTW88(REG4D0);	//SPIDMA_READDATA(0);
	dat0 = ReadTW88(REG4D1);	//SPIDMA_READDATA(1);
	cid  = ReadTW88(REG4D2);	//SPIDMA_READDATA(2);

	Printf("\n\tSPI JEDEC ID: %02bx %02bx %02bx", vid, dat0, cid );

	if(vid == 0x1C)			ret = SFLASH_VENDOR_EON;
	else if(vid == 0xC2) 	ret = SFLASH_VENDOR_MX; 
	else if(vid == 0xEF)	ret = SFLASH_VENDOR_WB;
	else if(vid == 0x20)	ret = SFLASH_VENDOR_MICRON; //numonyx
	else {
		Printf(" UNKNOWN SPIFLASH !!");
		return 0;
	}

	if(ret==SFLASH_VENDOR_MICRON && cid==0x18) {
		SPI_QuadInit_MICRON();
		return ret;
	}

	//----------------------------
	//read status register
	//----------------------------

	if (vid == 0xC2 || vid == 0x1c) { 							//C2:MX 1C:EON
		SpiFlashCmd(SPICMD_RDSR, 1);
		SpiFlashDmaReadLenByte(1);
		SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
		dat0 = SPIDMA_READDATA(0);
		temp = dat0 & 0x40;	//if 0, need to enable quad
	}
	else if (vid == 0xEF) {					// WB
		//if(cid == 0x18) {				//Q128 case different status read command
			SpiFlashCmd(SPICMD_RDSR2, 1);
			SpiFlashDmaReadLenByte(1);
			SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
			dat0 = SPIDMA_READDATA(0);
			dPrintf("\nStatus2 before QUAD: %02bx", dat0);
			temp = dat0;									  //dat0[1]:QE
		//}
		//else {
		//	SpiFlashCmd(SPICMD_RDSR, 1);
		//	SpiFlashDmaReadLenByte(2);
		//	SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
		//	dat0 = SPIDMA_READDATA(0);
		//	dat1 = SPIDMA_READDATA(1);
		//	dPrintf("\nStatus before QUAD: %02bx, %02bx", dat0, dat1 );	
		//	temp = dat1;
		//}
	}

	if(temp)
		return ret;

	//----------------------------
	// enable quad
	//----------------------------
	Puts("\nEnable quad mode" );
	if (vid == 0xC2 || vid == 0x1c) {
 		SpiFlashCmd(SPICMD_WREN, 1);
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,0, __LINE__);

#ifdef DEBUG_SPIFLASH	
		SpiFlashCmd(SPICMD_RDSR, 1);
		SpiFlashDmaReadLenByte(1);
		SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
		dat0 = SPIDMA_READDATA(0);
		Printf("\nStatus after Write enable %02bx", dat0 );
#endif	
		SpiFlashCmd2(SPICMD_WRSR,0x40);		//en QAUD mode
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,1, __LINE__);	// start + write + busycheck

		Puts("\nQUAD ENABLED" );
	
#ifdef DEBUG_SPIFLASH	
		SpiFlashCmd(SPICMD_RDSR, 1);
		SpiFlashDmaReadLenByte(1);
		SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
		dat0 = SPIDMA_READDATA(0);
		Printf("\nStatus after QUAD enable %02bx", dat0 );
#endif
			
		SpiFlashCmd(SPICMD_WRDI, 1);
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,0, __LINE__);
	}
	else if(vid == 0xEF) {
		SpiFlashCmd(SPICMD_WREN, 1);
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,0, __LINE__);

		SpiFlashCmd3(SPICMD_WRSR, 0x00, 0x02);	//cmd, en QAUD mode
	   	SpiFlashDmaStart(SPIDMA_WRITE,1, __LINE__);		// start 7 busycheck

		dPuts("\nQUAD ENABLED" );
#ifdef DEBUG_SPIFLASH
		//if(cid == 0x18) {				//Q128 case different status read command
			SpiFlashCmd(SPICMD_RDSR2, 1);
			SpiFlashDmaReadLenByte(1);
			SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
			dat0 = SPIDMA_READDATA(0);
			dPrintf("\nStatus2 before QUAD: %02bx", dat0);
		//}
		//else {
		//	SpiFlashCmd(SPICMD_RDSR, 1);
		//	SpiFlashDmaReadLenByte(2);
		//	SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
		//	dat0 = SPIDMA_READDATA(0);
		//	dat1 = SPIDMA_READDATA(1);
		//	dPrintf("\nStatus before QUAD: %02bx, %02bx", dat0, dat1 );	
		//}
#endif
		SpiFlashCmd(SPICMD_WRDI, 1);
		SpiFlashDmaReadLenByte(0);
		SpiFlashDmaStart(SPIDMA_WRITE,0, __LINE__);
	}
	return 0;
}

//====================================================
// FOSD.c for RTL
	#define FOSD_ACCESS_OSDRAM		0	//default
	#define FOSD_ACCESS_FONTRAM		1

void FOsdFontSetFifo(BYTE fOn)
{
	WriteTW88Page(PAGE3_FOSD);
	if(fOn)	WriteTW88(REG300, ReadTW88(REG300) & ~0x02);	//turn off bypass, so FIFO will be ON.
	else	WriteTW88(REG300, ReadTW88(REG300) |  0x02);	//turn on bypass, so FIFO will be OFF.
}
void FOsdSetAccessMode(BYTE fType)
{
	WriteTW88Page(PAGE3_FOSD);

	if(fType)	WriteTW88(REG304, ReadTW88(REG304)| 0x01);	// Font Ram Access
	else		WriteTW88(REG304, ReadTW88(REG304)& 0xFE);	// Osd Ram Access
}
#define	FONTWIN1_ST		REG310
#define	FONTWIN2_ST		REG320
#define	FONTWIN3_ST		REG330
#define	FONTWIN4_ST		REG340
#define	FONTWIN5_ST		REG350
#define	FONTWIN6_ST		REG360
#define	FONTWIN7_ST		REG370
#define	FONTWIN8_ST		REG380

#define	FONTWIN_ENABLE	0X00
#define	FONT_ALPHA		0x01

#ifdef SUPPORT_8BIT_CHIP_ACCESS
code	BYTE	FOSDWinBase[] = { FONTWIN1_ST, FONTWIN2_ST, FONTWIN3_ST, FONTWIN4_ST };
#else
#ifdef MODEL_TW8836
code	WORD	FOSDWinBase[] = { FONTWIN1_ST, FONTWIN2_ST, FONTWIN3_ST, FONTWIN4_ST, FONTWIN5_ST, FONTWIN6_ST, FONTWIN7_ST, FONTWIN8_ST };
#else
code	WORD	FOSDWinBase[] = { FONTWIN1_ST, FONTWIN2_ST, FONTWIN3_ST, FONTWIN4_ST };
#endif
#endif
void FOsdWinAlphaPixel(BYTE winno, BYTE lut, BYTE alpha)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	BYTE	index = FOSDWinBase[winno] + FONT_ALPHA;
#else
	WORD	index = FOSDWinBase[winno] + FONT_ALPHA;
#endif

	WriteTW88Page(PAGE3_FOSD );
	WriteTW88(REG352,  lut );	 			// first, select color index
	WriteTW88(index, alpha );				// second, write alpha value
}
#define FOSD_COLOR_IDX_DBLUE		1
#define FOSD_COLOR_IDX_BLANK		FOSD_COLOR_IDX_DBLUE

void FOsdWinInit(BYTE winno)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	BYTE index;
#else
	WORD index;
#endif

	WriteTW88Page(PAGE3_FOSD );

	index = FOSDWinBase[winno];
	//init all fontosd attributes
	WriteTW88(index, (ReadTW88(index) & 0x7F) | 0x40 );		//FOsdWinEnable(winno, OFF) & Enable Multi-Color

	WriteTW88(index+0x07, 0x00 );
	WriteTW88(index+0x08, 0x00 );
	WriteTW88(index+0x09, 0x00 );
	WriteTW88(index+0x0A, 0x00 );
	WriteTW88(index+0x0B, 0x00 );
	WriteTW88(index+0x0C, 0x00 );
	WriteTW88(index+0x0D, 0x00 );
	WriteTW88(index+0x0E, 0x00 );
	WriteTW88(index+0x0F, 0x00 );

	//clear alpha
	FOsdWinAlphaPixel(winno,FOSD_COLOR_IDX_BLANK,0);
}

#define FOSD_OSDRAM_WRITE_NORMAL	0
	#define FOSD_OSDRAM_WRITE_DATA		3	//Font Data Auto Mode

void FOsdRamSetWriteMode(BYTE fMode)
{
	BYTE value;
	WriteTW88Page(PAGE3_FOSD);
	value = ReadTW88(REG304) & 0xF3;
	value |= (fMode << 2);
	WriteTW88(REG304, value);
}
void FOsdRamSetAddress(WORD addr)
{
	WriteTW88Page(PAGE3_FOSD);
	WriteTW88(REG305, (ReadTW88(REG305) & 0xFE) | (addr >> 8));
	WriteTW88(REG306, (BYTE)addr);
}

void FOsdRamSetAttr(BYTE attr)
{
	WriteTW88Page(PAGE3_FOSD);
	WriteTW88(REG308, attr);
}
void FOsdRamSetData(WORD dat)
{
	WriteTW88Page(PAGE3_FOSD);
	if(dat&0x100)	WriteTW88(REG304,ReadTW88(REG304) |  0x20); 
	else			WriteTW88(REG304,ReadTW88(REG304) & ~0x20);
	WriteTW88(REG307, (BYTE)dat);
}

/**
* set Address and Attribute
*
* we assign the attr and then wirte a start font index.
* after this, you can just assign the font index value. 
* @param OsdRamAddr 0 to 511
* @param attr
*	- attr for 1BPP:	(bgColor << 4) | fgColor
*	- attr for MultiBPP	(LUT index >> 2)
*/
void FOsdRamSetAddrAttr(WORD OsdRamAddr, BYTE attr)
{
	FOsdRamSetWriteMode(FOSD_OSDRAM_WRITE_NORMAL);
	FOsdRamSetAddress(OsdRamAddr); 
	FOsdRamSetAttr(attr);																			

	FOsdRamSetWriteMode(FOSD_OSDRAM_WRITE_DATA);
	FOsdRamSetAddress(OsdRamAddr);	//NOTE: HW needs it.
	//							
	//now use FOsdRamSetData(dat).	
}
void FOsdWinScreenXY(BYTE winno, WORD x, WORD y)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	XDATA	BYTE index;
#else
	XDATA WORD index;
#endif
	XDATA BYTE temp;

	index = FOSDWinBase[winno];

	WriteTW88Page(PAGE3_FOSD );

	temp = x >> 8;
	temp <<= 4;
	temp += ( y >> 8 );
	WriteTW88(index+2,  temp );			// upper bit for position x, y
	WriteTW88(index+3, x );				// position x
	WriteTW88(index+4, y );				// position y

	//FontOsdInfo.win[winno].sx = x;
	//FontOsdInfo.win[winno].sy = y;
}
void FOsdWinScreenWH(BYTE winno, BYTE w, BYTE h)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	XDATA	BYTE index;
#else
	XDATA WORD index;
#endif

	index = FOSDWinBase[winno];

	WriteTW88Page(PAGE3_FOSD );
	WriteTW88(index+5, h );
	WriteTW88(index+6, w );

//	FontOsdInfo.win[winno].column = w;
//	FontOsdInfo.win[winno].line = h;
}
void FOsdWinZoom(BYTE winno, BYTE zoomH, BYTE zoomV)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	XDATA	BYTE index;
#else
	XDATA WORD index;
#endif
	XDATA BYTE temp;

	index = FOSDWinBase[winno];

	WriteTW88Page(PAGE3_FOSD );

	temp = (zoomH << 2) + zoomV;
	temp += (ReadTW88(index ) & 0xf0);
	WriteTW88(index, temp );
}
void FOsdWinMulticolor(BYTE winno, BYTE en)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	XDATA 	BYTE index;
#else
	XDATA 	WORD index;
#endif
	XDATA	BYTE dat;

	index = FOSDWinBase[winno] + FONTWIN_ENABLE;

	WriteTW88Page(PAGE3_FOSD );
	dat = ReadTW88(index);
	if( en ) dat |= 0x40;
	else     dat &= 0xBF;	 
	WriteTW88(index, dat);
}

//code FONT_SPI_INFO_t default_font 		   	= { 0x400000, 0x27F9, 12, 18, 0x100, 0x120, 0x15F, 0x17B, default_LUT_bpp2, default_LUT_bpp3, default_LUT_bpp4 };
void FOsdWinSetOsdRamStartAddr(BYTE winno, WORD addr)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	BYTE index;
#else
	WORD index;
#endif

	WriteTW88Page(PAGE3_FOSD );
	index = FOSDWinBase[winno];

	if(addr >=0x100)	WriteTW88(index+0x07, ReadTW88(index+0x07) |  0x10);
	else 				WriteTW88(index+0x07, ReadTW88(index+0x07) & ~0x10);
	WriteTW88(index+0x0F, (BYTE)addr);

//	FontOsdInfo.win[winno].osdram = addr;
}
void FOsdWinEnable(BYTE winno, BYTE en)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	XDATA 	BYTE index;
#else
	XDATA 	WORD index;
#endif
	XDATA	BYTE dat;

	index = FOSDWinBase[winno] + FONTWIN_ENABLE;

	WriteTW88Page(PAGE3_FOSD);
	dat = ReadTW88(index);
	if( en ) dat |= 0x80;
	else     dat &= 0x7F;
	WriteTW88(index, dat);
}

void DumpFont(void)
{
	WORD start,next,size;
	WORD /*dat,*/ addr;
	//BYTE value;
	BYTE w_cnt;
	WORD Y;

	//WIN0 for 1BPP
	FOsdWinInit(0);
	start = 0;

	next = 0x100;	//FontOsdInfo.font.bpp2;
	size = next - start;
	Printf("\n1BPP start:%d end:%d size:%d",start,next-1,size);
	FOsdRamSetAddrAttr(start, 0x0F);
	delay1ms(1);
	w_cnt = 0;
	for(addr=0; addr < next; addr++) {
		FOsdRamSetData(addr); //same as data
		w_cnt++;
		if(w_cnt==4) {
			w_cnt = 0;
			delay1ms(1);
		}
	}
	FOsdWinScreenXY(0, 0,0);
//	FOsdWinScreenWH(0, (size >=16 ? 16: size), (size >> 4) + (size & 0x0F ? 1 : 0));	Y = ((size >> 4) + 1)* FontOsdInfo.font.h + 10;
	FOsdWinScreenWH(0, (size >=16 ? 16: size), (size >> 4) + (size & 0x0F ? 1 : 0));	Y = ((size >> 4) + 1)* 18 + 10;
	FOsdWinZoom(0, 1/*0*/,0);
	FOsdWinMulticolor(0, ON);
	FOsdWinSetOsdRamStartAddr(0,start);
	FOsdWinEnable(0, ON);

}


void FOsdDownloadFontByDMA(WORD dest_loc, DWORD src_loc, WORD size)
{
	dPrintf("\nFOsdDownloadFontByDMA(%x,%lx,%x)",dest_loc, src_loc, size);

	//save clock mode & select PCLK
	WaitVBlank(1);	
//#ifdef MODEL_TW8835_EXTI2C
//	//BKTODO120323
//#else
	McuSpiClkToPclk(CLKPLL_DIV_2P0);				//with divider 1=1.5(72MHz). try 2
//#endif

	FOsdFontSetFifo(OFF);							//Turn Off FontRam FIFO.
	FOsdSetAccessMode(FOSD_ACCESS_FONTRAM);			//FontRam Access

	SpiFlashDmaRead(DMA_DEST_FONTRAM, dest_loc, src_loc, size);

	WaitVBlank(1);	
	FOsdFontSetFifo(ON);						//Turn On FontRam FIFO. return to default
	FOsdSetAccessMode(FOSD_ACCESS_OSDRAM);		//OsdRam Access Mode. return to default

	//restore clock mode
//#ifdef MODEL_TW8835_EXTI2C
//	//BKTODO120323
//#else
	McuSpiClkRestore();
//#endif
}


