//*****************************************************************************
//
//									CPU.c
//
//  Copyright(C) 2011-2012 Intersil Corporation
//*****************************************************************************
//
//
#include "config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "printf.h"
#include "I2C.h"
#include "CPU.h"
#include "Remo.h"
#include "TouchKey.h"


//===== Timer =======
DATA 	BYTE   	tic01=0;				//unit 1msec
DATA 	WORD   	tic_pc=0;
DATA 	WORD   	tic_task=0;
DATA 	DWORD	SystemClock=0L;			//unit 10msec
DATA	DWORD	OsdTimerClock=0L;

DATA 	WORD   	RemoTic=0;
#if defined(SUPPORT_I2CCMD_TEST_SLAVE)
DATA	WORD	ext_i2c_timer;
#endif

//===== WatchDog ======
#ifdef DEBUG_WATCHDOG
		bit		F_watch=0;
#endif
//		bit		F_WatchOn=0;

//===== Serial0 ======
XDATA 	BYTE	RS_buf[RS_BUF_MAX];
DATA 	BYTE	RS_in, RS_out;
		bit		RS_Xbusy=0;
		bit		RS0_Xbusy=0;
//===== Serial1 ======
#ifdef SUPPORT_UART1
XDATA 	BYTE	RS1_buf[RS_BUF_MAX];
DATA 	BYTE	RS1_in, RS1_out;
		bit		RS1_Xbusy=0;
#endif

//===== Remo ========
#define REMO_IN	P1_2

#ifdef REMO_RC5
	bit         RemoPhase1, RemoPhase2;
	IDATA BYTE	RemoDataReady=0;
	IDATA BYTE  RemoSystemCode, RemoDataCode;
	IDATA DWORD RemoReceivedTime;
#ifdef DEBUG_REMO
	IDATA BYTE  RemoSystemCode0, RemoDataCode0;
	IDATA BYTE  RemoSystemCode1, RemoDataCode1;
	IDATA BYTE  RemoSystemCode2, RemoDataCode2;
	IDATA BYTE  RemoCaptureDisable=0;
#endif
#elif defined REMO_NEC
	bit			RemoPhase=0;
	DATA BYTE	RemoStep=0;
	DATA BYTE	RemoHcnt, RemoLcnt;
	DATA BYTE	RemoData[4];
    IDATA BYTE  RemoDataReady=0;
	IDATA BYTE  RemoNum, RemoBit;
#ifdef DEBUG_REMO_NEC
	DATA BYTE	DebugRemoStep;
	DATA BYTE	DebugRemoHcnt;
	DATA BYTE   DebugRemoLcnt;
	DATA BYTE   DebugRemoNum;
	DATA BYTE   DebugRemoBit;
	DATA BYTE   DebugRemoPhase;
	DATA BYTE   DebugRemoDataReady;
#endif

#endif // REMO 
		bit		RM_get = 0;

//===== memory register ========

volatile BYTE	XDATA *DATA regTW88 = REG_START_ADDRESS;
//WORD XDATA reg_p;

//===== interrupt status ========

DATA	BYTE	INT_STATUS=0;
DATA	BYTE	INT_STATUS2=0;
DATA	BYTE	INT_STATUS_ACC=0;
DATA	BYTE	INT_STATUS2_ACC=0;
DATA	BYTE	EXINT_STATUS = 0;
DATA	WORD	MCU_INT_STATUS=0;
DATA	WORD	VH_Loss_Changed=0;




//==========================
// PROTOTYPE for Internal 
//==========================
void InitRemoTimer(void);



//*****************************************************************************
//*      Ext Int 0 Interrupt (Low / Falling) 	(Internal)	: <<Chip Status>>	P2.0
//*****************************************************************************
void	ext0_int(void) interrupt 0 using 1
{
	DECLARE_LOCAL_page

SFR_EX0=0;
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0001;
#endif


	ReadTW88Page(page);
	WriteTW88Page(0);

	INT_STATUS = ReadTW88(REG002);
	WriteTW88(REG002, INT_STATUS & 0xFF);		//clear	
	INT_STATUS2 = ReadTW88(REG004);
	WriteTW88(REG004, INT_STATUS2 & 0x07);		//clear

	if(INT_STATUS & 0x02)						//keep 0x02.
		VH_Loss_Changed++;

	INT_STATUS_ACC |= INT_STATUS;
	INT_STATUS2_ACC |= INT_STATUS2;

	WriteTW88Page(page);

SFR_EX0=1;
}




//*****************************************************************************
//*      Timer 0 Interrupt 									: <<System Timer>>                                               
//*****************************************************************************
//	1 msec timer tic
//  use 10us @27Mhz

#ifdef SW_I2C_SLAVE
DATA BYTE sw_i2c_index_old;
#endif

void timer0_int(void) interrupt 1 using 1
{

//PORT_DEBUG = 0;
SFR_ET0=0;
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0002;
#endif
	tic01++;

	tic01 %= 10;		
	if ( tic01 == 0 ) {
		SystemClock++;
		if(OsdTimerClock)
			OsdTimerClock--;
#if defined(SUPPORT_I2CCMD_TEST_SLAVE)
		if(ext_i2c_timer)
			ext_i2c_timer--;
#endif

	}
	tic_pc++;		//WORD size
	tic_task++;		//WORD size

SFR_ET0=1;
//PORT_DEBUG = 1;
}
//*****************************************************************************
//*      Ext Int 1 Interrupt  (Low / Falling)				: <<DE End>>		P2.1
//*****************************************************************************
BYTE ext1_intr_flag;
void ext1_int(void) interrupt 2 using 1
{
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0004;
#endif
	SFR_EX1 = 0;
}

//****************************************************************************
//*      Timer 1 Interrupt : for Touch sensing & key sensing                                           
//****************************************************************************
//extern 	WORD	TouchX, TouchY, Z1;
WORD	CpuTouchX, CpuTouchY, CpuZ1, CpuZ2;
bit		CpuTouchPressed=0;
WORD	CpuAUX0 = 0;
WORD	CpuAUX1 = 0;
WORD	CpuAUX2 = 0;
WORD	CpuAUX3 = 0;
BYTE	CpuAUX0_Changed = 0;
BYTE	CpuAUX1_Changed = 0;
BYTE	CpuAUX2_Changed = 0;
BYTE	CpuAUX3_Changed = 0;
BYTE	CpuTouchStep=0, CpuTouchChanged=0;

