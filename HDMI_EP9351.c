/**
 * @file
 * HDMI_EP9351.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	HDMI EP9351 device driver
*/

#include "Config.h"
#include "typedefs.h"

#include "cpu.h"
#include "printf.h"
#include "util.h"

#include "i2c.h"

#include "main.h"
#include "dtv.h"

//#include "EP9x53Controller.h"
#include "HDMI_EP9351.h"
#include "EP9x53RegDef.h"


#ifndef SUPPORT_HDMI_EP9351
//==========================================
//----------------------------
/**
* Trick for Bank Code Segment
*/
//----------------------------
CODE BYTE DUMMY_HDMI_EP9351_CODE;
void Dummy_HDMI_EP9351_func(void)
{
	BYTE temp;
	temp = DUMMY_HDMI_EP9351_CODE;
}
#else //..SUPPORT_HDMI_EP9351
//==========================================

/**
* EDID data
*/

#ifdef ON_CHIP_EDID_ENABLE
//comes from FW.
code BYTE HDMI_EDID_DATA[] = {
/*        0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F */
/*00*/ 0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x17,0x10,0x01,0x09,0x01,0x00,0x00,0x00,
/*10*/ 0x00,0x0E,0x01,0x03,0x80,0x73,0x41,0x78,0x2A,0x7C,0x11,0x9E,0x59,0x47,0x9B,0x27,
/*20*/ 0x10,0x50,0x54,0x00,0x00,0x00,0x81,0x80,0x90,0x40,0x01,0x01,0x01,0x01,0x01,0x01,
/*30*/ 0x01,0x01,0x01,0x01,0x01,0x01,0x02,0x3A,0x80,0x18,0x71,0x38,0x2D,0x40,0x58,0x2C,
/*40*/ 0x45,0x00,0x10,0x09,0x00,0x00,0x00,0x1E,0x8C,0x0A,0xD0,0x8A,0x20,0xE0,0x2D,0x10,
/*50*/ 0x10,0x3E,0x96,0x00,0x04,0x03,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0xFC,0x00,0x45,
/*60*/ 0x50,0x2D,0x48,0x44,0x4D,0x49,0x2D,0x52,0x58,0x0A,0x20,0x20,0x00,0x00,0x00,0xFD,
/*70*/ 0x00,0x3B,0x3D,0x0F,0x2E,0x08,0x00,0x0A,0x20,0x20,0x20,0x20,0x20,0x20,0x01,0x2E,
/*80*/ 0x02,0x03,0x1D,0x72,0x4A,0x90,0x04,0x05,0x13,0x14,0x01,0x02,0x03,0x11,0x12,0x23,
/*90*/ 0x09,0x07,0x07,0x83,0x01,0x00,0x00,0x65,0x03,0x0C,0x00,0x10,0x00,0x01,0x1D,0x00,
/*A0*/ 0x72,0x51,0xD0,0x1E,0x20,0x6E,0x28,0x55,0x00,0x10,0x09,0x00,0x00,0x00,0x1E,0x01,
/*B0*/ 0x1D,0x80,0x18,0x71,0x1C,0x16,0x20,0x58,0x2C,0x25,0x00,0x10,0x09,0x00,0x00,0x00,
/*C0*/ 0x9E,0x01,0x1D,0x00,0xBC,0x52,0xD0,0x1E,0x20,0xB8,0x28,0x55,0x40,0x10,0x09,0x00,
/*D0*/ 0x00,0x00,0x1E,0x01,0x1D,0x80,0xD0,0x72,0x1C,0x16,0x20,0x10,0x2C,0x25,0x80,0x10,
/*E0*/ 0x09,0x00,0x00,0x00,0x9E,0x8C,0x0A,0xD0,0x90,0x20,0x40,0x31,0x20,0x0C,0x40,0x55,
/*F0*/ 0x00,0x04,0x03,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF6
};
#endif


/*	HDCP Program

	HDCP key Area	$00~$27 : encrypted 40 56-bit HDCP keys
	BKSV key Area   $28     : 40-bit BKSV

	$48,Byte0[2] = 1; //EE_DIS: disable HDCP key downloading from external EE. HDCP keys are written by MCU.
	download HDCP
*/

/**
* HDCP key data
*/

#ifdef ON_CHIP_HDCP_ENABLE
//comes from source. org name was HDCP_KEY[]
//I am not using it.
/* WORKING 120621 */
//code BYTE HDMI_HDCP_KEY[] = {
removed...You need a HDCP Licence.
};
#endif													   		



