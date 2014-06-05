/**
 * @file
 * Monitor_SPI.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	Interface between TW_Terminal2 and Firmware.
*/
//*****************************************************************************
//
//								Monitor_SPI.c
//
//*****************************************************************************
//
//
#include "Config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "Global.h"
#include "CPU.h"
#include "printf.h"
#include "util.h"
#include "monitor_SPI.h"

#include "i2c.h"
#include "SPI.h"

#include "eeprom.h"
#include "host.h"

static void SPI_Status(void);
static void SPI_dump(DWORD spiaddr); 

//=============================================================================
//
//=============================================================================
void MonitorSPI(void)
{
#ifdef USE_SFLASH_EEPROM
	BYTE j;
#endif

	//---------------------- Dump SPI --------------------------
	if( !stricmp( argv[1], "D" ) ) {
		static DWORD spiaddr = 0;

		if( argc>=3 ) spiaddr = a2h( argv[2] );

		SPI_dump(spiaddr); 

		spiaddr += 0x80L;
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "status" ) ) {
		SPI_Status();
	}
//	else if( !stricmp( argv[1], "quad" ) ) {
//		SPI_quadio();
//	}
	//--------------------------------------------------------
#ifdef USE_SFLASH_EEPROM
	else if( !stricmp( argv[1], "t" ) ) {
	
		SPI_SectorErase( EE_SPI_SECTOR0 );
		
		for(j=0; j<128; j++) {
			SPI_Buffer[j] = j;
		}
		SPI_PageProgram( EE_SPI_SECTOR0, (WORD)SPI_Buffer, 128 );
	}
#endif
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "Sector" ) ) {
		static DWORD spiaddr = 0x10000L;

		if( argc>=3 ) spiaddr = a2h( argv[2] );

		Printf("\nSector Erase = %06lx", spiaddr);

		SPI_SectorErase(spiaddr);
		spiaddr += 0x1000L;
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "Block" ) ) {
		static DWORD spiaddr = 0x10000L;

		if( argc>=3 ) spiaddr = a2h( argv[2] );

		Printf("\nBlock Erase = %06lx", spiaddr);

		SPI_BlockErase(spiaddr);
		spiaddr += 0x1000L;
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "Copy" ) ) {
		DWORD source = 0x0L, dest = 0x10000L;
		DWORD cnt=0x100L;

		if( argc<4 ) {
			Printf("\nMissing Parameters !!!" );
			return;
		}
		source = a2h( argv[2] );
		dest   = a2h( argv[3] );
		cnt    = a2h( argv[4] );

		Printf("\nSPI copy from %06lx to %06lx", source, dest);
		Printf("  %03x bytes", (WORD)cnt);
		SpiFlashDmaRead2XMem(SPI_Buffer,source,cnt);

		SPI_PageProgram( dest, (WORD)SPI_Buffer, cnt );
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "rmode" ) ) {
		BYTE mod;

		if(argc < 3)
			mod = 0x05;		//QuadIO
		else
			mod = a2h( argv[2] );

		SPI_SetReadModeByRegister(mod);
		Printf("\nSPI Read Mode = %bd 0x%bx", mod, ReadTW88(REG4C0) & 0x07);
	}
	//---------------------- Write SPI --------------------------
