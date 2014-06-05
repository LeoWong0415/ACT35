/**
 * @file
 * SOsdMenuMap.c 
 * @author Brian Kang
 * @version 1.0
 * @section LICENSE
 *	Copyright (C) 2011~2012 Intersil Corporation
 * @section DESCRIPTION
 *	SpiOsd Menu Map
*/
//*****************************************************************************
//
//									TW8835 MENU-B.c
//
//*****************************************************************************
// TW8835 have 2KB XDATA, we have to move the static table datas to CODE segment.
// code segment size: 0x10000 64KB * 4Banks.
// xdata segment size: 0x1000 4KB.
//
//SOSD_WIN1 use LutOffset 0x000. SOsdHwBuffWrite2Hw() need it.	   

//parent
//parent_to_me_item_number.

//curr_menu
//curr_focused_item_number

#include "config.h"
#include "reg.h"
#include "typedefs.h"
#include "TW8835.h"

#include "Global.h"
#include "CPU.h"
#include "printf.h"
#include "util.h"

#include "I2C.h"
#ifdef USE_SFLASH_EEPROM
#include "spi.h"
#endif

#include "SOsd.h"
#include "FOsd.h"
#include "InputCtrl.h"
#include "SpiFlashMap.h"
#include "SOsdMenu.h"
#include "eeprom.h"
#include "ImageCtrl.h"
#include "Settings.h"
#include "Demo.h"

#ifdef SUPPORT_SPIOSD																			  
//==========================
// LEVEL 0 PAGE
//==========================


//-------------------------------------
// main MENU
//-------------------------------------
//
//		Video	Audio	System
//
//
//		GPS		Phone	Information
//
//
//						demo exit
//-------------------------------------
//description
//background: use 8bit RLE. 128 Color. Use SOSD_WIN1
//            color 1: transparent color. normally color 0 is a black.
//focused(pressed): use SOSD_WIN8
//				

code struct menu_sitem_info_s menu_main_page_items[]= {												            //  ^ < V > 
{ TYPE_BG, proc_main_bg,	0,0, 	  ALIGN_TOPLEFT,	&img_main_bg,NULL,     					0,SOSD_WIN1,0x000,  0,0,0,0 },	//0

{ 0, proc_demo_icon,  		697, 424, ALIGN_CENTER,  	&img_navi_demo,&img_navi_demo1,  		0,SOSD_WIN8,0x100,  6,2,3,2 },	//1	
{ 0, proc_exit_icon,  		750, 424, ALIGN_CENTER,  	&img_navi_close,&img_navi_close1,  		0,SOSD_WIN8,0x100,  6,1,3,1 },	//2	

{ 0, proc_main_input, 		149, 89,  ALIGN_CENTER, 	&img_main_input,&img_main_input1,  		0,SOSD_WIN8,0x100,  6,5,6,4 },	//3	
{ 0, proc_main_audio, 		350, 84,  ALIGN_CENTER, 	&img_main_audio,&img_main_audio1,  		0,SOSD_WIN8,0x100,  7,3,7,5 },	//4
{ 0, proc_main_system,		544, 84,  ALIGN_CENTER, 	&img_main_system,&img_main_system1, 	0,SOSD_WIN8,0x100,  8,4,8,3 },	//5
{ 0, proc_main_gps, 		147, 242, ALIGN_CENTER, 	&img_main_gps,&img_main_gps1,    		0,SOSD_WIN8,0x100,  3,8,2,7 },	//6   
{ 0, proc_main_phone, 		351, 243, ALIGN_CENTER, 	&img_main_phone,&img_main_phone1,  		0,SOSD_WIN8,0x100,  4,6,2,8 },	//7
{ 0, proc_main_info,  		551, 246, ALIGN_CENTER, 	&img_main_carinfo,&img_main_carinfo1,  	0,SOSD_WIN8,0x100,  5,7,2,6 }	//8

};
struct menu_page_info_s menu_main_page = {	MENU_TYPE_NORMAL, 3,3, 3,6,	menu_main_page_items, NULL };			


//==========================
// LEVEL 1 PAGE
//==========================

