//PC_WVGA.h	800x480
CONST struct _PCMDATA PCMDATA[] = {
//===========================================================================================================
//                       PC mode Table for WVGA Panel    09-Aug, 2002
//===========================================================================================================

//       SUPPORT  HAN      VAN     IVF  CLOCK    low      high     Hst  Vst  IPF  PPF  VBack
                                                                     
	{  1,     720+2,   400+2,  70,  0x383,   0x383,   0x383,   139, 37,  283, 370, 0x40 },	//430	//  0: DOS mode             
                                                                                                                    
	{  1,     640,     400+2,  85,  0x33f,   0x33f,   0x33f,   125, 43,  315, 460, 0x47 },		//  1: 640 x 400            
	{  0,     640,     400+2,  85,  0x33f,   0x33f,   0x33f,   157, 37,  315, 460, 0x47 },		//  2: 640 x 350 ???        
	{  0,     720,     400+2,  85,  0x33f,   0x33f,   0x33f,   163, 47,  315, 460, 0x40 },		//  3: 720 x 400 ???        
                                                                                                                    
	{  1,     640,     480,    60,  0x31f,   0x317,   0x327,   44, 35,  251, 380, 0x32 },		//  4: EE_VGA_60            
	{  1,     640,     480,    66,  0x35f,   0x357,   0x37f,   144, 35,  302, 380, 0x17 },		//  5: EE_VGA_66 *** MAC    
	{  1,     640,     480,    70,  0x32f,   0x327,   0x337,   155, 22,  300, 380, 0x17 },		//  6: EE_VGA_70            
	{  1,     640,     480,    72,  0x333,   0x32f,   0x347,   171, 30,  315, 400, 0x2a },		//  7: EE_VGA_72            
	{  1,     640,     480,    75,  0x347,   0x32f,   0x34f,   187, 20,  315, 440, 0x17 },		//  8: EE_VGA_75            
	{  0,     640,     480,    85,  0x33f,   0x337,   0x347,   139, 29,  360, 430, 0x1f },		//  9: EE_VGA_85            
                                                                                                                    
	{  1,     800,     600,  56,  0x3ff,   0x3f7,   0x407,   200, 24,  360, 410, 0x19 },		// 10: EE_SVGA_56           
	{  1,     800,     600,  60,  0x41f,   0x417,   0x427,   219, 28,  400, 420, 0x07 },	//06//1a	// 11: EE_SVGA_60           
	{  1,     800,     600,  70,  0x40f,   0x3e7,   0x417,   197, 25,  449, 430, 0x10 },		// 12: EE_SVGA_70           
	{  1,     800,     600,  72,  0x40f,   0x407,   0x417,   181, 30,  500, 440, 0x1a },		// 13: EE_SVGA_72           
	{  1,     800,     600,  75,  0x41f,   0x407,   0x427,   237, 25,  495, 450, 0x18 },		// 14: EE_SVGA_75           
	{  0,     800,     600,  85,  0x417,   0x40f,   0x427,   213, 31,  563, 440, 0x20 },		// 15: EE_SVGA_85           
                                                                                                                    
	{  1,     832,     624,    75,  0x47f,   0x45f,   0x487,   200, 30,  573, 440, 0x18 },		// 16: EE_832_75  *** MAC   
                                                                                                                    
	{  1,    1024,   768,  60,  0x53f,   0x527,   0x547,   292, 36,  650, 450, 0x19 },		// 17: EE_XGA_60            
	{  0,    1024,   768,  70,  0x52f,   0x527,   0x54f,   275, 36,  750, 470, 0x19 },		// 18: EE_XGA_70            
	{  0,    1024,   768,  72,  0x54f,   0x4ff,   0x557,   276, 33,  750, 470, 0x19 },		// 19: EE_XGA_72            
	{  0,    1024,   768,  75,  0x51f,   0x517,   0x54f,   268, 32,  789, 490, 0x18 },		// 20: EE_XGA_75            
	{  0,    1024,   768,  85,  0x55f,   0x557,   0x567,   300, 40,  945, 560, 0x19 },		// 21: EE_XGA_85            
                                                                                                                    
	{  0,    1152,     864,    60,  0x5ef,   0x5b7,   0x5f7,   300, 31,  847, 432, 39 },		// 22: EE_1152_60           
	{  0,    1152,     864,    70,  0x5ff,   0x5c7,   0x607,   308, 36,  966, 432, 39 },		// 23: EE_1152_70           
	{  0,    1152,     864,    75,  0x63f,   0x5e7,   0x647,   308, 36, 1080, 432, 39 },		// 24: EE_1152_75           
                                                                                                                    
	{  0,    1280,     1024,   60,  0x697,   0x697,   0x69f,   356, 42, 1080, 432, 39 },		// 25: EE_SXGA_60           
	{  0,    1280,     1024,   70,  0x6bf,   0x6bf,   0x6bf,   356, 42, 1350, 432, 39 },		// 26: EE_SXGA_70           
	{  0,    1280,     1024,   75,  0x697,   0x68f,   0x6a7,   356, 42, 1350, 432, 39 },		// 27: EE_SXGA_75           
	                                                                                                                
	{  0,     720-35,  480-24, 60,  0x359,   0x359,   0x359,   120, 43,  251, 378, 36 },		// 28: EE_RGB_480P        
	{  0,    1280-64,  720-50, 60,  0x671,   0x671,   0x671,   346, 43,  742, 410, 24 },		// 29: EE_RGB_720P        
	{  0,    1280-64,  720-50, 60,  0x7bb,   0x7bb,   0x7bb,   346, 25,  742, 378, 45 },		// 30: EE_RGB_720P50 ???  
	{  0,    1920-96,  540-40, 60,  0x897,   0x897,   0x897,   300, 42,  740, 324, 32 },		// 31: EE_RGB_1080I       
	{  0,    1920-120, 540-40, 50,  0xa4f,   0xa4f,   0xa4f,   300, 41,  740, 324, 32 },		// 32: EE_RGB_1080I50A    
	{  0,    1920-120, 540-40, 60,  0x897,   0x897,   0x897,   300, 42,  740, 324, 32 },		// 33: EE_RGB_1080I50B ???
	{  0,     720-35,  240-12, 60,  0x359,   0x359,   0x359,   152, 27,  123, 324, 42 },		// 34: EE_RGB_480I        
	{  0,     720-35,  288-14, 50,  0x35f,   0x35f,   0x35f,   130, 96,  131, 324, 32 },		// 35: EE_RGB_576I        
	{  0,     720-35,  576-28, 50,  0x35f,   0x35f,   0x35f,   130, 48,  250, 324, 36 },		// 36: EE_RGB_576P

														    	
	{  1,     720,  480, 60,  0x359,   0x359,   0x359,   140, 22,  251, 378, 10 },		// 37: EE_YPbPr_480P        
	{  0,    1280,  720, 60,  0x671,   0x671,   0x671,   346, 43,  742, 410, 52 },		// 38: EE_YPbPr_720P        
	{  0,    1280,  720, 60,  0x7bb,   0x7bb,   0x7bb,   346, 25,  742, 378, 45 },		// 39: EE_YPbPr_720P50 ???  
	{  0,    1920,  540, 60,  0x897,   0x897,   0x897,   300, 42,  740, 380, 52 },		// 40: EE_YPbPr_1080I       
	{  0,    1920,  540, 50,  0xa4f,   0xa4f,   0xa4f,   300, 41,  740, 324, 52 },		// 41: EE_YPbPr_1080I50A    
	{  0,    1920,  540, 60,  0x897,   0x897,   0x897,   300, 42,  740, 324, 52 },		// 42: EE_YPbPr_1080I50B ???
	{  1,     720,  240, 60,  0x359,   0x359,   0x359,   140, 10,  123, 360, 12 },		// 43: EE_YPbPr_480I        
	{  1,     720,  288, 50,  0x35f,   0x35f,   0x35f,   150, 20,  131, 324, 9 },		// 44: EE_YPbPr_576I        
	{  1,     720,  576, 50,  0x35f,   0x35f,   0x35f,   150, 40,  250, 324, 10 },		// 45: EE_YPBPR_576P
	{  0,       0,        0,    0,      0,       0,       0,     0,  0,    0,   0,  0 },			// 46: unknown

	{  1,    1280,     768,    60,  0x68f,   0x67f,   0x69f,   292, 36,  650, 431,  0x19 },		// 47: EE_WXGA_60
	{  1,    1280,     800,    60,  0x5af,   0x59f,   0x5bf,   280, 4,  650, 469,  4 },		// 48: EE_WXGA+_60
};


