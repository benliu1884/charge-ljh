/*
*********************************************************************************************************
*                                              主任务
*
* File         :
* By           :
* Version      :
* Description  : 系统主任务处理
*********************************************************************************************************
*/
#include "charger.h"
#include "cmu.h"
#include "delay.h"
#include "emu.h"
#include "gcard.h"
#include "includes.h"
#include "sim_pwm.h"
#include "uart.h"

static CP_TypeDef GetCPState( void );
static void       CheckCP( void );
static void       FaultHandle( uint32_t currTime );
static void       ReadMeter( void );

/**
 *日志缓存区
 */
uint8_t  gPrintBuff[ 128 ];
uint16_t gWrite = 0;

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar( int ch )
#else
#define PUTCHAR_PROTOTYPE int fputc( int ch, FILE* f )
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE
{
    if ( gWrite < sizeof( gPrintBuff ) ) {
        gPrintBuff[ gWrite++ ] = ch;
        if ( '\n' == ch ) {
            Uart_Send( &gUart0Handle, ( char* )gPrintBuff, gWrite );
            gWrite = 0;
        }
    } else {
        Uart_Send( &gUart0Handle, ( char* )gPrintBuff, sizeof( gPrintBuff ) );
        gWrite = 0;
    }
    return ch;
}

float GetCpuTemp( void )
{
    int values;

    uint16_t temp = HT_TBS_ValueRead( TBS_TMP );

    if ( temp >= 0x8000 ) {
        values = temp - 65536;
    } else {
        values = temp;
    }

    return ( float )( 12.9852f - 0.0028f * values );
}

CP_TypeDef GetCPState( void )
{
    CP_TypeDef cp = CP_ERROR;

    int values;

    uint16_t temp = HT_TBS_ValueRead( TBS_ADC0 );

    if ( temp >= 0x8000 ) {
        values = temp - 65536;
    } else {
        values = temp;
    }

    values = ( int )( 0.01285 * values + 421 );  // mv

    // cp电压判断
    // 12V----640mv /520mv
    // 9V------434mv/337mv
    // 6V-----215mv/143mv

    if ( values >= 550 ) {
        cp = CP_12V;
    } else if ( values > 240 ) {
        cp = CP_9V;
    } else if ( values > 120 ) {
        cp = CP_6V;
    } else {
        cp = CP_ERROR;
    }

    return cp;
}

void CheckCP( void )
{
    //获取cp电压
    CP_TypeDef curState = GetCPState();

    if ( curState != charger.gun[ 0 ].CP_STAT ) {
        //枪头连接
        if ( curState == CP_9V || curState == CP_6V ) {
            // kong-20210916-modiff
            LED_ON( LED1 );
            LED_OFF( LED2 | LED4 );
        } else if ( curState == CP_12V )  //拔枪
        {
            LED_OFF( LED2 );

            //过流恢复
            //			if(charger.gun[0].faultState.BIT.fault_out_over_current == 1)
            //			{
            //				charger.gun[0].faultState.BIT.fault_out_over_current = 0;
            //				charger.gun[0].faultState.BIT.fault_out_abnormal_current = 0;
            //			}
            // kong-20210916-modiff 漏电不恢复，只能掉电重启才能重新启动充电
            //漏电恢复
            //			if(charger.gun[0].faultState.BIT.fault_leakage == 1)
            //			{
            //				charger.gun[0].faultState.BIT.fault_leakage = 0;
            //			}
        }
        // cp 12V->9V/6V 插枪开始充电
        // if ( curState == CP_9V || curState == CP_6V ) {
            // StartCharger( 1 , NULL);
        // }
        charger.gun[ 0 ].CP_STAT = curState;
    }
    if (curState != CP_6V || curState != CP_9V) {
        charger.gun[0].startFlag = 0;
    }
}



//读取电表数据
void ReadMeter( void )
{
    static uint32_t tick_meter = 0;

    if ( OSTimeGet() - tick_meter >= 800 ) {
        tick_meter = OSTimeGet();

        //计量校验
        EMU_Proc();

        charger.gun[ 0 ].meter.voltage_an = ReadRMSU();
        charger.gun[ 0 ].meter.power      = ReadPower( 1 );
        charger.gun[ 0 ].meter.current_an = ReadRMSI( 1 );
        charger.gun[ 0 ].meter.electricity = ReadEnergy();
        charger.gun[ 0 ].meter.updateTime = OSTimeGet();

        // TODO zhoumin - 测试
        #if 1
        charger.gun[ 0 ].meter.voltage_an = 2192;
        charger.gun[ 0 ].meter.current_an = 12860;
        charger.gun[ 0 ].meter.electricity = ReadEnergy();
        #endif
    }
}

