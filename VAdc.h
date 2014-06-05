#ifndef __VIDEO_ADC__
#define	__VIDEO_ADC__

#if !defined(SUPPORT_COMPONENT) && !defined(SUPPORT_PC)
//----------------------------
//Trick for Bank Code Segment
//----------------------------
void Dummy_VADC_func(void);
#endif




extern XDATA	BYTE	InputVAdcMode;

#ifdef MODEL_TW8835FPGA
void SetExtVAdcI2C(BYTE addr, BYTE mode);
#endif

//void VAdcCheckReg(BYTE reg);

BYTE GetInputVAdcMode(void);

void SetExtVAdcI2C(BYTE addr, BYTE mode);
//void SetExtVideoAdcI2C(BYTE addr, BYTE type, BYTE mode);

BYTE VAdcSetVcoRange(DWORD _IPF);

void VAdcLLPLLSetDivider(WORD value, BYTE fInit);
WORD VAdcLLPLLGetDivider(void);

void VAdcSetLLPLLControl(BYTE value);

void VAdcSetClampModeHSyncEdge(BYTE fOn);
void VAdcSetClampPosition(BYTE value);


void VAdcSetPhase(BYTE value, BYTE fInit); 	//WithInit
BYTE VAdcGetPhase(void);
void VAdcSetFilterBandwidth(BYTE value, WORD delay);

void AutoColorAdjust(void);

BYTE VAdcCheckInput(BYTE type);
BYTE VAdcDoubleCheckInput(BYTE detected);
void VAdcSetPowerDown(void);

void VAdcSetChannelGainReg(WORD GainG,WORD GainB,WORD GainR);
WORD VAdcReadGChannelGainReg(void);
WORD VAdcReadBChannelGainReg(void);
WORD VAdcReadRChannelGainReg(void);

void VAdcSetDefaultFor(void);
//void VAdcSetDefaultForYUV(void);
//void VAdcSetDefaultForRGB(void);
//BYTE VAdcSetForRGB( void );
void VAdcSetPolarity(BYTE fUseCAPAS);
BYTE VAdcGetInputStatus(void);
//void VAdcAdjustByMode(BYTE mode);
void VAdcAdjustPhase(BYTE mode);
//void VAdcLoopFilter(BYTE value);

//void SetADCMode(BYTE mode);

BYTE WaitStableLLPLL(WORD delay);

//void VAdcUpdateLoopFilter(WORD delay, BYTE value);
BYTE VAdcLLPLLUpdateDivider(WORD divider, /*BYTE ctrl,*/ BYTE fInit, BYTE delay);

//BYTE FindInputModePC(WORD *vt);
//BYTE FindInputModeCOMP( void );


//void PCSetInputCrop( BYTE mode );
//void PCSetOutput( BYTE mode );

//void YUVSetOutput(BYTE mode);

//BYTE PCCheckMode(void);
void AdjustPixelClk(WORD divider, BYTE mode );

//BYTE CheckAndSetYPBPR( void );
BYTE CheckAndSetPC(void);
BYTE CheckAndSetComponent( void );


//=============================================================================
//setup menu interface
//=============================================================================
extern void PCRestoreH(void);
extern void PCRestoreV(void);
extern void PCResetCurrEEPROMMode(void);


#endif //__VIDEO_ADC__