WORD 	CpuTouchSkipCount=0;
#define ReadTscData(TscData) TscData = ReadTW88(REG0B2); TscData <<= 4; TscData += ReadTW88(REG0B3) 
//every 500usec. use 82uS @ 27MHz
void timer1_int(void) interrupt 3 using 1
{
#if defined(SUPPORT_TOUCH) || defined(SUPPORT_KEYPAD)
	DECLARE_LOCAL_page
	WORD diff;
	static WORD TX,TY;
	WORD TscData;
	static WORD	temp;

SFR_ET1=0;

 	if(CpuTouchSkipCount) {
		CpuTouchSkipCount--;
		SFR_ET1=1;
		return;
	}

	ReadTW88Page(page);
	WriteTW88Page(PAGE0_TOUCH);

	if ( CpuTouchStep == 0 ) {					// start Measure Z2
		//TSC MODE : 2 : Z2 measure
		ReadTscData(TscData);
		if ( TscData > 0x800 ) {					
			WriteTW88(REG0B0, 0x01 );		// adc  mode Z1
			CpuTouchStep++;					// 1
			temp = 0;						// clear
			CpuZ2 = TscData;							
		}
		else {
			CpuTouchStep = 5;
		}
	}
	else if ( CpuTouchStep == 1 ) {			 // check Z1
		//TSC MODE : 1 : Z1 measure
		ReadTscData(TscData);
		if ( TscData > temp )	diff = TscData - temp;
		else					diff = temp - TscData;

		if ( diff < 10 ) {					// find stable value
			if ( TscData < 100 ) {			// no touch
				CpuTouchPressed = 0;
				CpuTouchStep = 5;
				CpuTouchChanged++;
			}
			else {
				WriteTW88(REG0B0, 0 );		// adc start with mode XPOS
				CpuTouchStep++;				// restart Touch measurement
				temp = 0;					// clear
			}
		}
		else {
			temp = TscData;					// redo measure
			//WriteTW88(REG0B0, 0x01 );		// adc  mode Z1
		}
	}
	else if ( CpuTouchStep == 2 ) {			 // check XPOS
		//TSC MODE : 0 : X position measure
		ReadTscData(TscData);

		if ( TscData > temp )	diff = TscData - temp;
		else					diff = temp - TscData;
			
		if ( diff < 10 ) {					// find stable value
			WriteTW88(REG0B0, 0x03 );		// adc start with mode YPOS
			CpuTouchStep++;
			temp = 0;						// clear
			TX = TscData;
		}
		else {
			temp = TscData;					// redo measure
			//WriteTW88(REG0B0, 0 );			// adc start with mode XPOS
		}
	}
	else if ( CpuTouchStep == 3 ) {	 		// check YPOS
		//TSC MODE : 3 : Y position measure
		ReadTscData(TscData);

		if ( TscData > temp )	diff = TscData - temp;
		else					diff = temp - TscData;

		if ( diff < 10 ) {					// find stable value
			WriteTW88(REG0B0, 0x01 );		// adc start with mode Z1
			temp = 0;						// clear
			CpuTouchStep++;
			TY = TscData;
		}
		else {
			temp = TscData;					// redo measure
			//WriteTW88(REG0B0, 0x03 );		// adc start with mode YPOS
		}
	}
	else if ( CpuTouchStep == 4 ) {  			// check z1
		//TSC MODE : 1 : Z1 measure
		ReadTscData(TscData);

		if ( TscData > temp )	diff = TscData - temp;
		else				    diff = temp - TscData;

		if ( diff < 10 ) {					// find stable value
			if ( TscData < 100 ) {			// no touch, reset touch interrupt
				CpuTouchPressed = 0;
				CpuTouchChanged++;
			}
			else {
				//===================
				//
				//===================
				CpuTouchX = TX;
				CpuTouchY = TY;
				CpuZ1 = TscData;
				CpuTouchChanged++;
				CpuTouchPressed = 1;
			}
			CpuTouchStep++;
		}
		else {
			temp = TscData;					// redo measure
			//WriteTW88(REG0B0, 0x01 );		// adc start with mode Z
		}
	}
	else if ( CpuTouchStep == 5 ) {  			// Start AUX input check
		WriteTW88(REG0B0, 0x07 );			// write Start, erase Ready, mode AUX0
		CpuTouchStep++;
		temp = 0;
	}
	else if ( CpuTouchStep == 6 ) { 
		//TSC MODE : 7 : AUX3 measure
		ReadTscData(TscData);

		if ( TscData > temp )	diff = TscData - temp;
		else					diff = temp - TscData;
			
		if ( diff < 10 ) {					// find stable value
			CpuAUX3 = TscData;
			CpuAUX3_Changed++;
			WriteTW88(REG0B0, 2 );			// write Start, erase Ready, mode Z2
			temp = 0;
			CpuTouchStep = 0; 						
		}
		else {
			temp = TscData;					// redo measure
			//WriteTW88(REG0B0, 7 );			// write Start, erase Ready, mode AUX[3]
		}

	}
	WriteTW88Page(page );
//PORT_DEBUG = 1;

SFR_ET1=1;

#endif //..SUPPORT_TOUCH
}
#undef ReadTscData