#ifdef MODEL_TW8835_EXTI2C
	else if( !stricmp( argv[1], "W" ) ) {
		DWORD spiaddr;
		//BYTE *ptr = SPI_Buffer;
		//DWORD size;
		BYTE i;
		BYTE cnt;
		BYTE dat;
		volatile BYTE vdata;
		//WORD xaddr;
		BYTE page;

		if( argc<4 ) {
			Printf("\nMissing Parameters !!!" );
			return;
		}
		if( argc > 11 ) {
			Printf("\nonly support 8 bytes !!!" );
			argc = 11;
		}
		spiaddr = a2h( argv[2] );
		Printf("\nWrite SPI %06lx ", spiaddr);
 
		//??need single mode.
		//SPI_SetReadModeByRegister(1);	//FAST or single

		ReadTW88Page(page);
		//SPI_WriteEnable();
		WriteTW88Page(PAGE4_SPI );					// Set Page=4

/*
		WriteTW88(REG4C3,(DMA_DEST_CHIPREG << 6) | 1);
	 	WriteTW88(REG4C6, 0x04 );					// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );					// data Buffer address

	 	WriteTW88(REG4C8, 0x00 );					// data Buff count Mi
		WriteTW88(REG4C9, 0x00 );					// data Buff count Lo
		//REG4CA: CMD
		WriteTW88(REG4CB, spiaddr>>16 );			// SPI address
		WriteTW88(REG4CC, spiaddr>>8 );				// SPI address
		WriteTW88(REG4CD, spiaddr );				// SPI address
		//REG4CE: CMD BUFF 5
	 	WriteTW88(REG4DA, 0x00 );					// data Buff count Hi
*/


		WriteTW88(REG4C3,(DMA_DEST_CHIPREG << 6) | 1);	// cmd len 1
		WriteTW88(REG4CA, SPICMD_WREN );				// SPI Command = WRITE_ENABLE
		WriteTW88(REG4C9, 0x00 );						// data Buff count Lo
		WriteTW88(REG4C4, 0x03 );						// DMA-Write start

		//??status
		WriteTW88(REG4C3,(DMA_DEST_CHIPREG << 6) | 1);	// cmd len 1
		WriteTW88(REG4CA, SPICMD_RDSR);					// SPI Command
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4C9, 0x01 );						// data Buff count Lo
		WriteTW88(REG4C4, 0x01 );						// DMA-Read
		for(i=0; i < 100; i++) {
			vdata = ReadTW88(REG4D0);
			if(vdata & 0x02) //check WEL
				break;
			delay1ms(10);
		}
		Printf("[%bx, %02bx]",i,vdata);


		for(i=3,cnt=0; i <argc; i++,cnt++) {
			dat = a2h(argv[i]);
			Printf(" %02bx",dat);
			WriteTW88(REG4D0+cnt, dat); 
		}


		WriteTW88(REG4C3, (DMA_DEST_CHIPREG << 6) | 4);	// cmd len 1
		WriteTW88(REG4CA, SPICMD_PP );					// SPI Command = PAGE_PROGRAM
		WriteTW88(REG4CB, spiaddr>>16 );				// SPI address
		WriteTW88(REG4CC, spiaddr>>8 );					// SPI address
		WriteTW88(REG4CD, spiaddr );					// SPI address
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
	 	WriteTW88(REG4C8, 0 );							// Write count Middle
		WriteTW88(REG4C9, cnt );						// Write count Low
		WriteTW88(REG4C4, 0x07 );						// DMA-Write start, Busy check

		//move back to QuadIO
		//delay1ms(10);
		//SPI_SetReadModeByRegister(5);	//QuadIO

		WriteTW88(REG4C3, (DMA_DEST_CHIPREG << 6) | 1);	// cmd len 1
		WriteTW88(REG4CA,SPICMD_RDSR);					// CMD
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4C9, 0x01 );						// len of data buffer
		WriteTW88(REG4C4, 0x01 );						// start READ
		for(i=0; i < 100; i++) {
			vdata = ReadTW88(REG4D0);
			if((vdata & 0x01) == 0)	//check done
				break;
			delay1ms(10);
		}
		Printf("[%bx, %02bx]",i,vdata);


		WriteTW88Page(page);
	}
#else
	else if( !stricmp( argv[1], "W" ) ) {
		static DWORD spiaddr = 0;
		BYTE *ptr = SPI_Buffer;
		DWORD size;
		BYTE i;

		if( argc<3 ) {
			Printf("\nMissing Parameters !!!" );
			return;
		}
		if( argc > 7 ) {
			Printf("\nonly support 4 bytes !!!" );
		}
		

		spiaddr = a2h( argv[2] );
		//only support eeprom area....please
		//if((spiaddr < EE_SPI_SECTOR0) || (spiaddr > EE_SPI_SECTOR0+0x010000)) {
		//	Printf("\nout of range %06lx~%06lx!!!", EE_SPI_SECTOR0, EE_SPI_SECTOR0+0x010000 );
		//	return;
		//}
		Printf("\nWrite SPI %06lx ", spiaddr);


		size=0;
		for(i=3; i < argc; i++) {
			*ptr++ = (BYTE)a2h(argv[i]);
			Printf(" %02bx ",(BYTE)a2h(argv[i]));
			size++;
		}	
		SPI_PageProgram( spiaddr, (WORD)SPI_Buffer, (WORD)size);
	}
