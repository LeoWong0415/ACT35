#ifndef __SETTINGS_H__
#define __SETTINGS_H__

extern CODE BYTE TW8835_GENERAL[];
extern CODE BYTE TW8835_SSPLL[];
extern CODE BYTE TW8835_DECODER[];
extern CODE BYTE TW8835_ADCLLPLL[];
//extern CODE BYTE TW8835_SCALER[];
extern CODE BYTE TW8835_TCON[];
extern CODE BYTE TW8835_GAMMA[];
extern CODE BYTE TW8835_EN_OUTPUT[];
//extern CODE BYTE TW8835_INIT_TCON_SCALER[];
//extern CODE BYTE TW8835_INIT_ADC_PLL[];

void SW_Reset(void);
//for Monitor
void 	ClockHigh(void);
void 	ClockLow(void);
void 	Clock27(void);

void  SSPLL_PowerUp(BYTE fOn);

void  SspllSetFreqReg(DWORD FPLL);
DWORD SspllGetFreqReg(BYTE fHost);
void  SspllSetFreqAndPll(DWORD _PPF);
DWORD SspllFREQ2FPLL(DWORD FREQ, BYTE POST);
DWORD SspllFPLL2FREQ(DWORD FPLL, BYTE POST);
BYTE  SspllGetPost(BYTE fHost);
DWORD SspllGetPPF(BYTE fHost);
void  SspllSetAnalogControl(BYTE value);
BYTE  SspllGetPost(BYTE fHost);

void  PclkSetDividerReg(BYTE divider);
DWORD PclkGetFreq(BYTE fHost, DWORD sspll);
DWORD PclkoGetFreq(BYTE fHost,DWORD pclk);
void  PclkoSetDiv(/*BYTE pol,*/ BYTE div);
void  PclkSetPolarity(BYTE pol);

void  ClkPllSetSelectReg(BYTE ClkPllSet);
void  ClkPllSetDividerReg(BYTE divider);
void  ClkPllSetSelDiv(BYTE ClkPllSel, BYTE ClkPllDiv);
DWORD ClkPllGetFreq(BYTE fHost);

extern BYTE shadow_r4e0;
extern BYTE shadow_r4e1;
BYTE McuSpiClkToPclk(BYTE divider);
void McuSpiClkRestore(void);
BYTE McuSpiClkReadSelectReg(void);
#define MCUSPI_CLK_27M		0
#define MCUSPI_CLK_32K		1
#define MCUSPI_CLK_PCLK		2
#define CLKPLL_SEL_PCLK		0
#define CLKPLL_SEL_PLL		1
#define CLKPLL_DIV_1P0		0
#define CLKPLL_DIV_1P5		1
#define CLKPLL_DIV_2P0		2
#define CLKPLL_DIV_2P5		3
#define CLKPLL_DIV_5P0		7

void McuSpiClkSet(BYTE McuSpiClkSel, BYTE ClkPllSel, BYTE ClkPllDiv); 
void McuSpiClkSelect(BYTE McuSpiClkSel);
DWORD McuGetClkFreq(BYTE fHost);


DWORD SpiClkGetFreq(BYTE fHost, DWORD mcu_clk);

void LLPLLSetClockSource(BYTE use_27M);
void SetDefaultPClock(void);

void DumpClock(BYTE fHost);
void DumpRegister(BYTE page);

BYTE DCDC_On(BYTE step);
BYTE DCDC_StartUP(void);

void FP_BiasOnOff(BYTE fOn);
void FP_PWC_OnOff(BYTE fOn);
void FP_GpioDefault(void);


//void InitCVBS(void);
//void InitWithNTSC(void);
//void EnOutput2DCDC2LEDC(BYTE on);
//void InitTconScaler(void);
void InitWithNTSC(void);

//BYTE CheckVDLossAndSetFreeRun(void);

#endif