//*****************************************************************************
//      UART 0 Interrupt                                                   
//*****************************************************************************
void uart0_int(void) interrupt 4 using 1
{
#ifdef DP80390
	BYTE	count;
#endif
SFR_ES=0;	//SFR_EA = 0;
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0010;
#endif
	if( SFR_RI ) {					//--- Receive interrupt ----
		SFR_RI = 0;
#ifdef DP80390
		if ( SFR_UART0FIFO & 0x80 ) {			// use fifo?
			count = SFR_UART0FIFO & 0x1F;
			if ( count & 0x10) {
				SFR_UART0FIFO = 0x90;		// overflowed, buffer clear
			}
			else {
				while (count) {
					RS_buf[RS_in++] = SFR_SBUF;
					if( RS_in>=RS_BUF_MAX ) RS_in = 0;
					count--;
				};
			}
		}
		else 
#endif
		{
			RS_buf[RS_in++] = SFR_SBUF;
			if( RS_in>=RS_BUF_MAX ) RS_in = 0;
		}
	}

	if( SFR_TI ) {					//--- Transmit interrupt ----
		SFR_TI = 0;
		RS_Xbusy=0;
	}
SFR_ES=1;	//SFR_EA = 1;
}
//****************************************************************************
//*      Timer 2 Interrupt 									: <<Remo Timer>>
//****************************************************************************
#ifdef REMO_RC5
BYTE RemoCapture0[14+1];
BYTE RemoCapture1[14+1];
BYTE RemoCapture2[14+1];
void timer2_int(void) interrupt 5 using 1			// using register block 3
{
	BYTE	i;
	bit sample;

SFR_ET2=0;	//SFR_EA = 0;
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0020;
#endif
	SFR_T2IF &= 0xfc;				// Clear Interrupt Flag

	RemoTic++;
	sample = REMO_IN;

#ifdef DEBUG_REMO
	if(RemoTic==5) {
		if(RemoCaptureDisable==0)
			RemoCapture0[0] = 0x0F; //valid 0..4	
		if(RemoCaptureDisable==1)
			RemoCapture1[0] = 0x0F;	
		if(RemoCaptureDisable==2)
			RemoCapture2[0] = 0x0F;	
	}

	i = (RemoTic & 0x07);
	if(RemoTic <= 8*14) {
		if(sample) {
			if(RemoCaptureDisable==0)
				RemoCapture0[RemoTic >> 3] |= (1 << i);	
			if(RemoCaptureDisable==1)
				RemoCapture1[RemoTic >> 3] |= (1 << i);	
			if(RemoCaptureDisable==2)
				RemoCapture2[RemoTic >> 3] |= (1 << i);	
		}
	}
#endif


	i = RemoTic & 0x07;
	if     ( i==1 ) RemoPhase1 = sample; //REMO_IN;
	else if( i==5 )	RemoPhase2 = sample; //REMO_IN;

	//----- Received 1 Bit -----
	else if( i==0 ) {	//every 8 RemoTic
		if( RemoPhase1==RemoPhase2 ) {	// error
			ClearRemoTimer();			
			EnableRemoInt();

			SFR_ET2=1;	//SFR_EA = 1;

			return;
		}
		if( RemoTic<=(8*8) ) {				// SystemCode. Start1+Start2 + Tottle + 5 BIT ADDRESS
			RemoSystemCode <<=1;
			if( RemoPhase1==1 && RemoPhase2==0 )
				RemoSystemCode |=1;
		}
		else {								// DataCode.  6 BIT COMMAND
			RemoDataCode <<=1;
			if( RemoPhase1==1 && RemoPhase2==0 )
				RemoDataCode |=1;
		}
		//----- Received 1 Packet -----
		if( RemoTic >= (8*14) ) {
			RemoDataReady++;	// RemoDataReady = 1;				// new key
#ifdef DEBUG_REMO
			if(RemoCaptureDisable==0) {  RemoSystemCode0 = RemoSystemCode; RemoDataCode0 = RemoDataCode; }
			if(RemoCaptureDisable==1) {  RemoSystemCode1 = RemoSystemCode; RemoDataCode1 = RemoDataCode; }
			if(RemoCaptureDisable==2) {  RemoSystemCode2 = RemoSystemCode; RemoDataCode2 = RemoDataCode; }
			RemoCaptureDisable++;
#endif
			RemoReceivedTime = SystemClock;

			//if(RemoSystemCode&0x40 == RemoSystemCode&0x02)

			ClearRemoTimer();				
		}
	}
SFR_ET2=1;	//SFR_EA = 1;
}
#endif //..REMO_RC5
#ifdef REMO_NEC
void timer2_int(void) interrupt 5 using 1			// using register block 3
{
EA = 0;

	T2IF &= 0xfc;				// Clear Interrupt Flag

	RemoTic++;

	if( RemoDataReady ) {
		
		EA = 1;

		return;
	}

	switch( RemoStep ) {

	case 0:
		//wait 9ms.
		if( REMO_IN==0 ) {
			RemoLcnt++;
			if( RemoLcnt==0xff ) //wait 42.38...samples
				goto RemoError;
		}
		else {
			RemoHcnt = 0;
			RemoStep++;
		}
		break;

	case 1:
		//wait 4.5ms for normal........ wait 24.106 samples
		//wait 2.25ms for repeat code.. wait 12.053 samples
		if( REMO_IN==1 ) {
			RemoHcnt++;
			if( RemoHcnt==0xff )  //
				goto RemoError;
		}
		else {
			if( RemoLcnt>=15*3 && RemoLcnt<=17*3 ) {	//target 16*3 =	48
				
				if( RemoHcnt>=3*3 && RemoHcnt<=5*3 ) {	//target 4*3 = 12
					RemoStep = 3;
					#ifdef DEBUG_REMO_NEC
					if(DebugRemoStep==0) {
						DebugRemoStep = RemoStep;
						DebugRemoLcnt = RemoLcnt;
						DebugRemoHcnt = RemoHcnt;
					}
					#endif
					RemoDataReady = 2;					 //auto repeat..
					break;
				}
				else if( RemoHcnt>=7*3 && RemoHcnt<=9*3 ) {	//target 8*3 = 24
					RemoStep++;								//move to RemoStep 2.
					#ifdef DEBUG_REMO_NEC
					if(DebugRemoStep==0) {
						DebugRemoStep = RemoStep;
						DebugRemoLcnt = RemoLcnt;
						DebugRemoHcnt = RemoHcnt;
					}
					#endif
					RemoPhase = 0;
					RemoLcnt = 0;
					RemoNum  = 0;
					RemoBit  = 0;

					break;
				}
			}
			else goto RemoError;
		}
		break;

	case 2:
		if( RemoPhase==0 ) {
			if( REMO_IN==0 )					// Phase=0  Input=0
				RemoLcnt++;
			else {								// Phase=0  Input=1
				RemoPhase = 1;
				RemoHcnt = 0;
			}
		}
		else {								
			if( REMO_IN==1 )					// Phase=1  Input=1
				RemoHcnt++;
			else {								// Phase=1  Input=0
				RemoPhase = 0;
				if( RemoLcnt>=1 && RemoLcnt<=5 ) {
					if( RemoHcnt<=2*3 ) 			// bit 0
						RemoData[RemoNum] <<= 1;
					else if( RemoHcnt<=4*3 ) {		// bit 1
						RemoData[RemoNum] <<= 1;
						RemoData[RemoNum]++;
					}
					else goto RemoError;

					if( ++RemoBit>=8 ) {
						RemoBit = 0;
						if( ++RemoNum>=4 ) {
							RemoDataReady = 1;
							RemoStep++;
						}
					}
					RemoLcnt = 0;

				}
				else goto RemoError;
			}
		}
		break;

	case 3:
		break;
	
	}
	EA = 1;
	return;

RemoError:
	ClearRemoTimer();		//TimerFor208us();
	EnableRemoInt();		//EnableRemoconInt();

	EA = 1;
}
#endif	//..REMO_NEC