#endif
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "?" ) ) {
		Printf("\n\n  === Help for SPI command ===");
		Printf("\nSPI D [addr]      ; dump");
		Printf("\nSPI sector [addr] ; sector erase");
		Printf("\nSPI block [addr]  ; block erase");
	   	Printf("\nSPI rmode [mode]	; set the read mode");
		//Printf("\nSPI W addr data   ; write 1 byte. Use PageProgram");
		Printf("\nSPI W addr data ...  ; write max 8 byte. Use PageProgram");
	   	Printf("\nSPI copy [src] [dst] [cnt] ; copy");
		Printf("\nSPI status      	; status");
		Printf("\n");
	}
	//--------------------------------------------------------
	else
		Printf("\nInvalid command...");	
}
//=============================================================================
//
//=============================================================================
void HelpMonitorSPIC(void)
{
#if 0
	Printf("\n\n  === Help for SPIFlash command ===");
	Printf("\nSPIC r_cnt [cmd...]	;r_cnt:max 8, cmd:max 4");
	Printf("\nSPIC 3 9f			; RDID - read JEDEC ID");
	Printf("\nSPIC 1 5			; RDSR - read status register");
	Printf("\nSPIC 0 6			; WREN - Write Enable");
	Printf("\nSPIC 0 4			; WRDI - write disable");
	Printf("\nSPIC 8 3 0 0 0	; read 8 data at 0x000000");
#else
	Printf("\nSPIC 9f               ; SPICMD_RDID: read JEDEC id");
	Printf("\nSPIC  6               ; SPICMD_WREN: write enable");
	Printf("\nSPIC  4               ; SPICMD_WRDI: write disable");
	Printf("\nSPIC  5               ; SPICMD_RDSR: read status register");
	Printf("\nSPIC  35              ; SPICMD_RDSR2: read status register");
	Printf("\nSPIC  1 state1 state2 ; SPICMD_WRSR: write status register");
	Printf("\nSPIC  2 addr data..	; SPICMD_PP:PageProgram. Max 8 data");
	Printf("\nSPIC 20 addr          ; SPICMD_SE: sector erase");
	Printf("\nSPIC d8 addr          ; SPICMD_BE: block erase");
	Printf("\nSPIC  3 addr [n]      ; SPICMD_READ: read data. default 8 data. Need REG4C0[2:0]=0");
	Printf("\nSPIC  b addr [n]      ; SPICMD_FASTREAD: fast read data. default 8 data. Need REG4C0[2:0]=1");
	Printf("\nSPIC eb addr [n]      ; SPICMD_4READ: QuadIO read data. default 8 data. NG. Only 1Byte");
	Printf("\nSPIC  65              ; SPICMD_RDVEREG: read volatile status register");
	Printf("\nSPIC  61 data         ; SPICMD_WDVEREG: write volatile enhanced status register");
	Printf("\nSPIC  85              ; SPICMD_RDVREG: read volatile enhanced status register");
	Printf("\nSPIC  81 data         ; SPICMD_WDVREG: write volatile status register");
	Printf("\nSPIC  B5              ; SPICMD_RDNVREG: read non-volatile status register");
	Printf("\nSPIC  B1 data1 data0  ; SPICMD_WDNVREG: write non-volatile status register");
#endif
	Printf("\n");
}
void MonitorSPIC(void)
{
	DECLARE_LOCAL_page
	BYTE cmd;
	BYTE dat0,dat1,dat2;
	BYTE i;
	BYTE cnt;
	DWORD spiaddr;
	volatile BYTE vdata;

	if(argc < 2) {
		HelpMonitorSPIC();
		return;
	}
	ReadTW88Page(page);
	WriteTW88Page(4);

	cmd = a2h( argv[1] );
	if(cmd == SPICMD_RDID) {
		Printf("\nRDID(JEDEC) ");
		WriteTW88(REG4C3,(DMA_DEST_CHIPREG << 6) | 1);
		WriteTW88(REG4CA, SPICMD_RDID );
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4DA, 0x00 );						// data Buff count high
		WriteTW88(REG4C8, 0x00 );						// data Buff count middle
		WriteTW88(REG4C9, 0x03 );						// data Buff count Lo
		WaitVBlank(1);
		WriteTW88(REG4C4, 0x01 );						// DMA-Read
		//wait
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4C4);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		Printf(" wait:%bd,%bx data:",i,vdata);

		dat0 = ReadTW88(REG4D0);
		dat1 = ReadTW88(REG4D1);
		dat2 = ReadTW88(REG4D2);
		Printf("%02bx %02bx %02bx", dat0, dat1,dat2);
		switch(dat0) {
		case 0x1C:	 	Puts("\nEON");		break;
		case 0xC2:		Puts("\nMX");		break;
		case 0xEF:		Puts("\nWB");		break;
		case 0x20:		Puts("\nMicron");	break;
		default:		Puts("\nUnknown");	break;
		}
		if(dat2 == 0x18) Puts("128"); 
	}
	else if(cmd == SPICMD_WREN || cmd == SPICMD_WRDI) {
		WriteTW88(REG4C3,(DMA_DEST_CHIPREG << 6) | 1);
		if(cmd == SPICMD_WRDI)
			WriteTW88(REG4CA, SPICMD_WRDI );
		else 
			WriteTW88(REG4CA, SPICMD_WREN );
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4DA, 0x00 );						// data Buff count high
		WriteTW88(REG4C8, 0x00 );						// data Buff count middle
		WriteTW88(REG4C9, 0x00 );						// data Buff count Lo
		WaitVBlank(1);
		WriteTW88(REG4C4, 0x03 );						// DMA-Write
		//wait
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4C4);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		if(cmd == SPICMD_WRDI) Puts("\nWRDI ");
		else				   Puts("\nWREN ");
		Printf("wait:%bd,%bx data:%bx",i,vdata);
	}
	else if(cmd == SPICMD_RDSR || cmd == SPICMD_RDSR2) {
		WriteTW88(REG4C3,(DMA_DEST_CHIPREG << 6) | 1);
		WriteTW88(REG4CA, cmd );
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4DA, 0x00 );						// data Buff count high
		WriteTW88(REG4C8, 0x00 );						// data Buff count middle
		WriteTW88(REG4C9, 0x01 );						// data Buff count Lo
		WaitVBlank(1);
		WriteTW88(REG4C4, 0x01 );						// DMA-Read
		//wait
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4C4);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		dat0 = ReadTW88(REG4D0);
		if(cmd == SPICMD_RDSR2) {
			Puts("\nRDSR2 ");
			Printf("wait:%bd,%bx data:%bx",i,vdata,dat0);
			if(dat0 & 0x02) Puts(" Quad");
		}
		else {
			Puts("\nRDSR ");
			Printf("wait:%bd,%bx data:%bx",i,vdata,dat0);
			if(dat0 & 0x40) Puts(" Quad");
			if(dat0 & 0x02) Puts(" WEL");
			if(dat0 & 0x01) Puts(" WIP");
		}
	}
	else if(cmd == SPICMD_WRSR) {
		if( argc<4 ) {
			Printf("\nMissing Parameters !!!" );
			return;
		}
		dat0 = a2h(argv[2]);
		dat1 = a2h(argv[3]);
		WriteTW88(REG4C3, (DMA_DEST_CHIPREG << 6) | 3);	// cmd len 1
		WriteTW88(REG4CA, SPICMD_WRSR );				// SPI Command = 
		WriteTW88(REG4CB, dat0);
		WriteTW88(REG4CC, dat1);
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4DA, 0x00 );						// data Buff count high
		WriteTW88(REG4C8, 0x00 );						// data Buff count middle
		WriteTW88(REG4C9, 0x00 );						// data Buff count Low
		WaitVBlank(1);
		WriteTW88(REG4C4, 0x07 );						// DMA-Write start, Busy check
		//wait
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4C4);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		Printf("\nWRSR wait:%bd,%bx data:",i,vdata);
	}
	else if(cmd == SPICMD_PP) {
		if( argc<4 ) {
			Printf("\nMissing Parameters !!!" );
			return;
		}
		if( argc > 11 ) {
			Printf("\nonly support 8 bytes !!!" );
			argc = 11;
		}
		spiaddr = a2h( argv[2] );
		Printf("\nPP %06lx ", spiaddr);

		for(i=3,cnt=0; i <argc; i++,cnt++) {
			dat0 = a2h(argv[i]);
			Printf(" %02bx",dat0);
			WriteTW88(REG4D0+cnt, dat0); 
		}
		WriteTW88(REG4C3, (DMA_DEST_CHIPREG << 6) | 4);	// cmd len 1
		WriteTW88(REG4CA, SPICMD_PP );					// SPI Command = PAGE_PROGRAM
		WriteTW88(REG4CB, spiaddr>>16 );				// SPI address
		WriteTW88(REG4CC, spiaddr>>8 );					// SPI address
		WriteTW88(REG4CD, spiaddr );					// SPI address
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4DA, 0x00 );						// data Buff count high
		WriteTW88(REG4C8, 0x00 );						// data Buff count middle
		WriteTW88(REG4C9, cnt );						// data buff count Low
		WaitVBlank(1);
		WriteTW88(REG4C4, 0x07 );						// DMA-Write start, Busy check
		//wait
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4C4);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		Printf("wait:%bd,%bx data:",i,vdata);
#if 1
		WriteTW88(REG4C3,(DMA_DEST_CHIPREG << 6) | 1);
		WriteTW88(REG4CA, SPICMD_RDSR );
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4C9, 0x01 );						// data Buff count Lo
		WaitVBlank(1);
		WriteTW88(REG4C4, 0x01 );						// DMA-Read
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4D0);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		Printf("\n%bd: %02bx", i,vdata);
		if(vdata & 0x40) Puts(" Quad");
		if(vdata & 0x02) Puts(" WEL");
		if(vdata & 0x01) Puts(" WIP");