#ifdef UNCALLED_SEGMENT
void EP9351_Reset(BYTE mode, WORD wait);
void EP9351_Reset_Signal_Valid_Info(void);
void EP9351_Reset_HDMI_Info(void);
void EP9351_Reset_HDMI_Core(void);
void EP9351_AudioMuteEnable(BYTE flag);
void EP9351_VideoMuteEnable(BYTE flag);
void EP9351_DecodeTiming(void);


													   		
/**
* EP9351 Reset
* @param mode  0:PWR_RST,1:SOFT_RST,2:HDCP_RST
*/
void EP9351_Reset(BYTE mode, WORD wait)
{
	BYTE bTemp;
	bTemp = ReadSlowI2CByte(I2CID_EP9351,EP9351_General_Control_0);
	if(mode==2) bTemp |= 0x10;			//HDCP_RST
	else if(mode==1) bTemp |= 0x08;		//SOFT_RST
	else bTemp |= 0x04;					//PWR_RST	
	WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_0, bTemp);
	if(wait)
		delay1ms(wait);
	WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_0, bTemp & ~0x1C);
}

void EP9351_Reset_Signal_Valid_Info(void)
{
	EP9351_VideoMuteEnable(ON);
	EP9351_Reset_HDMI_Info();
}
void EP9351_Reset_HDMI_Info(void)
{
	EP9351_AudioMuteEnable(ON);
	WriteSlowI2CByte(I2CID_EP9351,EP9351_Select_Packet_1, 0x02);	
}
void EP9351_Reset_HDMI_Core(void)
{
	BYTE bTemp;
	bTemp = ReadSlowI2CByte(I2CID_EP9351,EP9351_General_Control_0);
	bTemp |= 0x08;
	WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_0, bTemp);
	Printf("\nEP9351: Soft-Rst");	
}
void EP9351_AudioMuteEnable(BYTE flag)
{
	BYTE bTemp;
	bTemp = ReadSlowI2CByte(I2CID_EP9351,EP9351_General_Control_3);
	if(flag) bTemp |= 0x01;
	else	 bTemp &= ~0x01; 
	WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_3, bTemp);
}
void EP9351_VideoMuteEnable(BYTE flag)
{
	BYTE bTemp;
	bTemp = ReadSlowI2CByte(I2CID_EP9351,EP9351_General_Control_3);
	if(flag) bTemp |= 0x02;
	else	 bTemp &= ~0x02; 
	WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_3, bTemp);
}

void EP9351_DecodeTiming(void)
{
#if 0
	//read timing register & decode
	if(is_HDMI) {
	}
	else { //DVI
		
	}
	Video_Status[0] = TempBit[0];
	//General_Control_2.

	//General_Control_2
	//TempBit07 =
	//TempBit06 =
	//TempBit05 =
	//TempBit04 =
	TempBit03
	TempBit02
	TempBit01  (AVI_Infomatopn[5] & 0x0F > 3)
	TempBit00
 #endif
}
#endif //..UNCALLED_SEGMENT





/**
* download EDID data from FW to EP9351 RAM
*
* On the develop
* @param addr. if 0, use HDMI_EDID_DATA[] on the code segment.
*/
#ifdef ON_CHIP_EDID_ENABLE
BYTE HdmiDownloadEdid(DWORD addr)
{
	BYTE bTemp;

	//--------------------
	//load EDID to chip
	//--------------------
	Puts("\nDownload EDID");

	bTemp = ReadSlowI2CByte(I2CID_EP9351, EP9351_General_Control_0);

	//Enable DDC_DIS & Disable ON chip EDID
	bTemp |= EP9351_General_Control_0__DDC_DIS;
	bTemp &= ~EP9351_General_Control_0__ON_CHIP_EDID;
	WriteSlowI2CByte(I2CID_EP9351, EP9351_General_Control_0,  bTemp );

	//Write EDID 256 Bytes
	if(addr) {
		Printf(" addr:%lx GIVEUP", addr);
		//BKTODO: add code here.
		//		 Read 256 EDID data from Flash. It needs 256 buffer that FW does not have.
	}
	else {
		WriteSlowI2C(I2CID_EP9351, EP9351_EDID_Data_Register, HDMI_EDID_DATA, 256);
	}
	delay1ms(70);	//70ms work. 65ms:fail with real EDID eeprom

	//Disable DDC_DIS &  Enable on chip EDID
	bTemp &= ~EP9351_General_Control_0__DDC_DIS;
	bTemp |= EP9351_General_Control_0__ON_CHIP_EDID;
	WriteI2CByte(I2CID_EP9351, EP9351_General_Control_0,  bTemp );

	Printf(" CheckSum:%02bx", ReadI2CByte(I2CID_EP9351, EP9351_EE_Checksum));
	//BKTODO:check CRC
	return ERR_SUCCESS;
}
#endif
/**
* download HDCP key from FW to EP9351 RAM
*
* On the develop
*/
#ifdef ON_CHIP_HDCP_ENABLE
BYTE HdmiDownloadDhcp(DWORD addr)
{
	BYTE bTemp;
	BYTE i;
	BYTE TempByte[6];

	//----------------------
	//load HDCP to chip
	//----------------------
	Puts("\nDownload HDCP");

	//Enable EE_DIS for HDCP keys are written by MCU
	bTemp = ReadSlowI2CByte(I2CID_EP9351, EP9351_General_Control_8);
	bTemp |= EP9351_General_Control_8__EE_DIS;
	WriteSlowI2CByte(I2CID_EP9351, EP9351_General_Control_8, bTemp );

	//Write HDCP
	if(addr) {
		Printf(" addr:%lx GIVEUP", addr);
		//BKTODO: add code here.
		//		 Read HDCP data from Flash. It needs 40*7 + 5 buffer that FW does not have.
	}
	else {
		for(i=0; i < 40; i++) {
			WriteSlowI2C(I2CID_EP9351, i, &HDMI_HDCP_KEY[i*8], 7);
		}
		//swap BKSV and Write BKSV 
		for(TempByte[0] = 0; TempByte[0] < 5; TempByte[0]++) {
			TempByte[TempByte[0] + 1] = HDMI_HDCP_KEY[40*8+TempByte[0]];
		}
		TempByte[0] = HDMI_HDCP_KEY[40*8+4];
		WriteSlowI2C(I2CID_EP9351, EP9351_HDMI_HDCP_BKSV, TempByte, 5);
#if 1 //debug
		Printf("\tBKSV $$%bx:",0x28);
		for(i=0; i < 5; i++)
			Printf("%02bx ",TempByte[i]);
#endif
	}
	return ERR_SUCCESS;
}