//*****************************************************************************
//      UART 1 Interrupt                                                   
//*****************************************************************************
#ifdef SUPPORT_UART1
void uart1_int(void) interrupt 6 using 1
{
#ifdef MODEL_TW8836
	BYTE	count;
#endif
EA = 0;
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0040;
#endif
	if( RI1 ) {					//--- Receive interrupt ----
		RI1 = 0;
#ifdef DP80390
		if ( SFR_UART1FIFO & 0x80 ) {			// use fifo?
			count = SFR_UART1FIFO & 0x1F;
			if ( count & 0x10) {
				SFR_UART1FIFO = 0x90;		// overflowed, buffer clear
			}
			else {
				while (count) {
					RS1_buf[RS1_in++] = SFR_SBUF1;
					if( RS1_in>=RS_BUF_MAX ) RS1_in = 0;
					count--;
				};
			}
		}
		else
#endif
		{
			RS1_buf[RS1_in++] = SFR_SBUF1;
			if( RS1_in >= RS_BUF_MAX ) RS1_in = 0;
		}
	}

	if( TI1 ) {					//--- Transmit interrupt ----
		TI1 = 0;
		RS1_Xbusy=0;
	}
EA = 1;
}
#endif
//*****************************************************************************
//*      Ext Int 2 Interrupt  (Low)			  				: <<DMA Done>>		P2.2	EXIF0
//*****************************************************************************
void ext2_int(void) interrupt 7 using 1
{
	DECLARE_LOCAL_page
	BYTE val;

SFR_EX1=0; //	SFR_EA = 0;
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0080;
#endif

	ReadTW88Page(page);
	WriteTW88Page(PAGE0_GENERAL);
	val = ReadTW88(REG002);
	WriteTW88(REG002, 0x80);	// Clear Int		

	WriteTW88Page(page);
SFR_EX1=1;	//	SFR_EA = 1;
}
//*****************************************************************************
//*      Ext Int 3 Interrupt (Low)							: <<Touch Ready>>	P2.3	EXIF1
//*****************************************************************************
void ext3_int(void) interrupt 8 using 1
{
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0100;
#endif
}
//*****************************************************************************
//*      Ext Int 4 Interrupt (Low)							: <<Pen Int>>		P2.4	EXIF2
//*****************************************************************************
void ext4_int(void) interrupt 9 using 1
{
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0200;
#endif
}
//*****************************************************************************
//*      Ext Int 5 Interrupt (Falling)						: reserved
//*****************************************************************************
void ext5_int(void) interrupt 10 using 1
{
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0400;
#endif
}
//*****************************************************************************
//*      Ext Int 6 Interrupt (Falling)						: reserved
//*****************************************************************************
void ext6_int(void) interrupt 11 using 1
{
#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x0800;
#endif
}
//*****************************************************************************
//*      Watchdog Interrupt									: <<Watchdog>>
//*****************************************************************************
/*
See the OK and NG code. If you use OPTIMIZE(9,), the compiler made a NG code.
OK:
	MOV     SFR_TA,#0AAH
	MOV     SFR_TA,#055H
	MOV     SFR_WDCON,#03H
	SETB    SFR_EWDI

NG:
	MOV     SFR_TA,#0AAH
	MOV     SFR_TA,#055H
	CLR     A
	MOV     SFR_WDCON,A
	CLR     SFR_EWDI
*/
#pragma SAVE
#pragma OPTIMIZE(8,SPEED)
//only for debug.
void watchdog_int(void) interrupt 12 using 0
{
	SFR_EWDI = 0;

#ifdef DEBUG_ISR
	MCU_INT_STATUS |= 0x1000;
#endif

#ifdef DEBUG_WATCHDOG
	F_watch = 1;
#endif

	SFR_TA = 0xaa;
	SFR_TA = 0x55;
	SFR_WDCON = 0x03;	// - - - - WDIF WTRF EWT RWT

	SFR_EWDI = 1;
}
#pragma RESTORE

DWORD IntCount = 0;

//*****************************************************************************
//*      Ext Int 7 Interrupt (Programable)					: <<INT 7>>			P1.0
//*****************************************************************************
void ext7_int(void) interrupt 13 using 1
{
SFR_EA = 0;
	EXINT_STATUS |= 1;
	IntCount++;

	SFR_E2IF  = 0x01;		// Clear Flag, if Edge triggered
SFR_EA = 1;
}
//*****************************************************************************
//*      Ext Int 8 Interrupt (Programable)					: <<INT 8>>			P1.1
//*****************************************************************************
void ext8_int(void) interrupt 14 using 1
{
SFR_EA = 0;
	EXINT_STATUS |= 2;

	SFR_E2IF  = 0x02;		// Clear Flag, if Edge triggered
SFR_EA = 1;
}
//*****************************************************************************
//*      Ext Int 9 Interrupt (Programable)					: <<INT 9>>			P1.2  Remocon
//*****************************************************************************
void ext9_int(void) interrupt 15 using 1
{
SFR_EA = 0;
	EXINT_STATUS |= 4;
	IntCount++;

	DisableRemoInt();
	SFR_E2IF  = 0x04;		// Clear Flag, if Edge triggered

	InitRemoTimer();
SFR_EA = 1;
}
//*****************************************************************************
//*      Ext Int 10 Interrupt (Programable)					: <<INT 10>>		P1.3
//*****************************************************************************
void ext10_int(void) interrupt 16 using 1
{
//PORT_DEBUG = !PORT_DEBUG;
//PORT_DEBUG = 0;
SFR_EA = 0;
	EXINT_STATUS |= 8;
	IntCount++;

	SFR_E2IF  = 0x08;		// Clear Flag, if Edge triggered

#ifndef MODEL_TW8835_MASTER
#ifdef MODEL_TW8835_SLAVE
	SFR_EX0=0;		//off ext0_int
	SFR_ET1=0;		//off timer1 that for tsc
	while(PORT_I2CCMD_GPIO_SLAVE==0);	//wait
	SFR_EX0=1;		//restore
#ifdef SUPPORT_TOUCH
	SFR_ET1=1;		//restore
#endif
#endif
#endif

SFR_EA = 1;
//PORT_DEBUG = !PORT_DEBUG;
//PORT_DEBUG = 1;
}
//*****************************************************************************
//*      Ext Int 11 Interrupt (Programable)					: <<INT 11>			P1.4
//*****************************************************************************
void ext11_int(void) interrupt 17 using 1
{
SFR_EA = 0;
	EXINT_STATUS |= 0x10;

	SFR_E2IF  = 0x10;		// Clear Flag, if Edge triggered
SFR_EA = 1;
}
//*****************************************************************************
//*      Ext Int 12 Interrupt (Programable)					: <<INT 12>>		P1.5
// PowerDown....
//*****************************************************************************
void ext12_int(void) interrupt 18 using 1
{
SFR_EA = 0;
	EXINT_STATUS |= 0x20;

	SFR_E2IF  = 0x20;		// Clear Flag, if Edge triggered
SFR_EA = 1;
}
//*****************************************************************************
//*      Ext Int 13 Interrupt (Programable)					: <<INT 13>>		P1.6
//*****************************************************************************


