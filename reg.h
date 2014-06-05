//=============================================================================
//		DP8051 SFRs
//=============================================================================
#ifndef __REGISTERS_H__
#define __REGISTERS_H__
/*
SFR Memory Map
===============
	0/8		1/9		2/A		3/B		4/C		5/D		6/E		7/E
	---		---		---		---		---		---		---		---
80:	P0		SP		DPL0	DPH0	DPL1	DPH1	DPS		PCON
88:	TCON	TMOD	TL0		TL1		TH0		TH1		CKCON
90:	P1		EIF		(WTST)	(DPX0)			(DPX1)
98:	SCON0	SBUF0	BANK	CACHE
A0:	P2
A8:	IE
B0:	P3
B8:	IP
C0:	SCON1	SBUF1	CCL1	CCH1	CCL2	CCH2	CCL3	CCH3
C8:	T2CON	T2IF	RLDL	RLDH	TL2		TH2		CCEN
D0:	PSW
D8:	WDCON
E0:	ACC				CAMC
E8:	EIE		STATUS	(MXAX) 	TA
F0:	B	
F8:	EIP				E2IF	E2IE	E2IP	E2IM	E2IT	__
------------------------------------------------------------------
Standard - standard 8051 register, not described in this document
bolded - new or modified nonstandard 8051 register, described in this document
( ) - reserved register, do not change
* Any reserved register bit shouldn.t be changed from reset value.

*/

//===========================================
//				BYTE Registers
//===========================================
sfr SFR_P0			= 0x80;
sfr SFR_SP			= 0x81;
sfr SFR_DPL			= 0x82;
sfr SFR_DPH			= 0x83;
sfr SFR_PCON		= 0x87;
sfr SFR_TCON		= 0x88;
sfr SFR_TMOD		= 0x89;
sfr SFR_TL0			= 0x8A;
sfr SFR_TL1			= 0x8B;
sfr SFR_TH0			= 0x8C;
sfr SFR_TH1			= 0x8D;
sfr SFR_CKCON		= 0x8E;	//*** New in DP8051

sfr SFR_P1			= 0x90;
sfr SFR_EXIF		= 0x91;	//*** New in DP8051
sfr SFR_WTST		= 0x92;
sfr SFR_DPX			= 0x93;	//
sfr     DPX			= 0x93;	//i2c.src need it
#ifdef DP80390
sfr SFR_DPX1		= 0x95;	//DP80390
#else
sfr	SFR_T2HIGH 		= 0x95;	//--- TW88 Core Extension
#endif
#ifndef DP80390
sfr	SFR_T2LOW 		= 0x96;	//--- TW88 Core Extension
#endif
sfr SFR_SCON		= 0x98;
sfr SFR_SBUF		= 0x99;
sfr TWBASE			= 0x9a;	//*** TW88xx base address
							//Code Bank Address
							//7-0	RG_PGMBASE	R/W	CodeBank Address	def:0
#ifndef DP80390
sfr	SFR_BANKREG		= 0x9A;	//--- TW88 Core Extension. NOT DP80390
#endif
#ifdef DP80390
sfr SFR_ESP			= 0x9B;	//DP80390
#else
sfr SFR_CACHE_EN	= 0x9B;	//Cache Control
							//1		PG_PWND		R/W PowerDown Cache
							//0		CACHE_EN	R/W 1:Enable Cache 0:Disable Cache
sfr	SFR_SPICONTROL 	= 0x9B;	//--- TW88 Core Extension
#endif
#ifdef DP80390
sfr SFR_CACHE_EN	= 0x9C;	//DP80390	Cache Control
#else
sfr	SFR_T0HIGH 		= 0x9C;	//--- TW88 Core Extension
#endif
#ifdef DP80390
sfr SFR_ACON		= 0x9D;	//DP80390
#else
sfr	SFR_T0LOW 		= 0x9D;	//--- TW88 Core Extension
#endif
#ifdef MODEL_TW8836
sfr	SFR_UART0FIFO 	= 0x9E;
#else
sfr	SFR_T1HIGH 		= 0x9E;	//--- TW88 Core Extension
#endif
#ifdef MODEL_TW8836
sfr	SFR_UART1FIFO 	= 0x9F;
#endif
#ifndef DP80390
sfr	SFR_T1LOW 		= 0x9F;	//--- TW88 Core Extension
#endif

