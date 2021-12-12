#include "pmu.h"
#include "cmu.h"
#include "emu.h"
#include "includes.h"
#include "delay.h"

void PMU_Init(void)
{
	HT_CMU->WPREG = 0xA55A;
	HT_PMU->PMUCON = 0x0008;
	//HT_PMU->VDETPCFG = 0x0007;             //300us, 2144ms
	
	HT_GPIOE->IOCFG |= 0x0200;             //PE9 as LVDIN1
    HT_PMU->VDETCFG	= 0x0089;								//VCC检测阈值=4.6V;BOR检测阈值=2.4V
	HT_PMU->VDETPCFG= 0x0020;								//VCC_DET,BOR_DET检测时间=300us,周期=67ms	
//	HT_PMU->VDETCFG = 0x0089;  
	
	HT_PMU->PMUIF = 0x0000;
	HT_PMU_ITConfig(PMU_PMUIE_VCCIE|PMU_PMUIE_LVD1IE,ENABLE);
	NVIC_EnableIRQ(PMU_IRQn);
}
