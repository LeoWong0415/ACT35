#ifndef	__TYPEDEFS_H__
#define	__TYPEDEFS_H__

#include <intrins.h>


#define DATA		data
#define PDATA		pdata
#define IDATA		idata
#define XDATA		xdata
#define CODE		code
#define CONST		code
#define CODE_P
#define FAR

//typedef	unsigned char	Register;
typedef	unsigned char	BYTE;
typedef	unsigned int	WORD;
typedef	unsigned long	DWORD;

#define	TRUE	1
#define	FALSE	0
#ifndef NULL
 #define NULL ((void *) 0)
#endif
#define NIL		0xFF

#define ERR_SUCCESS		0
#define ERR_FAIL		1

#define ON		1
#define OFF		0




//VInputStdDetectMode
#define AUTO	0

//VInputStd
#define NTSC	1			
#define PAL		2
#define SECAM	3
#define NTSC4	4
#define PALM	5
#define PALN	6
#define PAL60	7

// SUPPORT  HAN     VAN     VFREQ  	HTOTAL  VTOTAL  HS  VS	Hst  	Vst		OffsetH OffsetV Dummy0 	Dummy1 	Dummy2 	Dummy3
struct _PCMODEDATA{
	BYTE	support;
	WORD	han;		//H Active Length
	WORD	van;		//V Active Length
	BYTE	vfreq;
	WORD	htotal;		//H Total
	WORD	vtotal;		//V Total
	BYTE	hsyncpol;
	BYTE	vsyncpol;
	WORD	hstart;		//
	WORD	vstart;		//
	WORD	offseth;	//
	WORD	offsetv;	//
	BYTE	dummy0;
	BYTE	dummy1;
	WORD	dummy2;
	BYTE	dummy3;
} ;

struct RegisterInfo
{
   int	Min;
   int	Max;
   int	Default;
};
struct LongRegisterInfo
{
   WORD	Min;
   WORD	Max;
   WORD	Default;
};

 
typedef struct { 
    DWORD start; 
    DWORD length; 
    WORD left; 
    WORD top; 
    WORD right; 
    WORD bottom; 
} SPIIMAGE; 

typedef struct { 
    DWORD start; 
    DWORD length; 
} SLIDEIMAGE; 

#ifdef SUPPORT_HDMI_SiIRX
//new alternetive unsigned type names
typedef unsigned char  uint8_t;
typedef unsigned char  u8;		//v2.2.3
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;
typedef unsigned char  bool_t;	/* this type can be used in structures */

#define ROM   code       // 8051 type of ROM memory
#define IRAM  idata      // 8051 type of RAM memory


typedef  bit BOOL;			//v2.2.3

#endif


#endif // __TYPEDEFS_H__
