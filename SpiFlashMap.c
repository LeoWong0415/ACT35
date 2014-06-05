/**
 * @file
 * SpiFlashMap.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	SpiFlash Map for images
*/
//*****************************************************************************
//
//								SPI_MAP.c
//
//*****************************************************************************
// SPI FLASH total MAP
// +----------------------------+    
// |                            |
// |     CODE BANK              |
// |                            |
// +----------------------------+    
// | 080000~0x09FFFF            |
// |     EEPROM Emulation       |
// |                            |
// +----------------------------+    
// | 0A0000                     |
// |     Blank Space            |
// +----------------------------+    
// | 100000                     |
// |     Demo Image             |
// |                            |
// | ParkGrid                   |
// |  start 0x100000            |
// | Pigeon                     |
// |  start 0x170000            |
// | Rose                       |
// |  start 0x190D00            |
// | LUT for Pigion&Rose        |
// |  start 0x317700            |
// +----------------------------+    
// | 400000                     |
// |     Default Font           |
// |     max 0x2800             |
// | FontAll.bin                |
// |  start:0x400000 size:0x10F0|
// | consolas16x26_606C90.bin   |
// |  start:0x402000 size:0x1B38|
// |                            |
// | other test fonts           |
// | test16x32xA0_A0_A0_A0      |
// |  start:0x404000 size:0x2800|
// | test16x32xA0_20_20_20      |
// |  start:0x407000 size:0x2800|
// | consolas22_16x26_2BPP      |
// |  start:0x40A000 size:0x2700|
// | otehr                      |
// |  start:0x40D00             |
// +----------------------------+    
// | 410000                     |
// |     MENU IMG               |
// |                            |
// |                            |
// | 7FFFFF                     |
// +----------------------------+    
// | test image                 |
// |                            |
// |                            |
// +----------------------------+    
// 
//
// detail DEMO IMG MAP
// +----------------------------+    
// | PIGEON                     |
// +----------------------------+    
// | ROSE                       |
// +----------------------------+    
// | LUT for PIGEON&ROSE        |
// +----------------------------+    
// | GENESIS & LUT              |
// +----------------------------+    
// | GRID & LUT                 |
// +----------------------------+    
// | MESSAGE & LUT              |
// +----------------------------+    
// | COMPASS & LUT              |
// +----------------------------+    
// | Dynamic Grid & LUT         |
// +----------------------------+    
// | Dynamic Message & LUT      |
// +----------------------------+    

#include "Config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "Global.h"
#include "CPU.h"
#include "printf.h"
#include "util.h"
#include "monitor.h"

#include "I2C.h"
#include "InputCtrl.h"

#include "SOsd.h"
#include "FOsd.h"

#include "SpiFlashMap.h"
#include "SOsdMenu.h"

#ifdef SUPPORT_SPIOSD

#define MAP0_START	SFLASH_IMG_ADDR
//#define MAP0_START	0

//===================================
//BKANG test area
//0x300000
//===================================



code image_info_t img_main_test1_header = {1, 0x86, 800, 480 /*,0x01803E*/};
code image_info_t img_main_test2_header = {1, 0x86, 800, 480 /*,0x015307*/};
code image_info_t img_main_test3_header = {1, 0x86, 800, 480 /*,0x0136E8*/};


code image_item_info_t img_main_test1	= {0, MENU_TEST_FLASH_START+0x000000, &img_main_test1_header, 0xff};    
code image_item_info_t img_main_test2	= {0, MENU_TEST_FLASH_START+0x01803E, &img_main_test1_header, 0xff};    
code image_item_info_t img_main_test3	= {0, MENU_TEST_FLASH_START+0x02D345, &img_main_test1_header, 0xff};    





//===============
// DEFAULT FONT
// 0x400000
//===============


code WORD default_LUT_bpp2[4] 		= { 0x0000,0x001F,0xF800,0xFFFF };
code WORD default_LUT_bpp3[8] 		= { 0x0000,0x001F,0x07E0,0x07FF,0xF800,0xF81F,0xFFE0,0xFFFF };
code WORD default_LUT_bpp4[16] 		= { 0x0000,0x0010,0x0400,0x0410,0x8000,0x8010,0x8400,0x8410,
										0xC618,0x001F,0x07E0,0x07FF,0xF800,0xF81F,0xFFE0,0xFFFF	};

//code WORD consolas_LUT_bpp2[4] 	= {	0x0000,0x001F,0xF800,0xFFFF };
//code WORD consolas_LUT_bpp3[8] 	= { 0x0000,0x001F,0x07E0,0x07FF,0xF800,0xF81F,0xFFE0,0xFFFF };
//code WORD consolas_LUT_bpp4[16] 	= { 0x07FF,0x20E3,0xF79E,0x62E8,0xE104,0xA944,0x39A6,0x7BAC,
// 											0x51A6,0xC617,0x9CD1,0xB5B5,0x9BC9,0xDD85,0xF643,0xAC87 };

//code WORD graynum_LUT_bpp2[4] 	= { 0xF7DE,0x0000,0x5AAB,0xC000  };
code WORD graynum_LUT_bpp3[8] 		= { 0xFFFF,0x0000,0xDEDB,0x9492,0x6B6D,0xB5B6,0x4A49,0x2124 };
//code WORD graynum_LUT_bpp4[16] 	= {	0xD6BA,0x20E3,0xF79E,0x62E8,0xE104,0xA944,0x39A6,0x7BAC,
//										0x51A6,0xC617,0x9CD1,0xB5B5,0x9BC9,0xDD85,0xF643,0xAC87};


//TW8835 max FontRam size is 0x2800. But FW use 0x3000 space. The remain 0x800 will be used for Font Information.
//                                              loc,      size    W   H   2BPP   3BPP   4BPP   MAX    palette for 2bpp,3bpp,4bpp
code FONT_SPI_INFO_t default_font 		   	= { 0x400000, 0x27F9, 12, 18, 0x100, 0x120, 0x15F, 0x17B, default_LUT_bpp2, default_LUT_bpp3, default_LUT_bpp4 };
code FONT_SPI_INFO_t consolas16x26_606C90 	= { 0x403000, 0x2080, 16, 26, 0x060, 0x06C, 0x090, 0x0A0, NULL, NULL, NULL };
code FONT_SPI_INFO_t consolas16x26_graynum 	= { 0x406000, 0x0618, 16, 26, 0x000, 0x000, 0x01E, 0x01E, NULL, graynum_LUT_bpp3, NULL };
code FONT_SPI_INFO_t kor_font		 		= { 0x409000, 0x0A20, 12, 18, 0x000, 0x000, 0x000, 0x060, NULL, NULL, NULL };
code FONT_SPI_INFO_t ram_font		 		= { 0x40B000, 0x2080, 16, 18, 0x060, 0x06C, 0x090, 0x0A0, NULL, NULL, NULL };
//next 0x400000+0xB000
//next you have to move the menu images.


//===================================
// for TEST
FAR CONST MY_SLIDEIMAGE test_IMG[] = {
    { MAP0_START+0x0EF71A, 0x0100, 0x002B1D },    // Test_PBARPTR100_64
};
FAR CONST MY_RLE_INFO test_INFO[] = {
	{ 0x60, 327,45  },		//Test_PBARPTR100_64
};


