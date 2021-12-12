/*
*********************************************************************************************************
*                                              ������
*                                       
* File         : 
* By           : 
* Version      : 
* Description  : ϵͳ��������
*********************************************************************************************************
*/
#include "includes.h"
#include "cmu.h"
#include "emu.h"
#include "charger.h"
#include "uart.h"
#include "sim_pwm.h"
#include "delay.h"


static CP_TypeDef GetCPState(void);
static void CheckCP(void);
static void FaultHandle(uint32_t currTime);
static void ReadMeter(void);

/**
*��־������
*/
uint8_t  gPrintBuff[128];
uint16_t gWrite = 0;

#ifdef __GNUC__
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE
{
	if(gWrite < sizeof(gPrintBuff)) {
        gPrintBuff[gWrite++] = ch;
        if ('\n' == ch) {
			Uart_Send(&gUart0Handle,(char*)gPrintBuff,gWrite);
			gWrite = 0;
        }
    }else{
		Uart_Send(&gUart0Handle,(char*)gPrintBuff,sizeof(gPrintBuff));
		gWrite = 0;
    }
    return ch;
}

float GetCpuTemp(void)
{
	int values;
	
	uint16_t temp = HT_TBS_ValueRead(TBS_TMP);
	
    if(temp >= 0x8000){
        values = temp - 65536;
    }else{
        values = temp;
    }
    
    return  (float)(12.9852f-0.0028f*values);    
}


CP_TypeDef GetCPState(void)
{
	CP_TypeDef cp = CP_ERROR;
	
	int values;
	
	uint16_t temp = HT_TBS_ValueRead(TBS_ADC0);
	
	if(temp >= 0x8000){
        values = temp - 65536;
    }else{
        values = temp;
    }
	
	values = (int)(0.01285*values+421); //mv
	
	//cp��ѹ�ж�
	//12V----640mv /520mv	
	//9V------434mv/337mv
	//6V-----215mv/143mv

	if(values >= 550)
	{
		cp = CP_12V;
	}
	else if(values > 240)
	{
		cp = CP_9V;
	}
	else if(values > 120)
	{
		cp = CP_6V;
	}
	else
	{
		cp = CP_ERROR;
	}
	
	return cp;
}


void CheckCP(void)
{
	//��ȡcp��ѹ
	CP_TypeDef curState = GetCPState();
	
	if(curState != charger.gun[0].CP_STAT)
	{
		//ǹͷ����
		if(curState == CP_9V || curState == CP_6V)
		{
				//kong-20210916-modiff
				LED_ON(LED1);
				LED_OFF(LED2|LED4);
		}
		else if(curState == CP_12V)//��ǹ
		{
			LED_OFF(LED2);
			
			//�����ָ�
//			if(charger.gun[0].faultState.BIT.fault_out_over_current == 1)
//			{
//				charger.gun[0].faultState.BIT.fault_out_over_current = 0;
//				charger.gun[0].faultState.BIT.fault_out_abnormal_current = 0;
//			}
			//kong-20210916-modiff ©�粻�ָ���ֻ�ܵ����������������������
			//©��ָ�
//			if(charger.gun[0].faultState.BIT.fault_leakage == 1)
//			{
//				charger.gun[0].faultState.BIT.fault_leakage = 0;
//			}
		}
		
		//cp 12V->9V/6V  ��ǹ��ʼ���
		//if(charger.gun[0].CP_STAT == CP_12V && (curState == CP_9V || curState == CP_6V))
		if(curState == CP_9V || curState == CP_6V)
		{
			StartCharger(1);
		}
		
		charger.gun[0].CP_STAT = curState;
	}
}


//��ȡ�������
void ReadMeter(void)
{
	static uint32_t tick_meter = 0;
	
	if(OSTimeGet() - tick_meter >= 800)
	{
		tick_meter = OSTimeGet();
		
		//����У��
		EMU_Proc();
		
		charger.gun[0].meter.voltage_an = ReadRMSU();
		charger.gun[0].meter.power = ReadPower(1);
		charger.gun[0].meter.current_an = ReadRMSI(1);
		charger.gun[0].meter.updateTime = OSTimeGet();
	}
	//????--??2????30mA,????
//	static uint16_t Ims2;
//	Ims2 = ReadRMSI(2);
//	if(Ims2 > 30)
//	{
//		//charger.gun[0].faultState.BIT.fault_leakage = 1;
//		//????
//		//StopCharger(1,charger.gun[0].faultState.fault);
//	}
}



