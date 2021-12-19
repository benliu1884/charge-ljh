/**
 * @file ui_duanma.c
 * @author zhoumin (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2021-12-19
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "charger.h"
#include "cmu.h"
#include "drv_ht1621.h"
#include "os_timer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    SYS_IDLE  = 0,  //待机
    SYS_WORK  = 1,  //工作
    SYS_CARD  = 2,  //刷卡
    SYS_FINSH = 3,  //充电完成
    SYS_FAULT = 4,  //故障
} GuiState;

typedef struct
{
    uint8_t        state;       //界面状态
    uint8_t        last_state;  //上一个界面状态
    timer_handle_t offtimer;    //关闭lcd定时器
} gui_t;

static gui_t _gui;

typedef enum
{
    LCD_CLEAR   = 0,  //清除
    LCD_DISPLAY = 1,  //显示
} LCD_Display_TypeDef;

#define Tele_Roll_Time 10
#define Chg_Roll_Time 0
#define Finsh_Time 30
#define Card_Tmo_Time 5
#define Card_Display_Time 5

typedef enum
{
    UI_Tele_Vol   = 0,  //电压
    UI_Tele_Cur   = 1,  //电流
    UI_Tele_Pow   = 2,  //功率
    UI_Tele_Elec  = 3,  //电量
    UI_Tele_Money = 4,  //费用
} TeleType;

typedef enum
{
    UI_Chg_Null   = 0,  //显示空
    UI_Chg_First  = 1,  //显示一格进度条
    UI_Chg_Second = 2,  //显示两格进度条
    UI_Chg_Third  = 3,  //显示三格进度条
} ChgType;

typedef enum
{
    UI_Card_None    = 0,  //无
    UI_Card_Success = 1,  //成功
    UI_Card_Fail    = 2,  //失败
    UI_Card_Invalid = 3,  //无效
} CardAuth;

/**
 * @brief 打开背光
 */
static void LcdTurnOnLed( void )
{
    Ht1621_LightOn();
}

/**
 * @brief 关闭背光
 */
static void LcdTurnOffLed( void )
{
    Ht1621_LightOff();
}

/**
 * @brief 点亮所有SEG
 */
void LcdAllOn( void )
{
    for ( char i = 0; i < MaxSegNum; i++ )
        Ht1621WriteSegVal( i, 0xFF );
}

/**
 * @brief 关闭所有SEG
 */
static void LcdAllOff( void )
{
    for ( char i = 0; i < MaxSegNum; i++ )
        Ht1621WriteSegVal( i, 0x00 );
}

/**
 * @brief 点亮对应位
 *
 * @param seg 需要点亮的SEG(0 ~ MaxSegNum-1)
 * @param segVal SEG对应的COM,低四位(0x01 0x02 0x04 0x08)
 * @param flag 点亮还是关闭(LCD_Display_TypeDef)
 */
static void LcdDisplay( uint8_t seg, uint8_t segVal, LCD_Display_TypeDef flag )
{
    uint8_t lcdBuf = 0;
    lcdBuf         = Ht1621ReadSegVal( seg );
    if ( LCD_DISPLAY == flag )
        lcdBuf |= segVal;
    else
        lcdBuf &= ~segVal;
    Ht1621WriteSegVal( seg, lcdBuf );
}

/**
 * @brief 显示后台连接图标，网络信号图标，蓝牙连接图标
 */
static void GuiDisplay_Header()
{
    LcdDisplay( 0, 0x02 | 0x04, LCD_DISPLAY );
}

/**
 * @brief 切换界面清除上一个界面显示
 */
static void GuiClear_All()
{
    if ( _gui.state == _gui.last_state ) {
        return;
    }
    LcdAllOff();
    GuiDisplay_Header();
}

const uint8_t reg_time_value[ 10 ] = {
    0xBF,  // 0
    0x06,  // 1
    0x6D,  // 2
    0x4F,  // 3
    0xC6,  // 4
    0xCB,  // 5
    0xEB,  // 6
    0x0E,  // 7
    0xEF,  // 8
    0xCF,  // 9
};
const uint8_t reg_time_addr[ 6 ][ 2 ] = {
    { 2, 3 },    // hour-high
    { 4, 5 },    // hour-low
    { 6, 7 },    // min-high
    { 8, 9 },    // min-low
    { 10, 11 },  // sec-high
    { 12, 13 }   // sec-low
};

const uint8_t reg_nubmer_addr[ 4 ][ 2 ] = {
    { 25, 26 },    // 千
    { 23, 24 },    // 百
    { 21, 22 },    // 十
    { 19, 20 },    // 个
};

const uint8_t reg_nubmer_value1[ 10 ] = {
    0x5F,  // 0 abcdef
    0x50,  // 1 bc
    0x3D,  // 2 abdeg
    0x79,  // 3 abcdg
    0x72,  // 4 bcfg
    0x6B,  // 5 acdfg
    0x6F,  // 6 acdefg
    0x51,  // 7 abc
    0x7F,  // 8 abcdefg
    0x7B,  // 9 abcdfg
};

