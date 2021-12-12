#ifndef __INCLUDES_H__
#define __INCLUDES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ht6xxx_lib.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int  uint32_t;
typedef unsigned long long uint64_t;

typedef unsigned int time_t;


#define FW_VERSION                     (6)
#define FW_VERSION_SUB                  0


#define SystemFsysClock                 19660800						//ϵͳʱ��

#define EnWr_WPREG()		            HT_CMU->WPREG = 0xA55A			//��ر�д�������ܣ��û�����д�����������ļĴ���
#define DisWr_WPREG()		            HT_CMU->WPREG = 0x0000			//����д�������ܣ��û���ֹд�����������ļĴ���

#define	Feed_WDT()			            HT_WDT->WDTCLR = 0xAA3E			//4s
#define	Feed_WDT16S()					HT_WDT->WDTCLR = 0xAAFF			//FeedWDT per 16s


#define Disable_Int()		__disable_irq()					//��ֹ�ж�
#define Enable_Int()		__enable_irq()					//ʹ���ж�


#define Goto_Sleep()		{SCB->SCR = 0x0004;__WFI();__NOP();}		//����Sleepģʽ
#define Goto_Hold()			{__WFI();__NOP();}							//����holdģʽ
#define Reset_CPU()			NVIC_SystemReset()


#define GUN_CNT		1


#define MAX_OUTPUT_8A 	0
#define MAX_OUTPUT_10A	1
#define MAX_OUTPUT_16A	2          
#define MAX_OUTPUT_32A	3           
#define MAX_OUTPUT_40A	4           
#define MAX_OUTPUT_63A	5           


#define MAX_OUTPUT_CUUR	MAX_OUTPUT_16A


#if MAX_OUTPUT_CUUR==MAX_OUTPUT_8A
  #define DUTY_CYCLE    130
  #define CRITIAL_OVER_CUUR      8800
#elif MAX_OUTPUT_CUUR==MAX_OUTPUT_10A
  #define DUTY_CYCLE    200
  #define CRITIAL_OVER_CUUR      11000
#elif MAX_OUTPUT_CUUR==MAX_OUTPUT_16A
	#define DUTY_CYCLE    		 274
//  #define CRITIAL_OVER_CUUR      17600
//  #define CRITIAL_OVER_CUUR      20000
	#define CRITIAL_OVER_CUUR      15840//��Сһ��
  #define CRITIAL_OVER_ABNORMAL_CUUR 24000
#elif MAX_OUTPUT_CUUR == MAX_OUTPUT_32A
	#define DUTY_CYCLE    533
  #define CRITIAL_OVER_CUUR      35200
#elif MAX_OUTPUT_CUUR==MAX_OUTPUT_40A
	#define DUTY_CYCLE    660
  #define CRITIAL_OVER_CUUR      44000
#elif MAX_OUTPUT_CUUR==MAX_OUTPUT_63A
	#define DUTY_CYCLE    890
  #define CRITIAL_OVER_CUUR      69300
#endif

/*�����220V*/
//#define CRITIAL_OVER_VOL         2660
//#define CRITIAL_LOW_VOL          1760
/*�����110V*/
//#define CRITIAL_OVER_VOL         1265
//#define CRITIAL_LOW_VOL          920

/*�����110V~220V*/
#define CRITIAL_OVER_VOL         2650
#define CRITIAL_LOW_VOL          920

#define setbit(x,y) x|=(1<<y)           //��X�ĵ�Yλ��1
#define clrbit(x,y) x&=~(1<<y)          //��X�ĵ�Yλ��0
#define revbit(x,y) x^=(1<<y)           //��X�ĵ�Yλ��ת


#define THIS_LINE                       __LINE__
#define CL_LINE                         THIS_LINE
#define CL_FUNC                         __func__
#define CL_OK                           0
#define CL_FAIL                         (-1)
#define CL_TRUE                         1
#define CL_FALSE                        0


#define LOG(fmt,args...) do {    \
    printf("%s (%d) "fmt,__func__, __LINE__, ##args); \
}while(0)


#endif