#endif
		//Prompt();
		//Printf("\nMonAddress[%02bx]>", MonAddress);
	}
	else if(cmd == SPICMD_SE || cmd == SPICMD_BE) {
		if( argc<3 ) {
			Printf("\nMissing Parameters !!!" );
			return;
		}
		spiaddr = a2h( argv[2] );
		if(cmd==cmd == SPICMD_BE)
			Printf("\nBE %06lx ", spiaddr);
		else
			Printf("\nSE %06lx ", spiaddr);

		WriteTW88(REG4C3, (DMA_DEST_CHIPREG << 6) | 4);	// cmd len 1
		if(cmd == SPICMD_BE)
			WriteTW88(REG4CA, SPICMD_BE );				// SPI Command = 
		else
			WriteTW88(REG4CA, SPICMD_SE );				// SPI Command = 
		WriteTW88(REG4CB, spiaddr>>16 );				// SPI address
		WriteTW88(REG4CC, spiaddr>>8 );					// SPI address
		WriteTW88(REG4CD, spiaddr );					// SPI address
		WriteTW88(REG4DA, 0x00 );						// data Buff count high
		WriteTW88(REG4C8, 0x00 );						// data Buff count middle
		WriteTW88(REG4C9, 0 );							// data buff count Low
		WaitVBlank(1);
		WriteTW88(REG4C4, 0x07 );						// DMA-Write start, Busy check
		//wait
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4C4);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		Printf("wait:%bd,%bx data:",i,vdata);
	}
	else if(cmd == SPICMD_READ || cmd == SPICMD_FASTREAD || cmd == SPICMD_4READ ) {
		if( argc < 3 ) {
			Printf("\nMissing Parameters !!!" );
			return;
		}
		dat0 = ReadTW88(REG4C0) & 0x07;
		if(cmd==SPICMD_FASTREAD) {
		 	Puts("\nF_READ");
			if(dat0 != 1) {
				Puts(" need REG4C0[2:0]=1");
				WriteTW88(REG4C0, 1);		//BKTODO:TW8836 need REG4C0[7]
			}
		}
		else if(cmd==SPICMD_4READ) {
			Puts("\nQ_READ");
			if(dat0 != 5) {
				Puts(" need REG4C0[2:0]=5"); 
				WriteTW88(REG4C0, 5);		//BKTODO:TW8836 need REG4C0[7]
			}
		}
		else { // cmd==SPICMD_READ
			Puts("\nREAD");
			if(dat0 != 0) {
				Puts(" need REG4C0[2:0]=0");
				WriteTW88(REG4C0, 0);		//BKTODO:TW8836 need REG4C0[7]
			}  
		}
		spiaddr = a2h( argv[2] );
		Printf(" %06lx ", spiaddr);

		if(argc >= 4)
			cnt = a2h(argv[3]);
		else cnt = 8;

		if(cmd==SPICMD_FASTREAD) {
			WriteTW88(REG4C3, (DMA_DEST_CHIPREG << 6) | 5);	// cmd len 5
			WriteTW88(REG4CA, SPICMD_FASTREAD );			// SPI Command = 
		}
		else if(cmd==SPICMD_4READ) {
			WriteTW88(REG4C3, (DMA_DEST_CHIPREG << 6) | 7);	// cmd len 4
			WriteTW88(REG4CA, SPICMD_4READ );				// SPI Command = 
		}
		else {	//cmd==SPICMD_READ
			WriteTW88(REG4C3, (DMA_DEST_CHIPREG << 6) | 4);	// cmd len 1
			WriteTW88(REG4CA, SPICMD_READ );				// SPI Command = 
		}
		WriteTW88(REG4CB, spiaddr>>16 );				// SPI address
		WriteTW88(REG4CC, spiaddr>>8 );					// SPI address
		WriteTW88(REG4CD, spiaddr );					// SPI address

	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4DA, 0x00 );						// data Buff count high
		WriteTW88(REG4C8, 0x00 );						// data Buff count middle
		WriteTW88(REG4C9, cnt );						// data Buff count Lo

		WaitVBlank(1);
		WriteTW88(REG4C4, 0x05 );						// DMA-Read start, Busy check
		//wait
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4C4);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		Printf("wait:%bd,%bx data:",i,vdata);

		for(i=0; i < cnt; i++)
			Printf(" %02bx", ReadTW88(REG4D0+i));
	}
	else if(cmd == SPICMD_RDVEREG || cmd == SPICMD_RDVREG || cmd == SPICMD_RDNVREG) {
		//only for micron
		if(cmd== SPICMD_RDVEREG)
			Printf("\nRDVEREG:%bx",SPICMD_RDVEREG);
		else if(cmd== SPICMD_RDVREG)
			Printf("\nRDVREG:%bx",SPICMD_RDVREG);
		else
			Printf("\nRDNVREG:%bx",SPICMD_RDNVREG);

		WriteTW88(REG4C3,(DMA_DEST_CHIPREG << 6) | 1);
		WriteTW88(REG4CA, cmd );
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4DA, 0x00 );						// data Buff count high
		WriteTW88(REG4C8, 0x00 );						// data Buff count middle
		if(cmd== SPICMD_RDVEREG || cmd== SPICMD_RDVREG)
			WriteTW88(REG4C9, 0x01 );						// data Buff count Lo
		else
			WriteTW88(REG4C9, 0x02 );						// data Buff count Lo
		WaitVBlank(1);
		WriteTW88(REG4C4, 0x01 );						// DMA-Read
		//wait
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4C4);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		Printf(" wait:%bd,%bx data:",i,vdata);

		dat0 = ReadTW88(REG4D0);
		Printf("%02bx", dat0);
		if(cmd== SPICMD_RDNVREG) {
			dat1 = ReadTW88(REG4D1);
			Printf(" %02bx", dat1);
		}
		if(cmd== SPICMD_RDVREG) {
			Printf(" DummyCycle:%bd", dat0 >> 4);
		}
		else {
			Printf(" DummyCycle:%bd", dat1 >> 4);
		}

	}
	else if(cmd==SPICMD_WDVEREG || cmd==SPICMD_WDVREG || cmd==SPICMD_WDNVREG) {
		//only for micron
		BYTE cmd_len;
		if(cmd==SPICMD_WDVEREG || cmd==SPICMD_WDVREG) {
			if( argc<3 ) {
				Printf("\nMissing Parameters !!!" );
				return;
			}
			dat0 = a2h(argv[2]);
			cmd_len = 2;
		}
		else {
			if( argc<4 ) {
				Printf("\nMissing Parameters !!!" );
				return;
			}
			dat0 = a2h(argv[2]);
			dat1 = a2h(argv[3]);
			cmd_len = 3;
		}
		WriteTW88(REG4C3, (DMA_DEST_CHIPREG << 6) | cmd_len);	// cmd len 1
		WriteTW88(REG4CA, cmd );				// SPI Command = 
		WriteTW88(REG4CB, dat0);
		if(cmd==SPICMD_WDNVREG)
			WriteTW88(REG4CC, dat1);
	 	WriteTW88(REG4C6, 0x04 );						// data Buffer address 0x04D0
		WriteTW88(REG4C7, 0xD0 );						// data Buffer address
		WriteTW88(REG4DA, 0x00 );						// data Buff count high
		WriteTW88(REG4C8, 0x00 );						// data Buff count middle
		WriteTW88(REG4C9, 0x00 );						// data Buff count Low
		WaitVBlank(1);
		WriteTW88(REG4C4, 0x07 );						// DMA-Write start, Busy check
		//wait
		for(i=0; i < 200; i++) {
			vdata = ReadTW88(REG4C4);
			if((vdata & 0x01)==0)
				break;
			delay1ms(10);
		}
		Printf("\nwait:%bd,%bx data:",i,vdata);
	}
	else {
		Printf("\nUnknown CMD:%02bx",cmd);
	}

	WriteTW88Page(page);
}