const uint8_t reg_nubmer_value2[ 10 ] = {
    0xAF,  // 0 abcdef
    0xA0,  // 1 bc
    0x6D,  // 2 abdeg
    0xE9,  // 3 abcdg
    0xE2,  // 4 bcfg
    0xCB,  // 5 acdfg
    0xCF,  // 6 acdefg
    0xA1,  // 7 abc
    0xEF,  // 8 abcdefg
    0xEB,  // 9 abcdfg
};

static void LcdDisplay_TimeNmu( int position, int num )
{
    if ( num >= 10 ) {
        return;
    }
    LcdDisplay( reg_time_addr[ position ][ 1 ], 0x0F, LCD_CLEAR );
    LcdDisplay( reg_time_addr[ position ][ 0 ], 0x0F, LCD_CLEAR );
    LcdDisplay( reg_time_addr[ position ][ 1 ], reg_time_value[ num ] & 0x0F, LCD_DISPLAY );
    LcdDisplay( reg_time_addr[ position ][ 0 ], reg_time_value[ num ] >> 4, LCD_DISPLAY );
}

static void LcdDisplay_Number( int position, int num )
{
    if ( num >= 10 ) {
        return;
    }
    uint8_t *value = (uint8_t *)&reg_nubmer_value1[0];
    if (position == 3) {
        value = (uint8_t *)&reg_nubmer_value2[0];
    }

    LcdDisplay(reg_nubmer_addr[position][0], 0x0F, LCD_CLEAR);
    LcdDisplay(reg_nubmer_addr[position][1], 0x0F, LCD_CLEAR);
    LcdDisplay(reg_nubmer_addr[position][0], (value[num]>>4)&0x0F, LCD_DISPLAY);
    LcdDisplay(reg_nubmer_addr[position][1], value[num]&0x0F, LCD_DISPLAY);
}

static int get_high_number(int num)
{
    int tmp = num;

    while (num)
    {
        tmp = num % 10;
        num = num/10;
    }

    return tmp;
}

/**
 * @brief 显示数据 215.60
 *
 * @param num
 */
static void LcdDisplay_ChargeNumber( uint32_t num , int precision)
{
    int interger = num / precision;
    int decimals = num % precision;
    int temp = 0;

    LcdDisplay(21, 0x08, LCD_CLEAR);
    LcdDisplay(23, 0x08, LCD_CLEAR);
    LcdDisplay(25, 0x08, LCD_CLEAR);

    if (interger/1000) {
        LcdDisplay_Number(0, interger/1000);
        temp = interger%1000;
        LcdDisplay_Number(1, temp/100);
        temp = temp%100;
        LcdDisplay_Number(2, temp/10);
        temp = temp%10;
        LcdDisplay_Number(3, temp);
    } else if(interger/100) {
        LcdDisplay_Number(0, interger/100);
        temp = interger%100;
        LcdDisplay_Number(1, temp/10);
        temp = temp%10;
        LcdDisplay_Number(2, temp);
        LcdDisplay(21, 0x08, LCD_DISPLAY); //小数点
        LcdDisplay_Number(3, get_high_number(decimals));
    } else if(interger/10) {//25

    } else {

    }
}

/**
 * @brief 显示充电时间
 *
 * @param num
 */
static void LcdDisplay_ChargeTime( uint32_t charge_time )
{
    uint8_t hour = charge_time / 3600;
    uint8_t min  = charge_time % 3600 / 60;
    uint8_t sec  = charge_time % 60;

    // hour
    LcdDisplay_TimeNmu( 0, hour / 10 );
    LcdDisplay_TimeNmu( 1, hour % 10 );
    // min
    LcdDisplay_TimeNmu( 2, min / 10 );
    LcdDisplay_TimeNmu( 3, min % 10 );
    // sec
    LcdDisplay_TimeNmu( 4, sec / 10 );
    LcdDisplay_TimeNmu( 5, sec % 10 );

    LcdDisplay( 6, 0x01, LCD_DISPLAY );
    LcdDisplay( 10, 0x01, LCD_DISPLAY );
}

/**
 * @brief 显示充电进度条
 *
 * @param ChgType 要显示的进度
 */
static void GuiDisplay_ChgStatic( void )
{
    static ChgType type = UI_Chg_Null;

    switch ( type ) {
    case UI_Chg_Null:
        LcdDisplay( 30, 0xFF, LCD_CLEAR );
        LcdDisplay( 31, 0xFF, LCD_CLEAR );
        LcdDisplay( 30, 0x02, LCD_DISPLAY );
        type = UI_Chg_First;
        break;

    case UI_Chg_First:
        LcdDisplay( 30, 0xFF, LCD_CLEAR );
        LcdDisplay( 31, 0xFF, LCD_CLEAR );
        LcdDisplay( 30, 0x02, LCD_DISPLAY );
        LcdDisplay( 31, 0x02, LCD_DISPLAY );
        type = UI_Chg_Second;
        break;

    case UI_Chg_Second:
        LcdDisplay( 30, 0x01, LCD_CLEAR );
        LcdDisplay( 30, 0x02, LCD_DISPLAY );
        LcdDisplay( 31, 0x01 | 0x02, LCD_DISPLAY );
        type = UI_Chg_Third;
        break;

    case UI_Chg_Third:
        LcdDisplay( 30, 0x02 | 0x01, LCD_DISPLAY );
        LcdDisplay( 31, 0x01 | 0x02, LCD_DISPLAY );
        type = UI_Chg_Null;
        break;
    }
}