void ext13_int(void) interrupt 19 using 1
{
SFR_EA = 0;
	EXINT_STATUS |= 0x40;
//PORT_DEBUG = 0;

	SFR_E2IF  = 0x40;		// Clear Flag, if Edge triggered
SFR_EA = 1;
//PORT_DEBUG = 1;
}

/*
=====================
SINGLE
WRITE  S DEVID A REGIDX A DATA A P
READ   S DEVID A REGIDX A S DEVID+1 DATA A P
 
MULTI
WRITE  S DEVID A REGIDX A DATA A DATA A .. DATA A E(STOP)
READ   S DEVID A REGIDX A S DEVID+1 DATA A DATA A .. DATA A E(STOP)

Normal Write
=================
Master-+   +----+----..-----+----+----+----..----+----+----+----+-...--+   +---
SDA    |   |    |    ..     |    |    |    ..    |    |    |    |      |   |    
       +---+----+----..-----+    +----+----..----+    +----+----+-...--+---+    
SCL   ---+  +-+  +-+    +-+  +-+  +-+  +-+    +-+  +-+  +-+  +-+          +---+
         |  |7|  |6|    |0|  |A|  | |  | |    | |  |A|  | |  | |          |   |
         +--+ +--+ +-..-+ +--+ +--+ +--+ +..--+ +--+ +--+ +--+ +--......--+   +---
index  0    0	 0	    0 0	   0  1    1      1 1    1  2    2  	   			 	  	   		
	   0    1	 2	    8 9	   A  1    2      8 9    A  1    2	   			 	  	   		
Slave -+   +----+----..-----+   +----+----+----..----+    +---------+...----+  +---- 
SDA    |   |    |    ..     |   |    |    |    ..    |    |         |       |  |   
       +---+----+----..-----+---+----+----+----..----+----+---------+...    +--+
       |                                                                   |
       |                                                                   |
       +START                                                              +STOP
		   |<-   DevID  ->|	ACK  |<-   RegIdx  ->| ACK |  DATA  ... |ACK|    

Normal Read
===========
					+--Prepare Restart condition
		            |   +==RESTART
Master..----+W-R--+-W---W   +----+----..----+W-R--+----+----..----+----W NAK+----+--..
SDA   ..    |     | |   |   |    |    ..    |     |    |    ..    |    |A/N |    |     
      ..----+     +-+   +---+----+----..----+     +----+----..----+----W-ACK+----+--..   
SCL      +-+  +-+    +-----+  +-+  +-+    +-+  +-+  +-+  +-+    +-+  +-+  +-+  +-+    
         | |  |A|    |     |  |7|  |6|    |0|  |A|  |7|  |6|    |1|  |0|  |A|  | |    
      .--+ +--+ +----+     +--R +--R +-..-R R--+ R--+ +--+ +..--+ +--+ +--+ +--+ +--.. 
index    1 1    1    2( 2)0   0	   0	  0 0	 2    2           2    2  2 2    2  		   			 	  	   		
	     8 9    A    1( 2)0   1	   2	  8 9	 1    2           8    9  A 1    2	   	
					 		 DevID+1               READ DATA           A/N 
Slave                                  ..----+    +----+----+...   +---+W--R+  +---- 
SDA                                    ..    | A  | D7 | D6 |      | D0|    |  |   
                                       ..----W----W----W----+...   +W--+    +--+

Multi Read
==========
Master..----+W-R--+----+----..----+----+    +----+----..----+----W-NAK--+   W-----
SDA   ..    |     |    |    ..    |    |    |    |    ..    |    |      |   |   
      ..----+     +----+----..----+----W-ACK+----+----..----+----+      W---+   
SCL      +-+  +-+  +-+  +-+    +-+  +-+  +-+  +-+  +-+    +-+  +-+  +--+  +----    
         |0|  |A|  |7|  |6|    |1|  |0|  |A|  |7|  |6|    |1|  |0|  |A |  | STOP    
      ..-+ +--+ +--+ +--+ +..--+ +--+ +--+ +--+ +--+ +..--+ +--+ +--+  +--+  
index    0 0	2    2           2    2  2 2    2           2    2  2 0  		   			 	  	   		
	     8 9	1    2           8    9  A 1    2           8    9  A 0	   	
	             READ DATA           A/N 
Slave ..----+    +----+----+...   +---+W--R+----+----+...   +---+W--R- 
SDA   ..    | A  | D7 | D6 |      | D0|    | D7 | D6 |      | D0|        
      ..----W----W----W----+...   +W--+    W----W----+... --+W--+     


*/

//*****************************************************************************
//*      Ext Int 14 Interrupt (Programable)					: <<INT 14>>		P1.7
//*****************************************************************************
void ext14_int(void) interrupt 20 using 1
{
SFR_EA = 0;
	EXINT_STATUS |= 0x80;
	SFR_E2IF  = 0x80;		// Clear Flag, if Edge triggered
SFR_EA = 1;
}
//*****************************************************************************


//=============================================================================
//		Serial RX Check 												   
//=============================================================================
BYTE RS_ready(void)
{
	if( RS_in == RS_out ) return 0;
	else return 1;
}
#ifdef SUPPORT_UART1
BYTE RS1_ready(void)
{
	if( RS1_in == RS1_out ) return 0;
	else return 1;
}
#endif