#if 0
void MonitorSPIC_____OLD(void)
{
	DECLARE_LOCAL_page
	BYTE temp;
	BYTE r_cnt, c_cnt, i;
	BYTE w_cmd[5];

	if(argc < 3) {
		HelpMonitorSPIC();
		return;
	}

	r_cnt = (BYTE)a2h( argv[1] );
	if(r_cnt > 8)	r_cnt = 8;		//max

	c_cnt=0;
	temp = argc - 2;
	if(temp > 5)	temp=5;			//max
	for(i=0; i < temp; i++) {
		w_cmd[c_cnt++] = (BYTE)a2h(argv[2+i]);
	}

	Printf("\nSPIC r:%bd c:%bd cmd:",r_cnt, c_cnt);
 	for(i=0; i < c_cnt; i++)
		Printf(" %02bx", w_cmd[i]);		

	ReadTW88Page(page);		//save
	WriteTW88Page( PAGE4_SPI );

	WriteTW88(REG4C3, 0x40+c_cnt);			// use chip register
	Write2TW88(REG4C6,REG4C7, 0x04d0);		// DMA Page & Index. indecate DMA read/write Buffer at 0x04D0.
	WriteTW88(REG4DA,0 );					// DMA Length high
	Write2TW88(REG4C8,REG4C9, r_cnt);		// DMA Length middle & low

	//write command 
	for(i=0; i < c_cnt; i++)
		WriteTW88(REG4CA+i, w_cmd[i] );		// write cmd1

	if(r_cnt) {
		WriteTW88(REG4C4, 0x01 );			// start
		delay1ms(2);
	}
	else {
		//if(c_cnt > 1)	
		//	WriteTW88(REG4c4, 0x07 );		// start with write, with busycheck
		//else 			
			WriteTW88(REG4C4, 0x03 );		// start           , with busycheck
	}

	//read
	if(r_cnt)	Puts("\tREAD:");	
	for(i=0; i < r_cnt; i++) {
		temp = ReadTW88(REG4D0+i );
		Printf("%02bx ",temp);
	}							

	WriteTW88Page( page );	//restore
}
#endif

