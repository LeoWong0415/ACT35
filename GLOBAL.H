/*****************************************************************************/
/*                                                                           */
/*              Grobal Variables                                             */
/*                                                                           */
/*****************************************************************************/
#ifndef __GLOBAL_H__
#define __GLOBAL_H__

//---------------------------------- CPU --------------------------------------
extern	DATA 	BYTE   	tm001;
extern	DATA 	WORD   	tm01;
extern	DATA 	BYTE   	tic01;
extern	DATA 	WORD   	tic_pc;
extern	DATA 	WORD   	tic_task;
extern	DATA 	DWORD	SystemClock;
extern 	DATA	DWORD	OsdTimerClock;


//===== WatchDog ======
#ifdef DEBUG_WATCHDOG
extern			bit		F_watch;
#endif

//===== Serial0 ======
extern	XDATA 	BYTE	RS_buf[];
extern	DATA 	BYTE	RS_in, RS_out;
extern			bit		RS_Xbusy;

//===== Serial1 ======
#ifdef SUPPORT_UART1
extern	XDATA 	BYTE	RS1_buf[];
extern	DATA 	BYTE	RS1_in, RS1_out;
extern			bit		RS1_Xbusy;
#endif

#ifdef REMO_RC5
	extern	bit         RemoPhase1, RemoPhase2;
	extern	IDATA BYTE	RemoDataReady;
	extern	IDATA BYTE  RemoSystemCode, RemoDataCode;
	extern	IDATA BYTE  RemoSystemCode0, RemoDataCode0;
	extern	IDATA BYTE  RemoSystemCode1, RemoDataCode1;
	extern	IDATA BYTE  RemoSystemCode2, RemoDataCode2;
	extern	IDATA DWORD RemoReceivedTime;
	extern  IDATA BYTE  RemoCaptureDisable;
#elif defined REMO_NEC
	extern	bit			RemoPhase;
	extern	DATA BYTE	RemoStep;
	extern	DATA BYTE	RemoHcnt, RemoLcnt;
	extern	DATA BYTE	RemoData[4];
	extern	IDATA BYTE  RemoDataReady;
	extern	IDATA BYTE  RemoNum, RemoBit;
	//extern	IDATA BYTE  RemoDataCode;
#endif // REMO 


#ifdef SW_I2C_SLAVE
extern DATA BYTE sw_i2c_index;
extern DATA BYTE sw_i2c_data;
extern DATA BYTE sw_i2c_devid;		//0:NotMine, 8C:MasterWrite, 8D:MasterRead
extern DATA BYTE sw_i2c_regidx;
extern BYTE dbg_sw_i2c_sda_count;	//debug
extern WORD dbg_sw_i2c_scl_count;
extern BYTE dbg_sw_i2c_index[10]; //
extern BYTE dbg_sw_i2c_devid[10];
extern BYTE dbg_sw_i2c_regidx[10];
extern BYTE dbg_sw_i2c_data[10];
#endif

extern	DATA	BYTE	INT_STATUS;
extern	DATA	BYTE	INT_STATUS2;
extern	DATA	BYTE	INT_STATUS3;
extern	DATA	BYTE	EXINT_STATUS;
extern  DATA	WORD	MCU_INT_STATUS;
extern	DATA	WORD	VH_Loss_Changed;
extern	DATA	BYTE	INT_STATUS_ACC;
extern	DATA	BYTE	INT_STATUS2_ACC;

extern			bit		RM_get;

//-------------------------- SPI ----------------------------------------------
// see SPI_Buffer[SPI_BUFFER_SIZE]
//-------------------------- FontOsd Message ----------------------------------------------
extern BYTE FOsdMsgBuff[40];
//-------------------------- Debugging & Monitor ------------------------------

extern	XDATA	BYTE 	DebugLevel;
extern	XDATA	BYTE	MonAddress;	
extern	XDATA	BYTE	MonIndex;
extern	XDATA	BYTE	MonRdata, MonWdata;
extern	XDATA	BYTE	monstr[];		// buffer for input string
extern	XDATA	BYTE 	*argv[];
extern	XDATA	BYTE	argc;
extern			bit		echo;
extern	XDATA	BYTE	SW_key;

extern	XDATA	BYTE 	*argv1[];
extern	XDATA	BYTE	argc1;

extern			BYTE 	*MonString;

extern	struct st_animation AnimationData[];
extern          bit 	AnimationON;
extern			bit		access;

//===================== KeyRemo =================================================
extern WORD		TouchX, TouchY;
extern bit		TouchAuto, TraceAuto;
extern bit		tsc_debug;
//--from CPU
extern  WORD	CpuTouchX, CpuTouchY, CpuZ1, CpuZ2;
extern 	bit		CpuTouchPressed;
extern  WORD	CpuAUX0;
extern  WORD	CpuAUX1;
extern  WORD	CpuAUX2;
extern  WORD	CpuAUX3;
extern  BYTE	CpuAUX0_Changed;
extern  BYTE	CpuAUX1_Changed;
extern  BYTE	CpuAUX2_Changed;
extern  BYTE	CpuAUX3_Changed;
extern  BYTE	CpuTouchStep, CpuTouchChanged;
extern  WORD 	CpuTouchSkipCount;


//===================== debug =================================================
//extern DWORD GlobalBootTime;
//extern BYTE FirstInitDone;

void WaitVBlank(BYTE cnt);		//@inputctrl.h
void Wait1VBlank(void);

//temp
void UpdateOsdTimerClock(void);


#endif // __GLOBAL_H__