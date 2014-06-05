#ifndef	__I2C_H__
#define	__I2C_H__

extern	BYTE	xdata * data regTW88;

BYTE CheckI2C(BYTE i2cid);
BYTE ReadI2CByte(BYTE addr, BYTE index);
void ReadI2C(BYTE addr, BYTE index, BYTE *val, BYTE cnt);
BYTE ReadSlowI2CByte(BYTE addr, BYTE index);
void ReadSlowI2C(BYTE addr, BYTE index, BYTE *val, BYTE cnt);
BYTE WriteI2CByte(BYTE addr, BYTE index, BYTE val);
void WriteI2C(BYTE addr, BYTE index, BYTE *val, BYTE cnt);
BYTE WriteSlowI2CByte(BYTE addr, BYTE index, BYTE val);
void WriteSlowI2C(BYTE addr, BYTE index, BYTE *val, WORD cnt);

#ifdef MODEL_TW8835_MASTER
BYTE WriteI2CByteToTW88(BYTE index, BYTE value);
BYTE ReadI2CByteFromTW88(BYTE index);
#endif


#ifdef SUPPORT_I2C2
void WriteI2C2Byte(BYTE addr, BYTE index, BYTE val);
BYTE ReadI2C2Byte(BYTE addr, BYTE index);
#endif

void I2CDeviceInitialize(BYTE *RegSet, BYTE delay);

#if defined(MODEL_TW8835_MASTER) && defined(SUPPORT_EXTMCU_ISP)
BYTE I2cSpiProg(DWORD start, BYTE *ptr, DWORD len);
#endif

void ModifyI2CByte(BYTE slaveID, BYTE offset, BYTE mask, BYTE value);
void ToggleI2CBit(BYTE slaveID, BYTE offset, BYTE mask);



//=================
// I2C device
//=================
#define I2CID_TW9910		0x88
#define I2CID_SX1504		0x40	//4CH GPIO
#define I2CID_BU9969		0xE0	//Digital video encoder
#define I2CID_ADV7390		0xD6	//Digital video encoder. 12bit
#define I2CID_ADV7391		0x56	//Digital video encoder. 10bit

#define I2CID_SIL9127_DEV0	0x60	//SiliconImage HDMI receiver
#define I2CID_SIL9127_DEV1	0x68
#define I2CID_SIL9127_HDCP	0x74
#define I2CID_SIL9127_COLOR	0x64
#define I2CID_SIL9127_CEC	0xC0
#define I2CID_SIL9127_EDID	0xE0
#define I2CID_SIL9127_CBUS	0xE6

#define I2CID_EP9351		0x78	//Explorer HDMI receiver

#define I2CID_ISL97901		0x50	//ISL RGB LED Driver
#define I2CID_ADC121C021	0xAC	//12bit Analog2Digital Converter
#define I2CID_E330_FLCOS	0x7C	//FLCOS	 
#define I2CID_ISL97671		0x58
//#define I2CID_PCA9306		0xFF	//Dual Bidir I2C Bus & SMBus Voltage Level Translator


#endif	// __I2C_H__