#endif //..ON_CHIP_HDCP_ENABLE


/**
* init EP9351 HW.
*
* call from InitSystem.
* follow up the power up sequency. (curr, ignore)
* download EDID and HDCP.
* result:
*	PowerDown state & Mute state
*
* BugFix120802. Increase TempByte[6] to TempByte[20]
*/
void Hdmi_SystemInit_EP9351(void)
{
	BYTE bTemp;
	BYTE TempByte[20];

#ifdef ON_CHIP_EDID_ENABLE
	//disconnect DDC
	// HTPLG_R high. Eat 5V.
	WriteSlowI2CByte( I2CID_SX1504, 1, 0 );		// output enable
	WriteSlowI2CByte( I2CID_SX1504, 0, ReadSlowI2CByte(I2CID_SX1504, 0) | 0x04 );		
#endif

	//Reset EP9351 registers

	//assign the Pin Reset Value. We can skip it.
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_HDMI_INT,          0x00);
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_0, 0x00);
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_1, 0x00);
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_2, 0x00);
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_3, 0x00);
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_4, 0x02);
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_5, 0x00);
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_6, 0xE4);
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_8, 0x00);
	//WriteSlowI2CByte(I2CID_EP9351,EP9351_General_Control_9, 0x00);

#if 0
	//SW RESET
	//BKFYI: This function will executes the PWR_RST that includes the SOFT_RST & HDCP_RST.
	//       So, FW don't need the SOFT_RST.
	EP9351_Reset(1, 500);
#endif

	//----------------------------------
	//EP9351_Reset_Signal_Valid_Info();
	//----------------------------------

	// Mute Video and Audio
	bTemp = ReadSlowI2CByte(I2CID_EP9351, EP9351_General_Control_3);
	bTemp |= 0x02;	//VideoMuteEnable
	bTemp |= 0x01;	//AudioMuteEnable
	WriteSlowI2CByte(I2CID_EP9351, EP9351_General_Control_3, bTemp);
	Printf("\nEP9351: VMute&AMute Enabled");

	//selected Packet1. 
	WriteSlowI2CByte(I2CID_EP9351, EP9351_Select_Packet_1, 0x02/*PACKET_STD_Audio*/);

	//------------------------------------
	//clear HDMI Info Frame (AVI and ADO)
	//------------------------------------

	// Set Interrupt Enables
	bTemp = EP9351_HDMI_INT__AVI | 
			EP9351_HDMI_INT__ADO | 
			//EP9351_HDMI_INT__MS | 
			EP9351_HDMI_INT__SEL1 | 
			EP9351_HDMI_INT__SEL2 | 
			EP9351_HDMI_INT__AVMS | 
			EP9351_HDMI_INT__AVMC;
	WriteSlowI2CByte(I2CID_EP9351,EP9351_HDMI_INT,bTemp);

	// Set ACP InfoFrame
	WriteSlowI2CByte(I2CID_EP9351,EP9351_Select_Packet_2,0x04 /*PACKET_ACP */);

#ifdef ON_CHIP_EDID_ENABLE
	HdmiDownloadEdid(0);
#endif

	//----------------------
	// Power Down EP9351.
	Puts("\nEP9351 PD");
	bTemp = ReadI2CByte(I2CID_EP9351, EP9351_General_Control_0);
	bTemp |= EP9351_General_Control_0__PWR_DWN;
	WriteI2CByte(I2CID_EP9351, EP9351_General_Control_0,  bTemp );

