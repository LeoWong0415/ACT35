/**
 * @file
 * i2c.c
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	a device driver for the iic-bus interface 
 ******************************************************************************
 */
#include "Config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"
#include "global.h"

#include "printf.h"
#include "CPU.h"
#include "util.h"

#include "I2C.h"

#include "SOsd.h"

#include <intrins.h>
#if defined(SUPPORT_EXTMCU_ISP)
#include "spi.h"
#endif


//=============================
// I2C TIME CHART
//=============================
/**
* I2C TIME CHART
*
*
    -+      +----------+---------+...----+   ...-+     +-------+
SDA  |      |          |         |       |       |     |       |
     +------+----------+---------+...    +---...-+-----+       +--
												
SCL ---+     +----+      +----+      +----+          +------------+
       |     |    |      |    |      |    |          |         	  |
       +-----+    +------+    +-..---+    +--......--+            +--
     | |     |    |    | |           |  |          | | |
     |1|     |  3 |5   |6|           | 4|          |9|7|   8   |
     |                                  |              |
     |                                  |              |
     +START                              +RESTART      +STOP
   (4 1 5) (6  3   5)                              (9 7 8)
																	   TW8835 Slave	
																	   MIN		MAX
1: Hold Time START Condition											74ns	-
2: Clock Low Time(5+6)
3: Clock High Time								i2c_delay_clockhigh		
4: Setup Time for a Repeated START Condition							370ns	-
5: Data Hold Time								i2c_delay_datahold		50ns	900ns
6: Data Setup Time								i2c_delay_datasetup		74ns	-
7: Set-up time for STOP condition										370ns	-
8: Bus Free Time between a STOP and a START Condition 					740ns	 -
9: prepare time for STOP condition
A: ack wait time
*/

//-----------------------
// I2C DELAY
// Note: It depends on CACHE, MCUSPI clock, & SPIOSD.
//-----------------------

static void dd(BYTE delay)
{
	BYTE i;
	for(i=0; i < delay; i++);
}

#define N_O_P_2		_nop_();_nop_()	
#define N_O_P_3		_nop_();_nop_();_nop_()
#define N_O_P_5		_nop_();_nop_();_nop_();_nop_();_nop_()
#define N_O_P_10	N_O_P_5; N_O_P_5
#define N_O_P_20	N_O_P_10; N_O_P_10
#define N_O_P_25	N_O_P_10; N_O_P_10; N_O_P_5
#define N_O_P_50	dd(2)	//N_O_P_20; N_O_P_20; N_O_P_10
#define N_O_P_100	dd(5)	//N_O_P_50; N_O_P_50
#define N_O_P_200	dd(10)	//N_O_P_100; N_O_P_100


#if defined(MODEL_TW8835_EXTI2C)
	//test:1
	#define I2CDelay_1					//dd(1)		
	#define I2CDelay_2					
	#define I2CDelay_3		dd(1)						  //fix
	#define I2CDelay_4		dd(1)			
	#define I2CDelay_5		_nop_();_nop_()		/*;dd(1) */
	#define I2CDelay_6		_nop_()						
	#define I2CDelay_7				    //dd(2)	
	#define I2CDelay_8					//dd(2)	
	#define I2CDelay_9					//dd(2)
	#define I2CDelay_ACK	//dd(3)
#elif defined(MODEL_TW8835_EXTI2C___ORG)
	//base clock: 27MHz Cache:OFF
	//SCLK:48kHz
	//I2C read:
	//I2C write:
	//TSC Read: interval:20ms, 
	//	when touch is not pressed
	//		normal:4.5ms checkkeypad:7.5ms
	//	when touch is pressed
	//		normal:12.2ms checkkeypad:15.1ms
	#define I2CDelay_1		dd(1)			//need 2
	#define I2CDelay_2		dd(3)			//
	#define I2CDelay_3		dd(1)			//
	#define I2CDelay_4		dd(1)			//need 100
	#define I2CDelay_5		//dd(1)			//need 2
	#define I2CDelay_6						//need 2
	#define I2CDelay_7		dd(2)			//need 100
	#define I2CDelay_8		dd(2)			//need 200
	#define I2CDelay_9		dd(2)
	#define I2CDelay_ACK	dd(5)
#else
	//<==NORMAL
	#define I2CDelay_1		dd(1)			//need 2
	#define I2CDelay_2		dd(3)			//
	#define I2CDelay_3		dd(1)			//
	#define I2CDelay_4		dd(1)			//need 100
	#define I2CDelay_5		//dd(1)			//need 2
	#define I2CDelay_6						//need 2
	#define I2CDelay_7		dd(2)			//need 100
	#define I2CDelay_8		dd(2)			//need 200
	#define I2CDelay_9		dd(2)
	#define I2CDelay_ACK	dd(5)
#endif

//-----------------
// I2C assembler
//-----------------
#ifdef I2C_ASSEMBLER
DATA BYTE I2CD	_at_ 0x20;

#define I2CD0	00h
#define I2CD1	01h
#define I2CD2	02h
#define I2CD3	03h
#define I2CD4	04h
#define I2CD5	05h
#define I2CD6	06h
#define I2CD7	07h
#endif

#if 0
bdata unsigned char I2CDBit;
sbit I2CDBit0	= I2CD^0;
sbit I2CDBit1	= I2CD^1;
sbit I2CDBit2	= I2CD^2;
sbit I2CDBit3	= I2CD^3;
sbit I2CDBit4	= I2CD^4;
sbit I2CDBit5	= I2CD^5;
sbit I2CDBit6	= I2CD^6;
sbit I2CDBit6	= I2CD^7;
#endif


//=============================================================================
// I2C subroutines
//=============================================================================

static void I2CStart(void)
{
	I2C_SDA = 1;	
	I2C_SCL = 1;	
					I2CDelay_4;
	I2C_SDA = 0;	I2CDelay_1;
	I2C_SCL = 0;	I2CDelay_5;
}
static void I2CSetSclWait(void)
{
	I2C_SCL=1;	
	//NOTE:It can hangup the system.
	while(I2C_SCL==0);	
}