void FaultHandle( uint32_t currTime )
{
    static uint32_t tick_temp   = 0;
    static uint32_t tick_charge = 0;
    //获取cp电压;
    CP_TypeDef current_cp_state = GetCPState();

    if ( currTime - tick_temp >= 4000 ) {
        tick_temp = currTime;

        //过温处理
        float temp = GetCpuTemp();

        if ( temp > 65 ) {
            charger.gun[ 0 ].faultState.BIT.fault_pile_over_temp = 1;
        }
    }

    if ( charger.gun[ 0 ].meter.isVaild == 1 ) {
        charger.gun[ 0 ].faultState.BIT.fault_meter = 0;
        //过压
        if ( charger.gun[ 0 ].meter.voltage_an >= CRITIAL_OVER_VOL ) {
            if ( OSTimeGet() - charger.gun[ 0 ].overVolTime > 5000 ) {
                charger.gun[ 0 ].faultState.BIT.fault_in_over_voltage = 1;
                charger.gun[ 0 ].volBackTime                          = currTime + 3 * 1000;
            }
        } else {
            charger.gun[ 0 ].overVolTime = currTime;
        }

        //欠压
        if ( charger.gun[ 0 ].meter.voltage_an <= CRITIAL_LOW_VOL ) {
            if ( currTime - charger.gun[ 0 ].lowVolTime > 5000 ) {
                charger.gun[ 0 ].faultState.BIT.fault_in_low_voltage = 1;
                charger.gun[ 0 ].volBackTime                         = currTime + 3 * 1000;
            }
        } else {
            charger.gun[ 0 ].lowVolTime = currTime;
        }

        //过流
        if ( charger.gun[ 0 ].meter.current_an >= CRITIAL_OVER_CUUR ) {
            if ( currTime - charger.gun[ 0 ].overCuurTime > 2000 ) {
                charger.gun[ 0 ].faultState.BIT.fault_out_over_current = 1;
                if ( charger.gun[ 0 ].meter.current_an >= CRITIAL_OVER_ABNORMAL_CUUR )
                    charger.gun[ 0 ].faultState.BIT.fault_out_abnormal_current = 1;
            }
        } else {
            charger.gun[ 0 ].overCuurTime = currTime;
        }

        //电压恢复
        if ( charger.gun[ 0 ].meter.voltage_an > CRITIAL_LOW_VOL && charger.gun[ 0 ].meter.voltage_an < CRITIAL_OVER_VOL && currTime > charger.gun[ 0 ].volBackTime ) {
            charger.gun[ 0 ].faultState.BIT.fault_in_low_voltage  = 0;
            charger.gun[ 0 ].faultState.BIT.fault_in_over_voltage = 0;
            if ( (current_cp_state == CP_9V || current_cp_state == CP_6V) && charger.gun[0].startFlag) {
                StartCharger( 1 , NULL);
            }
        }
    } else {
        charger.gun[ 0 ].faultState.BIT.fault_meter = 1;
    }
    //	//接地 去掉接地检测
    //	if(READ_EARTH() == 0)
    //	{
    //		charger.gun[0].faultState.BIT.fault_pe_break = 0;
    //	}
    //	else
    //	{
    //		charger.gun[0].faultState.BIT.fault_pe_break = 1;
    //	}
    //有故障
    if ( charger.gun[ 0 ].faultState.fault != 0 ) {
        //		if(charger.gun[0].gunState != SysState_NONE)
        //		{
        //			StopCharger(1,charger.gun[0].faultState.fault);
        //		}

        /*******故障指示灯*******/
        //过热保护
        if ( charger.gun[ 0 ].faultState.BIT.fault_pile_over_temp == 1 ) {
            // printf( "over temp!LED_ON(LED4|LED3|LED2)...\r\n" );  // kong-debug-20210916
            LED_ON( LED4 | LED3 | LED2 );
            StopCharger( 1, charger.gun[ 0 ].faultState.fault );
        }
        //漏电
        else if ( charger.gun[ 0 ].faultState.BIT.fault_leakage == 1 ) {
            StopCharger( 1, charger.gun[ 0 ].faultState.fault );
            //每个周期灯闪烁4次
            for ( int i = 0; i < 4; i++ ) {
                LED_OFF( LED2 | LED3 | LED4 );
                Delay_mSec( 800 );
                LED_ON( LED1 | LED2 );
                Delay_mSec( 800 );
            }
            LED_OFF( LED2 | LED3 | LED4 );

            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 1000 );
        }
        //过载保护
        else if ( charger.gun[ 0 ].faultState.BIT.fault_out_abnormal_current == 1 ) {
            StopCharger( 1, charger.gun[ 0 ].faultState.fault );
            //每个周期灯闪烁2次
            for ( int i = 0; i < 2; i++ ) {
                Feed_WDT();
                LED_OFF( LED2 | LED3 | LED4 );
                Delay_mSec( 800 );
                LED_ON( LED1 | LED2 );
                Delay_mSec( 800 );
            }
            LED_OFF( LED2 | LED3 | LED4 );
            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 1000 );
        } else if ( charger.gun[ 0 ].faultState.BIT.fault_out_over_current == 1 ) {
            StopCharger( 1, charger.gun[ 0 ].faultState.fault );
            //每个周期灯闪烁2次
            for ( int i = 0; i < 2; i++ ) {
                Feed_WDT();
                LED_OFF( LED2 | LED3 | LED4 );
                Delay_mSec( 800 );
                LED_ON( LED1 | LED2 );
                Delay_mSec( 800 );
            }
            LED_OFF( LED2 | LED3 | LED4 );
            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 1000 );
        }
        //过压、欠压保护
        else if ( charger.gun[ 0 ].faultState.BIT.fault_in_low_voltage == 1 || charger.gun[ 0 ].faultState.BIT.fault_in_over_voltage == 1 ) {
            StopCharger( 1, charger.gun[ 0 ].faultState.fault );
            //每个周期灯闪烁1次
            LED_OFF( LED2 | LED3 | LED4 );
            Delay_mSec( 1200 );
            LED_ON( LED1 | LED2 );
            Delay_mSec( 1200 );
            LED_OFF( LED2 | LED3 | LED4 );
            Feed_WDT();
            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 1000 );
        } else  //其他故障
        {
            StopCharger( 1, charger.gun[ 0 ].faultState.fault );
            //每个周期灯闪烁5次
            for ( int i = 0; i < 5; i++ ) {
                Feed_WDT();
                LED_OFF( LED2 | LED3 | LED4 );
                Delay_mSec( 800 );
                LED_ON( LED1 | LED2 );
                Delay_mSec( 800 );
            }
            LED_OFF( LED2 | LED3 | LED4 );
            Feed_WDT();
            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 2000 );
            Feed_WDT();
            Delay_mSec( 1000 );
        }
    } else {
        if ( charger.gun[ 0 ].gunState == SysState_Finish )  //正常充电完成
        {
            LED_ON( LED1 );
            LED_OFF( LED2 | LED3 | LED4 );
            // printf( "SysState_Finish: LED_OFF(LED2|LED3|LED4)...\r\n" );
            if ( charger.gun[ 0 ].CP_STAT == CP_12V )  //已经拔枪
            {
                LED_OFF( LED2 );
                charger.gun[ 0 ].gunState = SysState_NONE;
            } else if ( charger.gun[ 0 ].CP_STAT == CP_ERROR ) {
                //每个周期灯闪烁3次
                for ( int i = 0; i < 3; i++ ) {
                    Feed_WDT();
                    LED_OFF( LED2 | LED3 | LED4 );
                    Delay_mSec( 800 );
                    LED_ON( LED1 | LED2 );
                    Delay_mSec( 800 );
                }
                LED_OFF( LED2 | LED3 | LED4 );
                Feed_WDT();
                Delay_mSec( 2000 );
                Feed_WDT();
                Delay_mSec( 2000 );
                Feed_WDT();
                Delay_mSec( 1000 );
            }
        } else if ( charger.gun[ 0 ].gunState == SysState_NONE )  //空闲状态
        {
            // printf( "SysState_NONE,LED_OFF(LED3|LED4)...\r\n" );
            LED_OFF( LED3 | LED4 );
            if (charger.gun[ 0 ].CP_STAT == CP_9V || charger.gun[ 0 ].CP_STAT == CP_6V) {
                charger.gun[ 0 ].gunState = SysState_Ready;
            }
        } else if ( charger.gun[ 0 ].gunState == SysState_WORKING )  //充电
        {
            // printf( "SysState_WORKING.\r\n" );
            if ( charger.gun[ 0 ].meter.current_an < 200 ) {
                LED_ON( LED3 );
            } else if ( currTime - tick_charge >= 1000 ) {
                tick_charge = currTime;
                LED_Toggle( LED3 );
            }
            if ( charger.gun[ 0 ].faultState.fault != 0 ) {
                LED_OFF( LED3 | LED4 );
            } else {
                LED_OFF( LED2 | LED3 );
                Delay_mSec( 1200 );
                // printf( "LED_ON(LED1|LED3|LED4)...\r\n" );
                LED_ON( LED1 | LED3 | LED4 );
                Delay_mSec( 1200 );
            }
        } else if ( charger.gun[ 0 ].gunState == SysState_FULL )  //充满
        {
            // printf( "charger.gun[0].gunState == SysState_FULL.\r\n" );
            if ( charger.gun[ 0 ].faultState.fault != 0 ) {
                // printf( "charger.gun[0].faultState.fault != 0,LED_OFF(LED3|LED4)...\r\n" );
                LED_OFF( LED3 | LED4 );
            } else {
                // printf( "LED_OFF(LED2|LED3|LED4)...\r\n" );
                LED_OFF( LED2 | LED3 | LED4 );
                LED_ON( LED1 );
            }
        } else if ( charger.gun[ 0 ].gunState == SysState_Ready )  //插枪
        {
            // printf( "charger.gun[0].gunState == SysState_Ready.\r\n" );
            if ( charger.gun[ 0 ].CP_STAT == CP_12V )  //已经拔枪
            {
                LED_OFF( LED2 );
                charger.gun[ 0 ].gunState = SysState_NONE;
            }
            if ( charger.gun[ 0 ].faultState.fault != 0 ) {
                // printf( "LED_OFF(LED3|LED4)...\r\n" );
                LED_OFF( LED3 | LED4 );
            } else {
                LED_OFF( LED2 | LED4 );
                // printf( "LED_ON(LED1|LED3)...\r\n" );
                LED_ON( LED1 | LED3 );
            }
        }
    }
}

