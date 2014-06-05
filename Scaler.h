#ifndef __SCALER_H__
#define __SCALER_H__

#define VIDEO_ASPECT_NORMAL		0
#define VIDEO_ASPECT_ZOOM		1
#define VIDEO_ASPECT_FILL		2
#define VIDEO_ASPECT_PANO		3
extern BYTE VideoAspect;

void ScalerSetOutputFixedVline(BYTE onoff /*, WORD lines*/);


//-------------------------------------
// Scale ratio
void ScalerWriteXUpReg(WORD value);
void ScalerWriteXDownReg(WORD value);
WORD ScalerReadXDownReg(void);
void ScalerWriteVScaleReg(WORD value);
WORD ScalerReadVScaleReg(void);
void ScalerSetHScaleReg(WORD down, WORD up);

void ScalerSetHScale(WORD Length);
void ScalerSetHScaleWithRatio(WORD Length, WORD ratio);
void ScalerSetVScale(WORD Length);
void ScalerSetVScaleWithRatio(WORD Length, WORD ratio);

//-------------------------------------
// Panorama WaterGlass effect
void ScalerPanoramaOnOff(BYTE fOn);
void ScalerSetPanorama(WORD px_scale, BYTE px_inc);

//-------------------------------------
// Line Buffer
void ScalerWriteLineBufferDelay(BYTE delay);
BYTE ScalerReadLineBufferDelay(void);
void ScalerSetLineBufferSize(WORD length);
void ScalerSetLineBuffer(BYTE delay, WORD length);
void ScalerSetFPHSOutputPolarity(BYTE fInvert);

void ScalerWriteOutputHBlank(WORD length);

//-------------------------------------
// HDE VDE
void ScalerWriteHDEReg(BYTE pos);
BYTE ScalerReadHDEReg(void);
WORD ScalerCalcHDE(void);

void ScalerWriteOutputWidth(WORD width);
WORD ScalerReadOutputWidth(void);

void ScalerSetOutputWidthAndHeight(WORD width, WORD height);

void ScalerWriteOutputHeight(WORD height);

void ScalerWriteVDEReg(BYTE pos);
BYTE ScalerReadVDEReg(void);
WORD ScalerCalcVDE(void);
void ScalerSetVDEPosHeight(BYTE pos, WORD len);

void ScalerSetVDEMask(BYTE top, BYTE bottom);

//-------------------------------------
//HSYNC VSYNC

void ScalerSetHSyncPosLen(BYTE pos, BYTE len);
void ScalerSetVSyncPosLen(BYTE pos, BYTE len);


//-------------------------------------
// Freerun
WORD ScalerCalcFreerunHtotal(void);
void ScalerWriteFreerunHtotal(WORD value);

WORD ScalerCalcFreerunVtotal(void);
void ScalerWriteFreerunVtotal(WORD value);


//-------------------------------------
// Scaler Freerun
void ScalerSetFreerunManual( BYTE on );
BYTE ScalerIsFreerunManual( void );
void ScalerSetFreerunAutoManual(BYTE fAuto, BYTE fManual);
void ScalerSetMuteAutoManual(BYTE fAuto, BYTE fManual);

void ScalerSetMuteManual( BYTE on );




void ScalerSetFreerunValue(BYTE fForce);
void ScalerCheckPanelFreerunValue(void);
void ScalerSetDeOnFreerun(void);

#define ScalerReadHActive()	ScalerReadOutputWidth()
#define ScalerSetWidthAndHeight(w,h)	ScalerSetOutputWidthAndHeight(w,h)	
#define ScalerSetBlackScreen(on)	ScalerSetMuteManual(!on)

#endif