void LcdDisplay_ChargeInfo(void)
{
    static uint8_t show_type = 0;

    LcdDisplay( 16, 0x0F, LCD_CLEAR );
    LcdDisplay( 17, 0x0F, LCD_CLEAR );
    LcdDisplay( 18, 0x0F, LCD_CLEAR );

    switch (show_type)
    {
    case 0: //电压
        LcdDisplay( 16, 0x08, LCD_DISPLAY );
        LcdDisplay_ChargeNumber(2156,10);
        show_type = 1;
        break;
    case 1: //电流
        LcdDisplay( 17, 0x08, LCD_DISPLAY );
        //TODO
        LcdDisplay_ChargeNumber(102,1);
        show_type = 2;
        break;
    case 2://电量
        LcdDisplay( 16, 0x04, LCD_DISPLAY );
        LcdDisplay( 17, 0x04, LCD_DISPLAY );
        LcdDisplay( 18, 0x04, LCD_DISPLAY );
        //TODO
        LcdDisplay_ChargeNumber(1002,1);
        show_type = 0;
        break;
    default:
        show_type = 0;
        break;
    }
    
}

/**
 * @brief 工作界面显示
 */
static void GuiDisplay_Work( uint32_t tick )
{
    int ct = 0;
    if ( _gui.last_state != _gui.state ) {
        LcdDisplay( 0, 0x01, LCD_CLEAR );            //
        LcdDisplay( 27, 0x08, LCD_DISPLAY );         //已充
        LcdDisplay( 15, 0x04, LCD_DISPLAY );         //实时信息
        LcdDisplay( 14, 0x01 | 0x04, LCD_DISPLAY );  // T5
        LcdDisplay( 1, 0x04 | 0x2, LCD_DISPLAY );    // S5/S7
    }

    static uint32_t tick_1s = 0;
    static uint32_t tick_2s = 0;
    /* 充电实时信息 */
    if ( tick >= tick_1s + 500 ) {
        tick_1s = tick;
        //更新充电动画
        GuiDisplay_ChgStatic();
        //更新充电时间
        ct = ( tick - charger.gun[ 0 ].startTime ) / 1000;
        LcdDisplay_ChargeTime( ct );
    }
    if ( tick >= tick_2s + 2000 ) {
        tick_2s = tick;
        //更新充电实时数据
        LcdDisplay_ChargeInfo();
    }

}

/**
 * @brief 刷卡界面显示
 */
static void GuiDisplay_Card() {}

/**
 * @brief 待机界面
 */
static void GuiDisplay_Idle()
{
    if ( _gui.last_state == _gui.state ) {
        return;
    }
    LcdDisplay( 0, 0x01, LCD_DISPLAY );  // 刷卡提示
}

/**
 * @brief 充电完成界面显示
 */
static void GuiDisplay_Finsh() {}

/**
 * @brief 故障界面显示
 */
static void GuiDisplay_Fault() {}

#include "charger.h"
void ui_display_loop( uint32_t tick )
{
    if ( charger.gun[ 0 ].faultState.fault ) {
        _gui.state = SYS_FAULT;
    } else if ( charger.gun[ 0 ].gunState == SysState_WORKING ) {
        _gui.state = SYS_WORK;
    } else if ( charger.gun[ 0 ].gunState == SysState_Finish ) {
        _gui.state = SYS_FINSH;
    } else if ( charger.gun[ 0 ].gunState == SysState_NONE ) {
        _gui.state = SYS_IDLE;
    }

    GuiClear_All();
    switch ( _gui.state ) {
    case SYS_IDLE:
        GuiDisplay_Idle();
        break;
    case SYS_CARD:
        GuiDisplay_Card();
        break;
    case SYS_WORK:
        GuiDisplay_Work( tick );
        break;
    case SYS_FINSH:
        GuiDisplay_Finsh();
        break;
    case SYS_FAULT:
        GuiDisplay_Fault();
        break;
    }

    _gui.last_state = _gui.state;
}

static void lcd_off_timeout( void* arg )
{
    LcdTurnOffLed();
}

void lcd_poweron_light( int time )
{
    if ( _gui.offtimer != -1 ) {
        Del_Timer( _gui.offtimer );
    }
    LcdTurnOnLed();
    _gui.offtimer = Set_Timer( time, lcd_off_timeout, NULL );
}

void ui_init_duanma( void )
{
    memset( &_gui, 0, sizeof( _gui ) );
    _gui.state      = SYS_IDLE;
    _gui.last_state = 0xFF;
    _gui.offtimer   = -1;
    Ht1621Init();
    lcd_poweron_light( 60 );
}