#if 0
void MonitorSPIC(void)
{
	BYTE page;
	BYTE reg, temp;
	BYTE cnt, i;
	//BYTE i;
	BYTE w_cmd[5];


	if(argc < 2) {
		HelpMonitorSPIC();
		return;
	}

	ReadTW88Page(page);		//save
	WriteTW88Page( PAGE4_SPI );

	reg = (BYTE)a2h( argv[1] );

	//--------------------------------------------------------
	//spic 9f r 3		read SPI JEDEC ID
	//spic 5 r 1		read status
	if( !stricmp( argv[2], "R" ) ) {

		cnt = (BYTE)a2h( argv[3] );

		Printf("\nspic 0x%bx R %bd==>",reg,cnt);

		WriteTW88(REG4c3, 0x41 );				// mode, cmd-len 1
		Write2TW88(REG4C6,REG4C7, 0x04d0);			// DMA Page & Index. indecate DMA read/write Buffer at 0x04D0.
		WriteTW88(REG4DA,0 );					// DMA Length high
		Write2TW88(REG4C8,REG4C9, cnt);				// DMA Length middle & low

		WriteTW88(REG4ca, reg );					// cmd1
		WriteTW88(REG4c4, 0x01 );				// start
		for(i=0; i < cnt; i++) {
			temp = ReadTW88(REG4d0+i );
			Printf("%02bx ",temp);
		}							
	}
	//--------------------------------------------------------
	//spic 6 w			  ; write enable
	//spic 4 w			  ; write disable
	//spic 1 w 1 value1	  ; write status
	else if( !stricmp( argv[2], "W" ) ) {

		if(reg==6 || reg==4) {
			cnt = 0;
		}
		else
		cnt = (BYTE)a2h( argv[3] );
		if(cnt > 4)
			cnt = 4;
		Printf("\nspic 0x%bx W %bd",reg,cnt);
		for(i=0; i < cnt; i++) {
			w_cmd[i] = (BYTE)a2h( argv[4+i] );
			Printf(" 0x%bx",w_cmd[i]);
		}
		Puts("==>");

		WriteTW88Page(4);							  	
		WriteTW88(REG4c3, 0x41+cnt );				// mode, cmd-len 1+cnt
		Write2TW88(REG4C6,REG4C7, 0x04d0);				// DMA Page & Index. indecate DMA read/write Buffer at 0x04D0.
		WriteTW88(REG4DA, 0 );						// DMA Length high
		WriteTW88(REG4C8, 0 );						// DMA Length middle
		WriteTW88(REG4C9, cnt);						// DMA Length low

		WriteTW88(REG4ca, reg );						// cmd1
		for(i=0; i < cnt; i++) {
			WriteTW88(REG4cb+i, w_cmd[i] );
		}							
		WriteTW88(REG4c4, 0x01 );					// start
	}

	else if( !stricmp( argv[1], "?" ) ) {
		HelpMonitorSPIC();
	}
	//--------------------------------------------------------
	else
		Printf("\nInvalid command...");	

	WriteTW88Page( page );	//restore
}
#endif