static void I2CStop(void)
{
	I2C_SDA = 0;		I2CDelay_9;
	I2CSetSclWait();	I2CDelay_7;
	I2C_SDA = 1;		I2CDelay_8;	
}
static BYTE I2CWriteData(BYTE value)
{
	BYTE error;
	BYTE i;

	for(i=0;i<8;i++) {
		if(value & 0x80) I2C_SDA = 1;
		else             I2C_SDA = 0;
							I2CDelay_6;
		I2C_SCL = 1; 		I2CDelay_3;
		I2C_SCL = 0;		I2CDelay_5;

		value <<=1;
	}
	I2C_SDA = 1;			//listen for ACK
	                        
	I2CSetSclWait();		I2CDelay_ACK;
	if(I2C_SDA)	error=1;
	else        error=0;
	                        
	I2C_SCL=0;				I2CDelay_5;

	return error;
}

static BYTE I2CReadData(BYTE fLast)
{
	BYTE i;
	BYTE val=0;

	for(i=0; i <8; i++) {
							I2CDelay_6;
		I2CSetSclWait();	I2CDelay_3;
		val <<= 1;
		if(I2C_SDA)
			val |= 1;
		I2C_SCL=0;			I2CDelay_5;
	}
	if(fLast)	I2C_SDA = 1;	//last byte
	else		I2C_SDA = 0;

	I2CSetSclWait();		I2CDelay_3;
	I2C_SCL=0;
	I2C_SDA=1;				I2CDelay_5;
	return val;
}


//=============================================================================
// I2C SLOW subroutines
//=============================================================================

//monitor can change these i2c_delay global variables.
BYTE i2c_delay_start = 160;
BYTE i2c_delay_restart = 2;
BYTE i2c_delay_datasetup = 0x40;
BYTE i2c_delay_clockhigh = 0x50;
BYTE i2c_delay_datahold = 0x60;

static void dd_start(void)
{
	BYTE i;
	for(i=0; i < i2c_delay_start; i++);
}

static void dd_restart(void)
{
	BYTE i;
	for(i=0; i < i2c_delay_restart; i++);
}

static void dd_datasetup(void)
{
	BYTE i;
	for(i=0; i < i2c_delay_datasetup; i++);
}
static void dd_clockhigh(void)
{
	BYTE i;
	for(i=0; i < i2c_delay_clockhigh; i++);
}
static void dd_datahold(void)
{
	BYTE i;
	for(i=0; i < i2c_delay_datahold; i++);
}
static void dd_clock_ack(void)
{
	BYTE i;
	for(i=0; i < 0x28; i++);
}

//-------------------
// slow routine
// For I2CID_SX1504 and to test the SW Slave I2C
//-------------------
static void I2CStartSlow(void)
{
	I2C_SDA = 1;	
	I2C_SCL = 1;	
					dd_restart();	//delay_4	
	I2C_SDA = 0;	dd_start();		//delay_1
	I2C_SCL = 0;	dd_datahold();	//delay_5
}
static void I2CStopSlow(void)
{
	I2C_SDA = 0;		dd_datasetup();
	I2CSetSclWait();	dd_clockhigh();	
	I2C_SDA = 1;			
}
static BYTE I2CWriteDataSlow(BYTE value)
{
	BYTE error;
	BYTE i;

	for(i=0;i<8;i++) {
		if(value & 0x80) I2C_SDA = 1;
		else             I2C_SDA = 0;
							dd_datasetup();
		I2CSetSclWait(); 	dd_clockhigh();	
		I2C_SCL = 0;		dd_datahold();	
		value <<=1;
	}
	I2C_SDA = 1;			//listen for ACK
	                        
	I2CSetSclWait();		dd_clock_ack();
	if(I2C_SDA)	error=1;
	else        error=0;
	                        
	I2C_SCL=0;				dd_datahold();

	return error;
}
static BYTE I2CReadDataSlow(BYTE fLast)
{
	BYTE i;
	BYTE val=0;
							//dd_datahold(); //NOTE.give time to slave...
	for(i=0; i <8; i++) {
							
		I2CSetSclWait();	
		val <<= 1;
		if(I2C_SDA)
			val |= 1;
		I2C_SCL=0;			dd_datahold();
	}
	if(fLast)	I2C_SDA = 1;	//last byte
	else		I2C_SDA = 0;
							dd_datasetup();
	I2CSetSclWait();		dd_clock_ack();
	I2C_SCL=0;
	I2C_SDA=1;				dd_datahold();
	return val;
}

//------------------
// 2ND I2C
//-----------------
#ifdef SUPPORT_I2C2
static void I2C2Start(void)
{
	I2C2_SDA = 1;
	I2C2_SCL = 1;
					dd(1);		//can skip
	I2C2_SDA = 0;	dd(1);		//NOTE1
	I2C2_SCL = 0;	dd(30);		//NOTE2
}
static void I2C2SetSclWait(void)
{
	I2C2_SCL=1;	
	while(I2C2_SCL==0);
}
static void I2C2Stop(void)
{
	I2C2_SDA = 0;		dd(30);
	I2C2SetSclWait();	dd(30);
	I2C2_SDA = 1;
}
static BYTE I2C2WriteData(BYTE value)
{
	BYTE error = 0;
	BYTE i;

	for(i=0;i<8;i++) {
		if(value & 0x80) I2C2_SDA = 1;
		else             I2C2_SDA = 0;

		I2C2SetSclWait();
		dd(30);			 //too fast
		I2C2_SCL = 0;
		dd(30);			 //too fast
		value <<=1;
	}
	I2C2_SDA = 1;	//listen for ACK
	I2C2SetSclWait();
	dd(30);			 //too fast
	if(I2C2_SDA)
		error=1;
	I2C2_SCL=0;
	dd(30);			 //too fast

	return error;
}
static BYTE I2C2ReadData(BYTE fLast)
{
	BYTE i;
	BYTE val=0;

	for(i=0; i <8; i++) {
		I2C2SetSclWait();
		dd(30);			 //too fast
		val <<= 1;
		if(I2C2_SDA)
			val |= 1;
		I2C2_SCL=0;
		dd(30);			 //too fast
	}
	if(fLast)	I2C2_SDA = 1;	//last byte
	else		I2C2_SDA = 0;

	I2C2SetSclWait();
	dd(30);			 //too fast
	I2C2_SCL=0;
	I2C2_SDA=1;
	dd(30);			 //too fast
	return val;
}
#endif