//====================================================
// MENU IMAGE MAP
// 0x410000
//====================================================

//for fast
code image_info_t img_navi_close0_header 	=  {1, 0x70, 0x30, 0x30 };
code image_info_t img_navi_close1_header 	=  {1, 0x70, 0x30, 0x30 };
code image_info_t img_navi_setup0_header 	=  {1, 0x70, 0x30, 0x30 };
code image_info_t img_navi_setup1_header 	=  {1, 0x70, 0x30, 0x30 };



code image_info_t img_main_input_header =  {1, 0x80, 118, 110 /*,0x01803E*/};
code image_info_t img_main_audio_header =  {1, 0x80, 106, 114 /*,0x01803E*/};
code image_info_t img_main_system_header = {1, 0x80, 138, 117 /*,0x01803E*/};
code image_info_t img_wait_header 		 = {1, 0x70,  48, 50 /*,0x01803E*/};

code image_info_t img_input_bg_bottom_header 	= {1, 0x88, 0x320, 0x042 }; //:45F13D:49 54 88 88 20 03 42 00 95 18 00 00 01 FF 60 01 
code image_info_t img_input_nodvi_bg_top_header = {1, 0x80, 0x320, 0x046 }; //:46EFD2:49 54 88 00 20 03 46 00 C0 DA 00 00 01 FF 60 01 
code image_info_t img_input_select_header		= {1, 0x80, 0x009, 0x009 }; //:47CEA2:49 54 88 00 09 00 09 00 51 00 00 00 01 FF 60 01 
code image_info_t img_input_cvbs0_header 	= {1, 0x80, 0x3E, 0x40 };
code image_info_t img_input_cvbs1_header 	= {1, 0x80, 0x3E, 0x40 };
code image_info_t img_input_svideo0_header 	= {1, 0x80, 0x4F, 0x3E };
code image_info_t img_input_svideo1_header 	= {1, 0x80, 0x4F, 0x3E };
code image_info_t img_input_ypbpr0_header 	= {1, 0x80, 0x46, 0x42 };
code image_info_t img_input_ypbpr1_header 	= {1, 0x80, 0x46, 0x42 };
code image_info_t img_input_pc0_header 		= {1, 0x80, 0x47, 0x3F };
code image_info_t img_input_pc1_header 		= {1, 0x80, 0x47, 0x3F };
//code image_info_t img_input_dvi0_header 	= {1, 0x80, 0x4F, 0x3E };
//code image_info_t img_input_dvi1_header 	= {1, 0x80, 0x4F, 0x3E };
code image_info_t img_input_hdmi0_header 	= {1, 0x80, 0x48, 0x3F };
code image_info_t img_input_hdmi1_header 	= {1, 0x80, 0x48, 0x3F };
code image_info_t img_input_ext0_header 	= {1, 0x80, 0x3D, 0x3D };
code image_info_t img_input_ext1_header 	= {1, 0x80, 0x3D, 0x3D };
code image_info_t img_input_return0_header 	= {1, 0x70, 0x22, 0x22 };
code image_info_t img_input_return1_header 	= {1, 0x70, 0x22, 0x22 };







