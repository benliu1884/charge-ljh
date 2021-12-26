#include "cmu.h"
#include "delay.h"


#define portNVIC_SYSTICK_CTRL_REG			( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG			( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG	( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSPRI2_REG				( * ( ( volatile uint32_t * ) 0xe000ed20 ) )
	
#define portNVIC_SYSTICK_CLK_BIT			( 1UL << 2UL )
#define portNVIC_SYSTICK_INT_BIT			( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )


static volatile uint32_t xTickCount = 0;


void SysTick_Handler(void)
{
	xTickCount++;
}


/**
 *��ȡϵͳʱ���
 */
uint32_t OSTimeGet (void)
{
	uint32_t  xTicks;
	
	Disable_Int();
	xTicks = xTickCount;
	Enable_Int();
	
	return xTicks;
}

void OSTimeDly(uint32_t tick)
{
	uint32_t time = OSTimeGet();
	
	while(xTickCount - time <= tick){
		Delay_mSec(1);
	}
}


void SystemClock_Config(void)
{
    CMU_InitTypeDef CMU_InitStruct;
    CMU_InitStruct.SysClkSel = SysPLL;
    CMU_InitStruct.CPUDiv = CPUDiv1;
    HT_CMU_Init(&CMU_InitStruct);
	//��ʼ��ϵͳʱ��
    while(HT_CMU_SysClkGet() != SystemFsysClock)
    {
        HT_CMU_Init(&CMU_InitStruct);
        Delay_mSec(1000);
        Feed_WDT();
    }
	
	//�ڲ�����ʱ��
	//SwitchTo_Fhrc();
	
	//��ʼ���δ�ʱ��
	SysTick_Config (SystemFsysClock/1000);
//	portNVIC_SYSTICK_CTRL_REG = 0UL;
//	portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
//	portNVIC_SYSTICK_LOAD_REG = ( SystemFsysClock / 1000 ) - 1UL;
//	portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}


/*******************************************************************************
����������	�л����ڲ�����ʱ��
���������
���ز�����
����˵����	8MHz
*******************************************************************************/
void SwitchTo_Fhrc(void)
{
	if ((HT_CMU->SYSCLKCFG != 0x0002)
	|| (HT_CMU->HRCADJ    != 0x003D)
	|| (HT_CMU->HRCDIV    != 0x0000)
	|| !(HT_CMU->CLKCTRL0 & 0x0020)
	|| (HT_CMU->SYSCLKDIV != 0x0000))
	{
		EnWr_WPREG();
		HT_CMU->HRCADJ    = 0x003D;				//���8MHz
		HT_CMU->HRCDIV    = 0x0000;				//Fhrc' = Fhrc = 8MHz
		HT_CMU->CLKCTRL0 |= 0x0020;				//ʹ��HRC
		HT_CMU->SYSCLKDIV = 0x0000;				//Fcpu  = Fsys = 8MHz
		while (HT_CMU->CLKSTA & 0x0008)
		{
			;
		}
		HT_CMU->SYSCLKCFG = 0x0082;				//Fsys = Fhrc'
		__NOP();
		HT_CMU->SYSCLKCFG = 0x0002;				//Fsys = Fhrc'
		DisWr_WPREG();
	}
}