//=============================================================================
//
//=============================================================================
//	Format is needed only once
//	Init is needed when starting program
#ifndef MODEL_TW8836RTL
#ifdef USE_SFLASH_EEPROM
void MonitorEE(void)
{
	BYTE dat, i, j;
	BYTE dat1;
	WORD index;

	index = a2h( argv[2] );
	dat   = a2h( argv[3] );

	//--------------------------------------------------------
	if( !stricmp( argv[1], "format" ) ) {
		Printf("\nFormat EEPROM...");
		EE_Format();
		return;
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "find" ) ) {
		Printf("\nFind EEPROM variables...");
		EE_FindCurrInfo();
		return;
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "repair" ) ) {
		Printf("\nRepair MoveDone error..call only when EE find have a MoveDone error");
		EE_RepairMoveDone();
		return;
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "init" ) ) {
		Printf("\nEE initialize........");
		ClearBasicEE();
		SaveDebugLevelEE(0);
		SaveFWRevEE( FWVER );
		EE_PrintCurrInfo();
		return;
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "check" ) ) {
		Printf("\nEE check");
		EE_Check();
		return;
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "clean" ) ) {
		Printf("\nEE clean blocks");
		EE_CleanBlocks();
		return;
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "W" ) ) {
		if( argc==4 ) {
			Printf("\nWrite EEPROM %03x:%02bx ", index, dat );
			EE_Write( index, dat );
			dat1 = EE_Read( index );  //BUG
			dat = EE_Read( index );
			Printf(" ==> Read EEPROM[%03x] = %02bx %02bx", index, dat1, dat );
		}
	}
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "R" ) ) {
		if( argc==3 ) {
			dat = EE_Read( index );
			Printf("\n ==> Read EEPROM[%03x] = %02bx ", index, dat );
		}
	}
	//--------------------------------------------------------
#ifdef USE_SFLASH_EEPROM
	else if( !stricmp( argv[1], "D" ) ) {
		Printf("\nDump EEPROM");
		for(j=0; j<EE_MAX_INDEX/16; j++) {
			Printf("\nEEPROM %02bx:", j*0x10);
			for(i=0; i<8; i++) Printf(" %02bx", EE_Read( j*16 + i ) );
			Printf("-");
			for(; i<16; i++) Printf("%02bx ", EE_Read( j*16 + i ) );
		}
	}
