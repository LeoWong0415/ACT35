/* 
DebugMsg.h
*/
#ifndef SUPPORT_HDMI_EP9351
void Dummy_DebugMsg_func(void);
#else //..SUPPORT_HDMI_EP9351

void DBG_PrintAviInfoFrame(void);
void DBG_PrintTimingRegister(void);
void DBG_DumpControlRegister(void);


#endif //..SUPPORT_HDMI_EP9351

void DumpDviTable(WORD hActive,WORD vActive);