//=============================================================================
//		Serial RX														   
//=============================================================================
BYTE RS_rx(void)
{
	BYTE	ret;

	SFR_ES = 0;
	ret = RS_buf[RS_out];
	RS_out++;
	if(RS_out >= RS_BUF_MAX) 
		RS_out = 0;
	SFR_ES = 1;

	return ret;
}
void RS_ungetch(BYTE ch)
{
	RS_buf[RS_in++] = ch;
	if( RS_in>=RS_BUF_MAX ) RS_in = 0;
}
#ifdef SUPPORT_UART1
BYTE RS1_rx(void)
{
	BYTE	ret;

	SFR_ES1 = 0;
	ret = RS1_buf[RS1_out];
	RS1_out++;
	if(RS1_out >= RS_BUF_MAX) 
		RS1_out = 0;
	SFR_ES1 = 1;

	return ret;
}
void RS1_ungetch(BYTE ch)
{
	RS1_buf[RS1_in++] = ch;
	if( RS1_in >=RS_BUF_MAX ) RS1_in = 0;
}
#endif

//=============================================================================
//		Serial TX														   
//=============================================================================
void RS_tx(BYTE tx_buf)
{
	while(RS_Xbusy);

	SFR_ES = 0;			// To protect interrupt between SBUF and RS_Xbusy when it's slow
	SFR_SBUF = tx_buf;
	RS_Xbusy=1;
	SFR_ES = 1;
}
#ifdef SUPPORT_UART1
void RS1_tx(BYTE tx_buf)
{
	while(RS1_Xbusy);

	SFR_ES1 = 0;			// To protect interrupt between SBUF and RS_Xbusy when it's slow
	SFR_SBUF1 = tx_buf;
	RS1_Xbusy=1;
	SFR_ES1 = 1;
}
#endif
//=============================================================================
//														   
//=============================================================================
//it is a 1ms delay.
//tic_pc will be increased every 1 timer0 interrupt that based 1ms.
//
//tic_pc: 0~0xffff and will not increased after 0xffff.
//cnt have to less then 65536. max 65sec delay
void delay1ms(WORD cnt_1ms)
{
	tic_pc = 0;

	while(tic_pc < cnt_1ms);
}

#ifndef MODEL_TW8835_MASTER
void delay1s(WORD cnt_1s, WORD line)
{
	WORD i;
	Printf("\nWait%ds @%d",cnt_1s,line);
	for(i=0; i < cnt_1s; i++) {
		delay1ms(1000);
	}
}
#endif

//=============================================================================
//                            Watchdog                                                   
//=============================================================================
#pragma SAVE
#pragma OPTIMIZE(8,SIZE)

#if defined(SUPPORT_WATCHDOG) || defined(DEBUG_WATCHDOG)
void RestartWatchdog(void)
{
#ifdef DEBUG_WATCHDOG
	SFR_EWDI = 0;		// Disable WDT Interrupt
#endif
	SFR_TA = 0xaa;
	SFR_TA = 0x55;
	SFR_WDCON = 0x03;	// - - - - WDIF WTRF EWT RWT.  Reset Watchdog
	
#ifdef DEBUG_WATCHDOG
	F_watch = 0;
	SFR_EWDI = 1;		// Enable WDT Interrupt (disable for test)
#endif
}
#endif

#ifndef MODEL_TW8835_MASTER
void EnableWatchdog(BYTE mode)
{
	Puts("\nEnableWatchlog");
#ifdef DEBUG_WATCHDOG
	SFR_EWDI = 0;		// Disable WDT Interrupt
#endif
	//ePuts("\nEnableWatchlog");

	SFR_CKCON &= 0x3F;
	switch(mode) {
	case 0: 	SFR_CKCON |= 0xc0;	break;	// WDT clock = 2^26
	case 1: 	SFR_CKCON |= 0x80;	break;	// WDT clock = 2^23: test only
	case 2: 	SFR_CKCON |= 0x40;	break;	// WDT clock = 2^20: test only
	case 3:		SFR_CKCON |= 0x00;	break;	// WDT clock = 2^17: test only
	default: 	SFR_CKCON |= 0xc0;	break;
	}

	SFR_TA = 0xaa;
	SFR_TA = 0x55;
	SFR_WDCON = 0x03;	// - - - - WDIF WTRF EWT RWT.  Reset Watchdog

	SFR_EIP |= 0x20;	//??BK120502
#ifdef DEBUG_WATCHDOG
	SFR_EWDI = 1;		// Enable WDT Interrupt (disable for test)
#endif
}
#endif
#pragma RESTORE

#pragma SAVE
#pragma OPTIMIZE(2,SIZE)
#ifndef MODEL_TW8835_MASTER
void DisableWatchdog(void)
{
	SFR_TA = 0xaa;
	SFR_TA = 0x55;
	SFR_WDCON = 0x00;	// - - - - WDIF WTRF EWT RWT

	SFR_EWDI = 0;		// Disable WDT Interrupt
	ePuts("\nDisableWatchlog");
}
#endif
#pragma RESTORE

#ifndef MODEL_TW8835_MASTER
void EnableInterrupt(BYTE intrn)
{
	SFR_E2IE |= (1 << intrn);	
}
void DisableInterrupt(BYTE intrn)
{
	SFR_E2IE &= ~(1 << intrn);	
}
#endif

//=============================================================================
//			Remocon
//=============================================================================
//BKFYI: It is in the ISR of INT9
#ifdef REMO_RC5

