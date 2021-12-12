#ifndef __RTC_H__
#define __RTC_H__

#include "includes.h"

void RTC_Init(void);
void SetRtcCount(time_t timestamp);
char* GetCurrentTime(void);
time_t GetTimeStamp(void);

#endif

