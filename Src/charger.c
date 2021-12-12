#include "charger.h"
#include "cmu.h"
#include "sim_pwm.h"

//充电接口信息
charger_info_t charger;

#define RELAY_ON()		HT_GPIO_BitsSet(HT_GPIOB,GPIO_Pin_0)
#define RELAY_OFF()		HT_GPIO_BitsReset(HT_GPIOB,GPIO_Pin_0)



static void Gpio_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
		
	//LED1
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init(HT_GPIOD,&GPIO_InitStruct);
		
	//LED2
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init(HT_GPIOD,&GPIO_InitStruct);
	
	//LED3
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init(HT_GPIOD,&GPIO_InitStruct);
	
	//LED4
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init(HT_GPIOD,&GPIO_InitStruct);
	
	//RELAY_L
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init(HT_GPIOB,&GPIO_InitStruct);
	
//	//RELAY_N
//    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
//    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IOOUT;
//    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
//    HT_GPIO_Init(HT_GPIOB,&GPIO_InitStruct);
	
	
	//PA7-接地
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IOIN;
    GPIO_InitStruct.GPIO_InputStruct = GPIO_Input_Up;
    HT_GPIO_Init(HT_GPIOA,&GPIO_InitStruct);
	
	RELAY_OFF();
	//led off
	LED_OFF(LED1|LED2|LED3|LED4);	
}


static void CP_Check_Init()
{
	//电压检测GPIO初始化
	GPIO_InitAFTypeDef GPIO_InitAFStruct;
    GPIO_InitAFStruct.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitAFStruct.GPIO_AFMode = PA12_ADCIN0;
    GPIO_InitAFStruct.GPIO_InputStruct = GPIO_Input_Up;
    HT_GPIO_AFInit(HT_GPIOA,&GPIO_InitAFStruct); 
	
	EnWr_WPREG();
	
	//ad采集
	HT_TBSConfig(TBS_TBSCON_ADC0En,ENABLE);      
	HT_TBS_PeriodSet(ADC0PRD,TBS_TBSPRD_ADC0PRD_1S);	
    //HT_TBS_ITConfig(TBS_TBSIE_ADC0IE,ENABLE);                
	
	//温度采集
	HT_TBSConfig(TBS_TBSCON_TMPEn,ENABLE);  
	HT_TBS->TBSCON |= 0x0300;			//采集8次取平均值
	//HT_TBS_PeriodSet(TMPPRD,TBS_TBSPRD_TMPPRD_1S);
	
//	NVIC_ClearPendingIRQ(TBS_IRQn);			//清除挂起状态
//	NVIC_SetPriority(TBS_IRQn, 3);			//设置优先级
//	NVIC_EnableIRQ(TBS_IRQn);				//使能TBS中断
}


void onOpenCharger_TimeOut(void *parg)
{
	gun_info_t *gun  = (gun_info_t *)parg;
	if(gun != NULL)
	{
		gun->openTimer = -1;
		gun->ChargerState = ChargerState_READY;
		
		if(charger.callback_start != NULL)
		{
			charger.callback_start(gun,CHARGER_RESULT_S2_TIMEOUT);
		}
		gun->gunState = SysState_NONE;
	}
}

static void ChargerStartOK(gun_info_t *gun,CHARGER_RESULT result)
{
	LOG("chargerStartOK,result=%d\r\n",(int)result);
	if(result == CHARGER_RESULT_OK)
	{
		gun->gunState = SysState_WORKING;
	}else{
		gun->gunState = SysState_NONE;
	}
	
	if(charger.callback_start != NULL)
	{
		charger.callback_start(gun,result);
	}
	if(gun->openTimer != -1)
	{
		Del_Timer(gun->openTimer);
	}
}


//充电完成
static void ChargerStopOK(gun_info_t *gun)
{
	LOG("ChargerStopOK.\r\n");
	if(charger.callback_complete)
	{
		charger.callback_complete(gun);
	}
	gun->lastChargerTime  = OSTimeGet();
	gun->gunState = SysState_Finish;
}

