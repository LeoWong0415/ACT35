/**
 * @file
 * junk.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	junk yard
*/

#if 0
/*BKFYI. 
If you want to execute the code at the end of code area, add this dummy code.
It will add a big blank code at the front of code area.
*/
code BYTE dummy_code[1023*5] = {
};
#endif

#if 0 //==============================
#ifdef I2C_ASSEMBLER
static void I2CWriteData(BYTE value)
{
	//BYTE error;

	I2CD = value;

#pragma asm
;----------------
;
;----------------
;	clr EA

	clr	I2C_SCL
	mov	c, I2CD7
	mov I2C_SDA, c
	lcall ddH0	;;;
	setb I2C_SCL
	lcall ddH

	clr	I2C_SCL
	mov	c, I2CD6
	mov I2C_SDA, c
	lcall ddH0	;;;
	setb I2C_SCL
	lcall ddH

	clr	I2C_SCL
	mov	c, I2CD5
	mov I2C_SDA, c
	lcall ddH0	;;;
	setb I2C_SCL
	lcall ddH

	clr	I2C_SCL
	mov	c, I2CD4
	mov I2C_SDA, c
	lcall ddH0	;;;
	setb I2C_SCL
	lcall ddH

	clr	I2C_SCL
	mov	c, I2CD3
	mov I2C_SDA, c
	lcall ddH0	;;;
	setb I2C_SCL
	lcall ddH

	clr	I2C_SCL
	mov	c, I2CD2
	mov I2C_SDA, c
	lcall ddH0	;;;
	setb I2C_SCL
	lcall ddH

	clr	I2C_SCL
	mov	c, I2CD1
	mov I2C_SDA, c
	lcall ddH0	;;;
	setb I2C_SCL
	lcall ddH

	clr	I2C_SCL
	mov	c, I2CD0
	mov I2C_SDA, c
	lcall ddH0	;;;
	setb I2C_SCL
	lcall ddH

;----------------
;
;----------------
;	setb EA

#pragma endasm

	I2C_SCL=0;	ddH();
	I2C_SDA = 1;	//listen for ACK

	I2C_SCL=1;	ddH();
	I2C_SCL=0;
}
#endif //..#ifdef I2C_ASSEMBLER
#ifdef I2C_ASSEMBLER
static BYTE I2CReadData(BYTE fLast)
{
	I2C_SDA = 1;

#pragma asm

;	clr		EA

	clr		I2C_SCL
	lcall	ddH
	setb	I2C_SCL
	lcall	ddH
	mov		c, I2C_SDA
	mov		I2CD7, c

	clr		I2C_SCL
	lcall	ddH
	setb	I2C_SCL
	lcall	ddH
	mov		c, I2C_SDA
	mov		I2CD6, c

	clr		I2C_SCL
	lcall	ddH
	setb	I2C_SCL
	lcall	ddH
	mov		c, I2C_SDA
	mov		I2CD5, c

	clr		I2C_SCL
	lcall	ddH
	setb	I2C_SCL
	lcall	ddH
	mov		c, I2C_SDA
	mov		I2CD4, c

	clr		I2C_SCL
	lcall	ddH
	setb	I2C_SCL
	lcall	ddH
	mov		c, I2C_SDA
	mov		I2CD3, c

	clr		I2C_SCL
	lcall	ddH
	setb	I2C_SCL
	lcall	ddH
	mov		c, I2C_SDA
	mov		I2CD2, c

	clr		I2C_SCL
	lcall	ddH
	setb	I2C_SCL
	lcall	ddH
	mov		c, I2C_SDA
	mov		I2CD1, c

	clr		I2C_SCL
	lcall	ddH
	setb	I2C_SCL
	lcall	ddH
	mov		c, I2C_SDA
	mov		I2CD0, c

;	setb	EA

#pragma endasm

	I2C_SCL=1;	ddH();
	I2C_SCL=0;

	//return I2CD;
}
#endif //..#ifdef I2C_ASSEMBLER
#endif //======================================================
//===================JUNK CODE============================

#ifdef UNCALLED_CODE
void HDMIPowerDown(void)
{
	ReadI2CByte(I2CID_EP9351,EP9351_General_Control_0, ReadI2C(I2CID_EP9351,EP9351_General_Control_0) |0x04);
	delay1ms(500);
}
#endif

// call from CheckAndSetHDMI
//BKTODO:Remove it.
#if 0
BYTE CheckAndSet_EP9351(void)
{
	volatile BYTE vTemp;
	BYTE TempByte[20];
	BYTE i;

#if 0
	//Hot Boot needs Soft Reset.  TODO:Need Verify. I don't know why it need.
	bTemp = ReadI2C(I2CID_EP9351, EP9351_Status_Register_0 );
	if ( (bTemp & 0x1C) != 0x1C ) {
		Printf("\nEP9351 $%bx read:%bx. Do SWReset",EP9351_Status_Register_0,bTemp);
		bTemp = ReadI2C(I2CID_EP9351,EP9351_General_Control_0);
		bTemp |= EP9351_General_Control_0__PWR_DWN;
		ReadI2CByte(I2CID_EP9351,EP9351_General_Control_0, bTemp);		 		// set to 0x40, Soft reset
		delay1ms(500);
		bTemp &= ~EP9351_General_Control_0__PWR_DWN;
		ReadI2CByte(I2CID_EP9351,EP9351_General_Control_0, bTemp);				// set to NORMAL
		Printf("=>read:%bx",ReadI2C(I2CID_EP9351, EP9351_Status_Register_0 ));
	}
#endif
	ReadI2CByte(I2CID_EP9351, EP9351_General_Control_1, ReadI2C(I2CID_EP9351,EP9351_General_Control_1) );		// make Positive polarity always
	ReadI2CByte(I2CID_EP9351, EP9351_General_Control_9, 0x01);			   	// enable EQ_GAIN

	for(i=0; i < 100; i++) {
		delay1ms(10);
		vTemp = ReadI2C(I2CID_EP9351,EP9351_Status_Register_0);
		if(vTemp)
			Printf("\n%bd:check status : %bx",i,vTemp);

		vTemp = ReadI2C(I2CID_EP9351,EP9351_HDMI_INT);
		if((vTemp & EP9351_HDMI_INT__AVI)) {
			//Printf("\ncheck INT end : %bd",i);
			break;
		}
	}
	if(i==100) {
		Printf("\nCheckAndSet_EP9351 FAIL");
		return ERR_FAIL;	//NO AVI_F
	}


	Printf("\nInit_HDMI read %bx:%bx @%bd",EP9351_HDMI_INT,vTemp,i);

	//read AVI InfoFrame at 0x2A
	ReadI2C(I2CID_EP9351, EP9351_AVI_InfoFrame, TempByte, 15);
	DBG_PrintAviInfoFrame();

	//color convert to RGB
	Puts("\nInput HDMI format ");
	i = (TempByte[2] & 0x60) >> 5;
	if(i == 0) 	{
		Puts("RGB");
		ReadI2CByte(I2CID_EP9351, EP9351_General_Control_2, 0x00 ); //0x42
	}
	else if (i==1)  	{
		Puts("YUV(422)");
		ReadI2CByte(I2CID_EP9351, EP9351_General_Control_2, 0x50 );
	}
	else if (i==2)  	{
		Puts("YUV(444)");
		ReadI2CByte(I2CID_EP9351, EP9351_General_Control_2, 0x10 );
	}

	//dump control register
	Puts("\nEP9351 General Control:");
	for(i=0; i < 10; i++) {
		Printf("%2bx ",ReadI2C(I2CID_EP9351,0x40+i));
	}

	ReadI2CByte(I2CID_EP9351,EP9351_General_Control_9, 0x09);			   	// enable EQ_GAIN. use 40uA PUMP.
	Printf( "=>%02bx", ReadI2C(I2CID_EP9351,EP9351_General_Control_9) );


	return 0;
}
#endif