#define DEVICE_SN_ADDR  0x1FFF0
uint8_t dev_sn[17] = {0};
extern void set_read_card_flag(int result);
void read_card(uint32_t tick)
{
    CardInfo card;
    static uint32_t last_tick = 0;
    static uint32_t last_read_card = 0;
    static uint8_t err_cnt = 0;
    int ret ;

    if (tick - last_tick < 700) {
        return ;
    }
    if (tick - last_read_card < 3000) {
        return;
    }
    last_tick = tick;

    if (charger.gun[ 0 ].faultState.fault && charger.gun[ 0 ].faultState.BIT.fault_cardreader != 1) {
        return ;
    }
    ret = card_read( &card );
    if (ret == CR_TIMEOUT) {
        if (err_cnt < 100)
            err_cnt++;
    } else {
        err_cnt = 0;
    }
    if ( ret == CR_SUCESS ) {
        // if (strcmp(dev_sn, card.userid) == 0)
        if (1)
        {
            printf( "read card success..%d\r\n" , charger.gun[ 0 ].gunState);
            last_read_card = tick;
            if ( charger.gun[ 0 ].gunState == SysState_WORKING ) {
                if (memcmp(card.serialid, charger.gun[ 0 ].card.serialid, 4) == 0) {
                    StopCharger( 1, CHARGER_RESULT_CARD );
                } else {
                    set_read_card_flag(3);
                }
            } else if ( charger.gun[ 0 ].gunState == SysState_Ready ) {
                StartCharger( 1, &card);
            } else {
                set_read_card_flag(3);
            }
        } else {
            set_read_card_flag(3);
        }
    } else if (ret == CR_INVALID || ret == CR_FRAME_ERROR) {
        set_read_card_flag(2);
    }

    if (err_cnt > 10) {
        charger.gun[0].faultState.BIT.fault_cardreader = 1;
    } else {
        charger.gun[0].faultState.BIT.fault_cardreader = 0;
    }
}

uint8_t charger_init = 0;

// char flag_cal = 0;//校准电表数据时用到
//主任务
void vMainTask( void* argc )
{
    uint32_t currTime = 0;
    uint32_t tick     = 0;
    charger_init      = 0;

    charger.gun[ 0 ].meter.isVaild = 1;
    //电源指示灯
    Delay_mSec( 500 );
    Feed_WDT();
    LED_ON( LED1 );
    HT_Flash_ByteRead(dev_sn, DEVICE_SN_ADDR, 16);

    printf( "..............system start..............\r\n" );
    printf("device sn=%s \n", dev_sn);

    while ( 1 ) {
        currTime = OSTimeGet();
        if ( currTime > 2000 ) {
            charger_init = 1;
        }
        //喂狗
        Feed_WDT();

        //读取计量数据
        ReadMeter();

        if ( currTime - tick >= 100 ) {
            tick = currTime;
            //故障处理
            FaultHandle( currTime );
        }

        //检测CP状态
        CheckCP();
        //充电处理
        ChargerTask();

        read_card(currTime);

        extern void ui_display_loop( uint32_t tick );
        ui_display_loop( tick );

        //		//电表校准测试函数
        //		if(flag_cal)
        //		{
        //			Calibration_meter();
        //		}
    }
}
