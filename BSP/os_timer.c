/*-------------------------------------------------------------------------------
�ļ����ƣ�os_timer.c
author:benliu
�ļ��������ඨʱ������
---------------------------------------------------------------------------------*/
#include "os_timer.h"
#include "includes.h"

static _timer_manage_t timer_manage;

void OSTimer_Task(void);


void TIMER_1_IRQHandler()
{
	if(SET == HT_TMR_ITFlagStatusGet(HT_TMR1, TMR_TMRIF_PRDIF))                /*!< �����ж�           */
    {
        HT_TMR_ClearITPendingBit(HT_TMR1, TMR_TMRIF_PRDIF);                    /*!< ����жϱ�־       */
		
		OSTimer_Task();
	}
}


void OSTimer_Task(void)
{
	for (uint8_t i=0; i< MAX_TIMER_CNT; i++) 
	{
		if(timer_manage.timer_info[i].state == TIMER_ON)
		{
			timer_manage.timer_info[i].elapse++;
			if(timer_manage.timer_info[i].timeout <= timer_manage.timer_info[i].elapse)
			{
				timer_manage.timer_info[i].callback(timer_manage.timer_info[i].argv);
				Del_Timer(i);
			}
		}
	}
}

static void Timer1_Init(void)	
{
	EnWr_WPREG();
    
    HT_TMR1->TMRCON |= 0x07;	
    HT_TMR1->TMRDIV = 19660-1;     //1KHZ
    HT_TMR1->TMRPRD = 1000;			//1s
    HT_CMU->CLKCTRL1 |= 0x0002; 
    HT_TMR1->TMRIE = 1;
	
	NVIC_SetPriority(TIMER_1_IRQn, 4);
	NVIC_ClearPendingIRQ(TIMER_1_IRQn);
	NVIC_EnableIRQ(TIMER_1_IRQn);
}

/**
 *ɾ��һ����ʱ������
 */
int Del_Timer(timer_handle_t handler)
{
	if(handler < 0 || handler > MAX_TIMER_CNT)
		return -1;
	timer_manage.timer_info[handler].callback = NULL;
	timer_manage.timer_info[handler].state = TIMER_OFF;
	timer_manage.timer_info[handler].timeout = 0;
	timer_manage.timer_info[handler].elapse = 0;
	return 0;
}

/**
 *����һ����ʱ������
 *timeout����ʱʱ�䣬��λ��
 *back����ʱ��Ļص�����
 */
timer_handle_t Set_Timer(int ticks,timer_callback back,void *argv)
{
	uint8_t i = 0;
	if(ticks < 0 || back == NULL)
		return -1;
	for ( ; i < MAX_TIMER_CNT; ++i) 
	{
		if(timer_manage.timer_info[i].state == TIMER_OFF){
			memset(&timer_manage.timer_info[i],0,sizeof(timer_manage.timer_info[i]));
			timer_manage.timer_info[i].timeout = ticks;
			timer_manage.timer_info[i].callback = back;
			timer_manage.timer_info[i].argv = argv;
			timer_manage.timer_info[i].elapse = 0;
			timer_manage.timer_info[i].state = TIMER_ON;
			break;
		}
	}
	if(i >= MAX_TIMER_CNT)
	{
		return (-1);
	}
	return (i);
}


void OSTimer_Init(void)
{
	Timer1_Init();
	
	int i = 0;
	for(i=0;i<MAX_TIMER_CNT;i++)
	{
		timer_manage.timer_info[i].callback = NULL;
		timer_manage.timer_info[i].callback = NULL;
		timer_manage.timer_info[i].state = TIMER_OFF;
		timer_manage.timer_info[i].timeout = 0;
		timer_manage.timer_info[i].elapse = 0;
	}
}