//code image_item_info_t img_ = {+0x000000, 0x0010F0 },    // FontAll 
code image_item_info_t img_logo							= {1, MENU_B_FLASH_START+0x000000, NULL,	0xff};    // Intersil-Techwell
code image_item_info_t img_navi_menu  					= {1, MENU_B_FLASH_START+0x0049B1, NULL,	0x00};    // img_navi_bg 
code image_item_info_t img_navi_return  				= {1, MENU_B_FLASH_START+0x0071D1, NULL,	0xff};    // img_navi_return 
code image_item_info_t img_navi_return1  				= {1, MENU_B_FLASH_START+0x007CE1, NULL,	0xff};    // img_navi_return1 
code image_item_info_t img_navi_home  					= {1, MENU_B_FLASH_START+0x0087F1, NULL,	0xff};    // img_navi_home 
code image_item_info_t img_navi_home1  					= {1, MENU_B_FLASH_START+0x009301, NULL,	0xff};    // img_navi_home1 
code image_item_info_t img_navi_close  					= {2, MENU_B_FLASH_START+0x009E11, &img_navi_close0_header,	0xff};    // img_navi_close 
code image_item_info_t img_navi_close1  				= {1, MENU_B_FLASH_START+0x00A921, NULL,	0xff};    // img_navi_close1 
code image_item_info_t img_navi_demo  					= {1, MENU_B_FLASH_START+0x00B431, NULL,	0xff};    // img_navi_demo 
code image_item_info_t img_navi_demo1  					= {1, MENU_B_FLASH_START+0x00BF41, NULL,	0xff};    // img_navi_demo1 
code image_item_info_t img_navi_setup  					= {2, MENU_B_FLASH_START+0x00CA51, &img_navi_setup0_header,	0xff};    // img_navi_setup 
code image_item_info_t img_navi_setup1  				= {1, MENU_B_FLASH_START+0x00D561, NULL,	0xff};    // img_navi_setup1 
code image_item_info_t img_main_bg  					= {1, MENU_B_FLASH_START+0x00E071, NULL,	0xff};    // img_main_bg 
code image_item_info_t img_main_input  					= {2, MENU_B_FLASH_START+0x0236A7, &img_main_input_header,	0xff};    // img_main_input 
code image_item_info_t img_main_input1  				= {1, MENU_B_FLASH_START+0x026D6B, NULL,	0xff};    // img_main_input1 
code image_item_info_t img_main_audio  					= {2, MENU_B_FLASH_START+0x02A42F, &img_main_audio_header,	0xff};    // img_main_audio 
code image_item_info_t img_main_audio1  				= {1, MENU_B_FLASH_START+0x02D773, NULL,	0xff};    // img_main_audio1 
code image_item_info_t img_main_system  				= {2, MENU_B_FLASH_START+0x030AB7, &img_main_system_header,	0xff};    // img_main_system 
code image_item_info_t img_main_system1  				= {1, MENU_B_FLASH_START+0x034DD9, NULL,	0xff};    // img_main_system1 
code image_item_info_t img_main_gps  					= {1, MENU_B_FLASH_START+0x0390FB, NULL,	0xff};    // img_main_gps 
code image_item_info_t img_main_gps1  					= {1, MENU_B_FLASH_START+0x03C925, NULL,	0xff};    // img_main_gps1 
code image_item_info_t img_main_phone  					= {1, MENU_B_FLASH_START+0x04014F, NULL,	0xff};    // img_main_phone 
code image_item_info_t img_main_phone1  				= {1, MENU_B_FLASH_START+0x043779, NULL,	0xff};    // img_main_phone1 
code image_item_info_t img_main_carinfo  				= {1, MENU_B_FLASH_START+0x046DA3, NULL,	0xff};    // img_main_carinfo 
code image_item_info_t img_main_carinfo1  				= {1, MENU_B_FLASH_START+0x04AF70, NULL,	0xff};    // img_main_carinfo1 
code image_item_info_t img_input_bg_bottom  			= {2, MENU_B_FLASH_START+0x04F13D, &img_input_bg_bottom_header,	0x5B};    // img_input_bg_bottom 
code image_item_info_t img_input_bg_top  				= {1, MENU_B_FLASH_START+0x050DE2, NULL,	0x5B};    // img_input_bg_top 
code image_item_info_t img_input_nodvi_bg_top  			= {2, MENU_B_FLASH_START+0x05EFD2, &img_input_nodvi_bg_top_header,	0x5B};    // img_input_nodvi_bg_top 
code image_item_info_t img_input_select  				= {2, MENU_B_FLASH_START+0x06CEA2, &img_input_select_header,	0x5B};    // img_input_select 
code image_item_info_t img_input_cvbs 		 			= {2, MENU_B_FLASH_START+0x06D303, &img_input_cvbs0_header,	0xff};    // img_input_cvbs 
code image_item_info_t img_input_cvbs1 		 			= {1, MENU_B_FLASH_START+0x06E693, NULL,	0xff};    // img_input_cvbs1 
code image_item_info_t img_input_svideo  				= {2, MENU_B_FLASH_START+0x06FA63, &img_input_svideo0_header,	0xff};    // img_input_svideo 
code image_item_info_t img_input_svideo1  				= {1, MENU_B_FLASH_START+0x071195, NULL,	0xff};    // img_input_svideo1 
code image_item_info_t img_input_ypbpr  				= {2, MENU_B_FLASH_START+0x0728C7, &img_input_ypbpr0_header,	0xff};    // img_input_Ypbpr 
code image_item_info_t img_input_ypbpr1  				= {1, MENU_B_FLASH_START+0x073EE3, NULL,	0xff};    // img_input_Ypbpr1 
code image_item_info_t img_input_pc  					= {2, MENU_B_FLASH_START+0x0754BD, &img_input_pc0_header,	0xff};    // img_input_pc 
code image_item_info_t img_input_pc1  					= {1, MENU_B_FLASH_START+0x076A46, NULL,	0xff};    // img_input_pc1 
code image_item_info_t img_input_dvi  					= {1, MENU_B_FLASH_START+0x078004, NULL,	0xff};    // img_input_dvi 
code image_item_info_t img_input_dvi1  					= {1, MENU_B_FLASH_START+0x079452, NULL,	0xff};    // img_input_dvi1 
code image_item_info_t img_input_hdmi  					= {2, MENU_B_FLASH_START+0x07A8A0, &img_input_hdmi0_header,	0xff};    // img_input_hdmi 
code image_item_info_t img_input_hdmi1  				= {1, MENU_B_FLASH_START+0x07BE68, NULL,	0xff};    // img_input_hdmi1 
code image_item_info_t img_input_ext  					= {2, MENU_B_FLASH_START+0x07D3B2, &img_input_ext0_header,	0xff};    // img_input_ext 
code image_item_info_t img_input_ext1  					= {1, MENU_B_FLASH_START+0x07E64B, NULL,	0xFF};    // img_input_ext1 
code image_item_info_t img_input_return  				= {2, MENU_B_FLASH_START+0x07F8E4, &img_input_return0_header,	0xff};    // img_input_return 
code image_item_info_t img_input_return1  				= {1, MENU_B_FLASH_START+0x07FF78, NULL,	0xFF};    // img_input_return1 
code image_item_info_t img_audio_bg			  			= {1, MENU_B_FLASH_START+0x08060C, NULL,	0xff};    // img_audio_bg 
code image_item_info_t img_system_bg_bottom  			= {1, MENU_B_FLASH_START+0x090CCC, NULL,	0x61};    // img_system_bg_bottom 
code image_item_info_t img_system_bg_top  				= {1, MENU_B_FLASH_START+0x09277E, NULL,	0x61};    // img_system_bg_top 
code image_item_info_t img_system_touch  				= {1, MENU_B_FLASH_START+0x0A096E, NULL,	0xff};    // img_system_touch 
code image_item_info_t img_system_touch1  				= {1, MENU_B_FLASH_START+0x0A19F0, NULL,	0xff};    // img_system_touch1 
code image_item_info_t img_system_display 				= {1, MENU_B_FLASH_START+0x0A2A72, NULL,	0xff};    // img_system_display 
code image_item_info_t img_system_display1 				= {1, MENU_B_FLASH_START+0x0A3D85, NULL,	0xff};    // img_system_display1 
code image_item_info_t img_system_btooth  				= {1, MENU_B_FLASH_START+0x0A5098, NULL,	0xff};    // img_system_btooth 
code image_item_info_t img_system_btooth1  				= {1, MENU_B_FLASH_START+0x0A627A, NULL,	0xff};    // img_system_btooth1 
code image_item_info_t img_system_restore  				= {1, MENU_B_FLASH_START+0x0A7422, NULL,	0xff};    // img_system_restore 
code image_item_info_t img_system_restore1  			= {1, MENU_B_FLASH_START+0x0A846E, NULL,	0xff};    // img_system_restore1 
code image_item_info_t img_system_sys_info  			= {1, MENU_B_FLASH_START+0x0A94BA, NULL,	0xff};    // img_system_sys_info 
code image_item_info_t img_system_sys_info1  			= {1, MENU_B_FLASH_START+0x0AA662, NULL,	0xff};    // img_system_sys_info1 
code image_item_info_t img_gps_bg			  			= {1, MENU_B_FLASH_START+0x0AB80A, NULL,	0xff};    // img-gps-bg 
code image_item_info_t img_phone_bg			  			= {1, MENU_B_FLASH_START+0x0DCEAE, NULL,	0xff};    // img-phone-bg
code image_item_info_t img_phone_00 					= {1, MENU_B_FLASH_START+0x0E8D44, NULL,	0xff};    // img_phone_00 	 
code image_item_info_t img_phone_01 					= {1, MENU_B_FLASH_START+0x0E9BE8, NULL,	0xff};    // img_phone_01 	 
code image_item_info_t img_phone_10 					= {1, MENU_B_FLASH_START+0x0EAAD2, NULL,	0xff};    // img_phone_10 	 
code image_item_info_t img_phone_11 					= {1, MENU_B_FLASH_START+0x0EB948, NULL,	0xff};    // img_phone_11 	 
code image_item_info_t img_phone_20 					= {1, MENU_B_FLASH_START+0x0EC832, NULL,	0xff};    // img_phone_20 	 
code image_item_info_t img_phone_21 					= {1, MENU_B_FLASH_START+0x0ED6A8, NULL,	0xff};    // img_phone_21 	 
code image_item_info_t img_phone_30 					= {1, MENU_B_FLASH_START+0x0EE592, NULL,	0xff};    // img_phone_30 	 
code image_item_info_t img_phone_31 					= {1, MENU_B_FLASH_START+0x0EF408, NULL,	0xff};    // img_phone_31 	 
code image_item_info_t img_phone_40 					= {1, MENU_B_FLASH_START+0x0F02F2, NULL,	0xff};    // img_phone_40 	 
code image_item_info_t img_phone_41 					= {1, MENU_B_FLASH_START+0x0F1168, NULL,	0xff};    // img_phone_41 	 
code image_item_info_t img_phone_50 					= {1, MENU_B_FLASH_START+0x0F2052, NULL,	0xff};    // img_phone_50 	 
code image_item_info_t img_phone_51 					= {1, MENU_B_FLASH_START+0x0F2EC8, NULL,	0xff};    // img_phone_51 	 
code image_item_info_t img_phone_60 					= {1, MENU_B_FLASH_START+0x0F3DB2, NULL,	0xff};    // img_phone_60 	 
code image_item_info_t img_phone_61 					= {1, MENU_B_FLASH_START+0x0F4C28, NULL,	0xff};    // img_phone_61 	 
code image_item_info_t img_phone_70 					= {1, MENU_B_FLASH_START+0x0F5B12, NULL,	0xff};    // img_phone_70 	 
code image_item_info_t img_phone_71 					= {1, MENU_B_FLASH_START+0x0F6988, NULL,	0xff};    // img_phone_71 	 
code image_item_info_t img_phone_80 					= {1, MENU_B_FLASH_START+0x0F7872, NULL,	0xff};    // img_phone_80 	 
code image_item_info_t img_phone_81 					= {1, MENU_B_FLASH_START+0x0F86E8, NULL,	0xff};    // img_phone_81 	 
code image_item_info_t img_phone_90 					= {1, MENU_B_FLASH_START+0x0F95D2, NULL,	0xff};    // img_phone_90 	 
code image_item_info_t img_phone_91 					= {1, MENU_B_FLASH_START+0x0FA448, NULL,	0xff};    // img_phone_91 	 
code image_item_info_t img_phone_star0 					= {1, MENU_B_FLASH_START+0x0FB332, NULL,	0xff};    // img_phone_star0 	 
code image_item_info_t img_phone_star1 					= {1, MENU_B_FLASH_START+0x0FC1A8, NULL,	0xff};    // img_phone_star1 	 
code image_item_info_t img_phone_sharp0 				= {1, MENU_B_FLASH_START+0x0FD092, NULL,	0xff};    // img_phone_sharp0  
code image_item_info_t img_phone_sharp1 				= {1, MENU_B_FLASH_START+0x0FDF08, NULL,	0xff};    // img_phone_sharp1  
code image_item_info_t img_phone_dial0 					= {1, MENU_B_FLASH_START+0x0FEDF2, NULL,	0xff};    // img_phone_dial0 	 
code image_item_info_t img_phone_dial1 					= {1, MENU_B_FLASH_START+0x101928, NULL,	0xff};    // img_phone_dial1 	 
code image_item_info_t img_phone_up0 					= {1, MENU_B_FLASH_START+0x104572, NULL,	0xff};    // img_phone_up0 	 
code image_item_info_t img_phone_up1 					= {1, MENU_B_FLASH_START+0x1053E8, NULL,	0xff};    // img_phone_up1 	 
code image_item_info_t img_phone_down0 					= {1, MENU_B_FLASH_START+0x1062D2, NULL,	0xff};    // img_phone_down0 	 
code image_item_info_t img_phone_down1 					= {1, MENU_B_FLASH_START+0x107148, NULL,	0xff};    // img_phone_down1 	 
code image_item_info_t img_phone_left0 					= {1, MENU_B_FLASH_START+0x108032, NULL,	0xff};    // img_phone_left0 	 
code image_item_info_t img_phone_left1 					= {1, MENU_B_FLASH_START+0x108EA8, NULL,	0xff};    // img_phone_left1 	 
code image_item_info_t img_phone_right0 				= {1, MENU_B_FLASH_START+0x109D92, NULL,	0xff};    // img_phone_right0  
code image_item_info_t img_phone_right1 				= {1, MENU_B_FLASH_START+0x10AC08, NULL,	0xff};    // img_phone_right1  
code image_item_info_t img_phone_check0 				= {1, MENU_B_FLASH_START+0x10BAF2, NULL,	0xff};    // img_phone_check0  
code image_item_info_t img_phone_check1 				= {1, MENU_B_FLASH_START+0x10C968, NULL,	0xff};    // img_phone_check1  
code image_item_info_t img_phone_help0 					= {1, MENU_B_FLASH_START+0x10D852, NULL,	0xff};    // img_phone_help0 	 
code image_item_info_t img_phone_help1 					= {1, MENU_B_FLASH_START+0x10E70D, NULL,	0xff};    // img_phone_help1 	 
code image_item_info_t img_phone_dir0 					= {1, MENU_B_FLASH_START+0x10F5B1, NULL,	0xff};    // img_phone_dir0 	 
code image_item_info_t img_phone_dir1 					= {1, MENU_B_FLASH_START+0x110FD5, NULL,	0xff};    // img_phone_dir1 	 
code image_item_info_t img_phone_set0 					= {1, MENU_B_FLASH_START+0x1129F9, NULL,	0xff};    // img_phone_set0 	 
code image_item_info_t img_phone_set1 					= {1, MENU_B_FLASH_START+0x11441D, NULL,	0xff};    // img_phone_set1 	 
code image_item_info_t img_phone_msg0 					= {1, MENU_B_FLASH_START+0x115EC7, NULL,	0xff};    // img_phone_msg0 	 
code image_item_info_t img_phone_msg1 					= {1, MENU_B_FLASH_START+0x1178EB, NULL,	0xff};    // img_phone_msg1 	 
code image_item_info_t img_phone_menu0 					= {1, MENU_B_FLASH_START+0x11930F, NULL,	0xff};    // img_phone_menu0 	 
code image_item_info_t img_phone_menu1 					= {1, MENU_B_FLASH_START+0x11AD33, NULL,	0xff};    // img_phone_menu1 	 
code image_item_info_t img_carinfo_bg			  		= {1, MENU_B_FLASH_START+0x11C757, NULL,	0xff};    // img_carinfo_bg 
code image_item_info_t img_demo_bg  					= {1, MENU_B_FLASH_START+0x1296CF, NULL,	0xff};    // img_demo_bg 
code image_item_info_t img_demo_grid  					= {1, MENU_B_FLASH_START+0x13BA74, NULL,	0xff};    // img_demo_grid  	
code image_item_info_t img_demo_grid1  					= {1, MENU_B_FLASH_START+0x13E9F0, NULL,	0xff};    // img_demo_grid1  	
code image_item_info_t img_demo_rose  					= {1, MENU_B_FLASH_START+0x14196C, NULL,	0xff};    // img_demo_rose  	
code image_item_info_t img_demo_rose1  					= {1, MENU_B_FLASH_START+0x144DC2, NULL,	0xff};    // img_demo_rose1  	
code image_item_info_t img_demo_ani		  				= {1, MENU_B_FLASH_START+0x148218, NULL,	0xff};    // img_demo_ani		
code image_item_info_t img_demo_ani1	  				= {1, MENU_B_FLASH_START+0x14B00C, NULL,	0xff};    // img_demo_ani1	
code image_item_info_t img_demo_palette	  				= {1, MENU_B_FLASH_START+0x14DE6A, NULL,	0xff};    // img_demo_palette	
code image_item_info_t img_demo_palette1  				= {1, MENU_B_FLASH_START+0x150F50, NULL,	0xff};    // img_demo_palette1
code image_item_info_t img_demo_demoA	  				= {1, MENU_B_FLASH_START+0x154036, NULL,	0xff};    // img_demo_demoA	
code image_item_info_t img_demo_demoA1	  				= {1, MENU_B_FLASH_START+0x156E2A, NULL,	0xff};    // img_demo_demoA1	
code image_item_info_t img_demo_demoB	  				= {1, MENU_B_FLASH_START+0x159C1E, NULL,	0xff};    // img_demo_demoB	
code image_item_info_t img_demo_demoB1	  				= {1, MENU_B_FLASH_START+0x15CA12, NULL,	0xff};    // img_demo_demoB1	
code image_item_info_t img_touch_bg  					= {1, MENU_B_FLASH_START+0x15F806, NULL,	0xff};    // img_touch_bg 
code image_item_info_t img_touch_bg_end					= {1, MENU_B_FLASH_START+0x1612C0, NULL,	0xff};    // img_touch_bg 
code image_item_info_t img_touch_button  				= {1, MENU_B_FLASH_START+0x16416B, NULL,	0xff};    // img_touch_button 
code image_item_info_t img_touch_button1  				= {1, MENU_B_FLASH_START+0x16437D, NULL,	0xff};    // img_touch_button1 
code image_item_info_t img_btooth_bg  					= {1, MENU_B_FLASH_START+0x16458F, NULL,	0xff};    // img_btooth_bg 
code image_item_info_t img_yuv_menu_bg  				= {1, MENU_B_FLASH_START+0x188842, NULL,	0x00};    // img_yuv_menu_bg 
code image_item_info_t img_yuv_bright  					= {1, MENU_B_FLASH_START+0x19B0D2, NULL,	0xff};    // img_yuv_bright 
code image_item_info_t img_yuv_bright1  				= {1, MENU_B_FLASH_START+0x19DCB8, NULL,	0xff};    // img_yuv_bright1 
code image_item_info_t img_yuv_contrast  				= {1, MENU_B_FLASH_START+0x1A089E, NULL,	0xff};    // img_yuv_contrast 
code image_item_info_t img_yuv_contrast1  				= {1, MENU_B_FLASH_START+0x1A3484, NULL,	0xff};    // img_yuv_contrast1 
code image_item_info_t img_yuv_hue  					= {1, MENU_B_FLASH_START+0x1A606A, NULL,	0xff};    // img_yuv_hue 
code image_item_info_t img_yuv_hue1  					= {1, MENU_B_FLASH_START+0x1A8C50, NULL,	0xff};    // img_yuv_hue1 
code image_item_info_t img_yuv_saturate  				= {1, MENU_B_FLASH_START+0x1AB836, NULL,	0xff};    // img_yuv_saturate 
code image_item_info_t img_yuv_saturate1  				= {1, MENU_B_FLASH_START+0x1AE41C, NULL,	0xff};    // img_yuv_saturate1 
code image_item_info_t img_yuv_sharp  					= {1, MENU_B_FLASH_START+0x1B1002, NULL,	0xff};    // img_yuv_sharp 
code image_item_info_t img_yuv_sharp1  					= {1, MENU_B_FLASH_START+0x1B3BE8, NULL,	0xff};    // img_yuv_sharp1 
code image_item_info_t img_rgb_menu_bg  				= {1, MENU_B_FLASH_START+0x1B67CE, NULL,	0x00};    // img_rgb_menu_bg 
code image_item_info_t img_rgb_bright  					= {1, MENU_B_FLASH_START+0x1C905E, NULL,	0xff};    // img_rgb_bright 
code image_item_info_t img_rgb_bright1 					= {1, MENU_B_FLASH_START+0x1CD8F4, NULL,	0xff};    // img_rgb_bright1 
code image_item_info_t img_rgb_contrast  				= {1, MENU_B_FLASH_START+0x1D218A, NULL,	0xff};    // img_rgb_contrast 
code image_item_info_t img_rgb_contrast1  				= {1, MENU_B_FLASH_START+0x1D6A20, NULL,	0xff};    // img_rgb_contrast1 
code image_item_info_t img_rgb_color 					= {1, MENU_B_FLASH_START+0x1DB2B6, NULL,	0xff};    // img_rgb_color 
code image_item_info_t img_rgb_color1 					= {1, MENU_B_FLASH_START+0x1DFB4C, NULL,	0xff};    // img_rgb_color1 
code image_item_info_t img_apc_menu_bg  				= {1, MENU_B_FLASH_START+0x1E43E2, NULL,	0x00};    // img_apc_menu_bg 
code image_item_info_t img_apc_bright  					= {1, MENU_B_FLASH_START+0x1F6C72, NULL,	0xff};    // img_apc_bright 
code image_item_info_t img_apc_bright1 					= {1, MENU_B_FLASH_START+0x1F8835, NULL,	0xff};    // img_apc_bright1 
code image_item_info_t img_apc_contrast  				= {1, MENU_B_FLASH_START+0x1FA3F8, NULL,	0xff};    // img_apc_contrast 
code image_item_info_t img_apc_contrast1  				= {1, MENU_B_FLASH_START+0x1FBFBB, NULL,	0xff};    // img_apc_contrast1 
code image_item_info_t img_apc_color	  				= {1, MENU_B_FLASH_START+0x1FDB7E, NULL,	0xff};    // img_apc_color 
code image_item_info_t img_apc_color1	  				= {1, MENU_B_FLASH_START+0x1FF741, NULL,	0xff};    // img_apc_color1
code image_item_info_t img_apc_position  				= {1, MENU_B_FLASH_START+0x201304, NULL,	0xff};    // img_apc_position 
code image_item_info_t img_apc_position1  				= {1, MENU_B_FLASH_START+0x202EC7, NULL,	0xff};    // img_apc_position1 
code image_item_info_t img_apc_phase  					= {1, MENU_B_FLASH_START+0x204A8A, NULL,	0xff};    // img_apc_phase 
code image_item_info_t img_apc_phase1  					= {1, MENU_B_FLASH_START+0x20664D, NULL,	0xff};    // img_apc_phase1 
code image_item_info_t img_apc_pclock  					= {1, MENU_B_FLASH_START+0x208210, NULL,	0xff};    // img_apc_pclock 
code image_item_info_t img_apc_pclock1 					= {1, MENU_B_FLASH_START+0x209DD3, NULL,	0xff};    // img_apc_pclock1 
code image_item_info_t img_apc_autoadj  				= {1, MENU_B_FLASH_START+0x20B996, NULL,	0xff};    // img_apc_autoadj 
code image_item_info_t img_apc_autoadj1  				= {1, MENU_B_FLASH_START+0x20D559, NULL,	0xff};    // img_apc_autoadj1 
code image_item_info_t img_apc_autocolor  				= {1, MENU_B_FLASH_START+0x20F11C, NULL,	0xff};    // img_apc_autocolor
code image_item_info_t img_apc_autocolor1  				= {1, MENU_B_FLASH_START+0x210CDF, NULL,	0xff};    // img_apc_autocolor1
code image_item_info_t img_hdmi_menu_bg  				= {1, MENU_B_FLASH_START+0x2128A2, NULL,	0x00};    // img_hdmi_menu_bg 
code image_item_info_t img_hdmi_mode  					= {1, MENU_B_FLASH_START+0x225132, NULL,	0xff};    // img_hdmi_mode 
code image_item_info_t img_hdmi_mode1  					= {1, MENU_B_FLASH_START+0x22BDA4, NULL,	0xff};    // img_hdmi_mode1 
code image_item_info_t img_hdmi_setting  				= {1, MENU_B_FLASH_START+0x232964, NULL,	0xff};    // img_hdmi_setting 
code image_item_info_t img_hdmi_setting1  				= {1, MENU_B_FLASH_START+0x2395D6, NULL,	0xff};    // img_hdmi_setting1 
code image_item_info_t img_display_bg  					= {1, MENU_B_FLASH_START+0x240248, NULL,	0x00};    // img_display_bg 
code image_item_info_t img_display_aspect  				= {1, MENU_B_FLASH_START+0x252AD8, NULL,	0xff};    // img_display_aspect 
code image_item_info_t img_display_aspect1 				= {1, MENU_B_FLASH_START+0x2556BE, NULL,	0xff};    // img_display_aspect1 
code image_item_info_t img_display_osd  				= {1, MENU_B_FLASH_START+0x2582A4, NULL,	0xff};    // img_display_osd 
code image_item_info_t img_display_osd1  				= {1, MENU_B_FLASH_START+0x25AE8A, NULL,	0xff};    // img_display_osd1 
code image_item_info_t img_display_flip  				= {1, MENU_B_FLASH_START+0x25DA70, NULL,	0xff};    // img_display_flip 
code image_item_info_t img_display_flip1  				= {1, MENU_B_FLASH_START+0x260656, NULL,	0xff};    // img_display_flip1 
code image_item_info_t img_display_backlight  			= {1, MENU_B_FLASH_START+0x26323C, NULL,	0xff};    // img_display_backlight 
code image_item_info_t img_display_backlight1  			= {1, MENU_B_FLASH_START+0x265E22, NULL,	0xff};    // img_display_backlight1 
code image_item_info_t img_display_resolution  			= {1, MENU_B_FLASH_START+0x268A08, NULL,	0xff};    // img_display_resolution 
code image_item_info_t img_display_resolution1  		= {1, MENU_B_FLASH_START+0x26B5EE, NULL,	0xff};    // img_display_resolution1 
code image_item_info_t img_osd_bg  						= {1, MENU_B_FLASH_START+0x26E1D4, NULL,	0x00};    // img_osd_bg 
code image_item_info_t img_osd_timer  					= {1, MENU_B_FLASH_START+0x280A64, NULL,	0xff};    // img_osd_timer 
code image_item_info_t img_osd_timer1  					= {1, MENU_B_FLASH_START+0x2876D6, NULL,	0xff};    // img_osd_timer1 
code image_item_info_t img_osd_trans  					= {1, MENU_B_FLASH_START+0x28E348, NULL,	0xff};    // img_osd_trans 
code image_item_info_t img_osd_trans1  					= {1, MENU_B_FLASH_START+0x294FBA, NULL,	0xff};    // img_osd_trans1 
code image_item_info_t img_dialog_ok		  			= {1, MENU_B_FLASH_START+0x29BC2C, NULL,	0xff};    // img_dialog_ok 
code image_item_info_t img_dialog_ok1	  				= {1, MENU_B_FLASH_START+0x29C83C, NULL,	0xff};    // img_dialog_ok1 
code image_item_info_t img_dialog_cancel  				= {1, MENU_B_FLASH_START+0x29D44C, NULL,	0xff};    // img_dialog_cancel 
code image_item_info_t img_dialog_cancel1  				= {1, MENU_B_FLASH_START+0x29E69C, NULL,	0xff};    // img_dialog_cancel1 
code image_item_info_t img_autoadj_bg  					= {1, MENU_B_FLASH_START+0x29F8EC, NULL,	0xff};    // img_autoadj_bg 
code image_item_info_t img_autocolor_bg  				= {1, MENU_B_FLASH_START+0x2A2FF3, NULL,	0xff};    // img_autocolor_bg 
code image_item_info_t img_flip_bg  					= {1, MENU_B_FLASH_START+0x2A602D, NULL,	0xff};    // img_flip_bg 
code image_item_info_t img_sysrestore_bg  				= {1, MENU_B_FLASH_START+0x2A81A9, NULL,	0xff};    // img_sysrestore_bg 
code image_item_info_t img_sysinfo_bg  					= {1, MENU_B_FLASH_START+0x2AB31C, NULL,	0xff};    // img_sysinfo_bg 
code image_item_info_t img_resolution_bg  				= {1, MENU_B_FLASH_START+0x2ACDD7, NULL,	0xff};    // img_resolution_bg 
code image_item_info_t img_slide_bg  					= {1, MENU_B_FLASH_START+0x2AE2B2, NULL,	0xff};    // img_slide_bg 
code image_item_info_t img_slide3_bg  					= {1, MENU_B_FLASH_START+0x2AFAF3, NULL,	0xff};    // img_slide3_bg 
code image_item_info_t img_slide_gray  					= {1, MENU_B_FLASH_START+0x2B1444, NULL,	0xff};    // img_slide_gray 
code image_item_info_t img_slide_red  					= {1, MENU_B_FLASH_START+0x2B1C45, NULL,	0xff};    // img_slide_red 
code image_item_info_t img_slide_left  					= {1, MENU_B_FLASH_START+0x2B2446, NULL,	0xff};    // img_slide_left 
code image_item_info_t img_slide_left1  				= {1, MENU_B_FLASH_START+0x2B2EEE, NULL,	0xff};    // img_slide_lef1t 
code image_item_info_t img_slide_right  				= {1, MENU_B_FLASH_START+0x2B3996, NULL,	0xff};    // img_slide_right 
code image_item_info_t img_slide_right1  				= {1, MENU_B_FLASH_START+0x2B443E, NULL,	0xff};    // img_slide_right1 
code image_item_info_t img_slide_backlight  			= {1, MENU_B_FLASH_START+0x2B4EE6, NULL,	0xff};    // img_slide_backlight 
code image_item_info_t img_slide_bright  				= {1, MENU_B_FLASH_START+0x2B55D5, NULL,	0xff};    // img_slide_bright 
code image_item_info_t img_slide_clock  				= {1, MENU_B_FLASH_START+0x2B5D8A, NULL,	0xff};    // img_slide_clock 
code image_item_info_t img_slide_contrast  				= {1, MENU_B_FLASH_START+0x2B6431, NULL,	0xff};    // img_slide_contrast 
code image_item_info_t img_slide_rgb	  				= {1, MENU_B_FLASH_START+0x2B6970, NULL,	0xff};    // img_slide_rgb 
code image_item_info_t img_slide_hue  					= {1, MENU_B_FLASH_START+0x2B6C7E, NULL,	0xff};    // img_slide_hue 
code image_item_info_t img_slide_phase  				= {1, MENU_B_FLASH_START+0x2B6F80, NULL,	0xff};    // img_slide_phase 
code image_item_info_t img_slide_saturate  				= {1, MENU_B_FLASH_START+0x2B7393, NULL,	0xff};    // img_slide_saturate 
code image_item_info_t img_slide_sharp  				= {1, MENU_B_FLASH_START+0x2B79B9, NULL,	0xff};    // img_slide_sharp 
code image_item_info_t img_slide_timer  				= {1, MENU_B_FLASH_START+0x2B813B, NULL,	0xff};    // img_slide_timer 
code image_item_info_t img_slide_trans  				= {1, MENU_B_FLASH_START+0x2B8530, NULL,	0xff};    // img_slide_trasn 
code image_item_info_t img_position_bg  				= {1, MENU_B_FLASH_START+0x2B8EC2, NULL,	0xff};    // img_position_bg 
code image_item_info_t img_position_box_gray  			= {1, MENU_B_FLASH_START+0x2BBEDF, NULL,	0xff};    // img_position_box_gray 
code image_item_info_t img_position_box_red  			= {1, MENU_B_FLASH_START+0x2C00FF, NULL,	0xff};    // img_position_box_red 
code image_item_info_t img_position_up  				= {1, MENU_B_FLASH_START+0x2C427B, NULL,	0xff};    // img_position_up 
code image_item_info_t img_position_down  				= {1, MENU_B_FLASH_START+0x2C4953, NULL,	0xff};    // img_position_down 
code image_item_info_t img_position_left  				= {1, MENU_B_FLASH_START+0x2C502B, NULL,	0xff};    // img_position_left 
code image_item_info_t img_position_right  				= {1, MENU_B_FLASH_START+0x2C5703, NULL,	0xff};    // img_position_right 
code image_item_info_t img_popup_aspect_bg  			= {1, MENU_B_FLASH_START+0x2C5DDB, NULL,	0xff};    // img_aspect_bg 
code image_item_info_t img_popup_aspect_normal 			= {1, MENU_B_FLASH_START+0x2C9F88, NULL,	0xff};    // img_aspect_normal 
code image_item_info_t img_popup_aspect_normal1 		= {1, MENU_B_FLASH_START+0x2CA8DE, NULL,	0xff};    // img_aspect_normal1 
code image_item_info_t img_popup_aspect_normal_select	= {1, MENU_B_FLASH_START+0x2CB219, NULL,	0xff};    // img_aspect_normal_select 
code image_item_info_t img_popup_aspect_zoom  			= {1, MENU_B_FLASH_START+0x2CBA40, NULL,	0xff};    // img_aspect_zoom 
code image_item_info_t img_popup_aspect_zoom1  			= {1, MENU_B_FLASH_START+0x2CC1C8, NULL,	0xff};    // img_aspect_zoom1 
code image_item_info_t img_popup_aspect_zoom_select		= {1, MENU_B_FLASH_START+0x2CC9D1, NULL,	0xff};    // img_aspect_zoom_select 
code image_item_info_t img_popup_aspect_full  			= {1, MENU_B_FLASH_START+0x2CD1DA, NULL,	0xff};    // img_aspect_full
code image_item_info_t img_popup_aspect_full1  			= {1, MENU_B_FLASH_START+0x2CD83C, NULL,	0xff};    // img_aspect_full1
code image_item_info_t img_popup_aspect_full_select		= {1, MENU_B_FLASH_START+0x2CDEC8, NULL,	0xff};    // img_aspect_full_select
code image_item_info_t img_popup_aspect_pano  			= {1, MENU_B_FLASH_START+0x2CE6A4, NULL,	0xff};    // img_aspect_pano
code image_item_info_t img_popup_aspect_pano1  			= {1, MENU_B_FLASH_START+0x2CF1DD, NULL,	0xff};    // img_aspect_pano1
code image_item_info_t img_popup_aspect_pano_sel		= {1, MENU_B_FLASH_START+0x2CFD31, NULL,	0xff};    // img_aspect_pano_sel
code image_item_info_t img_dvi_mode_bg  				= {1, MENU_B_FLASH_START+0x2D050D, NULL,	0xff};    // img_dvi_mode_bg 
code image_item_info_t img_dvi_mode_24bit  				= {1, MENU_B_FLASH_START+0x2E33E5, NULL,	0xff};    // img_dvi_mode_24bit 
code image_item_info_t img_dvi_mode_24bit1  			= {1, MENU_B_FLASH_START+0x2E4415, NULL,	0xff};    // img_dvi_mode_24bit1 
code image_item_info_t img_dvi_mode_16bit  				= {1, MENU_B_FLASH_START+0x2E5505, NULL,	0xff};    // img_dvi_mode_16bit 
code image_item_info_t img_dvi_mode_16bit1  			= {1, MENU_B_FLASH_START+0x2E65D3, NULL,	0xff};    // img_dvi_mode_16bit1 
code image_item_info_t img_dvi_mode_select24  			= {1, MENU_B_FLASH_START+0x2E76E5, NULL,	0xff};    // img_dvi_mode_select24 
code image_item_info_t img_dvi_mode_select16  			= {1, MENU_B_FLASH_START+0x2E834B, NULL,	0xff};    // img_dvi_mode_select16 
code image_item_info_t img_hdmi_mode_bg  				= {1, MENU_B_FLASH_START+0x2E8FB5, NULL,	0xff};    // img_hdmi_mode_bg 
code image_item_info_t img_hdmi_mode_pc  				= {1, MENU_B_FLASH_START+0x2FBE8D, NULL,	0xff};    // img_hdmi_mode_pc 
code image_item_info_t img_hdmi_mode_pc1  				= {1, MENU_B_FLASH_START+0x2FCD31, NULL,	0xff};    // img_hdmi_mode_pc1 
code image_item_info_t img_hdmi_mode_tv  				= {1, MENU_B_FLASH_START+0x2FDB79, NULL,	0xff};    // img_hdmi_mode_tv 
code image_item_info_t img_hdmi_mode_tv1  				= {1, MENU_B_FLASH_START+0x2FEA05, NULL,	0xff};    // img_hdmi_mode_tv1 
code image_item_info_t img_hdmi_mode_selectPC  			= {1, MENU_B_FLASH_START+0x2FF86F, NULL,	0xff};    // img_hdmi_mode_selectPC 
code image_item_info_t img_hdmi_mode_selectTV  			= {1, MENU_B_FLASH_START+0x30091F, NULL,	0xff};    // img_hdmi_mode_selectTV 
code image_item_info_t img_wait  						= {2, MENU_B_FLASH_START+0x301A02, &img_wait_header,	0x00};    // img_wait. 48x50x10 