#if 0
BYTE Init_HDMI_EP9X53__OLD(void)
{	
	BYTE i, cVal[15];
	volatile BYTE value;
	BYTE j;


#if 1	//Hot Boot need it. BK120319
	i = ReadI2C(I2CID_EP9351, EP9351_Status_Register_0 );
//	if ( (i & 0x1C) != 0x1C ) {
		ReadI2CByte(I2CID_EP9351,EP9351_General_Control_0, EP9351_General_Control_0__PWR_DWN);		 		// set to 0x40, Soft reset
		delay1ms(500);
		ReadI2CByte(I2CID_EP9351,EP9351_General_Control_0, 0x00);				// set to NORMAL
//	}
	delay1ms(500);

	//while ( ReadI2C(I2CID_EP9351,EP9351_General_Control_0) ) ;			// wait till correct val
	ReadI2CByte(I2CID_EP9351, EP9351_General_Control_1, ReadI2C(I2CID_EP9351,EP9351_General_Control_1) );		// make Positive polarity always
	ReadI2CByte(I2CID_EP9351, EP9351_General_Control_9, 0x01);			   	// enable EQ_GAIN

	cVal[13] = ReadI2C(I2CID_EP9351,EP9351_HDMI_INT);	  				//$29 interrupt flag
	cVal[14] = ReadI2C(I2CID_EP9351,EP9351_Status_Register_0);	  		//$3C	status0
	ReadI2C(I2CID_EP9351, EP9351_Timing_Registers, cVal, 13);		//$3B
	DBG_PrintTimingRegister();	//HDMI_DumpTimingRegister(cVal);
	Printf("\nR29:%bx,R3C:%bx",cVal[13],cVal[14]);
#endif
	for(i=0; i < 100; i++) {
		delay1ms(10);
		value = ReadI2C(I2CID_EP9351,EP9351_Status_Register_0);
		if(value)
			Printf("\n%bd:check status : %bx",i,value);

		value = ReadI2C(I2CID_EP9351,EP9351_HDMI_INT);
		if((value & EP9351_HDMI_INT__AVI)) {
			//Printf("\ncheck INT end : %bd",i);
			break;
		}
	}
	if(i==100) {
		return ERR_FAIL;	//NO AVI_F
	}

	Printf("\nInit_HDMI read %bx:%bx @%bd",EP9351_HDMI_INT,value,i);

	//read AVI InfoFrame at 0x2A
	ReadI2C(I2CID_EP9351, EP9351_AVI_InfoFrame, cVal, 15);
	DBG_PrintAviInfoFrame();

	//color convert to RGB
	Puts("\nInput HDMI format ");
	i = (cVal[2] & 0x60) >> 5;
	if(i == 0) 	{
		Puts("RGB");
		ReadI2CByte(I2CID_EP9351, EP9351_General_Control_2, 0x00 ); //0x42
	}
	else if (i==1)  	{
		Puts("YUV(422)");
		ReadI2CByte(I2CID_EP9351, EP9351_General_Control_2, 0x50 );
	}
	else if (i==2)  	{
		Puts("YUV(444)");
		ReadI2CByte(I2CID_EP9351, EP9351_General_Control_2, 0x10 );
	}

	//dump control register
	Puts("\nEP9351 General Control:");
	for(i=0; i < 10; i++) {
		Printf("%2bx ",ReadI2C(I2CID_EP9351,0x40+i));
	}

	ReadI2CByte(I2CID_EP9351,EP9351_General_Control_9, 0x09);			   	// enable EQ_GAIN. use 40uA PUMP.
	Printf( "=>%02bx", ReadI2C(I2CID_EP9351,EP9351_General_Control_9) );

	return ERR_SUCCESS;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
#if 0
void EP9351_Interrupt()
{
	// Interrupt Flags Register
	Event_HDMI_Info = 1;	
}
#endif

#if 0
BYTE EP9351_Task(void)
{
	status0 = ReadI2C(I2CID_EP9351, EP9351_Status_Register_0);
	status1 = ReadI2C(I2CID_EP9351, EP9351_Status_Register_1);

	if((status1 & 0xC0 == 0xC0) && !(is_powerdown)) {
		if(!is_Valid_Signal) {
			//No Signal -> Signal Valid
			is_Valid_Signal = 1;

			//VideoMuteDisable

			if(status0 & EP9351_Status_Register_0__HDMI) {
			}
			else {
			}

		}
	}
	else {
		if(is_Valid_Signal) {
			is_Valid_Signal = 0;
		}
	}

	if(is_NoSignal_Reset) {
	}

	if(is_HDMI) {
		ReadInterruptFlags();
	}

}
#endif


//-----------------------------------------------------------------------------
//desc
//parameter
//	number of bytes
//	read bytes
//	register
//	write cmd data
//return
#if 0 //BK111118
BYTE SPI_cmd_protocol(BYTE max, ...)
{
	va_list ap;
	BYTE page;
	BYTE temp;
	BYTE r_cnt, w_cnt, i;
	BYTE w_cmd[5];
	BYTE ret;

	//-------------------
	ret=0xff;
	if(max < 2)
		return 0xff;
	
	va_start(ap, max);

	w_cnt = max-1;
	r_cnt = va_arg(ap, BYTE);		//r_cnt
	for(i=0; i < w_cnt; i++) {
		w_cmd[i]=va_arg(ap, BYTE);		//reg
	}
	Printf("\nSPICMD[%bd] r:%bd w:%bd reg:%bx",max, r_cnt, w_cnt-1, w_cmd[0]);

	ReadTW88Page(page);		//save
	WriteTW88Page(PAGE4_SPI );

	WriteTW88(REG4C3, 0x40+w_cnt);
	Write2TW88(REG4C6,REG4C7, 0x04d0);			// DMA Page & Index. indecate DMA read/write Buffer at 0x04D0.
	WriteTW88(REG4DA,0 );					// DMA Length high
	Write2TW88(REG4C8,REG4C9, r_cnt);			// DMA Length middle & low

	//write 
	for(i=0; i < w_cnt; i++)
		WriteTW88(REG4CA+i, w_cmd[i] );		// write cmd1
	if(r_cnt) {
		WriteTW88(REG4C4, 0x01 );				// start
		//delay1ms(2);
		for(i=0; i < 100; i++)
			_nop_();

	}
	else {	
		WriteTW88(REG4C4, 0x07 );				// start, with write, with busycheck
	}

	//read
	if(r_cnt)	Puts("\tREAD:");	
	for(i=0; i < r_cnt; i++) {
		temp = ReadTW88(REG4D0+i );
		Printf("%02bx ",temp);
		if(i==0)
			ret = temp;
	}							

	WriteTW88Page(page );	//restore
	//-------------------
	va_end(ap);

	return ret;
}
#endif
#if 0 //test CRC8

//#define GP  0x107   /* x^8 + x^2 + x + 1 CRC-8-CCITT */
//#define DI  0x07

//#define GP  0x131   /* x^8 + x^5 + x^4 + 1 CRC-8 Dallas/Maxim */
//#define DI  0x31

#define GP  0x1D5   /* x^8 + x^7 + x^6 + x^4 + x^2 + 1 CRC-8 */
#define DI  0xD5

//#define GP  0x19B   /* x^8 + x^7 + x^4 + x^3 + x + 1 CRC-8-WCDMA */
//#define DI  0x9B


static unsigned char crc8_table[256];     /* 8-bit table */
/*
* Should be called before any other crc function.  
*/
static void init_crc8()
{
	int i,j;
	unsigned char crc;
	
	for (i=0; i<256; i++) {
		crc = i;
		for (j=0; j<8; j++)
			crc = (crc << 1) ^ ((crc & 0x80) ? DI : 0);
		crc8_table[i] = crc & 0xFF;
	}
	//for(i=0; i < 16; i++) {
	//	Printf("\n%x:",i);
	//	for(j=0; j <16; j++) {
	//		Printf(" %02bx", crc8_table[i*16+j]); 
	//	}
	//}
	//Puts("\n");

}

void crc8(unsigned char *crc, unsigned char m)
     /*
      * For a byte array whose accumulated crc value is stored in *crc, computes
      * resultant crc obtained by appending m to the byte array
      */
{
	*crc = crc8_table[(*crc) ^ m];
	*crc &= 0xFF;
}


void TestCrC8(void)
{
	BYTE crc;

	init_crc8();
	crc = 0;
	crc8(&crc, 0xF6	);
	crc8(&crc, 0x8B	);
	crc8(&crc, 0x3D	);
	crc8(&crc, 0x11	);
	crc8(&crc, 0x5D	);
	crc8(&crc, 0xB6	);
	crc8(&crc, 0x7B	);

	Printf("\nCRC8 %bx",crc);
}
#endif //test CRC8

//=============================================================================
//                                                                           
//=============================================================================
/*
#ifdef DEBUG
BYTE Getch(void)
{
	while(!RS_ready());
	return RS_rx();
}
#endif
*/
/*
BYTE Getche(void)
{
	BYTE ch;

	while(!RS_ready());
	ch = RS_rx();
	RS_tx(ch);

	return ch;
}
*/
#if 0
	Puts("\nINTR ");
	if(SFR_EX0)			Puts(" 0:ext0");
	if(SFR_ET0)			Puts(" 1:timer0");
	if(SFR_EX1)			Puts(" 2:ext1");
	if(SFR_ET1)			Puts(" 3:timer1");
	if(SFR_ES)			Puts(" 4:uart0");

	if(SFR_ET2)			Puts(" 5:timer2");
	if(SFR_ES1)			Puts(" 6:uart1");

	if(SFR_EINT2)		Puts(" 7:ext2");
	if(SFR_EINT3)		Puts(" 8:ext3");
	if(SFR_EINT4)		Puts(" 9:ext4");
	if(SFR_EINT5)		Puts(" 10:ext5");
	if(SFR_EINT6)		Puts(" 11:ext6");
	if(SFR_EWDI)		Puts(" 12:watchdog");
	if(SFR_E2IE & 0x01)	Puts(" 13:ext7");
	if(SFR_E2IE & 0x02)	Puts(" 14:ext8");
	if(SFR_E2IE & 0x04)	Puts(" 15:ext9");
	if(SFR_E2IE & 0x08)	Puts(" 16:ext10");
	if(SFR_E2IE & 0x10)	Puts(" 17:ext11");
	if(SFR_E2IE & 0x20)	Puts(" 18:ext12");
	if(SFR_E2IE & 0x40)	Puts(" 19:ext13");
	if(SFR_E2IE & 0x80)	Puts(" 20:ext14");
#endif


#if 0 //#ifdef MODEL_TW8835_EXTI2C
	//base 27MHz MCU clock.	1 clk cycle: 37nSec
	//SCLK:367kHz. almost 400kHz. one I2C read use 196uS.
	#define I2CDelay_1		_nop_(); _nop_()
	#define I2CDelay_3		dd(1*5)			//
	#define I2CDelay_4		dd(1*5)			//need 100
	#define I2CDelay_5		_nop_(); _nop_()
	#define I2CDelay_6		_nop_(); _nop_()
	#define I2CDelay_7		dd(1*5)			//need 100
	#define I2CDelay_8		dd(2*5)			//need 200
	#define I2CDelay_9		_nop_()
	#define I2CDelay_ACK	dd(1*5)
#endif
#if 0
	//base 72MHz MCU clock.	1 Clk cycle: 13.89nS
	//SCLK:???kHz. one I2C read use ???uS.
#define I2CDelay_1		N_O_P_20 	//6
#define I2CDelay_3		N_O_P_100	//??
#define I2CDelay_4		N_O_P_50	//27
#define I2CDelay_5		N_O_P_10 	//4		  5:NG
#define I2CDelay_6		N_O_P_20 	//6
#define I2CDelay_7		N_O_P_50	//27
#define I2CDelay_8		N_O_P_100	//53
#define I2CDelay_9		N_O_P_5 //_nop_()
#define I2CDelay_ACK	N_O_P_100
#endif
#if 0
	//base 72MHz MCU clock.	1 Clk cycle: 13.88nS
	//SCLK:???kHz. one I2C read use ???uS.
#define I2CDelay_1		dd(1)
#define I2CDelay_2		dd(10)
#define I2CDelay_3		dd(5)
#define I2CDelay_4		dd(5)
#define I2CDelay_5		dd(5)
#define I2CDelay_6		
#define I2CDelay_7		dd(3)
#define I2CDelay_8		dd(10)
#define I2CDelay_9		dd(10)
#define I2CDelay_ACK	dd(10)
#endif

//=============================================================================
// REMOVED
//=============================================================================
#if 0
//new 110909
void LoDecoderMode(BYTE mode)
{
	BYTE value;
	WriteTW88Page(PAGE1_DECODER);
	value = ReadTW88(REG102) & 0xCF;
	value |= (mode << 4);
	WriteTW88(REG102, value);
}

void InMuxPowerDown(BYTE Y, BYTE C, BYTE V)
{
	BYTE value;
	WriteTW88Page(PAGE1_DECODER);
	value = ReadTW88(REG106) & 0xF8;
	if(Y)	value |= 0x04;
	if(C)	value |= 0x02;
	if(V)	value |= 0x01;
	WriteTW88(REG106,value);		
}
//assume R102[6]=1;
void InMuxInput(BYTE Y, BYTE C, BYTE V)
{
	BYTE value;
	WriteTW88Page(PAGE1_DECODER);
	value = ReadTW88(REG102) & ~0x8F;
	value |= (Y << 2);
	if(C >= 2)		value |= 0x80;
	if(C & 0x01) 	value |= 0x02;
	value |= V;
	WriteTW88(REG102,value);
	
}
void AFESelectDecoderAndClock(BYTE fLoDecoder)
{
	BYTE r105,r1c0;

	WriteTW88Page(PAGE1_DECODER);
	r105 = ReadTW88(REG105) & 0xFE;
	r1c0 = ReadTW88(REG1C0) & 0xFE;
	if(fLoDecoder) {
		r105 |= 0x01;	//set decoder
		r1c0 |= 0x01;	//decoder use 27MHz
	}
	WriteTW88(REG105,r105);
	WriteTW88(REG1C0,r1c0);
	 
}

//	InputPort		PowerDown	AntiAliasingFilter
//	----			----		----
//Y	R102[3:2]		R106[2]		R105[3]
//C	R102[7]R102[1]	R106[1]		R105[2]
//V	R102[0]			R106[0]		R105[1]
//
// input		InputPort	PowerDown	AFE Path	ADC Clock	PGA select
// -----		---------	---------	----		---- 		----	
// CVBS			Y0			C&V			LoDecoder	27M			LowSpeed
// SVIDEO		Y1,C0		V			LoDecoder	27M			LowSpeed
// Component	Y2,C1,V0				HiDecoder	LLPLL		HighSpeed
// aPC			Y2,C1,V0				HiDecoder	LLPLL		HighSpeed
void AFESetInMuxInput_____TEST(BYTE InputMode)
{
	BYTE use_highspeed_path;

	use_highspeed_path = 0;	
	switch(InputMode) {
	case INPUT_CVBS:
		InMuxInput(0,0,0);
		InMuxPowerDown(0,1,1);
		LoDecoderMode(0);
		break;
	case INPUT_SVIDEO:
		InMuxInput(1,0,0);
		InMuxPowerDown(0,0,1);
		LoDecoderMode(1);
		break;
	case INPUT_COMP:
		InMuxInput(2,1,0);
		InMuxPowerDown(0,0,0);
		LoDecoderMode(0);
		use_highspeed_path = 1;
		break;
	case INPUT_PC:
		InMuxInput(2,1,0);
		InMuxPowerDown(0,0,0);
		LoDecoderMode(0);
		use_highspeed_path = 1;
		break;
	case INPUT_DVI:
	case INPUT_HDMIPC:
	case INPUT_HDMITV:
		InMuxInput(0,0,0);
		InMuxPowerDown(0,0,0);
		LoDecoderMode(0);
		break;
	case INPUT_BT656:
		InMuxInput(0,0,0);
		InMuxPowerDown(0,1,1);
		LoDecoderMode(0);
		break;
	}
	AFESelectDecoderAndClock(!use_highspeed_path);
}
#endif
#if 0
void AFESetInMuxYOUT()
{}
void AFEClampStartEnd(BYTE start, BYTE end)
{}
void AFEClampPosition(BYTE position)
{}
void AFEClampSetMode(BYTE mode)
{}
#endif

//===========================================
// TW9900
//===========================================
//---------------------------------------------
//		BYTE	CheckTW9900VDLOSS( BYTE n )
//---------------------------------------------
#ifdef USE_TW9900
BYTE	CheckTW9900VDLOSS( BYTE n )
{..}

//---------------------------------------------
//		BYTE	CheckTW9900STD( BYTE n )
//---------------------------------------------
BYTE	CheckTW9900STD( BYTE n )
{..}
#endif

//====================================
// VADC.C
//====================================
//The unit is sysclk
//480i(SOY) = 140 (92~213)
//576i(SOY) = 140 (92~213)
//480p(SOY) = 52  (30~83)
//576p(SOY) = 58  (30~83)
//1080i(SOY)= 24  (14~44)
//720p(SOY) = 38  (18~67)
//H/Vsync = 0
#if 0
//								 720    720    720    720 	1920   1280   1920
//							     480i,  576i,  480p, 576p,  1080i, 720p,  1080p
code	BYTE	ClampPos[]   = { 140,   140,   52,    58,    24,    38,    24 };
#endif



//for bank issue
#if 0
BYTE GetInputVAdcMode(void)
{...}
#endif

#if 0
//!void VAdcAdjustPhase(BYTE mode)
//!{
//!	//WriteTW88Page(PAGE1_VADC );
//!	if(InputVAdcMode >= EE_YUVDATA_START) {
//!	}
//!	else {
//!		if(mode==10) VAdcSetPhase(3, 0);	//WriteTW88(REG1C5, 3); 
//!		if(mode==18) VAdcSetPhase(17, 0);	 
//!	}
//!}
#endif

//!#if !defined(SUPPORT_COMPONENT) && !defined(SUPPORT_PC)
//!//----------------------------
//!//Trick for Bank Code Segment
//!//----------------------------
//CODE BYTE DUMMY_VADC_CODE;
//void Dummy_VADC_func(void)
//{
//	BYTE temp;
//	temp = DUMMY_VADC_CODE;
//}
//!#else //..!defined(SUPPORT_COMPONENT) && !defined(SUPPORT_PC)
//! other real code
//!#endif


							//       1      2       3      4      5      6      7      8      9     10
							//   	480i,  576i,   480p, 576p,1080i50,1080i60,720p50,720p60,1080p5,1080p6
#if 0
//scaled
code	WORD	YUVDividerPLL[] = { 858,   864,   858,   864,   2460,  2200,  1980,  1650,  2640,  2200 };
code	WORD	YUVVtotal[]     = { 262,   312,   525,   625,   562,   562,   750,   750,   1124,  1124 };

code	BYTE	YUVClampPos[]   = { 128,   128,   64,    58,    40,    32,    38,    38,    14,    14 };		// 0x1D7

code	WORD	YUVCropH[]      = { 720,   720,   720,   720,   1920,  1920,  1280,  1280,  1920,  1920 };
code	WORD	YUVCropV[]      = { 240,   288,   480,   576,   540,   540,   720,   720,   1080,  1080 };
code	WORD	YUVDisplayH[]   = { 700,   700,   700,   700,   1880,  1880,  1260,  1260,  1880,  1880 };		// 0x042[3:0],0x046
code	WORD	YUVDisplayV[]   = { 230,   278,   460,   556,   520,   520,   696,   696,   1040,  1040 };		// 0x042[6:4],0x044

code	WORD	YUVStartH[]     = { 112,   126,   114,   123,   230,   233,   293,   293,   233,   233 };		// 0x040[7:6],0x045 InputCrop
code	WORD	YUVStartV[]     = { 1,     1,     2,     2,     2,     2,     2,     2,     2,     2 };			// 0x043 InputCrop
code	BYTE	YUVOffsetH[]    = { 5,     4,     10,    6,     40,    40,    20,    20,    30,    30 };
code	BYTE	YUVOffsetV[]    = { 48,    48,    48,    48,    28,    26,    24,    25,    26,    26 };		// use as V-DE 0x215	
code	BYTE	YUVScaleVoff[]  = { 128,   128,   0,     0,     128,   128,   0,     0,     0,     0 };

code	WORD	MYStartH[]      = { 121,   131,   121,   131,   
	235-44,   
	235-44,   
	299-40,   
	299-40,   
	235-44,   
	235-44 };		// 0x040[7:6],0x045 InputCrop
code	WORD	MYStartV[]      = { 19,   21,   38,   44,   
	20,	20,   
	25, 25,   
	41, 41 };	

#endif //.#else
#if 0
							//   	480i,  576i,   480p, 576p,1080i50,1080i60,720p50,720p60,1080p5,1080p6
//total scan pixel & line
code	WORD	YUVDividerPLL[] = { 858,   864,   858,   864,   2460,  2200,  1980,  1650,  2640,  2200 };		//total horizontal pixels
code	WORD	YUVVtotal[]     = { 262,   312,   525,   625,   562,   562,   750,   750,   1124,  1124 };		//total vertical scan line

code	BYTE	YUVClampPos[]   = { 140,   140,   52,    58,    24,    32,    38,    38,    14,    14 };		//R1D7. clamp position offset.

//reduced resolution.
code	WORD	YUVDisplayH[]   = { 720,   720,   720,   720,   1920,  1920,  1280,  1280,  1920,  1920 };		// R042[3:0]R046[7:0] for overscan
code	WORD	YUVDisplayV[]   = { 240,   288,   480,   576,   540,   540,   720,   720,   1080,  1080 };		// R042[6:4]R044[7:0] for overscan

//resolution
code	WORD	YUVCropH[]      = { 720,   720,   720,   720,   1920,  1920,  1280,  1280,  1920,  1920 };		// horizontal resolution
code	WORD	YUVCropV[]      = { 240,   288,   480,   576,   540,   540,   720,   720,   1080,  1080 };		// vertical resolution

code	WORD	YUVStartH[]     = { 121-16,131-16,121-16,131-16,235-16,235-16,299-16,299-16,   235-16,235-16 };		// 0x040[7:6],0x045 InputCrop
code	WORD	YUVStartV[]     = { 1,     1,     2,     2,     2,     2,     2,     2,     2,     2 };			// 0x043 InputCrop

code	BYTE	YUVOffsetH[]    = { 5,     4,     10,    6,     40,    40,    20,    20,    30,    30 };
code	BYTE	YUVOffsetV[]    = { 42,    40,    40,    38,    20,    20,    18,    18,    10,    10 };		// use as V-DE 0x215	
//=>VDE value
							//   	480i,  576i,   480p, 576p,1080i50,1080i60,720p50,720p60,1080p5,1080p6
code	BYTE  YUV_VDE_NOSCALE[] = { 21,    24,    40,    46,    22,    22,    27,    27,    22,    22 };		// use as V-DE 0x215	



code	BYTE	YUVScaleVoff[]  = { 128,   128,   0,     0,     128,   128,   0,     0,     0,     0 };


code	WORD	MYStartH[]      = { 121,   131,   121,   131,   
	235-44,   
	235-44,   
	299-40,   
	299-40,   
	235-44,   
	235-44 };		// 0x040[7:6],0x045 InputCrop
code	WORD	MYStartV[]      = { 19,   21,   38,   44,   
	20,	20,   
	25, 25,   
	41, 41 };	
#endif

//=============================================================================
// REMOVED
//=============================================================================

//void SPI_cmd_protocol(BYTE max, ...)
#if 0
BYTE SPI_QUADInit_Test(void)
{
	BYTE ret;
	BYTE temp;
	ret = SFLASH_VENDOR_MICRON;

	SPI_cmd_protocol(2,	3, 0x9f);
	SPI_cmd_protocol(2,	1,5);
	SPI_cmd_protocol(5,	8,3,0,0,0);	 //BUGBUG......

	temp=SPI_cmd_protocol(2,	1,0x85);
	if((temp&0xF0)==0x60)
		Puts("\nOK.6 dummy clock @V");
	else
		Puts("\nFAIL.6 dummy clock @V");

	temp=SPI_cmd_protocol(2,	1,0x65);
	if(temp&0x80)
		Puts("\nFAIL.QuadIO@VE");
	else
		Puts("\nOK.QuadIO@VE");

	SPI_cmd_protocol(2,	2,0xB5);

	WriteTW88Page(PAGE4_SPI);
	temp = ReadTW88(REG4D0);
	if((temp&0xF0)==0x60)
		Puts("\nOK.6 dummy clock @NV");

 	temp = ReadTW88(REG4D1);
	if(temp&0x04)
		Puts("\nFAIL. QuadIO@NV");
	else
		Puts("\nOK.QuadIO@NV");
#if 0
	temp = SPI_cmd_protocol(2,	1,0xB5);			//read NV cof reg
	SPI_cmd_protocol(2,	0,6);						//write enable
	SPI_cmd_protocol(3,	0,0xB1, temp & ~0x08);		//update NV cof reg with Quid Input
	SPI_cmd_protocol(2,	1,0xB5);					//read NV cof reg
#endif

#if 0
	temp = SPI_cmd_protocol(2,	1,0x85);			//read V cof reg
	SPI_cmd_protocol(2,	0,6);						//write enable
	SPI_cmd_protocol(3,	0,0x81, temp & ~0x08);		//update V cof reg with Quid Input
	SPI_cmd_protocol(2,	1,0x85);					//read V cof reg
#endif

#if 0
	temp = SPI_cmd_protocol(2,	1,0x65);			//read VE cof reg
	SPI_cmd_protocol(2,	0,6);						//write enable
	SPI_cmd_protocol(2,	0,0x61, temp & ~0x80);		//update VE cof reg with Quid Input
	SPI_cmd_protocol(2,	1,0x65);					//read VE cof reg
#endif
	return 0;
}
#endif

//=============================================================================
//=============================================================================
// REMOVED
//=============================================================================
//=============================================================================
#if 0 //BK111013
//BKTODO111013 I don't know why it use a fixed value ????
void ScalerSetDeOnFreerun(void)
{
	BYTE fOn=OFF;
	BYTE HDE_value = 40;
	BYTE VDE_value = 48;
	WORD VTotal;

	switch(InputMain) {
	case INPUT_CVBS:
	case INPUT_SVIDEO:
		break;
	case INPUT_COMP:
	//V-DE:0
//!		wTemp = ScalerCalcHDE();
//!		ScalerWriteHDEReg(wTemp, PANEL_H);
//!		wTemp = FPGA_GetVDE();
//!		if(wTemp < 1)
//!			wTemp = 1;
//!		ScalerSetVDEPosHeight(wTemp,  PANEL_V);
		HDE_value = 40;
		VDE_value = 48;
		HDE_value=ScalerReadHDEReg();
		VDE_value=ScalerReadVDEReg();
		fOn = ON;
		break;
	case INPUT_PC:
	//V-DE:0
//!		wTemp = ScalerCalcHDE();
//!		ScalerWriteHDEReg(wTemp, PANEL_H);
//!		//----------------------
//!		//wTemp = FPGA_GetVDE();
//!		//if(wTemp<1)	 //temp
//!		//	wTemp=1;
//!		//----------------------
//!		wTemp = ScalerCalcVDE();
//!		ScalerSetVDEPosHeight(wTemp,  PANEL_V);
		HDE_value = 40;
		VDE_value = 48;
		fOn = ON;
		break;
	case INPUT_DVI:
//!		wTemp = ScalerCalcHDE();
//!		ScalerWriteHDEReg(wTemp, PANEL_H);
//!		wTemp = FPGA_GetVDE();
//!		ScalerSetVDEPosHeight(wTemp,  PANEL_V);
		HDE_value = 40;
		VDE_value = 48;
		fOn = ON;
		break;
	case INPUT_HDMIPC:
	case INPUT_HDMITV:
//!		wTemp = ScalerCalcHDE();
//!		ScalerWriteHDEReg(wTemp, PANEL_H);
//!		wTemp = FPGA_GetVDE();
//!	wTemp = 35;
//!		ScalerSetVDEPosHeight(wTemp,  PANEL_V);
		HDE_value = 40;
		VDE_value = 48;
		fOn = ON;
		break;
	case INPUT_BT656:
//!		wTemp = ScalerCalcHDE();
//!		ScalerWriteHDEReg(wTemp, PANEL_H);
//!		wTemp = FPGA_GetVDE();
//!		if(wTemp < 3)
//!			wTemp = 2;
//!		ScalerSetVDEPosHeight(wTemp,  PANEL_V);
		HDE_value = 40;
		VDE_value = 48;
		fOn = ON;
		break;
	default:
		break;
	}

	dPrintf("\nScalerSetDeOnFreerun HDE:%bd VDE:%bd",	ScalerReadHDEReg(),ScalerReadVDEReg());



	if(fOn) {
	
		VTotal = ScalerReadFreerunVtotal();
		if(VDE_value >= (VTotal - PANEL_V)) {
			VDE_value = VTotal - PANEL_V -1;
		}
		WaitVBlank(1); 	//InitLogo1 need it

		ScalerWriteHDEReg(HDE_value);
		ScalerWriteVDEReg(VDE_value);
		SpiOsdSetDeValue();	//InitLogo1 need it
	}
}
#endif


#if 0
//use x100 value for floating point
void ScalerSetHScale100(WORD Length)	
{
	DWORD	temp;

	WriteTW88Page(PAGE2_SCALER);

	if(PANEL_H >= (Length/100)) { 					
		//UP SCALE
		temp = (DWORD)Length * 0x2000L;
		temp /= 100;
		temp /= PANEL_H;
		ScalerWriteXUpReg(temp);				//set up scale
		ScalerWriteXDownReg(0x0400);			//clear down scale
		dPrintf("\nScalerSetHScale100(%d) UP:0x0400 DN:0x%04lx",Length, temp);
	}
	else {										
		//DOWN SCALE
		temp = (DWORD)Length * 0x0400L;						
		temp /= 100;
		temp /= PANEL_H;
		ScalerWriteXUpReg(0x2000);			//clear up scale
		ScalerWriteXDownReg(temp);			//set down scale
		dPrintf("\nScalerSetHScale100(%d) UP:0x%04lx DN:0x2000",Length, temp);
	}
}
#endif

#ifdef UNCALLED_SEGMENT
WORD GetHScaledRatio(WORD length)
{...}
#endif

#if 0
//use x100 for floating point
void ScalerSetVScale100(WORD Length)
{
	DWORD	temp;

	WriteTW88Page(PAGE2_SCALER);

	temp = Length * 0x2000L;
	temp += (PANEL_V / 2);
	temp /= PANEL_V;
	temp /= 100;
	dPrintf("\nScalerSetVScale(%d) 0x%04lx",Length, temp);

	ScalerWriteVScaleReg(temp);
}
#endif

//BKTODO: It comes from TW8823.
//	use ScalerSetVScale() & ScalerSetVDEPosHeight() with GetVScaledRatio()
//offset for V back porch
#ifdef UNCALLED_SEGMENT
void ScalerSetVScaleWithOffset(WORD Length, BYTE offset)
{
	DWORD	temp;

	WriteTW88Page(PAGE2_SCALER);

	temp = Length * 0x2000L;
	temp /= PANEL_V;
	temp += offset;

	dPrintf("\nScalerSetVScale(%d,%bd) 0x%04lx",Length, offset, temp);

	ScalerWriteVScaleReg(temp);
}
#endif

#if 0
WORD GetVScaledRatio(WORD length)
{
	DWORD dTemp;
	WORD wTemp;
	WORD wResult;
	WORD wTest;

	dPrintf("\nGetVScaledRatio(%d)",length);

	dTemp = length * 0x2000L;
	wTemp = ScalerReadVScaleReg();
	dPrintf(" ratio:	8192/%d",wTemp);

	wResult = (WORD)(dTemp / wTemp);
	dTemp += 0x1000L; //add (0x2000/2)  ..roundup..
	wTest = (WORD)(dTemp / wTemp);

	dPrintf(" result:%d test:%d",wResult,wTest);
	return wResult;
}
#endif


//old name: WORD FPGA_GetHDE(void)
#if 0
WORD ScalerCalcHDE___OLD(void)
{
	WORD wTemp;
	BYTE PCLKO;

	WriteTW88Page(PAGE2_SCALER );
	wTemp = ReadTW88(REG20b);
	PCLKO = ReadTW88(REG20d) & 0x03;
	if(PCLKO==3)
		PCLKO = 2;

#if 0
	return wTemp+32;
#else //new 110624
	return wTemp+33 - PCLKO;
#endif
}
#endif

//Note: it is available after meas.

/*
VStart = REG(0x536[7:0],0x537[7:0])
VPulse = REG(0x52a[7:0],0x52b[7:0])
VPol = REG(0x041[3:3])
VScale = REG(0x206[7:0],0x205[7:0])

result = ((VStart - (VPulse * VPol)) * 8192 / VScale) + 1
*/

#if 0
WORD ScalerCalcVDE___OLD(void)
{
	BYTE VPol;
	WORD VStart,VPulse,VScale;
	WORD wResult;

	WriteTW88Page(PAGE5_MEAS);
	Read2TW88(REG536,REG537, VStart);
	Read2TW88(REG52A,REG52B, VPulse);

	WriteTW88Page(PAGE0_INPUT);
	VPol = ReadTW88(REG041) & 0x08 ? 1: 0;
	
	VScale = ScalerReadVScaleReg();

	wResult = ((DWORD)(VStart - (VPulse * VPol)) * 8192 / VScale) + 1;
	return wResult;
}
#endif

#if 0
WORD GetCalcVDEStart(void)		??sameas ScalerCalcVDE
{
	WORD VStart,VPulse,VScale;
	BYTE VPol;
	WORD wResult;
	DWORD dTemp;

	dPrintf("\nGetVScaledRatio()");

	WriteTW88Page(PAGE2_SCALER);
	Read2TW88(REG206,REG207, VScale);

	WriteTW88Page(PAGE5_MEAS);
	Read2TW88(REG536,REG537, VStart);
	Read2TW88(REG52a,REG52B, VPulse);

	
	WriteTW88Page(PAGE0_INPUT);
	VPol = ReadTW88(REG041) & 0x08 ? 1: 0;

	dPrintf("VStart:%d VPulse:%d VPol:%bd VScale:%d ",VStart,VPulse,VPol,VScale);

	//wResult = ((VStart - (VPulse*VPol)) * 8192 / VScale) + 1;
	dTemp = VStart - (VPulse*VPol);
	dTemp *= 8192;
	dTemp /= VScale;
	dTemp += 1;
	wResult = (WORD)dTemp;
	dPrintf(" result:%d",wResult);
	
	return wResult;
}
#endif

//===================================================================
//
//===================================================================

#ifdef UNCALLED_SEGMENT
//parameter
//	type - input
//			0:CVBS+NTSC
//			1:CVBS+PAL
//			2:RGB + SVGA
void ScalerSetDefault(BYTE type)
{
	if(type==0) {
		//CVBS+NTSC		720x480
		ScalerSetHScale(720);		//input:720. line_buff:720 output:Panel:800 
		ScalerSetVScale(240-15);	//??-15	ScalerSetVScale(240);	//480 with interlaced.

		ScalerSetLineBuffer(0x62, 720);	 	//98, 720
		ScalerWriteOutputHBlank(2);

		ScalerWriteHDEReg(0x84);			// position:132, size:0x320=800(PANEL_H)
		ScalerWriteVDEReg(0x30);			//ScalerSetVDEPosHeight(0x2c, PANEL_V);		// 44, 0x1e0=480(PANEL_V)
		//ScalerSetOutputWidthAndHeight(PANEL_H,PANEL_V);
		ScalerSetHSyncPosLen(0, 1);				//ScalerSetHSyncPosLen(0, 4);				//position:0 len:4
		ScalerSetVSyncPosLen(0,5);
		ScalerSetOutputFixedVline(OFF /*,0*/);			//off
		ScalerSetVDEMask(0,0);
	}
	else if(type==1) {
		//CVBS+PAL		720x576

		//need a default settings

		ScalerSetHScale(720);
		ScalerSetVScale(288);	//576 with interlaced.

		ScalerSetLineBuffer(0x62, 720);	 	//98, 720
		ScalerWriteOutputHBlank(2);

		ScalerWriteHDEReg(0x84);					//
		ScalerWriteVDEReg(0x30);					//28
		//ScalerSetOutputWidthAndHeight(PANEL_H,PANEL_V);
		ScalerSetHSyncPosLen(0, 1);				//ScalerSetHSyncPosLen(0, 4);				//position:0 len:4
		ScalerSetVSyncPosLen(0,5);
		ScalerSetOutputFixedVline(OFF /*,0*/);			//off
		ScalerSetVDEMask(0,0);
	}
	else /* if(type==2) */ {	//RGB + SVGA

		//need a default settings

		ScalerSetHScale(PANEL_H);
		ScalerSetVScale(PANEL_V);

		ScalerSetLineBuffer(0x32, PANEL_H);	// 50, 800
		ScalerWriteOutputHBlank(2);

		ScalerWriteHDEReg(0x53);					//83
		ScalerWriteVDEReg(0x1c);					//28
		//ScalerSetOutputWidthAndHeight(PANEL_H,PANEL_V);
		ScalerSetHSyncPosLen(0, 1);				//ScalerSetHSyncPosLen(0, 4);				//position:0 len:4
		ScalerSetVSyncPosLen(0, 1);
	}
}
#endif


//R202

//R207, R208

//called from measure
//BKTODO - it is a TW8823 version
//parameter
//	length: HAN(HorizontalActiveNumber)
#if 0
void	SetHScaleFull( WORD Length, WORD VPeriod, DWORD VPeriod27 )
{
	dPrintf( "\nSetHScaleFull(Length:%d, VPeriod:%d, VPeriod27:%ld)", Length, VPeriod, VPeriod27 );
}
#endif


//=============================================================================
//	YPbPr Table
//=============================================================================
//								 720    720    720    720 	1920   1280   1920
//							     480i,  576i,  480p, 576p,  1080i, 720p,  1080p
//code	BYTE	ClampPos[]   = { 140,   140,   52,    58,    24,    38,    24 };		//BKTODO:I need it for aPC also.

/*

SDTV 480i/60M
	 576i/50	
	 480p SMPTE 267M-1995
HDTV 1080i/60M
	 1080i/50
	 720p/60M
	 720p/50
	 1080p = SMPTE 274M-1995 1080p/24 & 1080p/24M
	                         1080p/50 1080p/60M


			scan lines	 field1 field2	 half
480i/60M	525			 23~262 285~524	 142x
576i/50		625			 23~310 335~622
1080i		1125
720p		750

standard
480i/60M	SMPTE 170M-1994.
			ITU-R BT.601-4
			SMPTE 125M-1995
			SMPTE 259M-1997
*/

//=============================================================================
//JUNK
//=============================================================================
//test 2BPP intersil
//!code WORD consolas16x26_606C90_2BPP[4] = {
//!	0xF7DE,0x0000,0x5AAB,0xC000
//!}; 
//!void FOsdIntersil(BYTE winno)
//!{
//!	BYTE palette;
//!	DECLARE_LOCAL_page
//!	BYTE i;
//!
//!	ReadTW88Page(page);
//!
//!	WaitVBlank(1);
//!	FOsdWinEnable(winno,OFF);	//winno disable
//!
//!	FOsdWinScreenXY(winno, 0,26);	//start 0x,0x  4 colums 1 line
//!	FOsdWinScreenWH(winno, 4, 1);	//start 0x,0x  4 colums 1 line
//! 	FOsdWinZoom(winno, 0, 0);			//zoom 1,1
//!	FOsdWinMulticolor(winno, ON);
//!
//!	WriteTW88Page(PAGE3_FOSD );
//!
//!	palette = 40;
//!	FOsdSetPaletteColorArray(palette,consolas16x26_606C90_2BPP,4, 0);
//!	FOsdRamSetAddrAttr(120, palette>>2);	//addr,palette,mode
//!	for(i=0; i < 4; i++) {
//!		WriteTW88(REG307, BPP2_START+4 + i*2);	//intersil icon
//!	}
//!
//!	FOsdWinEnable(winno,ON);		//winno enable
//!	WriteTW88Page(page);
//!}


//=============================================================================
// 
//=============================================================================
//!#if 0
//!void FOsdRam_Set(WORD index, BYTE ch, BYTE attr, BYTE len)
//!{
//!	BYTE i,bTemp;
//!	WORD addr;
//!	
//!	WriteTW88(REG304, ReadTW88(REG304) & ~0x0D);
//!	for(i=0; i < len; i++) {
//!		addr = index + i;
//!		bTemp = ReadTW88(REG305) & 0xFE;
//!		if(addr > 0x100)
//!			bTemp |= 0x01;
//!		WriteTW88(REG305, bTemp);
//!
//!		WriteTW88(REG306, (BYTE)addr);	//FOsdRamSetAddress(OsdRamAddr);
//!		WriteTW88(REG307, ch);
//!
//!		WriteTW88(REG306, (BYTE)addr);	//FOsdRamSetAddress(OsdRamAddr);
//!		WriteTW88(REG308, attr );	
//!	}
//!}
//!
//!void FOsdRam_Clear(WORD index, BYTE bg_color_index, BYTE len)
//!{
//!	FOsdRam_Set(index,FOSD_ASCII_BLANK,bg_color_index,len);
//!}
//!#endif

//!struct FWIN_s {
//!	BYTE no;
//!	BYTE col, row;
//!	BYTE attr;
//!	WORD start;
//!	WORD addr;
//!} fwin;

//assume bank3. AutoInc
//!#if 0
//!static void FOsdRam_goto(BYTE win, BYTE w,BYTE y)
//!{
//!	WORD addr;
//!	BYTE bTemp;
//!
//!	addr = fwin.start+fwin.col*y+w;
//!	fwin.addr = addr;
//!
//!	FOsdRamSetAddress(addr);
//!}
//!
//!static void FOsdRam_setAttr(BYTE attr, BYTE cnt, BYTE auto_mode )
//!{
//!	WORD addr;
//!	BYTE i,j;
//!
//!	addr = fwin.addr;
//!	fwin.attr = attr;
//!
//!	FOsdSetAccessMode(FOSD_OSDRAM_WRITE_AUTO);	//WriteTW88(REG304, (ReadTW88(REG304)&0xF3)|0x04); // Auto addr increment with D or A
//!	FOsdRamSetAddress(addr);
//!
//!	if(cnt) {
//!		for (i=0; i<(cnt/8); i++) {
//!			for ( j=0; j<8; j++ )
//!				WriteTW88(REG308, attr);
//!			delay1ms(1);
//!		}
//!		for ( j=0; j<(cnt%8); j++ )
//!			WriteTW88(REG308, attr);
//!	}
//!	else {
//!		WriteTW88(REG308, attr);
//!	}
//!
//!	if(auto_mode==3) {
//!	    //reset addr for data
//!		WriteTW88(REG304, ReadTW88(REG304) | 0x0C); // Auto addr increment with wit previous attr
//!		FOsdRamSetAddress(addr); 
//!	}
//!}
//!#endif
//!
//!//assume bank3.
//!#if 0
//!static void FOsdRam_putch(WORD ch)
//!{
//!   	if(ch < 0x100)
//!		WriteTW88(REG304,ReadTW88(REG304) & ~0x20);	//lower	
//!	else
//!		WriteTW88(REG304,ReadTW88(REG304) | 0x20);	//UP256
//!	WriteTW88(REG307, (BYTE)ch);	
//!}
//!#endif
//!
//!#if 0
//!static void FOsdRam_puts(WORD *s)
//!{
//!	BYTE hi = 0;
//!
//!   	if(*s < 0x100) {
//!		WriteTW88(REG304,ReadTW88(REG304) & ~0x20);	//lower	
//!	}
//!	while(*s) {
//!		if(hi==0 && *s >= 0x100) {
//!			hi=1;
//!			WriteTW88(REG304,ReadTW88(REG304) | 0x20); //UP256
//!		}
//!		WriteTW88(REG307,(BYTE)*s++);
//!	}
//!}
//!#endif


//!#if 0
//!void FOsdDownloadFontCode( void )
//!{
//!#ifdef SUPPORT_UDFONT
//!BYTE	i, j
//!#endif
//!BYTE    page;
//!
//!	ReadTW88Page(page);
//!	WaitVBlank(1);	
//!	McuSpiClkToPclk(0x02);	//with divider 1=1.5(72MHz). try 2
//!
//!	WriteTW88Page(PAGE3_FOSD );
//!
//!	WriteTW88(REG350, 0x09 );					// default FONT height: 18 = 9*2
//!
//!	WriteTW88(REG300, ReadTW88(REG300) & 0xFD ); // turn OFF bypass for Font RAM
//!	WriteTW88(REG309, 0x00 ); //Font Addr
//!
//!	FOsdSetAccessMode(FOSD_ACCESS_FONTRAM);	
//!
//!=======================
//!#ifdef SUPPORT_UDFONT
//!	FOsdFontWrite(0x00,&ROMFONTDATA[0][0], 27, 0xA0);
//!	FOsdFontWrite(0xA0,&RAMFONTDATA[0][0], 27, 0x22);
//!	FOsdFontWrite(0xC2,&RAMFONTDATA[0x82][0], 27, 0x60-0x22);
//!#endif
//!========================
//!
//!
//!#ifdef SUPPORT_UDFONT
//!	i = 0;
//!	for ( i=0; i<0xA0; i++ ) {
//!		WriteTW88(REG309, i);
//!
//!		for ( j = 0; j<27; j++ ) {
//!			WriteTW88(REG30A, ROMFONTDATA[i][j] );
//!		}
//!	}
//!	for ( i=0; i<0x22; i++ ) {
//!		WriteTW88(REG309, i+0xa0);
//!
//!		for ( j = 0; j<27; j++ ) {
//!			WriteTW88(REG30A, RAMFONTDATA[i][j] );
//!		}
//!	}
//!	for ( i=0; i<(0x60-0x22); i++ ) {
//!		WriteTW88(REG309, i+0x22+0xa0);
//!
//!		for ( j = 0; j<27; j++ ) {
//!			WriteTW88(REG30A, RAMFONTDATA[i+0x82][j] );
//!		}
//!	}
//!#endif
//!	FOsdSetAccessMode(FOSD_ACCESS_OSDRAM);	
//!
//!	WriteTW88(REG30B, 0xF0 );	  					// 2bit color font start
//!	WriteTW88(REG_FOSD_MADD3, 0xF0 );
//!	WriteTW88(REG_FOSD_MADD4, 0xF0 );
//!
//!	McuSpiClkRestore();
//!	WriteTW88Page(page );
//!}
//!#endif
//!
//!#if 0
//!void FOsdDownloadFont2Code( void )
//!{
//!BYTE	i, j, page;
//!
//!	ReadTW88Page(page);
//!	WaitVBlank(1);	
//!	McuSpiClkToPclk(0x02);	//with divider 1=1.5(72MHz). try 2
//!
//!	WriteTW88Page(PAGE3_FOSD );
//!
//!	WriteTW88(REG_FOSD_CHEIGHT, 0x09 );					// default FONT height: 18 = 9*2
//!
//!	WriteTW88(REG300, ReadTW88(REG300) & 0xFD ); // turn OFF bypass for Font RAM
//!	WriteTW88(REG309, 0x00 ); //Font Addr
//!
//!	i = 0;
//!	FOsdSetAccessMode(FOSD_ACCESS_FONTRAM);	
//!#ifdef SUPPORT_UDFONT
//!	for ( i=0; i<0xA0; i++ ) {
//!		WriteTW88(REG309, i);
//!
//!		for ( j = 0; j<27; j++ ) {
//!			WriteTW88(REG30A, RAMFONTDATA[i][j] );
//!		}
//!	}
//!#endif
//!	FOsdSetAccessMode(FOSD_ACCESS_OSDRAM);
//!
//!	WriteTW88(REG30B, 0xF0 );	  					// 2bit color font start
//!	WriteTW88(REG_FOSD_MADD3, 0xF0 );
//!	WriteTW88(REG_FOSD_MADD4, 0xF0 );
//!
//!	McuSpiClkRestore();
//!	WriteTW88Page(page );
//!}
//!#endif
//!
//!
//!
//!
//!
//!
//!//with Attr
//!//without Attr
//!//void FOsdRamWriteStr(WORD addr, BYTE *str, BYTE len)
//!//{
//!//}
//!
//!//void FontOsdWinChangeBackColor(BYTE index, WORD color)
//!//{
//!//}
//!
//!
//!//bank issue
//!//ex:
//!//	for(i=0; i < 8; i++)
//!//		FontOsdBpp3Alpha_setLutOffset(i,your_table[i]);	
//!#if 0
//!void FontOsdBpp3Alpha_setLutOffset(BYTE i, BYTE order)
//!{
//!	BPP3_alpha_lut_offset[i] = order;
//!}
//!#endif
//!
//!#ifdef UNCALLED_SEGMENT_CODE
//!void FontOsdWinAlphaGroup(BYTE winno, BYTE level)
//!{
//!}
//!#endif
//!
//!
//!
//!#if 0
//!void FOsdWriteAllPalette(WORD color)
//!{
//!	BYTE i;
//!	BYTE r30c;
//!	BYTE page;
//!
//!	ReadTW88Page(page);
//!
//!	McuSpiClkToPclk(CLKPLL_DIV_2P0);
//!
//!	WriteTW88Page(PAGE3_FOSD );
//!	r30c = ReadTW88(REG30C) & 0xC0;
//!	for(i=0; i < 64; i++) {
//!		WriteTW88(REG30C, r30c | i );
//!		WriteTW88(REG30D, (BYTE)(color>>8));
//!		WriteTW88(REG30E, (BYTE)color);
//!	}
//!	
//!	McuSpiClkRestore();
//!		
//!	WriteTW88Page(page);
//!}
//!#endif

//void SPI_ReadData2xdata     ( DWORD spiaddr, BYTE *ptr, DWORD cnt );
//==>SpiFlashDmaRead(DMA_DEST_MCU_XMEM,(WORD)(BYTE *)ptr, DWORD spiaddr, DWORD cnt);
#if 0
// TODO: pls use, (dest,src,len)
void SPI_ReadData2xdata( DWORD spiaddr, BYTE *ptr, DWORD cnt )
{
	WORD xaddr;

	xaddr = (WORD)ptr;

	WriteTW88Page(PAGE4_SPI );					// Set Page=4

#ifdef FAST_SPIFLASH
	WriteTW88(REG4C3, 0xC0 | CMD_x_BYTES);	//DMAMODE_R_XDATA );		// Mode = SPI -> incremental xdata
	WriteTW88(REG4CA, CMD_x_READ );			// Read Command

	WriteTW88(REG4CB, spiaddr>>16 );			// SPI address
	WriteTW88(REG4CC, spiaddr>>8 );				// SPI address
	WriteTW88(REG4CD, spiaddr );				// SPI address

 	WriteTW88(REG4C6, xaddr>>8 );				// Buffer address
	WriteTW88(REG4C7, xaddr );					// Buffer address

	WriteTW88(REG4DA, cnt>>16 );				// Read count
 	WriteTW88(REG4C8, cnt>>8 );					// Read count
	WriteTW88(REG4C9, cnt );					// Read count

	WriteTW88(REG4C4, 0x01);					//DMA-Read start
#else

	

	//dPrintf("\nSPI_ReadData2xdata(%lx,,%lx)",spiaddr,cnt);
	//dPrintf(" CMD:%02bx cmdlen:%bd",CMD_x_READ,CMD_x_BYTES);

	SpiFlashDmaDestType(DMA_DEST_MCU_XMEM,0);
	SpiFlashCmd(SPICMD_x_READ, SPICMD_x_BYTES);
	SpiFlashDmaFlashAddr(spiaddr);
	SpiFlashDmaBuffAddr(xaddr);
	SpiFlashDmaReadLen(cnt);
	SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
#endif
}
#endif

//=============================================================================
//
//=============================================================================

//void SPI_quadio(void)
//{
//	SPI_WriteEnable();
//  
//	WriteTW88Page(PAGE4_SPI );			// Set Page=4
//
//	WriteTW88(REG4C3, 0x42 );			// Mode = command write, Len=2
//	WriteTW88(REG4CA, 0x01 );			// SPI Command = WRITE_ENABLE
// 	WriteTW88(REG4C8, 0x40 );			// Read count
//	WriteTW88(REG4C4, 0x03 );			// DMA-Write start
//}


//=============================================================================
//		SPI DMA (SPI --> Fixed Register)
//=============================================================================
#if 0
void SPI_ReadData2Reg( WORD index, DWORD spiaddr, DWORD size )
{
	WriteTW88Page(PAGE4_SPI);
#if 0
	WriteTW88(REG403, DMAMODE_RW_FIX_REG);		// Mode = SPI -> fixed register

	WriteTW88(REG40a, SPICMD_x_READ );			// Read Command

	WriteTW88(REG40b, spiaddr>>16 );			// SPI address
	WriteTW88(REG40c, spiaddr>>8 );				// SPI address
	WriteTW88(REG40d, spiaddr );				// SPI address

	WriteTW88(REG406, index>>8 );				// Buffer address
	WriteTW88(REG407, index );					// Buffer address

	WriteTW88(REG41a, size>>16 );					// Read count
	WriteTW88(REG408, size>>8 );					// Read count
	WriteTW88(REG409, size );						// Read count
	
	WriteTW88(REG404, 0x01 );					// DMA-Read start
#else
//	SpiFlashDmaDestType(dest_type,0);
//BUGBUG: only support slow & fast.
	SpiFlashCmd(SPICMD_x_READ, SPICMD_x_BYTES);
	SpiFlashDmaFlashAddr(spiaddr);
	SpiFlashDmaBuffAddr(index);
	SpiFlashDmaReadLen(size);						
	SpiFlashDmaStart(SPIDMA_READ, SPIDMA_BUSYCHECK, __LINE__);
#endif
}
#endif
//=============================================================================
//		SPI DMA (SPI --> Incremental Register)
//=============================================================================
/*
void SPI_ReadData2RegInc( WORD index, DWORD spiaddr, DWORD cnt )
{
	WriteTW88Page(PAGE4_SPI );				// Set Page=5

	WriteTW88(REG403, DMAMODE_RW_INC_REG );		// Mode = SPI -> incremental register
	WriteTW88(REG40a, SPICMD_x_READ );				// Read Command
	WriteTW88(REG40b, spiaddr>>16 );				// SPI address
	WriteTW88(REG40c, spiaddr>>8 );				// SPI address
	WriteTW88(REG40d, spiaddr );					// SPI address
 	WriteTW88(REG406, index>>8 );				// Buffer address
	WriteTW88(REG407, index );					// Buffer address
	WriteTW88(REG41a, cnt>>16 );					// Read count
 	WriteTW88(REG408, cnt>>8 );					// Read count
	WriteTW88(REG409, cnt );						// Read count

	WriteTW88(REG404, 0x01 );					// DMA-Read start
}
*/

//=============================================================================
//		SPI DMA (SPI --> Incremental XDATA)
//=============================================================================
//#include <intrins.h>
//	_nop_();

//---------------------------------------------
//description
//	input data format selection
//	if input is PC(aRGB),DVI,HDMI, you have to set.
//@param
//	0:YCbCr 1:RGB
//
//CVBS:0x40
//SVIDEO:0x54. IFSET:SVIDEO, YSEL:YIN1
//---------------------------------------------
#if 0
void DecoderSetPath(BYTE path)
{
	WriteTW88Page(PAGE1_DECODER );	
	WriteTW88(REG102, path );   		
}
#endif

//---------------------------------------------
//
//@param
//	input_mode	0:RGB mode, 1:decoder mode
//register
//	R105
//---------------------------------------------
#if 0
void DecoderSetAFE(BYTE input_mode)
{
	WriteTW88Page(PAGE1_DECODER );	
	if(input_mode==0) {
		WriteTW88(REG105, (ReadTW88(REG105) & 0xF0) | 0x04);	//? C is for decoder, not RGB	
	}
	else {
		WriteTW88(REG105, (ReadTW88(REG105) | 0x0F));	
	}
}
#endif

#if 0
void DumpRegister(BYTE page)
{
#ifdef SUPPORT_8BIT_CHIP_ACCESS
	BYTE i,j;
	WORD linestart;
	WriteTW88Page(page);

	Printf("\nP%02bd",page);
	for(i=0; i < 16; i++)
		Printf("-%01bx-", i);

	for(i=0; i < 16; i++) {
		linestart = page << 4;
		linestart += i;
		linestart <<= 4;
		Printf("\n%03x:",linestart);
		for(j=0; j < 16; j++) {
			Printf("%02bx ",ReadTW88(i*16+j));
		}
	}
#else
	BYTE i,j;
	WORD linestart;
	WORD addr;

	Printf("\nP%02bd",page);
	for(i=0; i < 16; i++)
		Printf("-%01bx-", i);

	for(i=0; i < 16; i++) {
		linestart = page << 4;
		linestart += i;
		linestart <<= 4;
		Printf("\n%03x:",linestart);
		addr = page;
		addr <<= 8;
		for(j=0; j < 16; j++) {
			Printf("%02bx ",ReadTW88(addr+(i*16+j)));
		}
	}
#endif
}
#endif


//=============================================================================
// REMOVED
//=============================================================================

//===========================================
// ADC(AUX)
//===========================================
//internal
//note: NO PAGE change. Parent have to take care.
#ifdef SUPPORT_ANALOG_SENSOR
#ifdef SUPPORT_TOUCH
//desc: Read ADC value
//@param
//	0:X position
//	1:Z1 position
//	2:Z2 position
//	3:Y position
//	4: Aux0 value
//	5: AUX1 value
//	6: AUX2 value
//	7: AUX3 value
//register
//	R0B0
//	R0B2[7:0]R0B3[3:0]	TSC ADC Data
//internal
#ifdef TEST_BK111109
#endif
#endif
#endif


#ifdef SUPPORT_TOUCH
#ifdef TEST_BK111109
//??//desc:Read Z (presure) data.
//??//return
//??//	presure data
//??//	if fail, return 0xFF;
//??//Note:assuem page 0.
//??BYTE _TscGetZ(void)
//??{
//??	WORD z1, z2;
//??	z1 = _TscGetAdcDataOutput(ADC_MODE_Z1);
//??	if(z1 == 0) return (255);
//??	
//??	z2 = _TscGetAdcDataOutput(ADC_MODE_Z2);
//??
//??	dPrintf("\nTouch Z1:%d, Z2:%d", z1, z2 );
//??	Z1 = z1;
//??	Z2 = z2;
//??	return (z2 - z1);		
//??}
//??//desc:Read X position value
//??//global
//??//	TouchX : Latest pressed X position value
//??//Note:assuem page 0.
//??void _TscGetX(void)
//??{
//??	TouchX = _TscGetAdcDataOutput(ADC_MODE_X);
//??	//dPrintf("\nTouch X:%d", TouchX );
//??}
//??//desc:Read Y position value
//??//global
//??//	TouchX : Latest pressed Y position value
//??//Note:assuem page 0.
//??void _TscGetY(void)
//??{
//??	TouchY = _TscGetAdcDataOutput(ADC_MODE_Y);
//??	//dPrintf("\nTouch Y:%d", TouchY );
//??}
#endif
#endif //..

//=============================================================================
//		AUX 
//=============================================================================
#if 0 
//#ifdef SUPPORT_KEYPAD
void InitTscAdc(BYTE mode)
{
	 WriteTW88Page(PAGE0_TOUCH );
	 WriteTW88(REG0B0, mode );				// 0000-0xxx & mode
	 if(mode & 0x04) {
	 	//aux
		WriteTW88(REG0B1, 0x00 );			// enable the ready interrupt
		WriteTW88(REG0B4, 0x08 | 0x04 );	// div32.Continuous sampling for TSC_ADC regardless of the START command 
	 }
	 else {
	 	//touch
		WriteTW88(REG0B1, 0x80 );			// disable the ready interrupt
		WriteTW88(REG0B4, 0x08 );			// div2. Continuous sampling for TSC_ADC regardless of the START command
	 }
}
#endif

//global --BKTODO:see InitTouch also..
//@param
//	auxn: 0~3
#if 0
//#ifdef SUPPORT_TOUCH
//#ifdef SUPPORT_KEYPAD
void InitAUX( BYTE auxn )
{
#if 0
	 WriteTW88Page(PAGE0_TOUCH );
	 WriteTW88(REG0B0, 0x04 | auxn);			//  
	 WriteTW88(REG0B1, 0x00 );				// enable Ready Interrupt
	 WriteTW88(REG0B4, 0x08 | 0x04 );		// continuous sampling. div32  
#endif
	//init AUX0
	InitTscAdc(auxn | 0x04);	
}
#endif


#ifdef SUPPORT_TOUCH
//desc
//	Check the touch presure (Z value) and update TouchX and TouchY.
//update
//	TouchPressed
//	TouchDown
//	TouchX & TouchY
#ifdef TEST_BK111109
#endif //..TEST_BK111109
#endif

#if 0
void UpdateTouchCalibXY(BYTE index,WORD x, WORD y)
{
	TouchCalibX[index] = x;
	TouchCalibY[index] = y;
}
#endif
//WORD TouchGetCalibedX(BYTE index) { return TouchCalibX[index]; }
//WORD TouchGetCalibedY(BYTE index) { return TouchCalibY[index]; }
//void TouchSetCalibedXY(BYTE index, WORD x, WORD y) 
//{
//	TouchCalibX[index] = x;
//	TouchCalibY[index] = y;
//}

//=============================================================================
//		SenseTouch 
//=============================================================================
//return
//	1:Yes. return the position info
//	0:No input.
#ifdef SUPPORT_TOUCH
//return
//	1: success
//	0: fail
#ifdef TEST_BK111109
//!BYTE SenseTouch( WORD *xpos, WORD *ypos)
//!{
//!	bit TouchPressedOld;
//!
//!	TouchPressedOld = TouchPressed;
//!
//!	CheckTouch();
//!	if (!TouchPressed ) {
//!		if(TouchPressedOld)
//!			dTscPuts("\nTsc UP");
//!		return(0);
//!	}
//!	//
//!	//
//!	//
//!	if ( TouchPressedOld ) {   		// before it pressed with start
//!		_TscGetScreenPos();
//!		*xpos = PosX;
//!		*ypos = PosY;
//!		dPuts("\nSenseTouch:");
//!		if(TouchPressedOld)	dPuts(" OLD ");
//!		if(TouchPressed) dPuts(" NEW");
//!		if(TouchDown) dPuts(" DN"); 
//!		return (1);
//!	}
//!	else {
//!		_TscGetScreenPos();
//!		*xpos = PosX;
//!		*ypos = PosY;
//!		dPuts("\nSenseTouch:");
//!		if(TouchPressedOld)	dPuts(" OLD ");
//!		if(TouchPressed) dPuts(" NEW"); 
//!		if(TouchDown) dPuts(" DN"); 
//!		return (1);
//!	}
//!	return ( 0 );
//!}
#endif //..TEST_BK111109
#endif

//=============================================================================
//		GetTouch 
//=============================================================================
#ifdef SUPPORT_TOUCH
//!void GetTouch( void )
//!{
//!	WORD movX, movY; //move value
//!	BYTE dirX, dirY; //move direction
//!	BYTE TC;		 //Touch change counter
//!	bit	 TP;		 //pressed status
//!WORD	/*pressEndTime,*/ tsc_dt;
//!
//!	//update value 
//!	EA = 0;
//!	TC = CpuTouchChanged;
//!	TP = CpuTouchPressed;
//!	TouchX = CpuTouchX;
//!	TouchY = CpuTouchY;
//!	EA = 1;
//!
//!	if ( TouchChangedOld == TC ) return;			// no measurement
//!	TouchChangedOld = TC;
//!
//!	if ( TouchPressedOld ) {   		// before it pressed with start
//!		//pressed->
//!		if ( TP ) {
//!			//pressed->pressed
//!			//pressEndTime = tic01;  
//!			TscTimeEnd = SystemClock;
//!			_TscGetScreenPos();
//!			//direction. 
//!			if ( OldPosX > PosX ) {
//!				dirX = 0;					// decrease
//!				movX = OldPosX - PosX;
//!			}
//!			else {
//!				dirX = 1;					// increase
//!				movX = PosX - OldPosX;
//!			}
//!			if ( OldPosY > PosY ) {
//!				dirY = 0;					// decrease
//!				movY = OldPosY - PosY;
//!			}
//!			else {
//!				dirY = 1;					// increase
//!				movY = PosY - OldPosY;
//!			}
//!			if ( TouchStatus > TOUCHMOVE ) {
//!#if 1	 //BK111109
//!				//update direction
//!				if ( movX > movY ) {
//!					if (( movX / movY ) > 2) {
//!						if (dirX)
//!							TouchStatus = TOUCHMOVERIGHT;
//!						else
//!							TouchStatus = TOUCHMOVELEFT;
//!					}
//!				}
//!				else {
//!					if (( movY / movX ) > 2) {
//!						if (dirY)
//!							TouchStatus = TOUCHMOVEDOWN;
//!						else
//!							TouchStatus = TOUCHMOVEUP;
//!					}
//!				}
//!#endif
//!				OldPosX = PosX;
//!				OldPosY = PosY;
//!			}
//!			else 
//!			if (( movX > TSC_MOVE_MIN ) || ( movY > TSC_MOVE_MIN )) {
//!				if ( movX > movY ) {
//!					if (( movX / movY ) > 2) {
//!						if (dirX)
//!							TouchStatus = TOUCHMOVERIGHT;
//!						else
//!							TouchStatus = TOUCHMOVELEFT;
//!					}
//!				}
//!				else {
//!					if (( movY / movX ) > 2) {
//!						if (dirY)
//!							TouchStatus = TOUCHMOVEDOWN;
//!						else
//!							TouchStatus = TOUCHMOVEUP;
//!					}
//!				}
//!				OldPosX = PosX;
//!				OldPosY = PosY;
//!			}
//!			//TscPrintf("\nTouch Postion: xpos=%d, ypos=%d, endTime:%d", PosX, PosY, pressEndTime );
//!//			TscPrintf("\rTouch Postion: xpos=%d ypos=%d %dx%d  z1:%d endTime:%d", PosX, PosY, TouchX, TouchY, Z1, pressEndTime );
//!//			TscPrintf("\nTouch Postion: xpos=%d ypos=%d %dx%d  z1:%d endTime:%d", PosX, PosY, TouchX, TouchY, Z1, pressEndTime );
//!//			dTscPrintf("\nTouch Postion: xpos=%d ypos=%d %dx%d  z:%d(0x%x-0x%x) endTime:%ld", PosX, PosY, TouchX, TouchY, Z2-Z1,Z2,Z1, TscTimeEnd - TscTimeStart );
//!//			dTscPrintf("\nTouch Postion: xpos=%d ypos=%d %dx%d  z:%d(0x%x-0x%x) endTime:%ld", PosX, PosY, TouchX, TouchY, CpuZ2-CpuZ1,CpuZ2,CpuZ1, TscTimeEnd - TscTimeStart );
//!//			dTscPrintf("\nTouch Postion: xpos=%d ypos=%d %dx%d  z:%d endTime:%ld", PosX, PosY, TouchX, TouchY, CpuZ1, TscTimeEnd - TscTimeStart );
//!			dTscPrintf("\nTouch Position:xypos=%dx%d TouchXY=%dx%d  z:%d(0x%x-0x%x) endTime:%ld", PosX, PosY, TouchX, TouchY, CpuZ2-CpuZ1,CpuZ2,CpuZ1, TscTimeEnd - TscTimeStart );
//!			PrintTouchStatus(TouchStatus,0,0,0);	
//!			//OsdWinScreen( 0, PosX, PosY, 10, 10 );
//!		}
//!		else {
//!			//pressed->detached
//!			//TouchStatus
//!			//	TOUCHMOVE|TOUCHMOVED=>TOUCHEND	
//!			//	TOUCHCLICK=>TOUCHDOUBLECLICK
//!			//	?=>TOUCHCLICK
//!
//!			//use previous TscTimeEnd that is saved at the last pressed state.
//!
//!			dTscPrintf("\nTouchEndTime:%ld", TscTimeEnd);
//!			if(TscTimeEnd < TscTimeStart) {
//!				//overflow
//!				tsc_dt = 0xFFFFFFFF - TscTimeStart + TscTimeEnd;
//!			}
//!			else
//!				tsc_dt = TscTimeEnd - TscTimeStart;
//!			dTscPrintf("  Touch duration:%d", tsc_dt); //max 65536ms.
//!
//!			if ( OldPosX > StartX ) {
//!				dirX = 0;					// decrease
//!				movX = OldPosX - StartX;
//!			}
//!			else {
//!				dirX = 1;					// increase
//!				movX = StartX - OldPosX;
//!			}
//!			if ( OldPosY > StartY ) {
//!				dirY = 0;					// decrease
//!				movY = OldPosY - StartY;
//!			}
//!			else {
//!				dirY = 1;					// increase
//!				movY = StartY - OldPosY;
//!			}
//!			veloX = 1000;
//!			veloX *= movX;
//!			veloX /= tsc_dt;
//!			veloY = 1000;
//!			veloY *= movY;
//!			veloY /= tsc_dt;
//!			dTscPrintf("\nVelocity X:%ld Y:%ld", veloX,veloY );
//!
//!#if 0 //BK111108
//!			if (( TouchStatus >= TOUCHMOVE) || ( TouchStatus == TOUCHMOVED ))
//!				TouchStatus = TOUCHEND;
//!#else
//!			if (TouchStatus >= TOUCHMOVE) {
//!				TouchStatus = TOUCHMOVED;
//!			}
//!#endif
//!			else	{
//!				if((TscTimeLastEnd + 10) > TscTimeStart) {
//!					TouchStatus = TOUCHCLICK;
//!				}
//!				else if((TscTimeLastEnd + 100) > TscTimeStart) {
//!					TouchStatus = TOUCHDOUBLECLICK;
//!				}
//!				else {
//!					if(tsc_dt > 1000)	TouchStatus = TOUCHLONGCLICK;		//more then 10sec, it is a special..
//!					else	TouchStatus = TOUCHCLICK;
//!				}
//!			}
//!			//EndX = OldPosX;
//!			//EndY = OldPosY;
//!			//dTscPrintf("\nTouch Postion: xpos=%d, ypos=%d", OldPosX, OldPosY);
//!			//dTscPrintf("\nTouch Move: movx=(%bd)%d, movy=(%bd)%d", dirX, movX, dirY, movY);
//!			//dTscPrintf("\nTouch Velocity: vx=%ld, vy=%ld", veloX, veloY );
//!			//dTscPrintf("\nTouchEndTime:%d", pressEndTime);
//!			LastTouchStatus = TouchStatus;
//!
//!
//!			//dTscPrintf("\nTouch End:  OLDxypos=%dx%d TouchXY=%dx%d  z:%d(0x%x-0x%x) endTime:%ld", OldPosX, OldPosY, TouchX, TouchY, CpuZ2-CpuZ1,CpuZ2,CpuZ1, TscTimeEnd - TscTimeStart );
//!			//dTscPrintf("\nTouch End:  OLDxypos=%dx%d Move: movx=%s%d, movy=%s%d Velocity: vx=%ld, vy=%ld TouchEndTime:%d", 
//!			//	OldPosX, OldPosY, 
//!			//	dirX ? "+" : "-", movX, 
//!			//	dirY ? "+" : "-", movY, 
//!			//	veloX, veloY, 
//!			//	pressEndTime );
//! 			dTscPrintf("\nTouch End:  OLDxypos=%dx%d Move: movx=%s%d, movy=%s%d Velocity: vx=%ld, vy=%ld", 
//!				OldPosX, OldPosY, 
//!				dirX ? "+" : "-", movX, 
//!				dirY ? "+" : "-", movY, 
//!				veloX, veloY);
//!			PrintTouchStatus(TouchStatus,0,0,0);
//!		}
//!	}
//!	else {
//!		//normal ->
//!		if ( TP ) {
//!			//OLD: normal->pressed
//!			//if((TouchStatus==TOUCHMOVED) & ((TscTimeLastEnd + 10) > TscTimeStart)) {
//!			if(((TouchStatus==TOUCHMOVED) || (TouchStatus==TOUCHEND && LastTouchStatus==TOUCHMOVED) )
//!			& ((TscTimeLastEnd + 10) > SystemClock)) {
//!				TouchStatus = TOUCHMOVE;	
//!
//!
//!				_TscGetScreenPos();
//!				/*OldPosX = StartX = PosX; */
//!				/*OldPosY = StartY = PosY; */
//!				dTscPrintf("\nTouch MOVE_AGAIN:xypos=%dx%d TouchXY=%dx%d  z:%d(0x%x-0x%x) endTime:%ld", PosX, PosY, TouchX, TouchY, CpuZ2-CpuZ1,CpuZ2,CpuZ1, TscTimeEnd - TscTimeStart );
//!			}
//!			else if ((TscTimeEnd+10) > SystemClock && TouchStatus==TOUCHMOVED ) {
//!				dTscPrintf("\nTouch MOVE_AGAIN: dt:%ld",SystemClock- TscTimeEnd);
//!				TouchStatus = TOUCHMOVE;
//!			}
//!			else {
//!				//normal->pressed
//!				TouchStatus = TOUCHPRESS;
//!				/*pressEndTime =*/ /*pressStartTime = tic01; */
//!				TscTimeLastEnd = TscTimeEnd;
//!				TscTimeStart = /*TscTimeEnd =*/ SystemClock;
//!				_TscGetScreenPos();
//!				OldPosX = StartX = PosX;
//!				OldPosY = StartY = PosY;
//!	//			TscPrintf("\nTouch Start with: xpos=%d, ypos=%d, startTime:%d", StartX, StartY, pressStartTime );
//!	//			TscPrintf("\rTouch Start : xpos=%d ypos=%d %dx%d  z1:%d startTime:%d", StartX, StartY, TouchX, TouchY, Z1, pressStartTime );
//!	//			TscPrintf("\rTouch Start : xpos=%d ypos=%d %dx%d  z1:%d startTime:%d", StartX, StartY, TouchX, TouchY, Z1, pressStartTime );
//!	//			dTscPrintf("\nTouch Start : xpos=%d ypos=%d %dx%d  z:%d startTime:%d", StartX, StartY, TouchX, TouchY, CpuZ1, pressStartTime );
//!				//OsdWinScreen( 0, PosX, PosY, 10, 10 );
//!				dTscPrintf("\nTouch Start:   xypos=%dx%d TouchXY=%dx%d  z:%d(0x%x-0x%x) endTime:%ld", PosX, PosY, TouchX, TouchY, CpuZ2-CpuZ1,CpuZ2,CpuZ1, TscTimeEnd - TscTimeStart );
//!			}
//!		}
//!		else {
//!		}
//!	}
//!	TouchPressedOld = TP;
//!}
//!
#endif	//..SUPPORT_TOUCH

//=============================================================================
//		GetVeloX 
//		GetVeloY 
//=============================================================================
#ifdef UNCALLED_SEGMENT
WORD GetVeloX( void )
{...}
WORD GetVeloY( void )
{...}
#endif

//=============================================================================
//		CheckTouchStatus 
//=============================================================================
#ifdef TEST_BK111109
//!BYTE CheckTouchStatus( WORD *xpos, WORD *ypos)
//!{
//!	static BYTE old_TouchStatus;
//!
//!	GetTouch();
//!	*xpos = OldPosX;
//!	*ypos = OldPosY;
//!	if(old_TouchStatus != TouchStatus) {
//!		PrintTouchStatus(TouchStatus,OldPosX,OldPosY, 1);
//!#if 0
//!		switch(TouchStatus) {
//!		case TOUCHPRESS: 		dPrintf("\nTOUCHPRESS xpos=%d ypos=%d",OldPosX,OldPosY); break;
//!		case TOUCHMOVED: 		dPuts  ("\nTOUCHMOVED"); break;
//!		case TOUCHEND: 			dPuts  ("\nTOUCHEND"); break;
//!		case TOUCHCLICK: 		dPrintf("\nTOUCHCLICK xpos=%d ypos=%d",OldPosX,OldPosY); break;
//!		case TOUCHDOUBLECLICK: 	dPrintf("\nTOUCHDOUBLECLICK xpos=%d ypos=%d",OldPosX,OldPosY); break;
//!		case TOUCHMOVE: 		dPuts  ("\nTOUCHMOVE"); break;
//!		case TOUCHMOVEUP: 		dPuts  ("\nTOUCHMOVEUP"); break;
//!		case TOUCHMOVEDOWN: 	dPuts  ("\nTOUCHMOVEDOWN"); break;
//!		case TOUCHMOVELEFT: 	dPuts  ("\nTOUCHMOVELEFT"); break;
//!		case TOUCHMOVERIGHT: 	dPuts  ("\nTOUCHMOVERIGHT"); break;
//!		default: 				dPrintf("\nTOUCH UNKNOWN:%bd", TouchStatus); break;
//!		}
//!#endif
//!		old_TouchStatus = TouchStatus;
//!	}
//!	return( TouchStatus );
//!}
#endif

#if 0  //remove 110207
//LutOffset. 0, 64, 126,...255
//size 256 table with 8bpp will be 256*4 = 0x400
//BK110207: LUT 256->512. Need 9bit
void SpiOsdLoadLUTS(WORD LutOffset, WORD size, DWORD	address )
{
}
#endif

//#include "Data\DataInitCVBSFull.inc"
#if 0
code BYTE default_register[] = {
// cmd  reg		mask	value
	w, 0xFF, 0x00,
	r,	0x02,	0x00,	0x00,
	r,	0x03,	0x00,	0xFF,
	r,	0x04,	0xF0,	0x00,
	r,	0x06,	0x00,	0x00,
	r,	0x07,	0x00,	0x00,
};
void CheckHWDefaultRegister()
{
}
#endif
