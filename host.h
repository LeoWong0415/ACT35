#ifndef __HOST_H__
#define __HOST_H__

#ifndef MODEL_TW8835_EXTI2C
void Dummy_HOST_func(void);

#define SpiFlashHostCmdRead(dest)  SpiFlashCmdRead(dest)
#define SpiFlashHostDmaDestType(dest, access_mode)  SpiFlashDmaDestType(dest, access_mode)
#define SpiFlashHostDmaRead(dest_type,dest_loc, src_loc, size)	SpiFlashDmaRead(dest_type,dest_loc, src_loc, size)
#define SpiFlashHostDmaRead2XMem(dest_loc, src_loc, size)	SpiFlashDmaRead2XMem(dest_loc, src_loc, size)
#define SPIHost_WriteEnable() SPI_WriteEnable()
#define SPIHost_SectorErase(spiaddr) SPI_SectorErase(spiaddr)
#define SPIHost_PageProgram(spiaddr, ptr, cnt )	SPI_PageProgram(spiaddr, ptr, cnt )
#else //..MODEL_TW8835_EXTI2C

void InitHostCore(BYTE fPowerUpBoot);
void InitHostSystem(BYTE fPowerUpBoot);

void SpiFlashHostDmaDestType(BYTE dest, BYTE access_mode);
void SpiFlashHostSetCmdLength(BYTE len);
void SpiFlashHostDmaStart(BYTE fWrite, BYTE fBusy);
void SpiFlashHostDmaBuffAddr(WORD addr);
void SpiFlashHostDmaReadLen(DWORD len);
void SpiFlashHostDmaReadLenByte(BYTE len_l);
void SpiFlashHostCmd(BYTE cmd, BYTE cmd_len);
void SpiFlashHostDmaFlashAddr(DWORD addr);
void SpiFlashHostDmaRead(BYTE dest_type,WORD dest_loc, DWORD src_loc, WORD size);
void SpiFlashHostDmaRead2XMem(BYTE *dest_loc, DWORD src_loc, WORD size);
void SpiFlashDmaReadForXMem(BYTE *dest_loc, DWORD src_loc, WORD size);

void SPIHost_SetReadModeByRegister( BYTE mode );
void SPIHost_WriteEnable(void);
void SPIHost_SectorErase( DWORD spiaddr );
void SPIHost_PageProgram( DWORD spiaddr, WORD xaddr, WORD cnt );

BYTE SPIHost_QUADInit(void);

void SSPLL_Host_PowerUp(BYTE fOn);
void FP_Host_GpioDefault(void);
void InitHostWithNTSC(void);



#endif //..MODEL_TW8835_EXTI2C

#endif //__HOST_H__