//=============================================================================
// I2C global Functions
//
//BYTE CheckI2C(BYTE i2cid)
//BYTE WriteI2CByte(BYTE i2cid, BYTE index, BYTE val)
//void WriteI2C(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
//BYTE ReadI2CByte(BYTE i2cid, BYTE index)
//void ReadI2C(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
//
//void WriteSlowI2CByte(BYTE i2cid, BYTE index, BYTE val)
//BYTE ReadSlowI2CByte(BYTE i2cid, BYTE index)
//	                                                                          
//=============================================================================
/**
* check I2C device
*
* I2C commands use a infinity loop. 
* Use CheckI2C() first before you use other I2C commands.
*
* @return
*	0: success
*	1: NAK
*	2: I2C dead
*/
BYTE CheckI2C(BYTE i2cid)
{
	BYTE error;
	BYTE i;
	BYTE value;

	value = i2cid;
SFR_EA=0;
	I2CStart();

	for(i=0;i<8;i++) {
		if(value & 0x80) I2C_SDA = 1;
		else             I2C_SDA = 0;
							I2CDelay_6;
		I2C_SCL = 1; 		I2CDelay_3;
		I2C_SCL = 0;		I2CDelay_5;

		value <<=1;
	}
	I2C_SDA = 1;			//listen for ACK
	/* NOTE: I am not using I2CSetSclWait(). */                        
	I2C_SCL=1; 				I2CDelay_ACK;
	dd(100);
	if(I2C_SCL==0)	error = 2;	 //I2C dead
	else {
		if(I2C_SDA)	error=1;	//NAK
		else        error=0;	//ACK
	}                        
	I2C_SCL=0;				I2CDelay_5;

	//stop routine
	I2C_SDA = 0;		I2CDelay_9;
	/* NOTE: I am not using I2CSetSclWait(). */                        
	I2C_SCL=1; 	 		I2CDelay_7;
	I2C_SDA = 1;		I2CDelay_8;	

SFR_EA=1;
	return error;
}

/**
* write one byte data to I2C slave device
*
* @param i2cid - 8bit.
* @param index 
* @param data
*/
BYTE WriteI2CByte(BYTE i2cid, BYTE index, BYTE val)
{
	BYTE ret;

SFR_EA=0;
	I2CStart();
	ret = I2CWriteData(i2cid);		ret <<=1;
	ret |= I2CWriteData(index);		ret <<= 1;
	ret |= I2CWriteData(val);
	I2CStop();
SFR_EA=1;

#if defined(DEBUG_I2C)
	if(ret)
		Printf("\nWriteI2CByte[%bx:%bx,%bx] FAIL:%bx",i2cid, index,val, ret);
#endif
	return ret;
}

