#ifndef _INPUTCTRL_H_
#define _INPUTCTRL_H_

#define	INPUT_CVBS		0   //composite
#define	INPUT_SVIDEO	1	//Y & C
#define INPUT_COMP		2	//INPUT_YUV. analog. SOG
#define INPUT_PC		3	//INPUT_RGB	 RGB with HSYNC & VSYHC
#define	INPUT_DVI		4	//BT656, BT709	- digital
#define INPUT_HDMIPC	5
#define INPUT_HDMITV	6
#define	INPUT_BT656		7	//DVI-8bit: BT656
#define INPUT_LVDS		8
#define INPUT_TOTAL		9


#define	INPUT_CHANGE_DELAY	300
#define	INPUT_CHANGE_DELAY_BASE10MS	30


//DVI, HDMI  -> digital
//PC         -> Analog RGB
//component  -> Analog YCbCr. Please Do not use YUV.

extern XDATA	BYTE	InputMain;
extern XDATA	BYTE	InputSubMode;
extern XDATA	BYTE	OldInputMain;

//extern BYTE last_position_h;
//extern BYTE last_position_v;
//extern BYTE temp_position_h;
//extern BYTE temp_position_v;



//--------------------------------
// input module
//--------------------------------
BYTE GetInputMain(void);			//friend
void SetInputMain(BYTE input);		//friend..only update InputMain global variable
void InitInputAsDefault(void);



//scaler input
#define INPUT_PATH_DECODER	0x00
#define INPUT_PATH_VADC		0x01
#define INPUT_PATH_DTV		0x02
#define INPUT_PATH_BT656	0x06	//DTV+2ndDTV_CLK

#define INPUT_FORMAT_YCBCR	0
#define INPUT_FORMAT_RGB		1
void InputSetSource(BYTE path, BYTE format);
void InputSetFieldPolarity(BYTE fInv);
void InputSetProgressiveField(fOn);
void InputSetCrop( WORD x, WORD y, WORD w, WORD h );
void InputSetCropStart( WORD x, WORD y);
void InputSetHStart( WORD x);
void InputSetVStart( WORD y);
WORD InputGetHStart(void);
WORD InputGetVStart(void);
WORD InputGetHLen(void);
WORD InputGetVLen(void);
void InputSetPolarity(BYTE V,BYTE H, BYTE F);

void BT656InputFreerunClk(BYTE fFreerun, BYTE fInvClk);
void BT656InputSetFreerun(BYTE fOn);




void PrintfInput(BYTE Input, BYTE debug);


void ChangeInput( BYTE mode );
void InputModeNext( void );

BYTE CheckInput( void );

//void SetDefault_Decoder(void);
void VInput_enableOutput(BYTE fRecheck);
void VInput_gotoFreerun(BYTE reason);


BYTE ChangeCVBS( void );
BYTE ChangeSVIDEO( void );
BYTE ChangeCOMPONENT( void );
BYTE ChangePC( void );
BYTE ChangeDVI( void );
BYTE ChangeHDMI(void);
BYTE ChangeBT656( void );

BYTE CheckAndSetDecoderScaler( void );
//BYTE CheckAndSetYPBPR( void );
//CheckAndSetPC @ measure.h
//CheckAndSetDVI @ measure0.h
BYTE CheckAndSetBT656(void);



#endif