sfr SFR_P2			= 0xA0;		
sfr SFR_IE			= 0xA8;		

sfr SFR_P3			= 0xB0;
sfr SFR_IP			= 0xB8;
sfr	SFR_CHPCON		= 0xBF;

sfr SFR_SCON1		= 0xC0;
sfr SFR_SBUF1		= 0xC1;
sfr	SFR_AL			= 0xC4;
sfr	SFR_AH			= 0xC5;
#ifdef DP80390
sfr SFR_MCON		= 0xC6;	//DP80390 	24bit RAM access
#endif
sfr	SFR_FD			= 0xC6;
sfr	SFR_CN			= 0xC7;
sfr SFR_T2CON		= 0xC8;	//*** Modified in DP8051
sfr SFR_T2IF		= 0xC9;	//*** New in DP8051
sfr SFR_RCAP2L		= 0xCA;
sfr SFR_RCAP2H		= 0xCB;
sfr SFR_CRCL		= 0xCA;	//??
sfr SFR_CRCH		= 0xCB;
sfr SFR_TL2			= 0xCC;		
sfr SFR_TH2			= 0xCD;

sfr SFR_PSW			= 0xD0;
sfr SFR_WDCON		= 0xD8;	//*** New in DP8051

sfr SFR_ACC			= 0xE0;
#ifndef DP80390
sfr SFR_CACHE		= 0xE2;	//--- TW88 Core Extension.....REMOVED
#endif
sfr SFR_E2			= 0xE2;	//Chip Access Mode Control. [0]=1b:16bit mode
sfr SFR_EIE			= 0xE8;	//*** New in DP8051
#ifdef DP80390
sfr	SFR_MXAX		= 0xEA;	//DP80390
#endif
sfr SFR_TA			= 0xEB;	//*** New in DP8051
						
sfr SFR_B			= 0xF0;
sfr	SFR_CHPENR		= 0xF6;
sfr SFR_EIP			= 0xF8;	//*** New in DP8051
sfr SFR_E2IF		= 0xFA;	//INT 14~7 Flag
sfr SFR_E2IE		= 0xFB;	//INT 14~7 Enable
sfr SFR_E2IP		= 0xFC;	//INT 14-7 Priority
sfr SFR_E2IM		= 0xFD;	//INT 14~7 Active Control. 1:Edge Active, 0:Level Active
sfr SFR_E2IT		= 0xFE;	//INT 14~7 Edge/Level Polarity. 1:Rising 0:Folling Edge or Low Active



//===========================================
//				BIT Registers
//===========================================

//--- P0
sbit P0_0  		= 0x80;
sbit P0_1  		= 0x81;
sbit P0_2  		= 0x82;
sbit P0_3  		= 0x83;
sbit P0_4  		= 0x84;
sbit P0_5  		= 0x85;
sbit P0_6  		= 0x86;
sbit P0_7  		= 0x87;

//--- TCON
sbit SFR_IT0   = 0x88;
sbit SFR_IE0   = 0x89;
sbit SFR_IT1   = 0x8A;
sbit SFR_IE1   = 0x8B;
sbit SFR_TR0   = 0x8C;
sbit SFR_TF0   = 0x8D;
sbit SFR_TR1   = 0x8E;
sbit SFR_TF1   = 0x8F;

//--- P1
sbit P1_0  		= 0x90;
sbit P1_1  		= 0x91;
sbit P1_2  		= 0x92;
sbit P1_3  		= 0x93;
sbit P1_4  		= 0x94;
sbit P1_5  		= 0x95;
sbit P1_6  		= 0x96;
sbit P1_7  		= 0x97;
//--- (P1)
sbit SFR_T2    	= 0x90;
sbit SFR_T2EX  	= 0x91;

//--- SCON
sbit SFR_RI    = 0x98;
sbit SFR_TI    = 0x99;
sbit SFR_RB8   = 0x9A;
sbit SFR_TB8   = 0x9B;
sbit SFR_REN   = 0x9C;
sbit SFR_SM2   = 0x9D;
sbit SFR_SM1   = 0x9E;
sbit SFR_SM0   = 0x9F;


