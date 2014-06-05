/* monitor.h */


void Mon_tx(BYTE ch);

void SoftReset(void);
void MonHelp(void);
BYTE MonGetCommand(void);
void Monitor(void);
void Monitor1(void);
void Prompt(void);
void Prompt1(void);
void WaitUserInput(void);

BYTE GetMonAddress(void);
void SetMonAddress(BYTE addr);
void MonReadI2CByte(void);
void MonWriteI2CByte(void);

void DeltaRGBPanelInit(void);

//=====monitor_mcu.c============
BYTE WriteSFR(BYTE addr, BYTE dat);
BYTE ReadSFR(BYTE addr);
void MonitorMCU(void);