#ifdef SUPPORT_HDMI_EP9553
	TempByte[0] = 0x00;
	TempByte[1] = 0x00;
	TempByte[2] = 0x00;
	WriteSlowI2C(I2CID_EP9351, EP9x53_RX_PHY_Control_Register, TempByte, 3);   //$4C
#endif


#ifdef ON_CHIP_HDCP_ENABLE
	HdmiDownloadDhcp(0);	
#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// BSTATUS
#ifdef SUPPORT_HDMI_EP9553
	TempByte[0] = 0x91;
#else
	TempByte[0] = 0x93;
#endif
	TempByte[1] = 0x00;
	TempByte[2] = 0x00;
	WriteSlowI2C(I2CID_EP9351, EP9351_BSTATUS, TempByte, 3);

	// Set V1 Register	
	TWmemset(TempByte, 0, 20);
	WriteSlowI2C(I2CID_EP9351, EP9351_V1_Register, TempByte, 20);

	// Set KSV 0-3
	TWmemset(TempByte, 0, 5);
	WriteSlowI2C(I2CID_EP9351, EP9351_KSV0, TempByte, 5);
	WriteSlowI2C(I2CID_EP9351, EP9351_KSV1, TempByte, 5);
	WriteSlowI2C(I2CID_EP9351, EP9351_KSV2, TempByte, 5);
	WriteSlowI2C(I2CID_EP9351, EP9351_KSV3, TempByte, 5);

#ifdef ON_CHIP_EDID_ENABLE
	//connect DDC
	// HTPLG_R low. Release 5V.
	WriteSlowI2CByte( I2CID_SX1504, 1, 0 );		// output enable
	WriteSlowI2CByte( I2CID_SX1504, 0, ReadSlowI2CByte(I2CID_SX1504, 0) & ~0x04 );		
#endif
	//------------------------------
	// EP9351 status
	//	Power Down
	//	Video & Audio Mute
	//
	// call HdmiInitEp9351Chip(); to power up and unmute.
	//------------------------------
}

/**
* init EP9351 CHIP
* Power up, unmute and assign some default values.
*/
void HdmiInitEp9351Chip(void)
{
	BYTE bTemp;
	//BYTE i;
	//volatile BYTE r3C,r3D;

	//powerup.
	//EP9351 needs some delay to get the status register 0,1 and AVI Info frame.
	Puts("\nEP9351 PU");
	bTemp = ReadI2CByte(I2CID_EP9351, EP9351_General_Control_0);
	bTemp &= ~EP9351_General_Control_0__PWR_DWN;
	WriteI2CByte(I2CID_EP9351, EP9351_General_Control_0,  bTemp );

	//WriteI2CByte(I2CID_EP9351, EP9351_General_Control_1
	//WriteI2CByte(I2CID_EP9351, EP9351_General_Control_2

	//-----------------------------------
	//BKFYI:You can unmute when EP9351 has a proper signal.
	//      But, curr FW, turns off MUTE here.
	bTemp = ReadI2CByte(I2CID_EP9351, EP9351_General_Control_3);
	bTemp &= ~0x03;
	WriteI2CByte(I2CID_EP9351, EP9351_General_Control_3,bTemp);
	Printf("\nEP9351: VMute/AMute Disable");

	//WriteI2CByte(I2CID_EP9351, EP9351_General_Control_4
	//WriteI2CByte(I2CID_EP9351, EP9351_General_Control_5
	//WriteI2CByte(I2CID_EP9351, EP9351_General_Control_6
	//WriteI2CByte(I2CID_EP9351, EP9351_General_Control_7

	bTemp = 0x08;
#ifdef ON_CHIP_HDCP_ENABLE		
	bTemp |= EP9351_General_Control_8__EE_DIS;
#endif
	WriteI2CByte(I2CID_EP9351, EP9351_General_Control_8,bTemp);
	WriteI2CByte(I2CID_EP9351, EP9351_General_Control_9,0xB1);


#if 0
	//here is a good check point to know how much time needs to get the correct $3C,$3D status register
	for(i=0; i < 60; i++) {
		r3C = ReadI2CByte(I2CID_EP9351, EP9351_Status_Register_0);
		r3D = ReadI2CByte(I2CID_EP9351, EP9351_Status_Register_1);
		if(r3D & 0xC0) {
			Printf("\nstatus $3C:%bx $3D:%bx @%bd(x20)ms",r3C,r3D, i);
			break;
		}
		delay1ms(20);
	}
	if(i==60) {
		Printf("\nstatus $3C:%02bx $3D:%02bx FAIL",r3C,r3D);	
	}
#endif
}

#endif //..ifdef SUPPORT_HDMI_EP9351