//--- SCON1
sbit SFR_RI1   = 0xc0;
sbit SFR_TI1   = 0xc1;
sbit SFR_RB18  = 0xc2;
sbit SFR_TB18  = 0xc3;
sbit SFR_REN1  = 0xc4;
sbit SFR_SM12  = 0xc5;
sbit SFR_SM11  = 0xc6;
sbit SFR_SM10  = 0xc7;

//--- P2
sbit P2_0  		= 0xA0;
sbit P2_1  		= 0xA1;
sbit P2_2  		= 0xA2;
sbit P2_3  		= 0xA3;
sbit P2_4  		= 0xA4;
sbit P2_5  		= 0xA5;
sbit P2_6  		= 0xA6;
sbit P2_7  		= 0xA7;

//--- IE
sbit SFR_EA    = 0xAF;
sbit SFR_ES1   = 0xAE;
sbit SFR_ET2   = 0xAD;
sbit SFR_ES    = 0xAC;
sbit SFR_ET1   = 0xAB;
sbit SFR_EX1   = 0xAA;
sbit SFR_ET0   = 0xA9;
sbit SFR_EX0   = 0xA8;

//--- P3
sbit P3_0  		= 0xB0;
sbit P3_1  		= 0xB1;
sbit P3_2  		= 0xB2;
sbit P3_3  		= 0xB3;
sbit P3_4  		= 0xB4;
sbit P3_5  		= 0xB5;
sbit P3_6  		= 0xB6;
sbit P3_7  		= 0xB7;
//--- (P3)
sbit SFR_RXD   	= 0xB0;
sbit SFR_TXD   	= 0xB1;
sbit SFR_INT0  	= 0xB2;
sbit SFR_INT1  	= 0xB3;
sbit SFR_T0    	= 0xB4;
sbit SFR_T1    	= 0xB5;
sbit SFR_WR    	= 0xB6;
sbit SFR_RD    	= 0xB7;

//--- IP
sbit SFR_PX0   = 0xB8;
sbit SFR_PT0   = 0xB9;
sbit SFR_PX1   = 0xBA;
sbit SFR_PT1   = 0xBB;
sbit SFR_PS    = 0xBC;
sbit SFR_PT2   = 0xBD;
sbit SFR_PS1   = 0xBE;

//--- T2CON
/*
sbit SFR_CP_RL2= 0xC8;
sbit SFR_C_T2  = 0xC9;
sbit SFR_TR2   = 0xCA;
sbit SFR_EXEN2 = 0xCB;
sbit SFR_TCLK  = 0xCC;
sbit SFR_RCLK  = 0xCD;
sbit SFR_EXF2  = 0xCE;
sbit SFR_TF2   = 0xCF;
*/



//--- PSW
sbit SFR_P     = 0xD0;
sbit SFR_OV    = 0xD2;
sbit SFR_RS0   = 0xD3;
sbit SFR_RS1   = 0xD4;
sbit SFR_F0    = 0xD5;
sbit SFR_AC    = 0xD6;
sbit SFR_CY    = 0xD7;

//--- WDCON
sbit SFR_RWT   = 0xd8;		// Run Watchdog Timer
sbit SFR_EWT   = 0xd9;		// Enable Watchdog Timer
sbit SFR_WTRF  = 0xda;		// Watchdog Timer Reset Flag
sbit SFR_WDIF  = 0xdb;		// Watchdog Interrupt Flag

//--- EIE 
sbit SFR_EINT2 = 0xe8;		// Enable Ex.Int 2
sbit SFR_EINT3 = 0xe9;		// Enable Ex.Int 3
sbit SFR_EINT4 = 0xea;		// Enable Ex.Int 4
sbit SFR_EINT5 = 0xeb;		// Enable Ex.Int 5
sbit SFR_EINT6 = 0xec;		// Enable Ex.Int 6
sbit SFR_EWDI  = 0xed;		// Enable WatchDog Int

//--- EIP
sbit SFR_PINT2 = 0xf8;		// Priority Ex.Int 2
sbit SFR_PINT3 = 0xf9;		// Priority Ex.Int 3
sbit SFR_PINT4 = 0xfa;		// Priority Ex.Int 4
sbit SFR_PINT5 = 0xfb;		// Priority Ex.Int 5
sbit SFR_PINT6 = 0xfc;		// Priority Ex.Int 6
sbit SFR_PWDI  = 0xfd;		// Priority Watchdog Int

#endif  //__REGISTERS_H__