void FaultHandle(uint32_t currTime)
{
	static uint32_t tick_temp = 0;
	static uint32_t tick_charge = 0;
		//��ȡcp��ѹ
	CP_TypeDef current_cp_state = GetCPState();
	
	if(currTime - tick_temp >= 4000)
	{
		tick_temp = currTime;
		
		//���´���
		float temp = GetCpuTemp();
		
		if(temp > 65)
		{
			charger.gun[0].faultState.BIT.fault_pile_over_temp = 1;
		}
	}
	
	if(charger.gun[0].meter.isVaild == 1)
	{
		charger.gun[0].faultState.BIT.fault_meter = 0;
		//��ѹ
		if(charger.gun[0].meter.voltage_an >= CRITIAL_OVER_VOL)
		{
			if(OSTimeGet() - charger.gun[0].overVolTime > 5000)
			{
				charger.gun[0].faultState.BIT.fault_in_over_voltage = 1;
				charger.gun[0].volBackTime = currTime + 3*1000;
			}
		}
		else
		{
			charger.gun[0].overVolTime = currTime;
		}
		
		//Ƿѹ
		if(charger.gun[0].meter.voltage_an <= CRITIAL_LOW_VOL)
		{
			if(currTime - charger.gun[0].lowVolTime > 5000)
			{
				charger.gun[0].faultState.BIT.fault_in_low_voltage = 1;
				charger.gun[0].volBackTime = currTime + 3*1000;
			}
		}
		else
		{
			charger.gun[0].lowVolTime = currTime;
		}
		
		//����
		if(charger.gun[0].meter.current_an >= CRITIAL_OVER_CUUR)
		{
			if(currTime - charger.gun[0].overCuurTime > 2000)
			{
				charger.gun[0].faultState.BIT.fault_out_over_current = 1;
				if(charger.gun[0].meter.current_an >= CRITIAL_OVER_ABNORMAL_CUUR)
					charger.gun[0].faultState.BIT.fault_out_abnormal_current = 1;				
			}
		}
		else
		{
			charger.gun[0].overCuurTime = currTime;
		}
		
		//��ѹ�ָ�
		if(charger.gun[0].meter.voltage_an > CRITIAL_LOW_VOL && charger.gun[0].meter.voltage_an < CRITIAL_OVER_VOL && currTime > charger.gun[0].volBackTime)
		{
			charger.gun[0].faultState.BIT.fault_in_low_voltage = 0;
			charger.gun[0].faultState.BIT.fault_in_over_voltage = 0;
			if(current_cp_state == CP_9V || current_cp_state == CP_6V)
			{
				StartCharger(1);
			}
		}
	}
	else
	{
		charger.gun[0].faultState.BIT.fault_meter = 1;
	}
	
	
//	//�ӵ� ȥ���ӵؼ��
//	if(READ_EARTH() == 0)
//	{
//		charger.gun[0].faultState.BIT.fault_pe_break = 0;
//	}
//	else
//	{
//		charger.gun[0].faultState.BIT.fault_pe_break = 1;
//	}
	
	//�й���
	if(charger.gun[0].faultState.fault != 0)
	{
//		if(charger.gun[0].gunState != SysState_NONE)
//		{
//			StopCharger(1,charger.gun[0].faultState.fault);
//		}
		
		/*******����ָʾ��*******/
		//���ȱ���
		if(charger.gun[0].faultState.BIT.fault_pile_over_temp == 1)
		{
			printf("over temp!LED_ON(LED4|LED3|LED2)...\r\n");//kong-debug-20210916
			LED_ON(LED4|LED3|LED2);
			StopCharger(1,charger.gun[0].faultState.fault);
		}
		//©��
		else if(charger.gun[0].faultState.BIT.fault_leakage == 1)
		{
			StopCharger(1,charger.gun[0].faultState.fault);
			//ÿ�����ڵ���˸4��
			for(int i = 0;i<4;i++)
			{
				LED_OFF(LED2|LED3|LED4);
				Delay_mSec(800);
				LED_ON(LED1|LED2);
				Delay_mSec(800);
			}
			LED_OFF(LED2|LED3|LED4);
			
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(1000);
		}
		//���ر���
		else if(charger.gun[0].faultState.BIT.fault_out_abnormal_current == 1)
		{
			StopCharger(1,charger.gun[0].faultState.fault);
			//ÿ�����ڵ���˸2��
			for(int i = 0;i < 2;i++)
			{
				Feed_WDT();
				LED_OFF(LED2|LED3|LED4);
				Delay_mSec(800);
				LED_ON(LED1|LED2);
				Delay_mSec(800);
			}
			LED_OFF(LED2|LED3|LED4);
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(1000);
		}		
		else if(charger.gun[0].faultState.BIT.fault_out_over_current == 1)
		{
			StopCharger(1,charger.gun[0].faultState.fault);
			//ÿ�����ڵ���˸2��
			for(int i = 0;i < 2;i++)
			{
				Feed_WDT();
				LED_OFF(LED2|LED3|LED4);
				Delay_mSec(800);
				LED_ON(LED1|LED2);
				Delay_mSec(800);
			}
			LED_OFF(LED2|LED3|LED4);
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(1000);
		}
		//��ѹ��Ƿѹ����
		else if(charger.gun[0].faultState.BIT.fault_in_low_voltage == 1 || charger.gun[0].faultState.BIT.fault_in_over_voltage == 1)
		{
			StopCharger(1,charger.gun[0].faultState.fault);
			//ÿ�����ڵ���˸1��
			LED_OFF(LED2|LED3|LED4);
			Delay_mSec(1200);
			LED_ON(LED1|LED2);
			Delay_mSec(1200);
			LED_OFF(LED2|LED3|LED4);
			Feed_WDT();
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(1000);
		}		
		else//��������
		{
			StopCharger(1,charger.gun[0].faultState.fault);
			//ÿ�����ڵ���˸5��
			for(int i=0;i<5;i++)
			{
				Feed_WDT();
				LED_OFF(LED2|LED3|LED4);
				Delay_mSec(800);
				LED_ON(LED1|LED2);
				Delay_mSec(800);		
			}
			LED_OFF(LED2|LED3|LED4);
			Feed_WDT();
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(2000);
			Feed_WDT();
			Delay_mSec(1000);
		}
	}
	else
	{
		if(charger.gun[0].gunState == SysState_Finish)	//����������
		{
			LED_ON(LED1);
			LED_OFF(LED2|LED3|LED4);
			printf("SysState_Finish: LED_OFF(LED2|LED3|LED4)...\r\n");
			if(charger.gun[0].CP_STAT == CP_12V)//�Ѿ���ǹ
			{
				LED_OFF(LED2);
				charger.gun[0].gunState = SysState_NONE;
			}
			else if(charger.gun[0].CP_STAT == CP_ERROR)
			{
				//ÿ�����ڵ���˸3��
				for(int i=0;i<3;i++)
				{
					Feed_WDT();
					LED_OFF(LED2|LED3|LED4);
					Delay_mSec(800);
					LED_ON(LED1|LED2);
					Delay_mSec(800);
				}
				LED_OFF(LED2|LED3|LED4);
				Feed_WDT();
				Delay_mSec(2000);
				Feed_WDT();
				Delay_mSec(2000);
				Feed_WDT();
				Delay_mSec(1000);
			}
		}
		else if(charger.gun[0].gunState == SysState_NONE)	//����״̬
		{
			printf("SysState_NONE,LED_OFF(LED3|LED4)...\r\n");
			LED_OFF(LED3|LED4);
		}
		else if(charger.gun[0].gunState == SysState_WORKING)//���
		{
			printf("SysState_WORKING.\r\n");
			if(charger.gun[0].meter.current_an < 200)
			{
				printf("charger.gun[0].meter.current_an < 200,LED_ON(LED3)...\r\n");
				LED_ON(LED3);
			}
			else if(currTime - tick_charge >= 1000) 
			{
				tick_charge = currTime;
				printf("currTime - tick_charge >= 1000,LED_Toggle(LED3)...\r\n");
				LED_Toggle(LED3);
			}
			if(charger.gun[0].faultState.fault != 0)
			{
				printf("charger.gun[0].faultState.fault != 0,LED_OFF(LED3|LED4)...\r\n");
				LED_OFF(LED3|LED4);
			}
			else
			{
				printf("LED_OFF(LED2|LED3)...\r\n");
				LED_OFF(LED2|LED3);
				Delay_mSec(1200);
				printf("LED_ON(LED1|LED3|LED4)...\r\n");
				LED_ON(LED1|LED3|LED4);
				Delay_mSec(1200);
			}
		}
		else if(charger.gun[0].gunState == SysState_FULL) //����
		{
			printf("charger.gun[0].gunState == SysState_FULL.\r\n");
			if(charger.gun[0].faultState.fault != 0)
			{
				printf("charger.gun[0].faultState.fault != 0,LED_OFF(LED3|LED4)...\r\n");
				LED_OFF(LED3|LED4);
			}
			else
			{
				printf("LED_OFF(LED2|LED3|LED4)...\r\n");
				LED_OFF(LED2|LED3|LED4);
				LED_ON(LED1);	
			}		
		}
		else if(charger.gun[0].gunState == SysState_Ready) //��ǹ
		{
			printf("charger.gun[0].gunState == SysState_Ready.\r\n");
			if(charger.gun[0].faultState.fault != 0)
			{
				printf("LED_OFF(LED3|LED4)...\r\n");
				LED_OFF(LED3|LED4);
			}
			else
			{
				LED_OFF(LED2|LED4);
				printf("LED_ON(LED1|LED3)...\r\n");
				LED_ON(LED1|LED3);
			}
		}
	}
}


uint8_t charger_init=0;

//char flag_cal = 0;//У׼�������ʱ�õ�
//������
void vMainTask(void* argc)
{	
	uint32_t currTime = 0;
	uint32_t tick = 0;
	charger_init = 0;
	//��Դָʾ��
	charger.gun[0].meter.isVaild = 1;
	Delay_mSec(500);
	Feed_WDT();
	LED_ON(LED1);
	//debug begin
	charger.gun[0].ChargerState = ChargerState_STOPING;
	charger.gun[0].gunState = SysState_NONE;
	//debug end
	while(1)
	{
		currTime = OSTimeGet();
		if(currTime > 2000){
			charger_init = 1;
		}
		//ι��
		Feed_WDT();
		
		//��ȡ��������
		ReadMeter();
		
		if(currTime - tick >= 100)
		{
			tick = currTime;
			//���ϴ���
			FaultHandle(currTime);
		}
		
		//���CP״̬
		CheckCP();
		//��紦��
		ChargerTask();
		
//		//���У׼���Ժ���
//		if(flag_cal)
//		{
//			Calibration_meter();
//		}
	}
}
