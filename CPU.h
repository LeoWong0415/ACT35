#ifndef __CPU_H__
#define __CPU_H__

//===================================================================
// TW8835 8051 Special Function Register (SFR)
//===================================================================

 
/*
I2C[8A]>mcu ds
Dump DP8051 SFR
SFR 80: FF 44 09 4B 00 00 00 C0 - D5 66 FF A3 FF 00 00 BF 
SFR 90: DF 04 07 00 BF 00 00 90 - 54 0D 01 01 00 90 00 BF 
SFR A0: FF BF BF BF BF BF BF BF - D7 BF BF BF BF BF BF BF 
SFR B0: 0F BF BF BF BF BF BF BF - 02 BF BF BF BF BF BF BF 
SFR C0: 50 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 BF 
SFR D0: 00 BF BF BF BF BF BF BF - 08 BF BF BF BF BF BF BF 
SFR E0: 00 BF 00 BF BF BF BF BF - 03 00 00 00 BF BF BF BF 
SFR F0: 01 BF BF BF BF BF BF BF - 00 BF 24 00 00 FF 00 BF 
I2C[8A]>mcu ws fb 4
MCU SFR Write: FB=04 (00) 00000000
I2C[8A]>mcu ds
Dump DP8051 SFR
SFR 80: FF 44 09 4B 00 00 00 C0 - D5 66 FF DA FF 00 00 BF 
SFR 90: DF 04 07 00 BF 00 00 90 - 54 0D 01 01 00 90 00 BF 
SFR A0: FF BF BF BF BF BF BF BF - F7 BF BF BF BF BF BF BF 
SFR B0: 0F BF BF BF BF BF BF BF - 02 BF BF BF BF BF BF BF 
SFR C0: 50 00 00 00 00 00 00 00 - 00 00 2E FF 3A FF 00 BF 
SFR D0: 00 BF BF BF BF BF BF BF - 08 BF BF BF BF BF BF BF 
SFR E0: 00 BF 00 BF BF BF BF BF - 03 00 00 00 BF BF BF BF 
SFR F0: 01 BF BF BF BF BF BF BF - 00 BF 20 04 00 FF 00 BF 
I2C[8A]>
I2C[8A]>
I2C[8A]>mcu ei
MCU extend Interrupt Status: 04, count: 2
I2C[8A]>
*/
#ifdef MODEL_TW8835_EXTI2C
//extern BYTE tsc_buff_index;
//#define TSC_BUFF_MAX 10
//extern WORD tsc_buff[TSC_BUFF_MAX+1][4];
extern BYTE timer1_intr_count;
extern BYTE eint10_intr_count;
extern void Ext0PseudoISR(void);
#endif
 

#define RS_BUF_MAX 	32

extern 			BYTE 	ext1_intr_flag;
extern DATA 	WORD 	tic_pc;
extern DATA 	WORD 	tic_task;


#define	I2C_SCL		P1_0
#define	I2C_SDA		P1_1

#ifdef SUPPORT_I2C2
#define	I2C_SCL2	P1_7
#define	I2C_SDA2	P1_6
#endif

#if defined(SUPPORT_I2CCMD_TEST_SLAVE)
extern DATA	WORD	ext_i2c_timer;
#endif


void InitCPU(void);
void InitISR(BYTE flag);
void EnableWatchdog(BYTE mode);
void DisableWatchdog(void);
void RestartWatchdog(void);

#define EXINT_7		0x01
//..
#define EXINT_14	0x80
void EnableInterrupt(BYTE intrn);
void DisableInterrupt(BYTE intrn);

BYTE RS_ready(void);
BYTE RS_rx(void);
void RS_ungetch(BYTE ch);
void RS_tx(BYTE tx_buf);

BYTE RS1_ready(void);
BYTE RS1_rx(void);
void RS1_ungetch(BYTE ch);
void RS1_tx(BYTE tx_buf);

#define ClearRemoTimer()	{ SFR_T2CON = 0x00; }
#define	EnableRemoInt()		{ RemoDataReady = 0;	SFR_E2IE |= 0x04; }
#define DisableRemoInt() 	{ SFR_E2IE &= 0xfb; }

//void InitRemoTimer(void);


void delay1ms(WORD cnt);
void delay1s(WORD cnt_1s, WORD line);
//void delay10ms(WORD cnt);

#ifdef SUPPORT_HDMI_SiIRX
void TIMER_Set(uint8_t index, uint16_t value);
BYTE TIMER_Expired(uint8_t index);
void TIMER_Wait(uint8_t index, uint16_t value);
#endif

WORD DiffTime_ms( WORD stime, WORD etime );

#ifdef SUPPORT_FOSD_MENU
WORD GetTime_ms(void);
BYTE GetTime_H(void);
BYTE GetTime_M(void);
BYTE GetSleepTimer(void);
void SetSleepTimer(BYTE stime);
#endif



#endif // __CPU_H__