//=================================
// FPGA TEST IMAGE
//=================================
code image_item_info_t img_fpga_200			= {1, FPGA_TEST_IMG+0x000000, NULL, 0xff};
code image_item_info_t img_fpga_300			= {1, FPGA_TEST_IMG+0x0005D7, NULL, 0xff};
code image_item_info_t img_fpga_400			= {1, FPGA_TEST_IMG+0x000AE9, NULL, 0xff};
code image_item_info_t img_fpga_800			= {1, FPGA_TEST_IMG+0x000FF3, NULL, 0xff};

#define FPGA_TEST_IMG2	0x330000
code image_item_info_t img_bgr_bar			= {1, FPGA_TEST_IMG2+0x000000, NULL, 0xff};
code image_item_info_t img_alpha_bar		= {1, FPGA_TEST_IMG2+0x013C90, NULL, 0xff};



typedef struct MonOsdData_s {
	struct image_item_info_s *image;
	BYTE name[10];	
} MonOsdData_t;

code MonOsdData_t MonSOsdImgTable[] = {
	{ &img_logo, 			"logo     "},
	{ &img_main_bg, 		"main_bg  "},
	{ &img_main_input,		"input0   " },
	{ &img_main_input1,		"input1   " },
	{ &img_main_system,		"system0  "},
	{ &img_main_system1,	"system1  "},
	{ &img_input_bg_bottom,	"input_bg0"},
	{ &img_input_bg_top,	"input_bg1"},
	{ &img_input_cvbs,		"cvbs0    "	},
	{ &img_input_cvbs1,		"cvbs1    " },
	{ &img_navi_return,		"return0  "},
	{ &img_navi_return1,	"return1  " },
	//===================================
	{ &img_fpga_200, 		"fpga_200 "},
	{ &img_fpga_300, 		"fpga_300 "},
	{ &img_fpga_400, 		"fpga_400 "},
	{ &img_fpga_800, 		"fpga_800 "},
	//===================================
	{ &img_bgr_bar, 		"bgr_bar "},
	{ &img_alpha_bar, 		"alpha_bar"},
};