void InitRemoTimer(void)
{
#ifdef DEBUG_REMO
	BYTE i;
#endif

	//WORD temp;
	ClearRemoTimer();			//T2CON = 0x00;				// 
	 
	SFR_ET2  = 0;				// Disable Timer2 Interrupt
	SFR_T2IF = 0x00;			// Clear Flag

	//temp = 0x10000 - 210;
	//TH2 = temp>>8;
	//TL2 = (BYTE)temp;

	SFR_CRCH = SFR_TH2 = 0xff;	// Reload Value
	SFR_CRCL = SFR_TL2 = 0x2e;	// 0xFF2E = 0x10000-0xD2 = 0x10000-210. it means 210 usec interval. 

	SFR_CRCL = SFR_TL2 = 0x2D;		
								//RC5 uses 14 bytes.
								//RC5 spec uses a 24892us for 14 byte.
								//TW8835 use a 8 sampling per one bit 
								//If remocon use a 24892us for 14 Bytes, we have to assign 222us interval.
								// 
								//When I measure a data1 signal from the first falling edge to the last up edge, it was 22800us.
								//and it starts from first falling edge.
								// 22800us = (14*8-4) * interval. 
								// the best interval value is 211us.


	SFR_T2CON = 0x12;			// 0001 0010 
								// |||| |||+-- T2I0 \ Timer2 Input Selection 
								// ||||	||+--- T2I1 / 00=No,  01=Timer,  10=Counter, 11=Gate
								// ||||	|+---- T2CM:  Compare mode
								// ||||	+----- T2R0 \ Timer2 Reload Mode 
								// |||+------- T2R1	/ 00=No,  01=No,     10=Auto,    11=pin T2EX
								// ||+-------- ---
								// |+--------- I3FR: Timer2 Compare0 Interrupt Edge...
								// +---------- T2PS: Timer2 Prescaler

	//start from...
	RemoTic = 4;
	RemoPhase1 = 1;

#ifdef DEBUG_REMO
	if(RemoCaptureDisable==0) {
		for(i=0; i <= 14; i++) {
			RemoCapture0[i]=0x00;
			RemoCapture1[i]=0x00;
			RemoCapture2[i]=0x00;
		}
	}
#endif
	//BKFYI.
	//the next timer2 ISR will be RemoTic 5, and ISR will capture the sample RemoPhase2.
	//The RemoPhase2 have to be 0 in our system(Active Low). 

	RemoSystemCode = 0;
	RemoDataCode = 0;

	SFR_ET2  = 1;					// Enable Timer 2 Interrupt
}

#elif defined REMO_NEC

void InitRemoTimer(void)
{
	WORD temp;

	ClearRemoTimer();			//T2CON = 0x00;				// 
	 
	ET2  = 0;					// Disable Timer2 Interrupt
	T2IF = 0x00;				// Clear Flag

	//186.667us 
	temp = 0x10000 - 187; //or 186	0x10000- BA. 186.667uS  

	CRCH = TH2 = temp>>8;
	CRCL = TL2 = (BYTE)(temp & 0xff);

	T2CON = 0x12;				// 0001 0010 
								// |||| |||+-- T2I0 \ Timer2 Input Selection 
								// ||||	||+--- T2I1 / 00=No,  01=Timer,  10=Counter, 11=Gate
								// ||||	|+---- T2CM:  Compare mode
								// ||||	+----- T2R0 \ Timer2 Reload Mode 
								// |||+------- T2R1	/ 00=No,  01=No,     10=Auto,    11=pin T2EX
								// ||+-------- ---
								// |+--------- I3FR: Timer2 Compare0 Interrupt Edge...
								// +---------- T2PS: Timer2 Prescaler


	RemoTic = 0;		//tm01 = 0;
	RemoStep  = 0;
	RemoPhase = 0;
	RemoHcnt  = 0;
	RemoLcnt  = 0;

//??
//P1_3 = 0;


	ET2  = 1;					// Enable Timer 2 Interrupt
}

#endif

