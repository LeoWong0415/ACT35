#ifndef _DECODER_H_
#define _DECODER_H_

BYTE DecoderCheckVDLOSS( BYTE n );
BYTE DecoderCheckSTD( BYTE n );

BYTE CheckTW9900VDLOSS( BYTE n );
BYTE CheckTW9900STD( BYTE n );

void InMuxSetInput(BYTE InputMode);
void DecoderPowerDown(BYTE fOn);
WORD DecoderGetVDelay(void);
void DecoderSetVDelay(WORD vDelay);
void DecoderSetDelayActive(WORD hdelay, WORD hactive, WORD vdelay, WORD vactive);

void DecoderSetVActive(WORD length);
WORD DecoderGetVActive(void);
void DecoderSetHDelay(WORD delay);
WORD DecoderGetHDelay(void);
void DecoderSetHActive(WORD length);
BYTE DecoderReadDetectedMode(void);
BYTE DecoderIsNoInput(void);
BYTE DecoderReadVInputSTD(void);
WORD DecoderGetHActive(void);



#define DECODER_FREERUN_AUTO	0
#define DECODER_FREERUN_60HZ	2
void DecoderFreerun(BYTE mode);


#endif