extern menu_image_header_t header_table;
extern void MenuPrepareImageHeader(struct image_item_info_s *image);
void MonSOsdImgInfo(void)
{
	struct image_item_info_s *image;
	//image_info_t *info;
	menu_image_header_t *header = &header_table;	//link header buffer.
	BYTE i;

	for(i=0; i < (12+4+2); i++) {
		Printf("\n%02bd %s",i,MonSOsdImgTable[i].name);
		image = MonSOsdImgTable[i].image;
		Printf(" loc:%lx alpha:%bx",image->loc,image->alpha);

		//prepare header
		MenuPrepareImageHeader(image);

		//header info
		Printf(" bpp%bd", header->bpp);
		Printf(" rle%bd", header->rle);
		Printf(" %dx%d", header->dx, header->dy);
		Printf(" alpha:%2bx",image->alpha);
		Printf(" lut%s size:%d*4",header->lut_type? "s": " ", header->lut_size >> 2);		 
	}
}
void MonOsdLutLoad(BYTE img_n, BYTE sosd_win, WORD lut)
{
	struct image_item_info_s *image;
	menu_image_header_t *header = &header_table;	//link header buffer.


	Printf("\nMonOsdLutLoad(%bd,%bd,%d)",img_n,lut);

	//BYTE i;
	image = MonSOsdImgTable[img_n].image;
	//prepare header
	MenuPrepareImageHeader(image);
	
	//Load Palette
	SpiOsdLoadLUT(sosd_win, header->lut_type, lut, header->lut_size, header->lut_loc);
}

