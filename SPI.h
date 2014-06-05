#ifndef __SPI_H__
#define __SPI_H__


//-----------------------------------------------------------------------------
// SPI FLASH Command
//-----------------------------------------------------------------------------
#define SPICMD_WREN				0x06	//write enable
#define SPICMD_WRDI				0x04	//write disable
#define SPICMD_RDID				0x9F	//read identification.
#define SPICMD_RDSR				0x05	//read status register
#define SPICMD_RDSR2			0x35	//read status2 register(WB). dat[1]:QE 
#define SPICMD_WRSR				0x01	//write status register
#define SPICMD_FASTDTRD			0x0D	//fast DT read
#define SPICMD_2DTRD			0xBD	//dual I/O DT Read
#define SPICMD_4DTRD			0xED	//Quad I/O DT Read
#define SPICMD_READ				0x03	//Read data
#define SPICMD_FASTREAD			0x0B	//fast read data
#define SPICMD_2READ			0xBB	//2x I/O read command
#define SPICMD_4READ			0xEB	//4x I/O read command
#define SPICMD_4PP				0x38	//quad page program
#define SPICMD_SE				0x20	//sector erase
#define SPICMD_BE				0xD8	//block erase 64KB
#define SPICMD_BE32K			0x52	//block erase 32KB
#define SPICMD_CE				0x60	//chip erase. 0x60 or 0xC7
#define SPICMD_PP				0x02	//Page program
#define SPICMD_CP				0xAD	//continusly program mode
#define SPICMD_DP				0xB9	//Deep power down
#define SPICMD_RDP				0xAB	//Release from deep power down
#define SPICMD_RES				0xAB	//read electronic ID
#define SPICMD_REMS				0x90	//read electronic manufacturer & device ID
#define SPICMD_REMS2			0xEF	//read ID for 2x I/O mode
#define SPICMD_REMS4			0xDF	//read ID for 4x I/O mode
#define SPICMD_REMS4D			0xCF	//read ID for 4x I/O DT mode
#define SPICMD_ENSO				0xB1	//enter secured OTP
#define SPICMD_EXSO				0xC1	//exit secured OTP
#define SPICMD_RDSCUR			0x2B	//read security register
#define SPICMD_WRSCUR			0x2F	//write security register
#define SPICMD_ESRY				0x70	//enable SO to output RY/BY#
#define SPICMD_DSRY				0x80	//disable SO to output RY/BY#
#define SPICMD_ENPLM			0x55	//enter parallel mode
#define SPICMD_EXPLM			0x45	//exit parallel mode
#define SPICMD_CLSR				0x30	//clear SR fail flags
#define SPICMD_HPM				0xA3	//high performance enable mode
#define SPICMD_WPSEL			0x68	//write protection selection
#define SPICMD_SBLK				0x36	//single block lock
#define SPICMD_SBULK			0x39	//single block unlock
#define SPICMD_RDBLOCK			0x3C	//block protect read
#define SPICMD_GBLK				0x7E	//gang block lock
#define SPICMD_GBULK			0x98	//gang block unlock

#define SPICMD_RDVEREG			0x65	//read volatile enhanced register(micron)
#define SPICMD_WDVEREG			0x61	//write volatile enhanced register(micron)
#define SPICMD_RDVREG			0x85	//read volatile register(micron)
#define SPICMD_WDVREG			0x81	//write volatile register(micron)
#define SPICMD_RDNVREG			0xB5	//read non-volatile register(micron)
#define SPICMD_WDNVREG			0xB1	//write non-volatile register(micron)

//-----------------------------------------------------------------------------
//		SPI Read Mode
//-----------------------------------------------------------------------------
#define	SPI_READ_SLOW		0
#define SPI_READ_FAST 		1
#define SPI_READ_DUAL	 	2
#define SPI_READ_QUAD  		3
#define SPI_READ_DUALIO		4
#define SPI_READ_QUADIO		5

//default read mode
#ifdef MODEL_TW8830
#define SPI_READ_MODE		SPI_READ_SLOW
#else
#define SPI_READ_MODE		SPI_READ_QUADIO
#endif

//--------------------
// SPI FLASH Vendor
//--------------------
#define SFLASH_UNKNOWN			0
#define SFLASH_VENDOR_MX		1	//MX25L12845(C2 20 18) -- defaut
#define SFLASH_VENDOR_EON		2	//EN25Q128 ID(1C 30 18)
#define SFLASH_VENDOR_WB		3
#define SFLASH_VENDOR_MICRON	4	//STM(20 BA 18) Now Micron
extern BYTE SpiFlashVendor;

#define SPI_PAGE	0x04
#define SPIDMA_READDATA(n)			ReadTW88(REG4D0+n)

