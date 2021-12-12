#ifndef __CMU_H__
#define __CMU_H__

#include "includes.h"

uint32_t OSTimeGet (void);
void OSTimeDly(uint32_t tick);

void SystemClock_Config(void);

void SwitchTo_Fhrc(void);

#endif