extern BYTE UseSOsdHwBuff;
extern WORD SOsdHwBuff_alpha;

void MonOsdImgLoad(BYTE img_n, BYTE sosd_win, WORD item_lut)
{
	struct image_item_info_s *image;
	menu_image_header_t *header = &header_table;	//link header buffer.
//	BYTE i;
	WORD sx,sy;

	Printf("\nMonOsdImgLoad(%bd,%bd,%d)",img_n,sosd_win,item_lut);

#if 0
	UseSOsdHwBuff=1;
#endif
	sx=sy=0;
//	SOsdWinBuffClean(0);

	image = MonSOsdImgTable[img_n].image;

	//prepare header
	MenuPrepareImageHeader(image);


	//see MenuDrawCurrImage
	//fill out sosd_buff
	SpiOsdWinImageLoc( sosd_win, header->image_loc); 
	SpiOsdWinImageSizeWH( sosd_win, header->dx, header->dy );
	SpiOsdWinScreen( sosd_win, sx, sy, header->dx, header->dy );
	if(sosd_win==0) {
		SpiOsdWin0ImageOffsetXY( 0, 0 );
		SpiOsdWin0Animation( 1, 0, 0, 0);
	}
	if(image->alpha != 0xFF)
		SpiOsdWinPixelAlpha( sosd_win, ON );
	else {
		SpiOsdWinGlobalAlpha( sosd_win, 0 /*EE_Read(EEP_OSD_TRANSPARENCY)*/);
	}
	SpiOsdWinPixelWidth(sosd_win, header->bpp);
	SpiOsdWinLutOffset(sosd_win, item_lut);

	SpiOsdWinBuffEnable( sosd_win, ON );
	//
	//write to HW
	//
#if 1
	if(UseSOsdHwBuff) 
	{
		if(header->rle)
			SOsdHwBuffSetRle(sosd_win,header->bpp,header->rle);
		SOsdHwBuffSetLut(sosd_win, /*header->lut_type,*/ item_lut, header->lut_size, header->lut_loc);
	
		//pixel alpha blending. after load Palette
		if(image->alpha != 0xFF)
			SOsdHwBuffSetAlpha(item_lut+image->alpha);

		SOsdWinBuffWrite2Hw(sosd_win, sosd_win); //SOsdHwBuffWrite2Hw();
		UseSOsdHwBuff = 0;

		//update ALPHA
		if(SOsdHwBuff_alpha != 0xFFFF) {
			WriteTW88Page(4);
			WriteTW88(REG410, 0xc3 );    		// LUT Write Mode, En & byte ptr inc.

			if(SOsdHwBuff_alpha >> 8)	WriteTW88(REG410, ReadTW88(REG410) | 0x08);	//support 512 palette
			else            			WriteTW88(REG410, ReadTW88(REG410) & 0xF7);
			WriteTW88(REG411, (BYTE)SOsdHwBuff_alpha ); 	// alpha index
			WriteTW88(REG412, 0x7F/*value*/ ); 			// alpha value

			SOsdHwBuff_alpha = 0xFFFF;
		}
	}
#endif
	//else 
#if 1
	{
		//WaitVBlank(1);
		if(header->rle) {	//need RLE ?
			//SpiOsdEnableRLC(ON);
			SpiOsdRLC( sosd_win, header->bpp,header->rle);
		}	
		else {
			//We using RLE only on the background.
			//if(item == 0) {
			//	SpiOsdDisableRLC(??winno)
			//		SpiOsdEnableRLC(OFF);		//disable RLE
			//	//SpiOsdEnableRLC is not enough. So, assign win0
			//	SpiOsdRLC( 0,0,0); //BK110217
			//}
		}
		WaitVBlank(1);
	
		//Load Palette
//		SpiOsdLoadLUT(header->lut_type, menu_item->lut, header->lut_size, header->lut_loc);
	
		//WaitVBlank(1);
		//update HW
		SOsdWinBuffWrite2Hw(sosd_win, sosd_win);
	
		//pixel alpha blending. after load Palette
		if(image->alpha != 0xFF) {
			SpiOsdPixelAlphaAttr(item_lut+image->alpha, 0x7F);
		}
	}
#endif
}


#endif //..SUPPORT_SPIOSD
