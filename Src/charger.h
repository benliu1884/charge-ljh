#ifndef __CHARGER_H__
#define __CHARGER_H__

#include "includes.h"
#include "os_timer.h"


#define OPENCHARGER_TIME      (TIMER_DURATION_1s*60)    //开启充电时长  9V---6V

#define CHARGER_RELAY         (1000*5)           //充电间隔延时--10s之内不能连续开启充电


#define READ_EARTH()		HT_GPIO_BitsRead(HT_GPIOA,GPIO_Pin_7)


enum
{
	LED1 = GPIO_Pin_8,
	LED2 = GPIO_Pin_9,
	LED3 = GPIO_Pin_10,
	LED4 = GPIO_Pin_11,
};


#define LED_ON(led) HT_GPIO_BitsSet(HT_GPIOD,led)
#define LED_OFF(led) HT_GPIO_BitsReset(HT_GPIOD,led)
#define LED_Toggle(led)	HT_GPIO_BitsToggle(HT_GPIOD,led)



typedef enum{
  ChargerState_READY=0,
  ChargerState_STARTING=1,
  ChargerState_STARTOK=2,
  ChargerState_CHARGEING = 3,
  ChargerState_SUSPEND=4,
  ChargerState_FULL = 5,
  ChargerState_STOPING=6,
  ChargerState_STOPOK=7,
}ChargerState_TypeDef;



//启动充电原因
typedef enum
{
  CHARGER_RESULT_OK = 0x00,			//成功
  CHARGER_RESULT_S2_TIMEOUT = 0x01,		//S2 开关动作超时
  CHARGER_RESULT_CPGUIDE_FAULT = 0x02,		//充电中控制导引故障
  CHARGER_RESULT_COMM_TIMEOUT = 0x03,		//充电控制器与计费控制单元通讯超时
  CHARGER_RESULT_EMERGENCY_BT = 0x04,		//急停按钮动作故障
  CHARGER_RESULT_DOOR_FAULT = 0x05,		//门禁故障
  CHARGER_RESULT_ASSERT_FAULT = 0x06,		//避雷器故障
  CHARGER_RESULT_SMKOE_FAULT = 0x07,		//烟感故障
  CHARGER_RESULT_AC_INPUT_ROUTE_FAULT = 0x08,	//交流输入断路器故障
  CHARGER_RESULT_AC_INPUT_FAULT = 0x09,		//交流输入故障（过压，欠压，缺相，过流）
  CHARGER_RESULT_PILE_TEMP_OVER = 0x0A,		//充电桩过温故障
  CHARGER_RESULT_JACK_TEMP_OVER = 0x0B,		//充电接口过温故障
  CHARGER_RESULT_FAULT_LOCK = 0x0C,       	//电子锁故障
  CHARGER_RESULT_CONTACTOR_FAULT_1 = 0x0D,	//接触器故障
  CHARGER_RESULT_CONTACTOR_FAULT_2 = 0x0E,	//接触器故障
  CHARGER_RESULT_AC_OUT_VOL_OVER = 0x0F,	//交流输出电压过压故障
  CHARGER_RESULT_AC_OUT_VOL_LOW = 0x10,		//交流输出电压欠压故障
  CHARGER_RESULT_AC_OUT_CURR_OVER = 0x11,	//交流输出电流过流故障
  CHARGER_RESULT_AC_OUT_SHORT = 0x12,		//交流输出短路故障
  CHARGER_RESULT_OTHER_FAULT = 0x13,		//充电桩其他故障
}CHARGER_RESULT;


//枪头状态
//enum
//{
//	GUN_NONE = 0,		//正常
//	GUN_OVERVOL ,		//过压
//	GUN_UNDERVOL,		//欠压
//	GUN_OVERLOAD,		//过载
//	GUN_LEAKAGE,		//漏电
//	GUN_OVERHEAT,		//过热
//	GUN_SCP,			//scp
//	GUN_COMMERROR,		//通信故障
//};


typedef enum{
  SysState_NONE=0,
  SysState_WORKING,   
  SysState_FULL,      
  SysState_SUSPEND ,
  SysState_Finish ,
  SysState_Ready,
}GunState_DefType;


typedef enum{
  CP_ERROR=0,
  CP_12V=1,
  CP_9V=2,
  CP_6V=3,
}CP_TypeDef;



typedef union{
	uint8_t fault;
	struct
	{
		uint8_t fault_pile_over_temp:1;
		uint8_t fault_in_over_voltage:1;
		uint8_t fault_in_low_voltage:1;
		uint8_t fault_out_over_current:1;
		
		uint8_t fault_pe_break:1;
		uint8_t fault_leakage:1;
		uint8_t fault_meter:1;
		uint8_t fault_out_abnormal_current:1;
	}BIT;
}FaultState;



//电表数据
typedef struct {
	uint8_t isVaild;			//是否有效
  	uint32_t updateTime;
	uint32_t electricity;
	uint16_t power;
	uint16_t voltage_an;
	uint16_t voltage_bn;
	uint16_t voltage_cn;
	uint16_t current_an;
	uint16_t current_bn;
	uint16_t current_cn;
}METER_VALUE;

//枪头信息
typedef struct{

	//枪头id
	uint8_t gun_id;

	//充电状态
	ChargerState_TypeDef ChargerState;
	
	//CP状态
	volatile CP_TypeDef CP_STAT;
	//volatile CP_TypeDef CP_STAT_Pre;

	//枪头状态
	volatile GunState_DefType gunState;

	//电表数据
	METER_VALUE meter;

	//过压时间
	uint32_t overVolTime ;
	
	//欠压时间
	uint32_t lowVolTime ;
	
	//过流时间
	uint32_t overCuurTime;
	
	//过流恢复时间
	uint32_t overCuurBackTime;

	//过压欠压恢复时间
	uint32_t volBackTime;

	//故障状态  0----正常 非0-故障
	FaultState faultState;

	//上一次充电时间---启动充电不能太频繁，
	uint32_t lastChargerTime;

	//开启充电倒计时
	timer_handle_t openTimer;

	//启动充电方式
	uint8_t chargerMethod;

	//停止原因
	uint8_t stopReason;
	
	//开启时间
	uint32_t startTime;

	//停止时间
	uint32_t stopTime;

	//开始电量
	uint32_t startElec;

	//结束电量
	uint32_t stopElec;
}gun_info_t;


typedef struct{

	//枪头信息
	gun_info_t gun[GUN_CNT];

	//充电启动完成回调
	void (*callback_start)(gun_info_t*,CHARGER_RESULT); //(result)

	//充电完成回调函数
	void (*callback_complete)(gun_info_t*);  
}charger_info_t;


void StartCharger(int gun_id);
void StopCharger(int gun_id,uint8_t stopReason);
void ChargerTask(void);
void Charger_Init(void);

extern charger_info_t charger;

#endif