//----------------------
// Source Select MENU
//main->input(video)
//----------------------
//	cvbs svideo Ypbpr rgb dvi hdmi
//
//
//
//
//					return setting exit
//------------------------------------------
//description
//background: use 8bit RLE. 128 Color. Use SOSD_WIN0
//            color 1: transparent color. normally color 0 is a black.
#ifdef SUPPORT_DVI
code struct menu_sitem_info_s menu_input_page_items[]= {													//  ^ < V >
{ TYPE_BG, proc_input_bg,	0,414,	ALIGN_TOPLEFT,	&img_input_bg_bottom,NULL,				0,SOSD_WIN1,0x000,  0,0,0,0  },	//bg_bottom:fixed
{ TYPE_MBG, NULL,			0,0,	ALIGN_TOPLEFT,	&img_input_bg_top,NULL,					0,SOSD_WIN2,0x000,  0,0,0,0  }, //bg_top:move down

{ 0, proc_return_icon,		651,430,ALIGN_CENTER,	&img_input_return,&img_input_return1,	0,SOSD_WIN8,0x100,  MENU_SRC_CVBS,MENU_SRC_EXIT,MENU_SRC_HDMI,MENU_SRC_SETTING },	//14 - 128
{ 0, proc_input_setting,	695,422,ALIGN_CENTER,	&img_navi_setup,&img_navi_setup1,		0,SOSD_WIN8,0x100,  MENU_SRC_CVBS,MENU_SRC_RETURN, MENU_SRC_HDMI,MENU_SRC_EXIT },//15 - 128
{ 0, proc_exit_icon,		750,423,ALIGN_CENTER,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x100,  MENU_SRC_CVBS,MENU_SRC_SETTING, MENU_SRC_HDMI,MENU_SRC_RETURN  },//16 - 128

{ 0, proc_input_cvbs,		104,0,	ALIGN_CENTER,	&img_input_cvbs,&img_input_cvbs1,		0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_BT656,MENU_SRC_RETURN,MENU_SRC_SVIDEO },	//2
{ 0, proc_input_svideo,		184,2,	ALIGN_CENTER,	&img_input_svideo,&img_input_svideo1,	0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_CVBS,MENU_SRC_RETURN,MENU_SRC_YPBPR },	//4
{ 0, proc_input_ypbpr,		276,0,	ALIGN_CENTER,	&img_input_ypbpr,&img_input_ypbpr1,		0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_SVIDEO,MENU_SRC_RETURN,MENU_SRC_PC },	//6
{ 0, proc_input_pc,			365,1,	ALIGN_CENTER,	&img_input_pc,&img_input_pc1,			0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_YPBPR,MENU_SRC_RETURN,MENU_SRC_DVI },	//8
{ 0, proc_input_dvi,		455,1,	ALIGN_CENTER,	&img_input_dvi,&img_input_dvi1,			0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_PC,MENU_SRC_RETURN,MENU_SRC_HDMI },	//10
{ 0, proc_input_hdmi,		539,1,	ALIGN_CENTER,	&img_input_hdmi,&img_input_hdmi1,		0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_DVI,MENU_SRC_RETURN,MENU_SRC_BT656 }, //12 need a disable option
{ 0, proc_input_bt656,		633,3,	ALIGN_CENTER,	&img_input_ext,&img_input_ext1,			0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_HDMI,MENU_SRC_RETURN,MENU_SRC_CVBS }, //12 need a disable option

//select
{ 0, NULL,					104,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //cvbs - max 128
{ 0, NULL,					184,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //svideo - max 128
{ 0, NULL,					276,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //ypbpr - max 128
{ 0, NULL,					365,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //pc - max 128
{ 0, NULL,					455,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //dvi - max 128
{ 0, NULL,					539,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //hdmi - max 128
{ 0, NULL,					633,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  } //ext - max 128
};
struct menu_page_info_s menu_input_page = {	MENU_TYPE_SCRLDN | MENU_TYPE_NOTE, 5,5, 5,7,	menu_input_page_items, NULL };	//auto focus	
#else
code struct menu_sitem_info_s menu_input_page_items[]= {													//  ^ < V >
{ TYPE_BG, proc_input_bg,	0,414,	ALIGN_TOPLEFT,	&img_input_bg_bottom,NULL,				0,SOSD_WIN1,0x000,  0,0,0,0  },	//bg_bottom:fixed
{ TYPE_MBG, NULL,			0,0,	ALIGN_TOPLEFT,	&img_input_nodvi_bg_top,NULL,			0,SOSD_WIN2,0x000,  0,0,0,0  }, //bg_top:move down

{ 0, proc_return_icon,		651,430,ALIGN_CENTER,	&img_input_return,&img_input_return1,	0,SOSD_WIN8,0x100,  MENU_SRC_CVBS,MENU_SRC_EXIT,MENU_SRC_HDMI,MENU_SRC_SETTING },
{ 0, proc_input_setting,	695,422,ALIGN_CENTER,	&img_navi_setup,&img_navi_setup1,		0,SOSD_WIN8,0x100,  MENU_SRC_CVBS,MENU_SRC_RETURN, MENU_SRC_HDMI,MENU_SRC_EXIT },
{ 0, proc_exit_icon,		750,423,ALIGN_CENTER,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x100,  MENU_SRC_CVBS,MENU_SRC_SETTING, MENU_SRC_HDMI,MENU_SRC_RETURN  },

{ 0, proc_input_cvbs,		149,0,	ALIGN_CENTER,	&img_input_cvbs,&img_input_cvbs1,		0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_BT656,MENU_SRC_RETURN,MENU_SRC_SVIDEO },	//2
{ 0, proc_input_svideo,		229,2,	ALIGN_CENTER,	&img_input_svideo,&img_input_svideo1,	0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_CVBS,MENU_SRC_RETURN,MENU_SRC_YPBPR },	//4
{ 0, proc_input_ypbpr,		321,0,	ALIGN_CENTER,	&img_input_ypbpr,&img_input_ypbpr1,		0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_SVIDEO,MENU_SRC_RETURN,MENU_SRC_PC },	//6
{ 0, proc_input_pc,			410,1,	ALIGN_CENTER,	&img_input_pc,&img_input_pc1,			0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_YPBPR,MENU_SRC_RETURN,MENU_SRC_HDMI },	//8
{ 0, proc_input_hdmi,		493,1,	ALIGN_CENTER,	&img_input_hdmi,&img_input_hdmi1,		0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_PC,MENU_SRC_RETURN,MENU_SRC_BT656 }, //12 need a disable option
{ 0, proc_input_bt656,		587,3,	ALIGN_CENTER,	&img_input_ext,&img_input_ext1,			0,SOSD_WIN8,0x100,  MENU_SRC_EXIT,MENU_SRC_HDMI,MENU_SRC_RETURN,MENU_SRC_CVBS }, //12 need a disable option

//select
{ 0, NULL,					149,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //cvbs - max 128
{ 0, NULL,					229,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //svideo - max 128
{ 0, NULL,					321,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //ypbpr - max 128
{ 0, NULL,					410,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //pc - max 128
{ 0, NULL,					493,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  }, //hdmi - max 128
{ 0, NULL,					587,54,	ALIGN_CENTER,	&img_input_select,NULL,					0,SOSD_WIN3,0x000,  0,0,0,0  } //ext - max 128
};
struct menu_page_info_s menu_input_page = {	MENU_TYPE_SCRLDN | MENU_TYPE_NOTE, 5,5, 5,6,	menu_input_page_items, NULL };	//auto focus	
#endif


//----------------------
//menu_audio_page
//main->audio
//------------------------------------------
//description
code struct menu_sitem_info_s menu_audio_page_items[] = {
{ TYPE_BG,proc_audio_bg,	0,0,	ALIGN_TOPLEFT,	&img_audio_bg,NULL,						0,SOSD_WIN1,0x000,  0,0,0,0},
{ 0,proc_return_icon,		697,423,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,		0,SOSD_WIN8,0x100,  2,2,2,2},
{ 0,proc_exit_icon,			750,423,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x100,  1,1,1,1}
};	  
struct menu_page_info_s menu_audio_page = { MENU_TYPE_NORMAL,1,1, 3,0,	menu_audio_page_items, NULL };		


//----------------------
// system setup MENU
//main->system
//----------------------
//------------------------------------------
//description
code struct menu_sitem_info_s menu_system_page_items[] = {														//  ^ < V >
{ TYPE_BG,	proc_system_bg,	0,417,	ALIGN_TOPLEFT,	&img_system_bg_bottom,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},
{ TYPE_MBG, NULL,			0,0,	ALIGN_TOPLEFT,	&img_system_bg_top,NULL,					0,SOSD_WIN2,0x000,  0,0,0,0},

{ 0, proc_return_icon,		697,426,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,			0,SOSD_WIN8,0x100,  4,3,8,3},	//2
{ 0, proc_exit_icon,		750,426,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,			0,SOSD_WIN8,0x100,  4,2,8,2},	//3

{ 0, proc_system_touch,		173,4,	ALIGN_TOPLEFT,	&img_system_touch,&img_system_touch1,	 	0,SOSD_WIN8,0x100,  2,8,2,5},	//4	 need a disabled option
{ 0, proc_system_display,	272,2,	ALIGN_TOPLEFT,	&img_system_display,&img_system_display1,	0,SOSD_WIN8,0x100,  2,4,2,6},	//5
{ 0, proc_system_btooth,	371,3,	ALIGN_TOPLEFT,	&img_system_btooth,&img_system_btooth1,	 	0,SOSD_WIN8,0x100,  2,5,2,7},	//6
{ 0, proc_system_restore,	473,5,	ALIGN_TOPLEFT,	&img_system_restore,&img_system_restore1,	0,SOSD_WIN8,0x100,  2,6,2,8},	//7
{ 0, proc_system_info,		573,3,	ALIGN_TOPLEFT,	&img_system_sys_info,&img_system_sys_info1,	0,SOSD_WIN8,0x100,  2,7,2,4}	//8
};
struct menu_page_info_s menu_system_page = { MENU_TYPE_SCRLDN, 4,4, 4,5,	menu_system_page_items, NULL };		


//------------------------------------------
//menu_gps_page
//main->gps
//------------------------------------------
//description
code struct menu_sitem_info_s menu_gps_page_items[] = {
{ TYPE_BG,proc_gps_bg,		0,0,	ALIGN_TOPLEFT,	&img_gps_bg,NULL,						0,SOSD_WIN1,0x000,  0,0,0,0},
{ 0,proc_return_icon,		696,423,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,		0,SOSD_WIN8,0x0C0,  2,2,2,2},
{ 0,proc_exit_icon,			749,424,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x0C0,  1,1,1,1}
};	  
struct menu_page_info_s menu_gps_page = { MENU_TYPE_NORMAL, 1,1, 3,0,	menu_gps_page_items, NULL };		


//------------------------------------------
//menu_phone_page
//main->phone
//------------------------------------------
//description
code struct menu_sitem_info_s menu_phone_page_items[] = {
{ TYPE_BG,proc_phone_bg,	0,0,	ALIGN_TOPLEFT,	&img_phone_bg,NULL,						0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0,proc_phone_return,		698,423, ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,		0,SOSD_WIN8,0x0C0,  3,25,2,2},	//1
{ 0,proc_exit_icon,			749,423,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x0C0,  3,1,1,21},	//2

{ 0,proc_phone_1,			 70, 90,ALIGN_TOPLEFT,	&img_phone_10,&img_phone_11,			0,SOSD_WIN8,0x0C0,  1,5,6,4},	//3
{ 0,proc_phone_2,			150, 90,ALIGN_TOPLEFT,	&img_phone_20,&img_phone_21,			0,SOSD_WIN8,0x0C0,  1,3,7,5},	//4
{ 0,proc_phone_3,			230, 90,ALIGN_TOPLEFT,	&img_phone_30,&img_phone_31,			0,SOSD_WIN8,0x0C0,  1,4,8,3},	//5

{ 0,proc_phone_4,			 70,145,ALIGN_TOPLEFT,	&img_phone_40,&img_phone_41,			0,SOSD_WIN8,0x0C0,  3,8,9,7},	//6
{ 0,proc_phone_5,			150,145,ALIGN_TOPLEFT,	&img_phone_50,&img_phone_51,			0,SOSD_WIN8,0x0C0,  4,6,10,8},	//7
{ 0,proc_phone_6,			230,145,ALIGN_TOPLEFT,	&img_phone_60,&img_phone_61,			0,SOSD_WIN8,0x0C0,  5,7,11,6},	//8

{ 0,proc_phone_7,			 70,200,ALIGN_TOPLEFT,	&img_phone_70,&img_phone_71,			0,SOSD_WIN8,0x0C0,  6,11,12,10},//9
{ 0,proc_phone_8,			150,200,ALIGN_TOPLEFT,	&img_phone_80,&img_phone_81,			0,SOSD_WIN8,0x0C0,  7,9,13,11},	//10
{ 0,proc_phone_9,			230,200,ALIGN_TOPLEFT,	&img_phone_90,&img_phone_91,			0,SOSD_WIN8,0x0C0,  8,10,14,19},//11

{ 0,proc_phone_star,		 70,255,ALIGN_TOPLEFT,	&img_phone_star0,&img_phone_star1,		0,SOSD_WIN8,0x0C0,  9,14,15,13},//12
{ 0,proc_phone_0,			149,255,ALIGN_TOPLEFT,	&img_phone_00,&img_phone_01,			0,SOSD_WIN8,0x0C0,  10,12,15,14},//13
{ 0,proc_phone_sharp,		230,255,ALIGN_TOPLEFT,	&img_phone_sharp0,&img_phone_sharp1,	0,SOSD_WIN8,0x0C0,  11,13,15,12},//14

{ 0,proc_phone_dial,		 70,315,ALIGN_TOPLEFT,	&img_phone_dial0,&img_phone_dial1,		0,SOSD_WIN8,0x0C0,  13,20,21,16},//15

{ 0,proc_phone_up,			351,315,ALIGN_TOPLEFT,	&img_phone_up0,&img_phone_up1,			0,SOSD_WIN8,0x0C0,  1,15,1,17},	//16
{ 0,proc_phone_down,		431,315,ALIGN_TOPLEFT,	&img_phone_down0,&img_phone_down1,		0,SOSD_WIN8,0x0C0,  1,16,1,18},	//17
{ 0,proc_phone_check,		511,315,ALIGN_TOPLEFT,	&img_phone_check0,&img_phone_check1,	0,SOSD_WIN8,0x0C0,  1,17,1,19},	//18
{ 0,proc_phone_left,		591,315,ALIGN_TOPLEFT,	&img_phone_left0,&img_phone_left1,		0,SOSD_WIN8,0x0C0,  1,18,1,20},	//19
{ 0,proc_phone_right,		671,315,ALIGN_TOPLEFT,	&img_phone_right0,&img_phone_right1,	0,SOSD_WIN8,0x0C0,  1,19,1,15},	//20

{ 0,proc_phone_help,		 15,422,ALIGN_TOPLEFT,	&img_phone_help0,&img_phone_help1,		0,SOSD_WIN8,0x0C0,  15,2,3,22},	//21
{ 0,proc_phone_dir,			123,422,ALIGN_TOPLEFT,	&img_phone_dir0,&img_phone_dir1,		0,SOSD_WIN8,0x0C0,  15,21,3,23},//22
{ 0,proc_phone_set,			263,422,ALIGN_TOPLEFT,	&img_phone_set0,&img_phone_set1,		0,SOSD_WIN8,0x0C0,  15,22,3,24},//23
{ 0,proc_phone_msg,			403,422,ALIGN_TOPLEFT,	&img_phone_msg0,&img_phone_msg1,		0,SOSD_WIN8,0x0C0,  15,23,3,25},//24
{ 0,proc_phone_menu,		543,422,ALIGN_TOPLEFT,	&img_phone_menu0,&img_phone_menu1,		0,SOSD_WIN8,0x0C0,  15,24,3,1}	//25

};	  
code struct menu_fitem_info_s menu_phone_text_items[] = {
//	{ 0,0,	520-220,324-260,	10,2, 0,0,	0xF0,	 16,0x07E0	},
//	{ 0,0,	396,163,	10,2, 0,0,	0xF0,	 16,0x07E0	},
	{ 0,0,	396,163,	15,2, 0,0,	0x1A,	 16,0x07E0	},	//win0,osdram0~29
	{ 1,40,	396,241,	15,1, 0,0,	0x1A,	 16,0x07E0	},	//win1,osdram40~69
};
struct menu_page_info_s menu_phone_page = { MENU_TYPE_NORMAL, 1,1, 3,23,	menu_phone_page_items, menu_phone_text_items };		




//------------------------------------------
//menu_car_info_page
//main->car_info
//------------------------------------------
//description
code struct menu_sitem_info_s menu_carinfo_page_items[] = {
{ TYPE_BG,proc_carinfo_bg,	0,0,	ALIGN_TOPLEFT,	&img_carinfo_bg,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},
{ 0,proc_return_icon,		697,423,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,		0,SOSD_WIN8,0x0C0,  2,2,2,2},
{ 0,proc_exit_icon,			748,423,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x0C0,  1,1,1,1}
};	  
struct menu_page_info_s menu_carinfo_page = { MENU_TYPE_NORMAL, 1,1, 3,0,	menu_carinfo_page_items, NULL };		

//------------------------------------------
//menu_demo_page
//main->demo
//test 4BPP RLE
//------------------------------------------
//proc_pigeon_demo_start
code struct menu_sitem_info_s menu_demo_page_items[] = {												   //  ^ < V >
{ TYPE_BG,proc_demo_bg,		0,0,	ALIGN_TOPLEFT,	&img_demo_bg,NULL,							0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0,proc_return_icon,		701,425,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return,			0,SOSD_WIN8,0x100,  6,2,3,2}, //1
{ 0,proc_exit_icon,			752,426,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,			0,SOSD_WIN8,0x100,  6,1,3,1}, //2

{ 0,proc_grid_demo_start,	149,90,ALIGN_TOPLEFT,	&img_demo_grid,&img_demo_grid1,				0,SOSD_WIN8,0x100,  6,5,6,4}, //3
{ 0,proc_rose_demo_start,	344,89,ALIGN_TOPLEFT,	&img_demo_rose,&img_demo_rose1,				0,SOSD_WIN8,0x100,  7,3,7,5}, //4
{ 0,proc_wait_demo_start,	559,90,ALIGN_TOPLEFT,	&img_demo_ani,&img_demo_ani1,				0,SOSD_WIN8,0x100,  8,4,8,3}, //5
{ 0,proc_test1_demo_start,	147,242,ALIGN_TOPLEFT,	&img_demo_palette,&img_demo_palette1,		0,SOSD_WIN8,0x100,  3,8,2,7}, //6
{ 0,proc_test1_demo_start,	351,243,ALIGN_TOPLEFT,	&img_demo_demoA,&img_demo_demoA1,			0,SOSD_WIN8,0x100,  4,6,2,8}, //7
{ 0,proc_test1_demo_start,	551,246,ALIGN_TOPLEFT,	&img_demo_demoB,&img_demo_demoB1,			0,SOSD_WIN8,0x100,  5,7,2,6}  //8
};	  
struct menu_page_info_s menu_demo_page = { MENU_TYPE_NORMAL, 3,3, 3,6,	menu_demo_page_items, NULL };		


//------------------------
// level 2 page
//------------------------



//----------------------
//touch calibration page
//main->setting->touch
//----------------------
code struct menu_sitem_info_s menu_touch_page_items[]= {												 //  ^ < V >
{ TYPE_BG, proc_touch_bg,	0,0,	ALIGN_TOPLEFT,		&img_touch_bg,NULL,						0,SOSD_WIN1,0x000,  0,0,0,0},
{ TYPE_BG, proc_touch_bg_end,0,0,	ALIGN_TOPLEFT,		&img_touch_bg_end,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0, proc_touch_ok,			312,329,ALIGN_TOPLEFT,		&img_dialog_ok,&img_dialog_ok1,			0,SOSD_WIN8,0x100,  3,3,3,3},//2
{ 0, proc_touch_cancel,		386,329,ALIGN_TOPLEFT,		&img_dialog_cancel,&img_dialog_cancel1,	0,SOSD_WIN8,0x100,  2,2,2,2},//3

{ 0, proc_touch_button1,	80-10,48-9,	ALIGN_TOPLEFT,	&img_touch_button,&img_touch_button1,	0,SOSD_WIN8,0x080,  5,5,5,5},//4
{ 0, proc_touch_button2,	720-10,48-9,ALIGN_TOPLEFT,	&img_touch_button,&img_touch_button1,	0,SOSD_WIN8,0x080,  6,6,6,6},//5
{ 0, proc_touch_button3,	720-10,432-9,ALIGN_TOPLEFT,	&img_touch_button,&img_touch_button1,	0,SOSD_WIN8,0x080,  7,7,7,7},//6
{ 0, proc_touch_button4,	80-10,432-9,ALIGN_TOPLEFT,	&img_touch_button,&img_touch_button1,	0,SOSD_WIN8,0x080,  8,8,8,8},//7
{ 0, proc_touch_button5,	400-10,240-9,ALIGN_TOPLEFT,	&img_touch_button,&img_touch_button1,	0,SOSD_WIN8,0x080,  2,2,2,2} //8
};
struct menu_page_info_s menu_touch_page = { MENU_TYPE_NORMAL, 4,4, 4,5, menu_touch_page_items, NULL };		



//==========================
// LIST MENU
//==========================

//----------------------
// internal decoder image Select MENU
//manu->input->setting with CVBS or SVideo
//----------------------
//					return setting exit
// Brightness
// Contrast
// Hue
// Saturation
// Sharpness
//
//------------------------------------------
//description
//use global input
//background: use 8bit RLE. 128 Color. Use SOSD_WIN0
//            color 1: transparent color. normally color 0 is a black.
code struct menu_sitem_info_s menu_yuv_list_page_items[]= {											  //  ^ < V >
{ TYPE_BG, proc_yuv_list_bg,	620,420,ALIGN_TOPLEFT,	&img_navi_menu,NULL,					0,SOSD_WIN2,0x180,  0,0,0,0},//0
{ TYPE_MBG, NULL,				0,0,	ALIGN_TOPLEFT,	&img_yuv_menu_bg,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},//1

{ 0, proc_menu_list_return,		623,424,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,		0,SOSD_WIN8,0x100,  5,4,9,3},//2
{ 0, proc_home_icon,			682,424,ALIGN_TOPLEFT,	&img_navi_home,&img_navi_home1,			0,SOSD_WIN8,0x100,  5,2,9,4},//3
{ 0, proc_exit_icon,			743,424,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x100,  5,3,9,2},//4

{ 0, proc_yuv_list_bright,		0,61,	ALIGN_TOPLEFT,	&img_yuv_bright,&img_yuv_bright1,		0,SOSD_WIN8,0x100,  9,2,6,2},//5
{ 0, proc_yuv_list_contrast,	0,133,	ALIGN_TOPLEFT,	&img_yuv_contrast,&img_yuv_contrast1,	0,SOSD_WIN8,0x100,  5,2,7,2},//6
{ 0, proc_yuv_list_hue,			0,205,	ALIGN_TOPLEFT,	&img_yuv_hue,&img_yuv_hue1,				0,SOSD_WIN8,0x100,  6,2,8,2},//7
{ 0, proc_yuv_list_saturate,	0,277,	ALIGN_TOPLEFT,	&img_yuv_saturate,&img_yuv_saturate1,	0,SOSD_WIN8,0x100,  7,2,9,2},//8
{ 0, proc_yuv_list_sharp,		0,349,	ALIGN_TOPLEFT,	&img_yuv_sharp,&img_yuv_sharp1,			0,SOSD_WIN8,0x100,  8,2,5,2} //9
};
struct menu_page_info_s menu_yuv_list_page = { MENU_TYPE_LIST, 5,5, 5,5,	menu_yuv_list_page_items, NULL };		


code struct menu_sitem_info_s menu_rgb_list_page_items[]= {											//  ^ < V >
{ TYPE_BG, proc_rgb_list_bg,	620,420,ALIGN_TOPLEFT,	&img_navi_menu,NULL,					0,SOSD_WIN2,0x180,  0,0,0,0},
{ TYPE_MBG, NULL,				0,0,	ALIGN_TOPLEFT,	&img_rgb_menu_bg,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0, proc_menu_list_return,		623,424,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,		0,SOSD_WIN8,0x100,  5,4,7,3},//2
{ 0, proc_home_icon,			682,424,ALIGN_TOPLEFT,	&img_navi_home,&img_navi_home1,			0,SOSD_WIN8,0x100,  5,2,7,4},//3
{ 0, proc_exit_icon,			743,424,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x100,  5,3,7,2},//4

{ 0, proc_rgb_list_bright,		0,61,	ALIGN_TOPLEFT,	&img_rgb_bright,&img_rgb_bright1,		0,SOSD_WIN8,0x100,  7,2,6,2},//5
{ 0, proc_rgb_list_contrast,	0,181,	ALIGN_TOPLEFT,	&img_rgb_contrast,&img_rgb_contrast1,	0,SOSD_WIN8,0x100,  5,2,7,2},//6
{ 0, proc_rgb_list_color,		0,301,	ALIGN_TOPLEFT,	&img_rgb_color,&img_rgb_color1,			0,SOSD_WIN8,0x100,  6,2,5,2} //7
};
struct menu_page_info_s menu_rgb_list_page = { MENU_TYPE_LIST, 5,5, 5,3,	menu_rgb_list_page_items, NULL };		

//----------------------
// ADC image Select MENU
//manu->input->setting with RGB or HDMI
//----------------------
//					return setting exit
// R Gain
// G Gain
// B Gain
// Phase
//
//------------------------------------------
//description
//use global input
//background: use 8bit RLE. 128 Color. Use SOSD_WIN0
//            color 1: transparent color. normally color 0 is a black.


code struct menu_sitem_info_s menu_apc_list_page_items[]= {											//  ^ < V >
{ TYPE_BG, proc_apc_list_bg,	620,420,ALIGN_TOPLEFT,	&img_navi_menu,NULL,					0,SOSD_WIN2,0x180,  0,0,0,0},//0
{ TYPE_MBG, NULL,				0,0,	ALIGN_TOPLEFT,	&img_apc_menu_bg,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},//1

{ 0, proc_menu_list_return,		623,424,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,		0,SOSD_WIN8,0x100,  5,4,12,3},//2
{ 0, proc_home_icon,			682,424,ALIGN_TOPLEFT,	&img_navi_home,&img_navi_home1,			0,SOSD_WIN8,0x100,  5,2,12,4},//3
{ 0, proc_exit_icon,			743,424,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x100,  5,3,12,2},//4

{ 0, proc_apc_list_bright,		0,61,	ALIGN_TOPLEFT,	&img_apc_bright,&img_apc_bright1,		0,SOSD_WIN8,0x100,  12,2,6,2},//5
{ 0, proc_apc_list_contrast,	0,106,	ALIGN_TOPLEFT,	&img_apc_contrast,&img_apc_contrast1,	0,SOSD_WIN8,0x100,  5,2,7,2}, //6
{ 0, proc_apc_list_color,		0,151,	ALIGN_TOPLEFT,	&img_apc_color,&img_apc_color1,			0,SOSD_WIN8,0x100,  6,2,8,2}, //7
{ 0, proc_apc_list_position,	0,196,	ALIGN_TOPLEFT,	&img_apc_position,&img_apc_position1,	0,SOSD_WIN8,0x100,  7,2,9,2}, //8
{ 0, proc_apc_list_phase,		0,241,	ALIGN_TOPLEFT,	&img_apc_phase,&img_apc_phase1,			0,SOSD_WIN8,0x100,  8,2,10,2}, //9
{ 0, proc_apc_list_clock,		0,286,	ALIGN_TOPLEFT,	&img_apc_pclock,&img_apc_pclock1,		0,SOSD_WIN8,0x100,  9,2,11,2},//10
{ 0, proc_apc_list_autoadj,		0,331,	ALIGN_TOPLEFT,	&img_apc_autoadj,&img_apc_autoadj1,		0,SOSD_WIN8,0x100,  10,2,12,2},//11
{ 0, proc_apc_list_autocolor,	0,376,	ALIGN_TOPLEFT,	&img_apc_autocolor,&img_apc_autocolor1,	0,SOSD_WIN8,0x100,  11,2,5,2} //12
};
struct menu_page_info_s menu_apc_list_page = { MENU_TYPE_LIST,	5,5, 5,8,	menu_apc_list_page_items, NULL };		




code struct menu_sitem_info_s menu_hdmi_list_page_items[]= {											//  ^ < V >
{ TYPE_BG, proc_hdmi_list_bg,	620,420,ALIGN_TOPLEFT,	&img_navi_menu,NULL,					0,SOSD_WIN2,0x180,  0,0,0,0},
{ TYPE_MBG, NULL,				0,0,	ALIGN_TOPLEFT,	&img_hdmi_menu_bg,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0, proc_menu_list_return,		623,424,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,		0,SOSD_WIN8,0x100,  5,4,6,3},//2
{ 0, proc_home_icon,			682,424,ALIGN_TOPLEFT,	&img_navi_home,&img_navi_home1,			0,SOSD_WIN8,0x100,  5,2,6,4},//3
{ 0, proc_exit_icon,			743,424,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,		0,SOSD_WIN8,0x100,  5,3,6,2}, //4

{ 0, proc_hdmi_list_mode,		0,61,	ALIGN_TOPLEFT,	&img_hdmi_mode,&img_hdmi_mode1,			0,SOSD_WIN8,0x100,  6,2,6,2},//5
{ 0, proc_hdmi_list_setting,	0,241,	ALIGN_TOPLEFT,	&img_hdmi_setting,&img_hdmi_setting1,	0,SOSD_WIN8,0x100,  5,2,5,2} //6

};
struct menu_page_info_s menu_hdmi_list_page = {	MENU_TYPE_LIST, 5,5, 5,2,	menu_hdmi_list_page_items, NULL };		

#if 0
//manu->input->setting with RGB/HDMI->color
//code struct menu_sitem_info_s menu_hdmipc_list_page_items[]= {										 // ^ < V >
//	{ 2, proc_hdmipc_list_bg,		650,430,ALIGN_TOPLEFT,	&img_navi_menu,			0,SOSD_WIN2,0x1C0,  0,0,0,0},
//	{ 3, NULL,						0,0,	ALIGN_TOPLEFT,	&img_hdmipc_menu_bg,	0,SOSD_WIN1,0x000,  0,0,0,0},
//  
//	{ 0, proc_menu_list_return,		650,431,ALIGN_TOPLEFT,	&img_navi_return,		0,SOSD_WIN8,0x100,  5,4,7,3},//2
//	{ 0, proc_home_icon,			701,429,ALIGN_TOPLEFT,	&img_navi_home,			0,SOSD_WIN8,0x100,  5,2,7,4},//3
//	{ 0, proc_exit_icon,			755,430,ALIGN_TOPLEFT,	&img_navi_close,		0,SOSD_WIN8,0x100,  5,3,7,2}, //4
//  
//	{ 0, proc_hdmipc_list_bright,	16,88,	ALIGN_TOPLEFT,	&img_hdmipc_bright,		0,SOSD_WIN8,0x100,  7,2,6,2},//5
//	{ 0, proc_hdmipc_list_contrast,	27,178,	ALIGN_TOPLEFT,	&img_hdmipc_contrast,	0,SOSD_WIN8,0x100,  5,2,7,2},//6
//	{ 0, proc_hdmipc_list_phase,	36,268,	ALIGN_TOPLEFT,	&img_hdmipc_phase,		0,SOSD_WIN8,0x100,  6,2,5,2} //7
//
//};
//struct menu_page_info_s menu_hdmipc_list_page = { MENU_TYPE_LIST, 5,5, 5,3,	menu_hdmipc_list_page_items, NULL };		
#endif

//----------------------
//ImageEnhanced block image on the Back-End
//main->setting->display
//----------------------
//					
//
//
//
// 
//
//------------------------------------------
//description
code struct menu_sitem_info_s menu_display_page_items[]= {																//  ^ < V >
{ TYPE_BG, proc_display_bg,		620,420,ALIGN_TOPLEFT,	&img_navi_menu,NULL,							0,SOSD_WIN2,0x180,  0,0,0,0},
{ TYPE_MBG, NULL,				0,0,	ALIGN_TOPLEFT,	&img_display_bg,NULL,							0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0, proc_menu_list_return,		623,424,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,				0,SOSD_WIN8,0x100,  5,4,9,3},//2
{ 0, proc_home_icon,			682,424,ALIGN_TOPLEFT,	&img_navi_home,&img_navi_home1,					0,SOSD_WIN8,0x100,  5,2,9,4},//3
{ 0, proc_exit_icon,			743,424,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,				0,SOSD_WIN8,0x100,  5,3,9,2},//4

{ 0, proc_display_aspect,		0,61,	ALIGN_TOPLEFT,	&img_display_aspect,&img_display_aspect1,		0,SOSD_WIN8,0x100,  8,2,6,2},//5
{ 0, proc_display_flip,			0,134,	ALIGN_TOPLEFT,	&img_display_flip,&img_display_flip1,			0,SOSD_WIN8,0x100,  5,2,7,2},//6
{ 0, proc_display_osd,			0,205,	ALIGN_TOPLEFT,	&img_display_osd,&img_display_osd1,				0,SOSD_WIN8,0x100,  6,2,8,2},//7
{ 0, proc_display_backlight,	0,277,	ALIGN_TOPLEFT,	&img_display_backlight,&img_display_backlight1,	0,SOSD_WIN8,0x100,  7,2,9,2},//8
{ 0, proc_display_resolution,	0,349,	ALIGN_TOPLEFT,	&img_display_resolution,&img_display_resolution1,0,SOSD_WIN8,0x100,  8,2,5,2} //9
};
struct menu_page_info_s menu_display_page = { MENU_TYPE_LIST, 5,5, 5,5,	menu_display_page_items, NULL };		



//----------------------
// main->system->display->OSD MENU
//----------------------
//					
//
//
//
// 
//
//------------------------------------------
//description
code struct menu_sitem_info_s menu_osd_page_items[] = {														//  ^ < V >
{ TYPE_BG,proc_osd_bg,		620,420,ALIGN_TOPLEFT,	&img_navi_menu,NULL,				0,SOSD_WIN2,0x180,  0,0,0,0},
{ TYPE_MBG,NULL,			0,0,	ALIGN_TOPLEFT,	&img_osd_bg,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0,proc_menu_list_return,	623,424,ALIGN_TOPLEFT,	&img_navi_return,&img_navi_return1,	0,SOSD_WIN8,0x100,  5,4,6,3},//2
{ 0,proc_home_icon,			682,424,ALIGN_TOPLEFT,	&img_navi_home,&img_navi_home1,		0,SOSD_WIN8,0x100,  5,2,6,4},//3
{ 0,proc_exit_icon,			743,424,ALIGN_TOPLEFT,	&img_navi_close,&img_navi_close1,	0,SOSD_WIN8,0x100,  5,3,6,2},//4
																									
{ 0,proc_osd_time,			0,61,	ALIGN_TOPLEFT,	&img_osd_timer,NULL,				0,SOSD_WIN8,0x100,  6,2,6,2},//5
{ 0,proc_osd_trans,			0,241,	ALIGN_TOPLEFT,	&img_osd_trans,NULL,				0,SOSD_WIN8,0x100,  5,2,5,2} //6
};
struct menu_page_info_s menu_osd_page = { MENU_TYPE_LIST, 5,5, 5,2,	menu_osd_page_items, NULL };		



//======================================
// DIALOG
//======================================
//manu->input->setting with DVI
code struct menu_sitem_info_s menu_dvi_mode_page_items[]= {											 			// ^ < V >
{ TYPE_BG, proc_dvi_mode_bg,220,220,ALIGN_TOPLEFT,	&img_dvi_mode_bg,NULL,						0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0, proc_dvi_mode_ok,		306,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,				0,SOSD_WIN8,0x100,  3,2,4,2},//1
{ 0, proc_dvi_mode_cancel,	381,385,ALIGN_TOPLEFT,	&img_dialog_cancel,&img_dialog_cancel1,		0,SOSD_WIN8,0x100,  3,1,4,1},//2

{ 0, proc_dvi_mode_24bit,	285,328,ALIGN_TOPLEFT,	&img_dvi_mode_24bit,&img_dvi_mode_24bit1,	0,SOSD_WIN8,0x100,  1,4,1,4},//3
{ 0, proc_dvi_mode_16bit,	436,327,ALIGN_TOPLEFT,	&img_dvi_mode_16bit,&img_dvi_mode_16bit1,	0,SOSD_WIN8,0x100,  1,3,1,3},//4
//select
{ 0, NULL,					296,273,ALIGN_TOPLEFT,	&img_dvi_mode_select24,NULL,				0,SOSD_WIN2,0x180,  0,0,0,0},//3+2 
{ 0, NULL,					445,274,ALIGN_TOPLEFT,	&img_dvi_mode_select16,NULL,				0,SOSD_WIN2,0x180,  0,0,0,0} //4+2 
};
struct menu_page_info_s menu_dvi_mode_page = { MENU_TYPE_DIALOG | MENU_TYPE_NOTE, 3,3, 3,2,	menu_dvi_mode_page_items, NULL };		


//manu->input->setting with HDMI
code struct menu_sitem_info_s menu_hdmi_mode_page_items[]= {												 				//  ^ < V >
{ TYPE_BG, proc_hdmi_mode_bg,	220,220,ALIGN_TOPLEFT,	&img_hdmi_mode_bg,NULL,						0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0, proc_hdmi_mode_ok,			306,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,				0,SOSD_WIN8,0x100,  3,2,4,2},//1
{ 0, proc_hdmi_mode_cancel,		381,385,ALIGN_TOPLEFT,	&img_dialog_cancel,&img_dialog_cancel1,		0,SOSD_WIN8,0x100,  3,1,4,1},//2
																									
{ 0, proc_hdmi_mode_pc,			274,327,ALIGN_TOPLEFT,	&img_hdmi_mode_pc,&img_hdmi_mode_pc1,		0,SOSD_WIN8,0x100,  1,4,1,4},//3
{ 0, proc_hdmi_mode_tv,			424,328,ALIGN_TOPLEFT,	&img_hdmi_mode_tv,&img_hdmi_mode_tv1,		0,SOSD_WIN8,0x100,  1,3,1,3},//4
//select area																							
{ 0, proc_hdmi_mode_pc,			282,271,ALIGN_TOPLEFT,	&img_hdmi_mode_selectPC,NULL, 				0,SOSD_WIN2,0x180,  0,0,0,0},//3+2 
{ 0, proc_hdmi_mode_tv,			436,267,ALIGN_TOPLEFT,	&img_hdmi_mode_selectTV,NULL,				0,SOSD_WIN2,0x180,  0,0,0,0} //4+2 
};
//struct menu_page_info_s menu_hdmi_mode_page = {	MENU_TYPE_DIALOG | MENU_TYPE_NOTE, 3,3, 3,2,	menu_hdmi_mode_page_items, NULL };		
struct menu_page_info_s menu_hdmi_mode_page = {	MENU_TYPE_DIALOG | MENU_TYPE_NOTE, 3,3, 3,2,	menu_hdmi_mode_page_items, NULL };		


//----------------------
// autoadj popup MENU
// main->input->adc->display->autoadj MENU
//----------------------
//					
//------------------------------------------
//description
code struct menu_sitem_info_s menu_autoadj_page_items[] = {
{ TYPE_BG,proc_autoadj_bg,		220,260,ALIGN_TOPLEFT,	&img_autoadj_bg,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},
{ 0,proc_autoadj_ok,			306,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,			0,SOSD_WIN8,0x080,  2,2,2,2},
{ 0,proc_autoadj_cancel,		381,385,ALIGN_TOPLEFT,	&img_dialog_cancel,&img_dialog_cancel1,	0,SOSD_WIN8,0x080,  1,1,1,1},
//wait image. I need a image & WIN & LUT loc.
{ 0,proc_wait_menu_start,		376,215,ALIGN_TOPLEFT,	&img_wait,NULL,							0,SOSD_WIN0,0x100,  0,0,0,0}
};	  
struct menu_page_info_s menu_autoadj_page = { MENU_TYPE_DIALOG, 1,1, 3,0,	menu_autoadj_page_items, NULL };		



code struct menu_sitem_info_s menu_autocolor_page_items[] = {
{ TYPE_BG,proc_autocolor_bg,	220,260,ALIGN_TOPLEFT,	&img_autocolor_bg,NULL,						0,SOSD_WIN1,0x000,  0,0,0,0},
{ 0,proc_autocolor_ok,			306,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,				0,SOSD_WIN8,0x080,  2,2,2,2},
{ 0,proc_autocolor_cancel,		381,385,ALIGN_TOPLEFT,	&img_dialog_cancel,&img_dialog_cancel1,		0,SOSD_WIN8,0x080,  1,1,1,1},
//wait image. I need a image & WIN & LUT loc.
{ 0,proc_wait_menu_start,		376,215,ALIGN_TOPLEFT,	&img_wait,NULL,								0,SOSD_WIN0,0x100,  0,0,0,0}
};	  
struct menu_page_info_s menu_autocolor_page = { MENU_TYPE_DIALOG, 1,1, 3,0,	menu_autocolor_page_items, NULL };		

// aspect
//----------------------
// aspect popup MENU
//main->system->display->aspect
//----------------------
//------------------------------------------
//description
code struct menu_sitem_info_s menu_aspect_page_items[] = {												   //^ < V >

{ TYPE_BG,proc_aspect_bg,	220,220,ALIGN_TOPLEFT,	&img_popup_aspect_bg,NULL,		0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0,proc_aspect_ok,			306,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,					0,SOSD_WIN8,0x080,  3,2,6,2},//1
{ 0,proc_aspect_cancel,		381,385,ALIGN_TOPLEFT,	&img_dialog_cancel,&img_dialog_cancel1,			0,SOSD_WIN8,0x080,  3,1,6,1},//2

{ 0,proc_aspect_normal,		229,327,ALIGN_TOPLEFT,	&img_popup_aspect_normal,&img_popup_aspect_normal1,	0,SOSD_WIN8,0x080,  1,6,1,4},//3
{ 0,proc_aspect_zoom,		318,329,ALIGN_TOPLEFT,	&img_popup_aspect_zoom,&img_popup_aspect_zoom1,		0,SOSD_WIN8,0x080,  1,3,1,5},//4
{ 0,proc_aspect_full,		410,328,ALIGN_TOPLEFT,	&img_popup_aspect_full,&img_popup_aspect_full1,		0,SOSD_WIN8,0x080,  1,4,1,6},//5
{ 0,proc_aspect_pano,		468,328,ALIGN_TOPLEFT,	&img_popup_aspect_pano,&img_popup_aspect_pano1,		0,SOSD_WIN8,0x080,  1,5,1,3},//6

{ 0, proc_aspect_normal,	240,284,ALIGN_TOPLEFT,	&img_popup_aspect_normal_select,NULL,				0,SOSD_WIN2,0x180,  0,0,0,0},//3+4 
{ 0, proc_aspect_zoom,		324,284,ALIGN_TOPLEFT,	&img_popup_aspect_zoom_select,NULL,					0,SOSD_WIN2,0x180,  0,0,0,0},//4+4 
{ 0, proc_aspect_full,		408,284,ALIGN_TOPLEFT,	&img_popup_aspect_full_select,NULL,					0,SOSD_WIN2,0x180,  0,0,0,0},//5+4 
{ 0, proc_aspect_pano,		491,284,ALIGN_TOPLEFT,	&img_popup_aspect_pano_sel,NULL,					0,SOSD_WIN2,0x180,  0,0,0,0} //6+4 
};
struct menu_page_info_s menu_aspect_page = { MENU_TYPE_DIALOG | MENU_TYPE_NOTE, 3,3, 3,4,	menu_aspect_page_items, NULL };		

//----------------------
// flip popup MENU
// main->system->display->flip MENU
//----------------------
//
//------------------------------------------
//description
code struct menu_sitem_info_s menu_mirror_page_items[] = {
{ TYPE_BG,proc_flip_bg,		220,260,ALIGN_TOPLEFT,	&img_flip_bg,NULL,			0,SOSD_WIN1,0x000,  0,0,0,0},
{ 0,proc_flip_ok,			306,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,				0,SOSD_WIN8,0x080,  2,2,2,2},
{ 0,proc_flip_undo,			381,385,ALIGN_TOPLEFT,	&img_dialog_cancel,&img_dialog_cancel1,		0,SOSD_WIN8,0x080,  1,1,1,1}
};
struct menu_page_info_s menu_mirror_page = { MENU_TYPE_DIALOG, 1,1, 3,0,	menu_mirror_page_items, NULL };		

//----------------------
// flip popup MENU
// main->system->display->resolution MENU
//----------------------
//
//------------------------------------------
//description
code struct menu_sitem_info_s menu_disp_resolution_page_items[] = {
{ TYPE_BG,proc_disp_resolution_bg,	220,260,ALIGN_TOPLEFT,	&img_resolution_bg,NULL,			0,SOSD_WIN1,0x000,  0,0,0,0},  //BK110811. I need a new image
{ 0,proc_disp_resolution_ok,		363,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,		0,SOSD_WIN8,0x0C0,  1,1,1,1}
};
code struct menu_fitem_info_s menu_disp_resolution_text_items[] = {	//BK110811
//	{ 0,0,	400-220,300-260,	4,1, 0,0,	0x1F,	 16,0x07E0	}, //input_hwidth
//	{ 1,10,	400-220,350-260,	3,1, 0,0,	0x1F,	 16,0x07E0	}, //input_vwidth
//	{ 2,20,	500-220,300-260,	3,1, 0,0,	0x1F,	 16,0x07E0	}, //output hwidth
//	{ 3,30,	500-220,350-260,	3,1, 0,0,	0x1F,	 16,0x07E0	}, //output vwidth
	{ 0,0,	350-220,310-260,	20,1, 0,0,	0x1F,	 16,0x07E0	}, //input_hwidth
	{ 1,20,	350-220,342-260,	20,1, 0,0,	0x1F,	 16,0x07E0	}, //output panel

//input freq
};
struct menu_page_info_s menu_disp_resolution_page = { MENU_TYPE_DIALOG, 1,1, 2,0,	menu_disp_resolution_page_items, menu_disp_resolution_text_items };		

//----------------------
// restore popup MENU
// main->system->restore MENU
//----------------------//
//------------------------------------------
//description
code struct menu_sitem_info_s menu_restore_page_items[] = {
{ TYPE_BG,proc_restore_bg,	220,260,ALIGN_TOPLEFT,	&img_sysrestore_bg,NULL,		0,SOSD_WIN1,0x000,  0,0,0,0},
{ 0,proc_restore_ok,		306,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,				0,SOSD_WIN8,0x080,  2,2,2,2},
{ 0,proc_restore_undo,		381,385,ALIGN_TOPLEFT,	&img_dialog_cancel,&img_dialog_cancel1,		0,SOSD_WIN8,0x080,  1,1,1,1}
};
struct menu_page_info_s menu_restore_page = { MENU_TYPE_DIALOG, 1,1, 3,0,	menu_restore_page_items, NULL };		




//----------------------
// info popup MENU
// main->system->sys_info MENU
//----------------------
//					
//------------------------------------------
//description
code struct menu_sitem_info_s menu_sys_info_page_items[] = {
{ TYPE_BG,proc_info_bg,		220,260,ALIGN_TOPLEFT,	&img_sysinfo_bg,NULL,		0,SOSD_WIN1,0x000,  0,0,0,0},
{ 0,proc_info_ok,			363,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,				0,SOSD_WIN8,0x0C0,  1,1,1,1}
};
code struct menu_fitem_info_s menu_sys_info_text_items[] = {			//BK110811
	{ 0,0,	306-220,313-260,	10,1, 0,0,	0x1F,	 16,0x07E0	}, 	//EVB
	{ 1,10,	420-220,344-260,	10,1, 0,0,	0x1F,	 16,0x07E0	} 	//FW REV
};
struct menu_page_info_s menu_sys_info_page = { MENU_TYPE_DIALOG, 1,1, 2,0,	menu_sys_info_page_items, menu_sys_info_text_items };		


//=====================
// SLIDER
//=====================

//------------------------------------------
//description
//	item[2].x item[2].y  : start of slide bar
//	item[3].x item[3].y  :   end of slide bar
code struct menu_sitem_info_s menu_slider_page_items[] = {											//  ^ < V >
{ TYPE_BG, proc_slider_bg,		220,260,ALIGN_TOPLEFT,	&img_slide_bg,NULL,						0,SOSD_WIN1,0x000,  0,0,0,0},
{ TYPE_BG, NULL,				243,285,ALIGN_TOPLEFT,	&img_slide_bg,NULL,						0,SOSD_WIN2,0x040,  0,0,0,0}, //title

{ 0, proc_slider_ok,			306,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,			0,SOSD_WIN8,0x100,  5,3,5,3},	//2	 
{ 0, proc_slider_undo,			381,385,ALIGN_TOPLEFT,	&img_dialog_cancel,&img_dialog_cancel1,	0,SOSD_WIN8,0x100,  5,2,5,2},	//3

#if 0
{ TYPE_SLIDE, proc_slider_ctrl,	242-220, 320-260,ALIGN_TOPLEFT,	&img_slide_gray,NULL,			0,SOSD_WIN3,0x080,  2,0,2,0},	//start x,y. Gray 
{ TYPE_SLIDE, proc_slider_ctrl,	449-220, 320-260,ALIGN_TOPLEFT,	&img_slide_red,NULL,			0,SOSD_WIN8,0x100,  2,0,2,0},	//end x,y.   Red
#else
{ TYPE_SLIDE, proc_slider_ctrl,	242-220, 320-260,ALIGN_TOPMIDDLE,&img_slide_gray,NULL,			0,SOSD_WIN3,0x080,  2,0,2,0},	//start x,y. Gray 
{ TYPE_SLIDE, proc_slider_ctrl,	488-220, 370-260,ALIGN_TOPMIDDLE,&img_slide_red,NULL,			0,SOSD_WIN8,0x100,  2,0,2,0},	//end x,y.   Red
#endif

{ 0, proc_slider_left_tsc,		239, 385,ALIGN_TOPLEFT,	NULL,&img_slide_left1,					0,SOSD_WIN4,0x180,  0,0,0,0},	//
{ 0, proc_slider_right_tsc,		496, 386,ALIGN_TOPLEFT,	NULL,&img_slide_right1,					0,SOSD_WIN4,0x180,  0,0,0,0}	//


};
code struct menu_fitem_info_s menu_slider_text_items[] = {
	{ 0,0,	520-220,324-260,	3,1, 0,0,	0xF0,	 16,0x07E0	},
};
//struct menu_page_info_s menu_slider_page = { MENU_TYPE_SLIDE, 4,4, 4,1,	 menu_slider_page_items, menu_slider_text_items }; //note: 1 item
struct menu_page_info_s menu_slider_page = { MENU_TYPE_SLIDE, 4,4, 4,1*2+2,	 menu_slider_page_items, menu_slider_text_items }; //note: 1 item

//#define SLIDE3_1	3
//#define SLIDE3_2	5
//#define SLIDE3_3	7
//#define SLIDE3_OK	8
//#define SLIDE3_CANCEL	9
//BKTODO:Add 3 slide bar for RGB contrast & RGB brightness
code struct menu_sitem_info_s menu_slider3_page_items[] = {
{ TYPE_BG, proc_slider3_bg,		220,169,ALIGN_TOPLEFT,			&img_slide3_bg,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},	//init
{ TYPE_BG, NULL,				243,190,ALIGN_TOPLEFT,			&img_slide3_bg,NULL,					0,SOSD_WIN2,0x040,  0,0,0,0}, 	//title

{ 0, proc_slider_ok,			306,385,ALIGN_TOPLEFT,			&img_dialog_ok,&img_dialog_ok1,			0,SOSD_WIN8,0x100,  9,3,5,3},	//2 
{ 0, proc_slider_undo,			381,385,ALIGN_TOPLEFT,			&img_dialog_cancel,&img_dialog_cancel1,	0,SOSD_WIN8,0x100,  9,2,5,2},	//3

#if 0
{ TYPE_SLIDE, proc_slider_ctrl,	242-220,228-169,ALIGN_TOPLEFT,	&img_slide_gray,NULL,					0,SOSD_WIN3,0x080,  0,0,0,0},	//  start x,y  //slider R
{ TYPE_SLIDE, proc_slider_ctrl,	449-220,228-169,ALIGN_TOPLEFT,	&img_slide_red,NULL,					0,SOSD_WIN8,0x100,  2,0,7,0},	//5 end x,y
{ TYPE_SLIDE, proc_slider_ctrl,	242-220,273-169,ALIGN_TOPLEFT,	&img_slide_gray,NULL,					0,SOSD_WIN4,0x080,  0,0,0,0},	// start x,y  //slider G
{ TYPE_SLIDE, proc_slider_ctrl,	449-220,273-169,ALIGN_TOPLEFT,	&img_slide_red,NULL,					0,SOSD_WIN8,0x100,  5,0,9,0},	//7 end x,y
{ TYPE_SLIDE, proc_slider_ctrl,	242-220,320-169,ALIGN_TOPLEFT,	&img_slide_gray,NULL,					0,SOSD_WIN5,0x080,  0,0,0,0},	// start x,y   //slider B
{ TYPE_SLIDE, proc_slider_ctrl,	449-220,320-169,ALIGN_TOPLEFT,	&img_slide_red,NULL,					0,SOSD_WIN8,0x100,  7,0,2,0},	//9 end x,y
#else
{ TYPE_SLIDE, proc_slider_ctrl,	242-220,228-169,ALIGN_TOPMIDDLE,&img_slide_gray,NULL,					0,SOSD_WIN3,0x080,  0,0,0,0},	//  start x,y  //slider R
{ TYPE_SLIDE, proc_slider_ctrl,	488-220,268-169,ALIGN_TOPMIDDLE,&img_slide_red,NULL,					0,SOSD_WIN8,0x100,  2,0,7,0},	//5 end x,y
{ TYPE_SLIDE, proc_slider_ctrl,	242-220,273-169,ALIGN_TOPMIDDLE,&img_slide_gray,NULL,					0,SOSD_WIN4,0x080,  0,0,0,0},	// start x,y  //slider G
{ TYPE_SLIDE, proc_slider_ctrl,	488-220,313-169,ALIGN_TOPMIDDLE,&img_slide_red,NULL,					0,SOSD_WIN8,0x100,  5,0,9,0},	//7 end x,y
{ TYPE_SLIDE, proc_slider_ctrl,	242-220,320-169,ALIGN_TOPMIDDLE,&img_slide_gray,NULL,					0,SOSD_WIN5,0x080,  0,0,0,0},	// start x,y   //slider B
{ TYPE_SLIDE, proc_slider_ctrl,	488-220,360-169,ALIGN_TOPMIDDLE,&img_slide_red,NULL,					0,SOSD_WIN8,0x100,  7,0,2,0},	//9 end x,y
#endif

{ 0, proc_slider_left_tsc,		239, 385,ALIGN_TOPLEFT,			NULL,&img_slide_left1,					0,SOSD_WIN6,0x180,  0,0,0,0},	//
{ 0, proc_slider_right_tsc,		496, 386,ALIGN_TOPLEFT,			NULL,&img_slide_right1,					0,SOSD_WIN6,0x180,  0,0,0,0}	//
};
code struct menu_fitem_info_s menu_slider3_text_items[] = {
	//just test
	{ 0,0,	520-220,235-169,	3,1, 0,0,	0xF0,	 0x08,	FOSD_COLOR_VALUE_RED	},	//R, or F81F:Magenta
	{ 1,3,	520-220,278-169,	3,1, 0,0,	0xF0,	 0x18,	FOSD_COLOR_VALUE_LIME	},	//G
	{ 2,6,	520-220,324-169,	3,1, 0,0,	0xF0,	 0x10,	FOSD_COLOR_VALUE_BLUE	},	//B, or 07FF:Cyan 
};
//BKFYI110216. Palette from 32, does not support the Alpha.
//				If you use under 0x10, it will hurt your original palette value.
//struct menu_page_info_s menu_slider3_page = { MENU_TYPE_SLIDE3, 5,5, 5,3,	 menu_slider3_page_items, menu_slider3_text_items }; //note 3 items
//struct menu_page_info_s menu_slider3_page = { MENU_TYPE_SLIDE3, 5,5, 5,3+1+2,	 menu_slider3_page_items, menu_slider3_text_items }; //note 3 items

//item_start 4
//slide items 3
//left/right 2
struct menu_page_info_s menu_slider3_page = { MENU_TYPE_SLIDE3, 4,4, 4,3*2+2,	 menu_slider3_page_items, menu_slider3_text_items }; //note 3 items

//=====================
// POSITION
//=====================


//----------------------
// position popup MENU
//manu->input->setting with RGB/HDMI->display->position
//----------------------
//					
//
//
//
// 
//
//------------------------------------------
//description
code struct menu_sitem_info_s menu_position_page_items[] = {
{ TYPE_BG,proc_position_bg,		220,179,ALIGN_TOPLEFT,	&img_position_bg,NULL,					0,SOSD_WIN1,0x000,  0,0,0,0},

{ 0,proc_position_ok,			306,385,ALIGN_TOPLEFT,	&img_dialog_ok,&img_dialog_ok1,			0,SOSD_WIN8,0x080,  3,2,3,2}, //1
{ 0,proc_position_undo,			381,385,ALIGN_TOPLEFT,	&img_dialog_cancel,&img_dialog_cancel1,	0,SOSD_WIN8,0x080,  3,1,3,1}, //2

{ 5,proc_position_box,			312,229,ALIGN_TOPLEFT,	&img_position_box_red,NULL,				0,SOSD_WIN8,0x080,  0,0,0,0},  //3	//NOTE:type 2->5
{ 0,proc_position_box,			312,229,ALIGN_TOPLEFT,	&img_position_box_gray,NULL,			0,SOSD_WIN2,0x100,  0,0,0,0}, //4	//NOTE:type 2->5

{ 0, proc_position_left_tsc,	266, 262,ALIGN_TOPLEFT,	NULL,&img_position_left,				0,SOSD_WIN4,0x180,  0,0,0,0},	//
{ 0, proc_position_right_tsc,	486, 262,ALIGN_TOPLEFT,	NULL,&img_position_right,				0,SOSD_WIN4,0x180,  0,0,0,0},	//
{ 0, proc_position_up_tsc,		376, 189,ALIGN_TOPLEFT,	NULL,&img_position_up,					0,SOSD_WIN4,0x180,  0,0,0,0},	//
{ 0, proc_position_down_tsc,	376, 334,ALIGN_TOPLEFT,	NULL,&img_position_down,				0,SOSD_WIN4,0x180,  0,0,0,0}	//

};	
code struct menu_fitem_info_s menu_position_text_items[] = {
//	{ 0,0,	100,50,	3,1, 0,0,	0xF0,	16,0x07E0		},	//FOSD0, OsdRam:0
//	{ 1,256,100,100,3,1, 0,0,		},	//FOSD1, OsdRam:0x100
//	{ 0,0,	518-220,198-179,	3,1, 0,0,	0xF0,	 0x10,	0x07E0	},	//H
//	{ 1,3,	518-220,226-179,	3,1, 0,0,	0xF0,	 0x18,	0x07E0	},	//V
	{ 0,0,	518-220,198-179,	3,1, 0,0,	0xA0,	 0x10,	0x07E0	},	//H
	{ 1,3,	518-220,226-179,	3,1, 0,0,	0xA0,	 0x18,	0x07E0	},	//V
};  
struct menu_page_info_s menu_position_page = { MENU_TYPE_DIALOG, 3,3, 3,1*2+4,	menu_position_page_items, menu_position_text_items }; //note 1 item		

												 

//----------------------
// language MENU
//----------------------
//					
//------------------------------------------
//description
//code struct menu_sitem_info_s menu_lang_info[] = {
//	{ proc_lang_bg,			300,200,ALIGN_TOPLEFT,	image_lang_bg,	0xC0, SOSD_WIN2, 0,0,0,0},
//	{ proc_lang_eng,		300,200,ALIGN_TOPLEFT,	image_lang_eng,	0xC0, SOSD_WIN3, 0,0,0,0},
//	{ proc_lang_kor,		450,350,ALIGN_TOPLEFT,	image_lang_kor,	0xC0, SOSD_WIN4, 0,0,0,0},
//	{ proc_lang_jpn,		450,350,ALIGN_TOPLEFT,	image_lang_jpn,	0xC0, SOSD_WIN4, 0,0,0,0},
//	{ proc_lang_chn,		450,350,ALIGN_TOPLEFT,	img_ok,	0xC0, SOSD_WIN4, 0,0,0,0},
//	{ proc_lang_ok,			450,350,ALIGN_TOPLEFT,	img_ok,	0xC0, SOSD_WIN4, 0,0,0,0},
//};

#endif //..SUPPORT_SPIOSD