/**
* write data to I2C slave device
*
* @param i2cid - 8bit
* @param index 
* @param *val. NOTE: Do not use a CodeSegment
* @param count
*/
void WriteI2C(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
{
	BYTE i;
	BYTE ret;

SFR_EA=0;
	I2CStart();
	ret = I2CWriteData(i2cid);		ret <<=1;
	ret |= I2CWriteData(index);		ret <<=1;
	for(i=0;i<cnt;i++) { 
		ret |= I2CWriteData(val[i]);
	}
	I2CStop();
SFR_EA=1;
#if defined(DEBUG_I2C)
	if(ret)
		Printf("\nWriteBlock2C[%bx:%bx,%bx] FAIL:%bx",i2cid, index,val, ret);
#endif
}
//-------------------------------------
/**
* slow WriteI2CByte
* @see WriteI2CByte
*/
BYTE WriteSlowI2CByte(BYTE i2cid, BYTE index, BYTE val)
{
	BYTE ret;
//SFR_EA=0;
	I2CStartSlow();
	ret=I2CWriteDataSlow(i2cid);	ret<<=1;
	ret+=I2CWriteDataSlow(index);	ret<<=1;
	ret+=I2CWriteDataSlow(val);
	I2CStopSlow();
//SFR_EA=1;
#if defined(DEBUG_I2C)
	if(ret)
		Printf("\nWriteSlowI2CByte[%bx:%bx,%bx] FAIL:%bx",i2cid, index,val, ret);	
#endif
	return ret;
}

/**
* write data to I2C slave device
*
* @param i2cid - 8bit
* @param index 
* @param *val. NOTE: Do not use a CodeSegment
* @param count
*/
//void WriteSlowI2C(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
void WriteSlowI2C(BYTE i2cid, BYTE index, BYTE *val, WORD cnt)
{
	WORD i;
	BYTE ret;

SFR_EA=0;
	I2CStartSlow();
	ret = I2CWriteDataSlow(i2cid);		ret <<=1;
	ret |= I2CWriteDataSlow(index);		ret <<=1;
	for(i=0;i<cnt;i++) { 
		ret |= I2CWriteDataSlow(val[i]);
	}
	I2CStopSlow();
SFR_EA=1;
#if defined(DEBUG_I2C)
	if(ret)
		Printf("\nWriteSlowI2C[%bx:%bx,%bx] FAIL:%bx",i2cid, index,val, ret);
#endif
}

/**
* read one byte data from I2C slave device
*
* @param i2cid - 8bit
* @param index 
* @return data
*/
BYTE ReadI2CByte(BYTE i2cid, BYTE index)
{
	BYTE value;
	BYTE ret;

SFR_EA=0;
	I2CStart();		  
	ret= I2CWriteData(i2cid);		ret<<=1;
	ret+=I2CWriteData(index);		ret<<=1;
	I2CStart();
	ret+=I2CWriteData(i2cid | 0x01);
	value=I2CReadData(1);
	I2CStop();
SFR_EA=1;

#if defined(DEBUG_I2C)
	if(ret)
		Printf("\nReadI2CByte[%bx:%bx] FAIL:%bx",i2cid,index, ret);
#endif
	return value;
}

/**
* read data from I2C slave device
*
* @param i2cid - 8bit
* @param index 
* @param *val - read back buffer
* @param count
*/
void ReadI2C(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
{
	BYTE i;
	BYTE ret;
SFR_EA=0;	
	I2CStart();
	ret  = I2CWriteData(i2cid);		ret <<=1;
	ret |= I2CWriteData(index);		ret <<=1;
	I2CStart();
	ret |= I2CWriteData(i2cid | 0x01);
	cnt--;
	for(i=0; i<cnt; i++){
		val[i]=I2CReadData(0);
	}
	val[i]=I2CReadData(1);

	I2CStop();
SFR_EA=1;
#if defined(DEBUG_I2C)
	if(ret)
		Printf("\nReadI2C[%bx:%bx] FAIL:%bx",i2cid,index, ret);
#endif
}



/**
* slow ReadI2CByte
* @see ReadI2CByte
*/
BYTE ReadSlowI2CByte(BYTE i2cid, BYTE index)
{
	BYTE val;
	BYTE ret;
//SFR_EA=0;
	I2CStartSlow();		  
	ret = I2CWriteDataSlow(i2cid);		ret<<=1;
	ret += I2CWriteDataSlow(index);		ret<<=1;
#ifdef SW_I2C_SLAVE
	dd(200);
#endif
	I2CStartSlow();

	ret += I2CWriteDataSlow(i2cid | 0x01);
	val=I2CReadDataSlow(1);
	I2CStopSlow();
//SFR_EA=1;
#if defined(DEBUG_I2C)
	if(ret)
		Printf("\nReadSlowI2CByte[%bx:%bx] FAIL:%bx",i2cid,index,ret);	
#endif
	return val;
}

/**
* read data from I2C slave device
*
* @param i2cid - 8bit
* @param index 
* @param *val - read back buffer
* @param count
*/
void ReadSlowI2C(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
{
	BYTE i;
	BYTE ret;
SFR_EA=0;	
	I2CStartSlow();
	ret  = I2CWriteDataSlow(i2cid);		ret <<=1;
	ret |= I2CWriteDataSlow(index);		ret <<=1;
	I2CStart();
	ret |= I2CWriteDataSlow(i2cid | 0x01);
	cnt--;
	for(i=0; i<cnt; i++){
		val[i]=I2CReadDataSlow(0);
	}
	val[i]=I2CReadDataSlow(1);

	I2CStopSlow();
SFR_EA=1;
#if defined(DEBUG_I2C)
	if(ret)
		Printf("\nReadSlowI2C[%bx:%bx] FAIL:%bx",i2cid,index, ret);
#endif
}

#ifdef SUPPORT_I2C2
/**
* second I2C ReadI2CByte
* @see ReadI2CByte
*/
BYTE ReadI2C2Byte(BYTE addr, BYTE index)
{
	BYTE val;
//SFR_EA=0;
	I2C2Start();
	I2C2WriteData(addr);
	I2C2WriteData(index);
	I2C2Start();
	I2C2WriteData(addr | 0x01);
	val=I2C2ReadData(1);
	I2C2Stop();
//SFR_EA=1;
	return val;
}
/**
* second I2C WriteI2CByte
* @see WriteI2CByte
*/
void WriteI2C2Byte(BYTE addr, BYTE index, BYTE val)
{
//SFR_EA=0;
	I2C2Start();
	I2C2WriteData(addr);
	I2C2WriteData(index);
	I2C2WriteData(val);
	I2C2Stop();
//SFR_EA=1;
}
#endif


//=============================================================================
//                                                                           
//=============================================================================
/**
* initialize registers with text array
*
*	format
*		0xII, 0x00	//start. If II is not 00, use WriteI2CByte.  
*		0xff, 0xXX	//assign page
*		0xRR, 0xDD	//register & data
*		...
*		0xff, 0xXX	//assign page
*		0xRR, 0xDD	//register & data
*		...
*		0xff, 0xff	//end
*/
void I2CDeviceInitialize(BYTE *RegSet, BYTE delay)
{
	int	cnt=0;
	BYTE addr, index, val;
#ifndef SUPPORT_8BIT_CHIP_ACCESS
	WORD w_page=0;
#endif

	addr = *RegSet;
#ifdef DEBUG_TW88
	dPrintf("\nI2C address : %02bx", addr);
#endif
	cnt = *(RegSet+1);	//ignore cnt
	RegSet+=2;

	while (( RegSet[0] != 0xFF ) || ( RegSet[1]!= 0xFF )) {			// 0xff, 0xff is end of data
		index = *RegSet;
		val = *(RegSet+1);

		if ( addr == 0 ) {
#ifdef SUPPORT_8BIT_CHIP_ACCESS
			WriteTW88(index, val);
#else
			if(index==0xFF) {
				w_page=val << 8;
			}
			else {
				WriteTW88(w_page+index, val);
			}
#endif
		}
		else
			WriteI2CByte(addr, index, val);

		if(delay)
			delay1ms(delay);
#ifdef DEBUG_TW88
#ifdef SUPPORT_8BIT_CHIP_ACCESS
		dPrintf("\n    addr=%02x  index=%02x   val=%02x", (WORD)addr, (WORD)index, (WORD)val );
#else
		dPrintf("\n    addr=%02x  index=%03x   val=%02x", (WORD)addr, w_page | index, (WORD)val );
#endif
#endif
		RegSet+=2;
	}														   
}

#ifdef MODEL_TW8835_MASTER
/**
* I2C write command from the external MCU to TW8835 internal MCU. 
*
* use an extra GPIO to hold TW8835 internal MCU during Write/Read I2C.
* to slove a chip access arbitration issue between I2C & Internal MCU on TW8835.
*/
BYTE WriteI2CByteToTW88(BYTE index, BYTE value)
{
	BYTE ret;
	PORT_I2CCMD_GPIO_MASTER=0;  //Start
	//delay1ms(1); 
	ret=WriteI2CByte(TW88I2CAddress, index, value); 
	PORT_I2CCMD_GPIO_MASTER=1;	 //Stop

	return ret;
}
/**
* I2C read command from the external MCU to TW8835 internal MCU. 
*
* use an extra GPIO to hold TW8835 internal MCU during Write/Read I2C.
* to slove a chip access arbitration issue between I2C & Internal MCU on TW8835.
*/
BYTE ReadI2CByteFromTW88(BYTE index)
{
	BYTE ret;
	PORT_I2CCMD_GPIO_MASTER=0;  //Start
	//delay1ms(1); 
	ret=ReadI2CByte(TW88I2CAddress,index);
	PORT_I2CCMD_GPIO_MASTER=1;	 //Stop
	return ret;
}
#if 0
/**
* write block function
* 
* @see WriteI2C
* @see WriteI2CByteToTW88
*/
void WriteI2CtoTW88(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
{
	PORT_I2CCMD_GPIO_MASTER=0;  //Start
	WriteI2C(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
	PORT_I2CCMD_GPIO_MASTER=1;	 //Stop
}
/**
* read block function
* 
* @see ReadI2CByteFromTW88
* @see ReadI2C
*/
void ReadI2CfromTW88(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
{
	PORT_I2CCMD_GPIO_MASTER=0;  //Start
	ReadI2C(BYTE i2cid, BYTE index, BYTE *val, BYTE cnt)
	PORT_I2CCMD_GPIO_MASTER=1;	 //Stop
}
#endif

#endif


//===================================================
// I2CISP
// download FW & image thru I2C to SPIFlash
//===================================================
#if defined(MODEL_TW8835_MASTER) && defined(SUPPORT_EXTMCU_ISP)

//same as SpiFlashDmaWait
BYTE WaitSpiDmaDone(BYTE wait, BYTE delay)
{
	BYTE i;
	volatile BYTE vdata;
	//assume page4
	for(i=0; i < wait; i++) {
		vdata = ReadI2CByte(TW88I2CAddress,REG4C4);
		if((vdata & 0x01)==0)	//check self clear bit
			break;
		if(delay)
			delay1ms(delay);
	}
	if(i==wait)
		return ERR_FAIL;
	//Printf("\n1:wait:%bd,%bx",i,vdata);
	return ERR_SUCCESS;
}



#define I2CSPI_SECTOR_SIZE		0x1000		//4K or 32K
#define I2CSPI_BLOCK_SIZE		0x8000		//32K or 64K
#define I2CISP_PROGRESSBAR
/**
* ExtI2C ISP program
*
*	download FW and Image from External MCU to the SLIFlash on the slave system.
*	Goto the slave to STOP mode first. PORT_EXTMCU_ISP = 0;
*	If the slave is a STOP mode, we can use ReadI2CByte/WriteI2CByte withoud GPIO.
* @param start
* @param ptr: if NULL, it will download a test pattern
* @param len
*/
BYTE I2cSpiProg(DWORD start, BYTE *ptr, DWORD len)
{
	BYTE page;
	DWORD addr;
	DWORD spiaddr;
	BYTE cnt;
	BYTE i,j;
	WORD w_checksum,r_checksum;
	BYTE w_data;
	DWORD BootTime;
	DWORD progress;
#ifdef I2CISP_PROGRESSBAR
	DWORD horizontal;
#endif
	BYTE vlen_high;
	BYTE ret;
#ifdef MODEL_TW8836
	WORD SPI_BUFF_addr;
	WORD SPI_BUFF_len;
#endif

	Printf("\nI2cSpiProg(%lx,%s,%lx)", start,ptr==NULL ? "(NULL)":"DATA", len);
	if(start & 0x00000F)
		Printf("\nAddr need 8byte align");
	if(len & 0x00000F)
		Printf("\nLen need 8byte align");


	BootTime = SystemClock;
	page = ReadI2CByte(TW88I2CAddress,0xFF);	//save page	

	WriteI2CByte(TW88I2CAddress,0xFF,4);		//select page 4

#ifdef I2CISP_PROGRESSBAR
	//disable all SPIOSD windows
	WriteI2CByte(TW88I2CAddress,REG420,0);
	for(i=1; i <= 8; i++)
		WriteI2CByte(TW88I2CAddress,REG430+i*0x10, 0);
	WriteI2CByte(TW88I2CAddress,REG406,0);

	//turn on SPIOSD Enable.
	WriteI2CByte(TW88I2CAddress,REG400, ReadI2CByte(TW88I2CAddress,REG400) |  0x04);	

	//------------------------
	// download 4 palettes color. B,G,R, and White.
	// to download, it needs a PCLK.

	//need PCLK	&.
	WriteI2CByte(TW88I2CAddress,REG4E1, 0);		//27MHz first
	WriteI2CByte(TW88I2CAddress,REG4E0, 0);		//PCLK
	WriteI2CByte(TW88I2CAddress,REG4E1, 0x22);	//CLKPLL %2

	//download B,G,R,White palette.
	WriteI2CByte(TW88I2CAddress,REG410, 0xA0 );	//assign byte ptr	
	WriteI2CByte(TW88I2CAddress,REG411, 0);		//address
	WriteI2CByte(TW88I2CAddress,REG412, 0xFF);	//B		
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//G
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//R
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//A

	WriteI2CByte(TW88I2CAddress,REG410, 0xA0 );	//assign byte ptr	
	WriteI2CByte(TW88I2CAddress,REG411, 0x01);	//address
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//B		
	WriteI2CByte(TW88I2CAddress,REG412, 0xFF);	//G
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//R
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//A

	WriteI2CByte(TW88I2CAddress,REG410, 0xA0 );	//assign byte ptr	
	WriteI2CByte(TW88I2CAddress,REG411, 0x02);	//address
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//B		
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//G
	WriteI2CByte(TW88I2CAddress,REG412, 0xFF);	//R
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//A

	WriteI2CByte(TW88I2CAddress,REG410, 0xA0 );	//assign byte ptr	
	WriteI2CByte(TW88I2CAddress,REG411, 0x03);	//address
	WriteI2CByte(TW88I2CAddress,REG412, 0xFF);	//B		
	WriteI2CByte(TW88I2CAddress,REG412, 0xFF);	//G
	WriteI2CByte(TW88I2CAddress,REG412, 0xFF);	//R
	WriteI2CByte(TW88I2CAddress,REG412, 0x00);	//A
#endif

	//need PLL108M
	WriteI2CByte(TW88I2CAddress,REG4E1, 0);		//27MHz first
	WriteI2CByte(TW88I2CAddress,REG4E0, 0x01);	//PLL108M
	WriteI2CByte(TW88I2CAddress,REG4E1, 0x21);	//CLKPLL %1.5, 72MHz
	//Printf("\nREG4E0:%bx, REG4E1:%bx",ReadI2CByte(TW88I2CAddress, REG4E0),ReadI2CByte(TW88I2CAddress,REG4E1));

#ifdef I2CISP_PROGRESSBAR
delay1ms(1000);

	//WIN5 XY:50x400 WH:700x40. FillColor:3
	SPI_Buffer[0]  = 0x85;
	SPI_Buffer[1]  = 0x10;
	SPI_Buffer[2]  = 0x32;
	SPI_Buffer[3]  = 0x90;
	SPI_Buffer[4]  = 0x02;
	SPI_Buffer[5]  = 0xBC;
	SPI_Buffer[6]  = 0x28;
	SPI_Buffer[7]  = 0x00;
	SPI_Buffer[8]  = 0x00;
	SPI_Buffer[9]  = 0x00;
	SPI_Buffer[10] = 0x02;
	SPI_Buffer[11] = 0xBC;
	SPI_Buffer[12] = 0x00;
	SPI_Buffer[13] = 0x00;
	SPI_Buffer[14] = 0x03;
	WriteI2C(TW88I2CAddress,REG480,SPI_Buffer,15);
	//WIN6~8 XY:100x410 WH:600x20. FillColor:0 to 2.
	SPI_Buffer[0]  = 0x85;
	SPI_Buffer[1]  = 0x10;
	SPI_Buffer[2]  = 0x64;
	SPI_Buffer[3]  = 0x9A;
	SPI_Buffer[4]  = 0x00;	//max 600(0x258). start from 0
	SPI_Buffer[5]  = 0x00;
	SPI_Buffer[6]  = 0x14;
	SPI_Buffer[7]  = 0x00;
	SPI_Buffer[8]  = 0x00;
	SPI_Buffer[9]  = 0x00;
	SPI_Buffer[10] = 0x02;
	SPI_Buffer[11] = 0x58;
	SPI_Buffer[12] = 0x00;
	SPI_Buffer[13] = 0x00;
	SPI_Buffer[14] = 0x01;	//Green
	WriteI2C(TW88I2CAddress,REG490,SPI_Buffer,15);  //win6
	SPI_Buffer[14] = 0x02;	//Red
	WriteI2C(TW88I2CAddress,REG4A0,SPI_Buffer,15);  //win7
	SPI_Buffer[14] = 0x00;	//Blue
	WriteI2C(TW88I2CAddress,REG4B0,SPI_Buffer,15);  //win8
#endif

	//-----------------------
	// sector/block erase
	Printf("\nSE");

//#if 0	//test
//	WriteI2CByte(TW88I2CAddress,REG4C1, 0x01 );					// start at vertical blank
//#endif
															// DMA buff use REG4D0~REG4D7
	SPI_Buffer[0] = 0x04;									//REG4C6	DMA buff page(or addr high)
	SPI_Buffer[1] = 0xD0;									//REG4C7	DMA buff index(or addr low)
	SPI_Buffer[2] = 0x00;									//REG4C8	DMA len middle.
	SPI_Buffer[3] = 0x00;									//REG4C9	DMA Len lo.
	WriteI2C(TW88I2CAddress,REG4C6,SPI_Buffer,4);  
	WriteI2CByte(TW88I2CAddress,REG4DA, 0x00 );					//REG4DA	DMA len high.

	progress = 0;
	vlen_high = ReadI2CByte(TW88I2CAddress, REG494) & 0xF0;	// keep high Vertical lenght [11:8].
	for(addr=start; addr < (start+len); addr+=I2CSPI_SECTOR_SIZE) {
#ifdef I2CISP_PROGRESSBAR
		progress += I2CSPI_SECTOR_SIZE;
		horizontal = progress * 600 / len;
		WriteI2CByte(TW88I2CAddress, REG494, vlen_high | ((horizontal>> 8) & 0x0F));
		WriteI2CByte(TW88I2CAddress, REG495, horizontal);
#endif
		Printf("\rSE:%06lx",addr);

		//WREN
															// assume REG4DA:REG4C8:REG4C9 as 0.
		WriteI2CByte(TW88I2CAddress,REG4CA, SPICMD_WREN );		//REG4CA	cmd WREN
		SPI_Buffer[0] = (DMA_DEST_CHIPREG << 6) | 1;		//REG4C3	CHIPREG, cmd len 1(see WREN)
		SPI_Buffer[1] = 0x03;								//REG4C4	DMA Write Start.
		WriteI2C(TW88I2CAddress,REG4C3,SPI_Buffer,2); 
		ret=WaitSpiDmaDone(200,2);
		if(ret)
			return 1;

		//sector erase
		spiaddr = addr;
															// assume REG4DA:REG4C8:REG4C9 as 0.
		SPI_Buffer[0] = SPICMD_SE; 							//REG4CA	cmd SE
		SPI_Buffer[1] = spiaddr>>16;						//REG4CB 	SPI address
		SPI_Buffer[2] = spiaddr>>8;							//REG4CC
		SPI_Buffer[3] = spiaddr;							//REG4CD
		WriteI2C(TW88I2CAddress,REG4CA,SPI_Buffer,4);	  
		SPI_Buffer[0] = (DMA_DEST_CHIPREG << 6) | 4;		//REG4C3	CHIPREG, cmd len 4(see SE)
		SPI_Buffer[1] = 0x07;								//REG4C4	DMA Write Start, Busy check
		WriteI2C(TW88I2CAddress,REG4C3,SPI_Buffer,2);  
		ret=WaitSpiDmaDone(200,2);
		if(ret)
			return 2;
	}
	Printf("\rSE:%06lx",addr-1);
	Printf(" - Done ");
	BootTime = SystemClock - BootTime;
	Printf(" Use:%ld.%ldsec", BootTime/100, BootTime%100 );
#ifdef I2CISP_PROGRESSBAR
delay1ms(1000);
#endif


	BootTime = SystemClock;
	//-----------------------
	// page program
	Printf("\nPP");
	w_checksum = 0;
	if(ptr==NULL)   w_data = 0;
	else			w_data = *ptr++;
	progress = 0;

#ifdef MODEL_TW8836
	//==========================================================================
	//TW8836 will use XMEM DMA
	//SPI_Buffer[SPI_BUFFER_SIZE]  //128byte

	//REG4DB[3:0]REG4DC[7:0]	I2C to XMEM DMA address
	WriteI2CByte(TW88I2CAddress, REG4DB, );
	WriteI2CByte(TW88I2CAddress, REG4DC, );


	for(addr=start; addr < (start+len); addr+=SPI_BUFFER_SIZE) {
		//WREN
		SPI_Cmd_Buffer[0] = 0x00;								//REC4C9	DMA len low. Assume REG4DA,REG4C8 as 0
		SPI_Cmd_Buffer[1] = SPICMD_WREN;						//REG4CA	cmd WREN
		WriteI2C(TW88I2CAddress,REG4C9,SPI_Cmd_Buffer,2);  
		SPI_Cmd_Buffer[0] = (DMA_DEST_CHIPREG << 6) | 1;		//REG4C3	CHIPREG, cmd len 1(see WREN)
		SPI_Cmd_Buffer[1] = 0x03;								//REG4C4	DMA Write Start.
		WriteI2C(TW88I2CAddress,REG4C3,SPI_Cmd_Buffer,2);  
		ret=WaitSpiDmaDone(200,2);
		if(ret)
			return 5;

		//PP
		if((addr+8) > (start+len))	cnt = start+len-addr;
		else						cnt = 8;

		SPI_Cmd_Buffer[0] = cnt;								//REG4C9	DMA len low. Assume REG4DA,REG4C8 as 0
		SPI_Cmd_Buffer[1] = SPICMD_PP;							//REG4CA	cmd PP
		SPI_Cmd_Buffer[2] = spiaddr>>16; 						//REG4CB	SPI address
		SPI_Cmd_Buffer[3] = spiaddr>>8;							//REG4CC
		SPI_Cmd_Buffer[4] = spiaddr;							//REG4CD
		SPI_Cmd_Buffer[5] = 0;									//REG4CE	dummy. If use WriteI2CByte(), we don't need to update REG4CE,REG4CF
		SPI_Cmd_Buffer[6] = 0x1F;								//REG4CF	keep default value
		for(i=0; i < cnt; i++) {
			SPI_Cmd_Buffer[7+i] = w_data;						//REG4D0~REG4D7	
			w_checksum += w_data;
			if(ptr==NULL) w_data++;
			else		  w_data = *ptr++;
		}
		WriteI2C(TW88I2CAddress,REG4C9,SPI_Cmd_Buffer,cnt+7);  
		
				
		SPI_Cmd_Buffer[0] = (DMA_DEST_CHIPREG << 6) | 4;		//REG4C3	CHIPREG, cmd len 4(see PP)
		SPI_Cmd_Buffer[1] = 0x07;								//REG4C4	DMA Write Start, Busy check
		WriteI2C(TW88I2CAddress,REG4C3,SPI_Cmd_Buffer,2);  

		ret=WaitSpiDmaDone(200,2);
		if(ret)
			return 6;

		Printf("\rPP:%06lx",addr);
	}
	//==========================================================================
#else //..MODEL_TW8836
	vlen_high = ReadI2CByte(TW88I2CAddress, REG4A4) & 0xF0;	// keep high Vertical lenght [11:8].
	for(addr=start; addr < (start+len); addr+=8) {
#ifdef I2CISP_PROGRESSBAR
		progress += 8;
		horizontal = progress * 600 / len;
		WriteI2CByte(TW88I2CAddress, REG4A4, vlen_high | ((horizontal>> 8) & 0x0F));
		WriteI2CByte(TW88I2CAddress, REG4A5, horizontal);
#endif
		spiaddr = addr;
				
		//WREN
		SPI_Buffer[0] = 0x00;								//REC4C9	DMA len low. Assume REG4DA,REG4C8 as 0
		SPI_Buffer[1] = SPICMD_WREN;						//REG4CA	cmd WREN
		WriteI2C(TW88I2CAddress,REG4C9,SPI_Buffer,2);  
		SPI_Buffer[0] = (DMA_DEST_CHIPREG << 6) | 1;		//REG4C3	CHIPREG, cmd len 1(see WREN)
		SPI_Buffer[1] = 0x03;								//REG4C4	DMA Write Start.
		WriteI2C(TW88I2CAddress,REG4C3,SPI_Buffer,2);  
		ret=WaitSpiDmaDone(200,2);
		if(ret)
			return 5;

		//PP
		if((addr+8) > (start+len))	cnt = start+len-addr;
		else						cnt = 8;

		SPI_Buffer[0] = cnt;								//REG4C9	DMA len low. Assume REG4DA,REG4C8 as 0
		SPI_Buffer[1] = SPICMD_PP;							//REG4CA	cmd PP
		SPI_Buffer[2] = spiaddr>>16; 						//REG4CB	SPI address
		SPI_Buffer[3] = spiaddr>>8;							//REG4CC
		SPI_Buffer[4] = spiaddr;							//REG4CD
		SPI_Buffer[5] = 0;									//REG4CE	dummy. If use WriteI2CByte(), we don't need to update REG4CE,REG4CF
		SPI_Buffer[6] = 0x1F;								//REG4CF	keep default value
		for(i=0; i < cnt; i++) {
			SPI_Buffer[7+i] = w_data;						//REG4D0~REG4D7	
			w_checksum += w_data;
			if(ptr==NULL) w_data++;
			else		  w_data = *ptr++;
		}
		WriteI2C(TW88I2CAddress,REG4C9,SPI_Buffer,cnt+7);  

		SPI_Buffer[0] = (DMA_DEST_CHIPREG << 6) | 4;		//REG4C3	CHIPREG, cmd len 4(see PP)
		SPI_Buffer[1] = 0x07;								//REG4C4	DMA Write Start, Busy check
		WriteI2C(TW88I2CAddress,REG4C3,SPI_Buffer,2);  

		ret=WaitSpiDmaDone(200,2);
		if(ret)
			return 6;

		Printf("\rPP:%06lx",addr);
	}
#endif //MODEL_TW8836
	Printf("\rPP:%06lx",addr-1);
	Printf(" - Done ");
	BootTime = SystemClock - BootTime;
	Printf(" Use:%ld.%ldsec", BootTime/100, BootTime%100 );
#ifdef I2CISP_PROGRESSBAR
delay1ms(1000);
#endif

	//------------------------
	// check checksum.
	//------------------------

	Printf("\nChecksum ");
	//----------------------
	//save SPI Read Mode and set it as FAST MODE. Reuse w_data.
	//FYI. QuadRead can not use DMA_DEST_CHIPREG.
	w_data = ReadI2CByte(TW88I2CAddress,REG4C0) & 0x07;
	if(w_data != 1) {
		WriteI2CByte(TW88I2CAddress,REG4C0, 1);
	}

	spiaddr = 0x00FFFFFF;
	r_checksum = 0;
	j=0;
	progress =0;
	vlen_high = ReadI2CByte(TW88I2CAddress, REG494) & 0xF0;	// keep high Vertical lenght [11:8].
	for(addr=start; addr < (start+len); addr+=8) {
#ifdef I2CISP_PROGRESSBAR
		progress += 8;
		horizontal = progress * 600 / len;
		WriteI2CByte(TW88I2CAddress, REG4B4, vlen_high | ((horizontal>> 8) & 0x0F));
		WriteI2CByte(TW88I2CAddress, REG4B5, horizontal);
#endif
		if(j <= 0x40) {
			if(j%2==0) Printf("\nSPI %06lx:",addr);
			else 	   Puts(" -");
		}
		else 
			Printf("\rSPI %06lx:",addr);

		SPI_Buffer[0] = 8;									//REG4C9	DMA len low. assume REG4DA,REG4C8 as 0
		SPI_Buffer[1] = SPICMD_FASTREAD;					//REG4CA	cmd FASTREAD
		SPI_Buffer[2] = addr>>16; 							//REG4CB 	SPI address
		SPI_Buffer[3] = addr>>8;							//REG4CC
		SPI_Buffer[4] = addr;								//REG4CD
		WriteI2C(TW88I2CAddress,REG4C9,SPI_Buffer,5);  

		spiaddr = addr;

		SPI_Buffer[0] = (DMA_DEST_CHIPREG << 6) | 5;		//REG4C3	cmd len 5
		SPI_Buffer[1] = 0x05;								//REG4C4	DMA Read Start, Busy Check
		WriteI2C(TW88I2CAddress,REG4C3,SPI_Buffer,2);  


		ret=WaitSpiDmaDone(200,2);
		if(ret)
			return 7;

		if((addr+8) > (start+len))	cnt = start+len-addr;
		else						cnt = 8;
		ReadI2C(TW88I2CAddress,REG4D0,SPI_Buffer,cnt);
		for(i=0; i < cnt; i++) {
			if(j < 0x40)
				Printf(" %02bx", SPI_Buffer[i]);

			//check BUG. 
			if(ptr==NULL) {
				if(SPI_Buffer[i] != (BYTE)(addr + i)) {
					Printf("\nBUG? addr:%lx data:%bx",addr+i,SPI_Buffer[i]);
					SPI_Buffer[i] = ReadI2CByte(TW88I2CAddress,REG4D0+i);
					Printf("=>%bx\n",SPI_Buffer[i]);
				}
			}
			r_checksum += SPI_Buffer[i];

		}
		if(j < 0x41)
			j++;
	}
	Printf("\rSPI %06lx:",addr-1);

	//----------------------
	//restore SPI ReadMode.
	if(w_data != 1) {
		WriteI2CByte(TW88I2CAddress,REG4C0, w_data);
	}

	
	WriteI2CByte(TW88I2CAddress,0xFF,page);		//restore page

	if(r_checksum == w_checksum) { 
		Printf(" success");
		//return 0;
	}
	else {
		Printf(" fail w:%x r:%x",w_checksum, r_checksum);
		Printf("\nstart:%lx len:%lx last_addr:%lx, last_cnt:%bx",start, len, addr, cnt);
		//return 8;
	}

	if(r_checksum != w_checksum)
		return 8;

	return ERR_SUCCESS;
}
#endif

#ifdef SUPPORT_HDMI_SiIRX
void ModifyI2CByte(BYTE slaveID, BYTE offset, BYTE mask, BYTE value)
{
    BYTE aByte;

    aByte = ReadI2CByte(slaveID, offset);

    aByte &= (~mask);        //first clear all bits in mask
    aByte |= (mask & value); //then set bits from value

    WriteI2CByte(slaveID, offset, aByte);
}

void ToggleI2CBit(BYTE slaveID, BYTE offset, BYTE mask)
{
    BYTE aByte;

    aByte = ReadI2CByte(slaveID, offset);

    aByte |=  mask;  //first set the bits in mask
    WriteI2CByte(slaveID, offset, aByte);    //write register with bits set

    aByte &= ~mask;  //then clear the bits in mask
    WriteI2CByte(slaveID, offset, aByte);  //write register with bits clear
}
#endif




