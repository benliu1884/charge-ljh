#include "charger.h"
#include "cmu.h"
#include "delay.h"
#include "emu.h"
#include "includes.h"
#include "os_timer.h"
#include "pmu.h"
#include "sim_pwm.h"
#include "uart.h"

extern void vMainTask( void* argc );

uint32_t RstsrTmp;  //��λ��־

static void Init_TaskSystem( void )
{
    PMU_Init();  //��Դ��ʼ��
    Delay_mSec( 500 );
    SystemClock_Config();  //ʱ�ӳ�ʼ��
}

extern void ui_init_duanma(void);

static void BSP_Init()
{
    //	RTC_Init();
    OSTimer_Init();
    UART_init();
    EMU_Init();
    ui_init_duanma();
    SimPwm_Init();
}

static void Usr_Init()
{
    Charger_Init();
}

int main()
{
    RstsrTmp       = HT_PMU->RSTSTA;
    HT_PMU->RSTSTA = 0x0000;
    Feed_WDT();

    HT_CMU->WPREG = 0xA55A;
    HT_CMU->CLKCTRL0 &= ~0x0380;  //�ر�ͣ����
    HT_CMU->WPREG = 0x0;

    Init_TaskSystem();

    BSP_Init();

    Usr_Init();

    vMainTask( NULL );
}