#endif
	//--------------------------------------------------------
	else if( !stricmp( argv[1], "?" ) ) {
		Printf("\n\n  === Help for EE command ===");
		Printf("\nEE format         ; format and initialize");
		Printf("\nEE find           ; initialze internal variables");
		Printf("\nEE init           ; initialze default EE values");
		Printf("\nEE check          ; report map,dump,corrupt");
		Printf("\nEE clean          ; move & clean bank sector");
		Printf("\nEE repair         ; call when you have a movedone error");
		Printf("\nEE w [idx] [dat]  ; write data");
	   	Printf("\nEE r [idx]        ; read data");
		Printf("\nEE d              ; dump all data");
		Printf("\nFYI %bx:DebugLevel %bx:InputMain ",EEP_DEBUGLEVEL,EEP_INPUTSELECTION);
		Printf("\n");
	}

	else
		Printf("\nInvalid command...");	
	
}
#endif
#endif

//=============================================================================
/**
* SPI Read Status
*
*/
static void SPI_Status(void)
{
	BYTE dat0,dat1;
	BYTE vid;
	BYTE cid;

	WriteTW88Page(PAGE4_SPI);

	SpiFlashDmaDestType(DMA_DEST_CHIPREG, 0);
	SpiFlashDmaBuffAddr(DMA_BUFF_REG_ADDR);
	SpiFlashDmaReadLen(0);	//clear high & middle bytes 

	SpiFlashCmd(SPICMD_RDID, 1);	
	SpiFlashDmaReadLenByte(3);
	SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
	vid  = SPIDMA_READDATA(0);
	dat1 = SPIDMA_READDATA(1);
	cid  = SPIDMA_READDATA(2);

	Printf("\nJEDEC ID: %02bx %02bx %02bx", vid, dat1, cid );

	switch(vid) {
	case 0x1C:	 	Puts("\nEON");		break;
	case 0xC2:		Puts("\nMX");		break;
	case 0xEF:		Puts("\nWB");		if(cid == 0x18) Puts("128"); break;
	case 0x20:		Puts("\nMicron");	break;
	default:		Puts("\nUnknown");	break;
	}

	if (vid == 0xC2 || vid == 0x1c) {
		SpiFlashCmd(SPICMD_RDSR, 1);
		SpiFlashDmaReadLenByte(1);
		SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
		dat0 = SPIDMA_READDATA(0);
		Printf("	CMD:%02bx Data:%02bx",SPICMD_RDSR,dat0);
		if(dat0 & 0x40)	Puts(" Quad Enabled");
	}
	else if (vid == 0xEF) {					// WB
		if(cid == 0x18) {				//Q128 case different status read command
			SpiFlashCmd(SPICMD_RDSR2, 1);
			SpiFlashDmaReadLenByte(1);
			SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
			dat0 = SPIDMA_READDATA(0);
			Printf("	CMD:%02bx Data:%02bx",SPICMD_RDSR2,dat0);
		}
		else {
			SpiFlashCmd(SPICMD_RDSR, 1);
			SpiFlashDmaReadLenByte(2);
			SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
			dat0 = SPIDMA_READDATA(0);
			dat1 = SPIDMA_READDATA(1);
			dPrintf("\nStatus before QUAD: %02bx, %02bx", dat0, dat1 );	
			Printf("	CMD:%02bx Data:%02bx,%02bx",SPICMD_RDSR,dat0,dat1);
		}
	}
	else if(vid==0x20) {
		if(cid !=0x18) {
			Puts(" NEED 128b!!!");
			return;
		}
		// Volatile
		SpiFlashCmd(SPICMD_RDVREG, 1);	//cmd, read Volatile register
		SpiFlashDmaReadLenByte(1);
		SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
		dat0 = SPIDMA_READDATA(0);
		dPrintf("	Volatile Register: %02bx", dat0 );

		// non-Volatile
		SpiFlashCmd(SPICMD_RDNVREG, 1);	//cmd, read Non-Volatile register
		SpiFlashDmaReadLenByte(2);
		SpiFlashDmaStart(SPIDMA_READ,0, __LINE__);
		dat0 = SPIDMA_READDATA(0);
		dat1 = SPIDMA_READDATA(1);
		dPrintf("	Non-Volatile Register: %02bx, %02bx", dat0, dat1 );
	}
}

//=============================================================================
/**
* read and dump SPIFLASH data
*/
static void SPI_dump(DWORD spiaddr) 
{
	BYTE *ptr = SPI_Buffer;
	DWORD cnt = 0x80L;
	BYTE i, j, c;

	SpiFlashDmaRead2XMem(SPI_Buffer, spiaddr, cnt);  //same SpiFlashDmaRead 

	for (j=0; j<8; j++) {
		Printf("\nSPI %06lx: ", spiaddr + j*0x10);
		for(i=0; i<8; i++) Printf("%02bx ", SPI_Buffer[j*0x10+i] );
		Printf("- ");
		for(; i<16; i++) Printf("%02bx ", SPI_Buffer[j*0x10+i] );
		Printf("  ");
		for(i=0; i<16; i++) {
			c = SPI_Buffer[j*0x10+i];
			if( c>=0x20 && c<0x80 ) Printf("%c", c);
			else Printf(".");
		}
	}
}