//=============================================================================
//                            Initialize CPU                                                   
//=============================================================================
void InitCPU(void)
{
	TWBASE = 0x00;					// Base address of TW88xx
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	SFR_E2 = 1;						//Chip Access Mode Control. [0]=1b:16bit mode
#endif

	//---------- Initialize Timer Divider ---------
	WriteTW88Page(PAGE4_CLOCK);

	WriteTW88(REG4E2, 0x69);		// Timer0 Divider : system tic 0. 
	WriteTW88(REG4E3, 0x78);		// 27M/27000 = 1msec

//	WriteTW88(REG4E4, 0x01);		// Timer1 Divider : for Touch
//	WriteTW88(REG4E5, 0x0e);		// 27M/270 = 10usec	
//	WriteHost(REG4E4, 0x0A);		// Timer1 Divider : for Touch
//	WriteHost(REG4E5, 0x8C);		// 27MHz/2700 = 100usec
	WriteHost(REG4E4, 0x15);		// Timer1 Divider : for Touch
	WriteHost(REG4E5, 0x18);		// 27MHz/5400 = 200usec
//	WriteHost(REG4E4, 0x69);		// Timer1 Divider : for Touch
//	WriteHost(REG4E5, 0x78);		// 27M/27000 = 1msec

	WriteTW88(REG4E6, 0);			// Timer2 Divider : remo timer
	WriteTW88(REG4E7, 0x1b);		// 27M/27 = 1usec

	WriteTW88(REG4E8, 0);			// Timer3 Divider : baudrate for UART0
	WriteTW88(REG4E9, 0x0c);		// (22.1184M/16) / 12 = 115200bps on SM0=1	

	WriteTW88(REG4EA, 0);			// Timer4 Divider : baudrate for UART1
	WriteTW88(REG4EB, 0x18);		// (22.1184M/16) / 24 = 57600bps on SM1=1	
	//---------- Initialize interrupt -------------

	SFR_CKCON = 0x00;		// Clock control register			
							// 0000 0000
							// |||| |||+-- MD0 \.
							// |||| ||+--- MD1 	> MD[2:0] Stretch RD/WR timing
							// |||| |+---- MD2 /
							// |||| +----- T0M:  Timer0 Pre-Divider 0=div by 12,  1=div by 4
							// |||+------- T1M:  Timer1 Pre-Divider 0=div by 12,  1=div by 4
							// ||+-------- ---
							// |+--------- WD0 \ Watchdong Timeout Period
							// +---------- WD1 / 00=2^17,  01=2^20,  10=2^23,  11=2^26

    SFR_TMOD = 0x66;		// 0110 0110
							// |||| ||||   << Timer 0 >>
							// |||| |||+-- M0 \  00= 8bit timer,counter /32  01= 16bit timer,counter
							// |||| ||+--- M1 /  10= 8bit auto reload        11= 8bit timer,counter
							// |||| |+---- CT:   0=Timer Mode,    1=Counter Mode
							// |||| +----- GATE: 0=GATE not used, 1=GATE used
							// ||||        << Timer 1 >>
							// |||+------- M0 \  00= 8bit timer,counter /32  01= 16bit timer,counter
							// ||+-------- M1 /  10= 8bit auto reload        11= 8bit timer,counter
							// |+--------- CT:   0=Timer Mode,    1=Counter Mode
							// +---------- GATE: 0=GATE not used, 1=GATE used

    SFR_TCON = 0x55;		// 0101 0101
							// |||| |||+-- IT0:  INT0 Trigger 0=level, 1=edge
							// |||| ||+--- IE0:  INT0 Interrupt Flag
							// |||| |+---- IT1:  INT1 Trigger 0=level, 1=edge
							// |||| +----- IE1:  INT1 Interrupt Flag
							// |||+------- TR0:  Timer0 Run
							// ||+-------- TF0:  Timer0 Flag
							// |+--------- TR1:  Timer1 Run
							// +---------- TF0:  Timer1 Flag
							
	SFR_TH0 = 0xff;			// 1 msec
	SFR_TL0 = 0xff;			//

							// for TOUCH SAR sensing timer
#if 0  //normal
	SFR_TH1 = 206;			// 
							// TH1 = 156. 1ms
							// TH1 = 206. 0.5ms = 50*10usec
#endif
	SFR_TH1 = 156;			// TH1 = 156. 10ms	= 100*100usec. if REG4E4:REG4E5=0x0A8C
							//            20ms  = 100*200usec. if REG4E4:REG4E5=0x1518
							//            100ms	= 100*1msec.   if REG4E4:REG4E5=0x6978

	SFR_PCON = 0xc0;		// 1100 0000
							// |||| |||+-- PMM:  Power Management Mode 0=Disable,  1=Enable
							// |||| ||+--- STOP: Stop Mode             0=Disable,  1=Enable
							// |||| |+---- SWB:  Switch Back from STOP 0=Disable,  1=Enable
							// |||| +----- ---
							// |||+------- PWE:	 (Program write Enable)
							// ||+-------- ---
							// |+--------- SMOD1:UART1 Double baudrate bit
							// +---------- SMOD0:UART0 Double baudrate bit

	SFR_SCON = 0x50;		// 0101 0000
							// |||| |||+-- RI:   Receive Interrupt Flag
							// |||| ||+--- TI:   Transmit Interrupt Flag
							// |||| |+---- RB08: 9th RX data
							// |||| +----- TB08: 9th TX data
							// |||+------- REN:	 Enable Serial Reception
							// ||+-------- SMO2: Enable Multiprocessor communication
							// |+--------- SM01 \   Baudrate Mode
							// +---------- SM00 / 00=f/12,  01=8bit var,  10=9bit,f/32,f/64,  11=9bit var

	SFR_SCON1 = 0x50;		// 0101 0000
							// |||| |||+-- RI:   Receive Interrupt Flag
							// |||| ||+--- TI:   Transmit Interrupt Flag
							// |||| |+---- RB08: 9th RX data
							// |||| +----- TB08: 9th TX data
							// |||+------- REN:	 Enable Serial Reception
							// ||+-------- SMO2: Enable Multiprocessor communication
							// |+--------- SM11 \   Baudrate Mode
							// +---------- SM10 / 00=f/12,  01=8bit var,  10=9bit,f/32,f/64,  11=9bit var

	SFR_IP	 = 0x02;		// 0000 0000 interrupt priority					
							// -  PS1 PT2 PS PT1 PX1 PT0 PX0	 		         

	//---------- Enable Individual Interrupt ----------
							// IE(0xA8) = EA ES1 ET2 ES  ET1 EX1 ET0 EX0
//	if(PORT_NOINIT_MODE == 1)	
//			SFR_EX0=0;
//	else	
			SFR_EX0  = 1;	// INT0		: Chip Interrupt

	SFR_ET0  = 1;			// Timer0	: System Tic
//	if(PORT_NOINIT_MODE == 1)	
			SFR_EX1 = 0;
//	else	SFR_EX1  = 1;	// INT1		: DE end
	SFR_ET1  = 0;			// Timer1	: touch
	SFR_ET2  = 0;			// Timer2	: Remote
	SFR_ES   = 1;			// UART0  	: Debug port
#ifdef DP80390
	SFR_UART0FIFO = 0x80;	//          : UART0 FIFO
#endif
#ifdef SUPPORT_UART1
	SFR_ES1  = 1;			// UART1  	: External MCU
#ifdef DP80390
	SFR_UART1FIFO = 0x80;	//          : UART1 FIFO
#endif
#else
	SFR_ES1  = 0;			// UART1  	: External MCU
#endif

	SFR_EA   = 1;			// Global Interrupt

	//---------- Extended Interrupt -------------------
							// 0xe8	xx xx EWDI EINT6 EINT5 EINT4 EINT3 EINT2
	//if(PORT_NOINIT_MODE == 1)	
			SFR_EINT2 = 0;
	//else	SFR_EINT2 = 1;	// INT2		: SPI-DMA done
	SFR_EINT3 = 0;			// INT3		: Touch Ready
	SFR_EINT4 = 0;			// INT4		: Pen 
	SFR_EINT5 = 0;			// INT5		: reserved
	SFR_EINT6 = 0;			// INT6		: reserved
	//---------- Extended Interrupt 7~14 Config. ---------------
							// EINT14 ~ EINT7
							//
							// EINT7	
							// EINT8
							// EINT9 : Remocon
							// EINT10: VideoISR
							//
							//																	  SFR
	SFR_E2IF  = 0x00;		// 0000 0000 : Interrupt Flag                                		  0xFA
	SFR_E2IE  = 0x00;		// 0000 0000 : Interrupt Enable. SW will enable 9 later.			  0xFB
	SFR_E2IP  = 0x00;		// 0000 0000 : Interrupt Priority									  0xFC
	SFR_E2IM  = 0xff;		// 0000 0000 : Interrupt Mode       (0=Edge,        1=Level)		  0xFD
	SFR_E2IT  = 0x00;		// 0000 0000 : Level/Edge Polarity  (0=Low/Falling, 1=High/Rising)	  0xFE

	Puts("\n\n\n\n\nInitCPU ");
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	Puts("8Bit Access");
#else
	Puts("16Bit Access");
#endif

	//------- Remote Controller (INT9, Timer2) --------

	SFR_T2CON = 0x00;		//ClearRemoTimer. RemoINTR(EINT9) will be activateed in RemoTimer		

	//cache on :: sfr 9b = 1;
	Puts(" CACHE");
	SFR_CACHE_EN = 0x01;	//cache ON. No Power Down

#ifdef MODEL_TW8836FPGA
	if(1)
#else
	if(PORT_NOINIT_MODE == 1)
#endif
	{
		Printf("\nSKIP EX0 EX1 EINT2 E2IE[2] ET1");
	}
}



