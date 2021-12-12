#ifndef __TIMER_MANAGER_H
#define __TIMER_MANAGER_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define	MAX_TIMER_CNT 	3     			//最大定时任务个数

#define TIMER_DURATION_1s		(1)

typedef int timer_handle_t;
typedef void (* timer_callback)(void *argv);


typedef enum{
  
  TIMER_OFF=0,
  TIMER_ON=1,
  
}TIMER_STATE;

typedef struct _timer_manage
{
    struct _timer_info
    {
        TIMER_STATE state;				//on or off
        int timeout;			//超时时间 单位
        int elapse;				//0~timeout
        timer_callback callback;
        void *argv;
    }timer_info[MAX_TIMER_CNT];
}_timer_manage_t;

void OSTimer_Init(void);
int Del_Timer(timer_handle_t handler);
timer_handle_t Set_Timer(int ticks,timer_callback back,void *argv);

#endif