void ChargerTask(void)
{
	gun_info_t *gun = NULL;
	for(int i = 0;i<GUN_CNT;i++)
	{
		gun = &charger.gun[i];
		
//		if(gun->gunState == SysState_NONE)
//		{
//			continue;
//		}
		switch((uint8_t)gun->ChargerState)
		{
			case ChargerState_READY://northing
				
				break;
			case  ChargerState_STARTING://等待12V--6V或者9V
				if(gun->CP_STAT == CP_6V || gun->CP_STAT == CP_9V)
				{
					StartPwm();
					gun->ChargerState = ChargerState_STARTOK;
					gun->gunState = SysState_Ready;
				}
				else {
					gun->ChargerState = ChargerState_STOPING;
				}
				break;
			
			case ChargerState_STARTOK://等待9V---6V
				if(gun->CP_STAT == CP_6V)
				{
					RELAY_ON();
					gun->ChargerState = ChargerState_CHARGEING;
					ChargerStartOK(gun,CHARGER_RESULT_OK);
				}			
				else if(gun->CP_STAT == CP_12V || gun->CP_STAT == CP_ERROR)
				{
					gun->ChargerState = ChargerState_STOPING;
				}
				break;
			
			case ChargerState_CHARGEING://充电ing
				if(gun->CP_STAT == CP_9V)
				{
					RELAY_OFF();
					gun->ChargerState = ChargerState_FULL;
					gun->gunState = SysState_FULL;
				}
				if(gun->CP_STAT == CP_12V || gun->CP_STAT == CP_ERROR)
				{
					gun->ChargerState = ChargerState_STOPING;
				}
				if(charger.gun[0].faultState.BIT.fault_in_low_voltage == 1 || charger.gun[0].faultState.BIT.fault_in_over_voltage == 1)
				{
					RELAY_OFF();
					gun->ChargerState = ChargerState_FULL;
					gun->gunState = SysState_FULL;
				}
				break;
			
			case ChargerState_SUSPEND://挂起状态
				if(gun->CP_STAT == CP_6V)
				{
					RELAY_ON();
					gun->ChargerState = ChargerState_CHARGEING;
				}else if(gun->CP_STAT == CP_12V || gun->CP_STAT == CP_ERROR)
				{
					gun->ChargerState = ChargerState_STOPING;
				}
				break;
			case ChargerState_FULL://充满
				if(gun->CP_STAT == CP_12V || gun->CP_STAT == CP_ERROR)
				{
					gun->ChargerState = ChargerState_STOPING;
				}
				else if(charger.gun[0].faultState.BIT.fault_in_low_voltage == 1 || charger.gun[0].faultState.BIT.fault_in_over_voltage == 1)
				{
					;
				}
				else if(gun->CP_STAT == CP_6V)
				{
					RELAY_ON();
					gun->ChargerState = ChargerState_CHARGEING;
					gun->gunState = SysState_WORKING;
				}
				break;
			case ChargerState_STOPING:
				StopPwm();
				RELAY_OFF();
				gun->ChargerState = ChargerState_STOPOK;
				break;
			case ChargerState_STOPOK:
				ChargerStopOK(gun);
				gun->ChargerState = ChargerState_READY;
				break;
		}
	}
}


void StartCharger(int gun_id)
{
	if(gun_id > GUN_CNT)
	{
		return ;
	}
	
	gun_info_t *gun  = &charger.gun[gun_id-1];
	if(gun->faultState.fault != 0) //故障
	{
		return ;
	}
	if(gun->ChargerState == ChargerState_READY)
		gun->ChargerState = ChargerState_STARTING;
	//开启充电定时器
//	if(gun->openTimer != -1)
//	{
//		Del_Timer(gun->openTimer);
//	}
//	gun->openTimer = Set_Timer(OPENCHARGER_TIME,onOpenCharger_TimeOut,gun);
}

//0 表示所有枪头
void StopCharger(int gun_id,uint8_t stopReason)
{
	if(gun_id > GUN_CNT)
	{
		return ;
	}
	gun_info_t *gun  = NULL;
	if(gun_id == 0)
	{
		for(uint8_t i = 0;i<GUN_CNT;i++)
		{
			gun  = &charger.gun[i];
			if(gun->gunState != SysState_NONE)
			{
				gun->ChargerState = ChargerState_STOPING;
			}
		}
	}
	else
	{
		gun  = &charger.gun[gun_id-1];
		if(gun->gunState != SysState_NONE)
		{
			gun->ChargerState = ChargerState_STOPING;
		}
	}
}

void Charger_Init(void)
{
	Gpio_Init();
	
	CP_Check_Init();
	
	memset(&charger,0,sizeof(charger));
	charger.callback_complete = NULL;
	charger.callback_start = NULL;
	
	for(int i = 0;i<GUN_CNT;i++)
	{
		charger.gun[i].gun_id  = i+1;
		charger.gun[i].lastChargerTime = 0;
//		charger.gun[i].CP_STAT = CP_ERROR;
		charger.gun[i].CP_STAT = CP_12V;
		//charger.gun[i].CP_STAT_Pre = CP_ERROR;
		charger.gun[i].openTimer = -1;
		charger.gun[i].gunState = SysState_NONE;
		charger.gun[i].ChargerState = ChargerState_READY;
		charger.gun[i].faultState.fault = 0;
	}
}