//----------------------------------------------
//SPI_Buffer[]
//Note: SPI_Buffer[] is a largest buffer in TW8835 FW.
//		measure.c also uses this SPI_Buffer[].
#define SPI_BUFFER_SIZE		128
extern  XDATA BYTE SPI_Buffer[SPI_BUFFER_SIZE];


//==============================================
//==============================================
void SpiFlashDmaStop(void);
void SpiFlashSetReadMode(BYTE mode);
BYTE SpiFlashGetReadMode(void);
#define DMA_DEST_FONTRAM			0
#define DMA_DEST_CHIPREG			1
#define DMA_DEST_SOSD_LUT			2
#define DMA_DEST_MCU_XMEM			3
void SpiFlashDmaDestType(BYTE dest, BYTE access_mode);
void SpiFlashSetCmdLength(BYTE len);
#define	SPIDMA_READ					0
#define SPIDMA_WRITE				1
#define SPIDMA_BUSYCHECK			1
void SpiFlashDmaStart(BYTE fWrite, BYTE fBusy, WORD cmd_line);
#define DMA_BUFF_REG_ADDR_PAGE		0x04
#define DMA_BUFF_REG_ADDR_INDEX		0xD0
#define DMA_BUFF_REG_ADDR			0x04D0
void SpiFlashDmaBuffAddr(WORD addr);
void SpiFlashDmaReadLen(DWORD len);
void SpiFlashDmaReadLenByte(BYTE len_l);
void SpiFlashCmd(BYTE cmd, BYTE cmd_len);
void SpiFlashDmaFlashAddr(DWORD addr);
void SpiFlashDmaRead(BYTE dest_type,WORD dest_loc, DWORD src_loc, WORD size);
void SpiFlashDmaRead2XMem(BYTE * dest_loc, DWORD src_loc, WORD size);
void SpiFlashCmdRead(BYTE dest);

//==============================================
//==============================================
//SPI Mode:	0=Slow, 1=fast, 2=dual, 3=quad, 4=dual-io, 5=quad-io
void SPI_SetReadModeByRegister(BYTE mode);
BYTE SPI_QUADInit(void);

void SPI_WriteEnable        (void);
void SPI_SectorErase        ( DWORD spiaddr );
void SPI_BlockErase         ( DWORD spiaddr );
void SPI_PageProgram        ( DWORD spiaddr, WORD x_addr, WORD cnt );

//#if defined(MODEL_TW8835_EXTI2C) || defined(MODEL_TW8835_MASTER)
//void SPIHost_SetReadModeByRegister(BYTE mode);
//BYTE SPIHost_QUADInit(void);
//void SPIHost_WriteEnable(void);
//void SPIHost_SectorErase( DWORD spiaddr );
//void SPIHost_PageProgram( DWORD spiaddr, WORD xaddr, WORD cnt );
//#endif


//========== EEPROM Emulation ==========
//
// size = SPI_SECTOR_SIZE * EE_SPI_BANKS * EE_BLOCKS
//	    = 4K              * 4             * 8
//		= 128K = 0x20000.
#ifdef USE_SFLASH_EEPROM

#define SPI_SECTOR_SIZE		0x1000		// 4K
#define EE_SPI_SECTOR0		0x080000L	// EE Start Address
#define EE_SPI_BANKS		4

#define EE_MAX_INDEX		512			//256  //--->I think we need it
#define EE_INDEX_PER_BLOCK	64			// 32 or 64
#define EE_BLOCKS			(EE_MAX_INDEX / EE_INDEX_PER_BLOCK)

#define EE_BUF_SIZE			SPI_BUFFER_SIZE		//64 or 128

#define EE_ITEM_SIZE		4
#define EE_MAX_WRITE_POS	(SPI_SECTOR_SIZE / EE_ITEM_SIZE)
#define EE_MAX_ITEMS		EE_MAX_WRITE_POS

void EE_PrintCurrInfo		(void);
void EE_Format              (void);
BYTE EE_FindCurrInfo		(void);
void EE_RepairMoveDone		(void);
void EE_CleanBlocks(void);
void EE_Check(void);

BYTE EE_Read                (WORD index);
void EE_Write               (WORD index, BYTE dat);

#elif defined(NO_EEPROM)  //..USE_SFLASH_EEPROM
BYTE EE_Read                (WORD index);
void EE_Write               (WORD index, BYTE dat);
#else
#define EE_Read(index)		ReadI2CByte(ADDRESS_EEPROM + ((index >> 8) << 1),(BYTE)index)
#define EE_Write(index,dat)	WriteI2CByte(ADDRESS_EEPROM + ((index >> 8) << 1),(BYTE)index,dat)
#endif //..USE_SFLASH_EEPROM


#endif // __SPI_H__
