#include "lcd.h"
#include "includes.h"

#define Ht1621_CS_1() HT_GPIO_BitsSet( HT1621_CS_PORT, HT1621_CS_PIN )
#define Ht1621_CS_0() HT_GPIO_BitsReset( HT1621_CS_PORT, HT1621_CS_PIN )

#define Ht1621_WR_1() HT_GPIO_BitsSet( HT1621_WR_PORT, HT1621_WR_PIN )
#define Ht1621_WR_0() HT_GPIO_BitsReset( HT1621_WR_PORT, HT1621_WR_PIN )

#define Ht1621_RD_1() HT_GPIO_BitsSet( HT1621_RD_PORT, HT1621_RD_PIN )
#define Ht1621_RD_0() HT_GPIO_BitsReset( HT1621_RD_PORT, HT1621_RD_PIN )

#define Ht1621_DATA_1() HT_GPIO_BitsSet( HT1621_DATA_PORT, HT1621_DATA_PIN )
#define Ht1621_DATA_0() HT_GPIO_BitsReset( HT1621_DATA_PORT, HT1621_DATA_PIN )

#define Ht1621_GET_DATA() HT_GPIO_BitsRead( HT1621_DATA_PORT, HT1621_DATA_PIN )

#define HT1621_DATA_IN() HT1621_DATA_PORT->PTDIR &= ~( ( uint32_t )HT1621_DATA_PIN )
#define HT1621_DATA_OUT() HT1621_DATA_PORT->PTDIR |= ( uint32_t )HT1621_DATA_PIN


static void delay_nop( int cnt )
{
    for ( int i = 0; i < cnt; i++ ) {
        __NOP();
    }
}

void Ht1621LightOn( void ) {}

void Ht1621LightOff( void ) {}

/**
***Ht1621 写数据函数时序
***Data: 要发送的数据，cnt:要发送的数据位
**/
void Ht1621Wr_Data( uint8_t Data, uint8_t cnt )
{
    uint8_t i = 0;
    // rt_pin_mode( HT1621_DATA, PIN_MODE_OUTPUT );  // gpio_init(LCD_DATA_GPIO, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LCD_DATA_PIN);
    HT1621_DATA_OUT();

    for ( i = 0; i < cnt; i++ ) {
        Ht1621_WR_0();
        delay_nop( 10 );
        if ( ( Data & 0x80 ) == 0x80 ) {
            Ht1621_DATA_1();
        } else {
            Ht1621_DATA_0();
        }
        Ht1621_WR_1();
        delay_nop( 10 );
        Data <<= 1;
    }
}
/**
***Ht1621读某段所映射地址存储数据内容的时序(D0-D3)
***
**/
uint8_t Ht1621Rd_Data( void )
{
    uint8_t SegVal = 0;

    HT1621_DATA_IN();
    Ht1621_WR_1();
    __NOP();
    __NOP();
    __NOP();
    for ( uint8_t i = 0; i < 4; i++ ) {
        Ht1621_RD_0();
        if ( 1 == Ht1621_GET_DATA() ) {
            SegVal |= ( 0x08 >> i );
        } else {
            SegVal |= ( 0x0 >> i );
        }
        __NOP();
        __NOP();
        __NOP();
        Ht1621_RD_1();
        __NOP();
        __NOP();
        __NOP();
    }

    return SegVal;
}

/**
***Ht1621写指令函数时序
***
**/
void Ht1621WrCmd( uint8_t cmd )
{
    Ht1621_CS_0();
    __NOP();
    Ht1621Wr_Data( 0x80, 4 );  //写入命令标志100
    Ht1621Wr_Data( cmd, 8 );   //写入命令数据

    Ht1621_CS_1();
    __NOP();
}

/**
***Ht1621读取段的值时序
***
**/
uint8_t Ht1621ReadSegVal( uint8_t segAddr )
{
    uint8_t segVal = 0;

    Ht1621_CS_0();
    __NOP();
    __NOP();
    __NOP();
    Ht1621_RD_1();
    __NOP();
    Ht1621Wr_Data( 0xC0, 3 );  //写入命令标志110
    __NOP();
    Ht1621Wr_Data( segAddr << 2, 6 );  //写入地址(A5-A0)

    segVal = Ht1621Rd_Data();
    Ht1621_CS_1();
    __NOP();
    return segVal;
}

/**
***Ht1621写一个数据函数
***
**/
void Ht1621WriteSegVal( uint8_t segAddr, uint8_t val )
{
    Ht1621_CS_0();
    delay_nop( 10 );
    Ht1621Wr_Data( 0xa0, 3 );  //写入数据标志101
    delay_nop( 10 );
    Ht1621Wr_Data( segAddr << 2, 6 );  //写入地址数据(A5-A0)
    delay_nop( 10 );
    Ht1621Wr_Data( val << 4, 4 );  //写入数据的后4位
    delay_nop( 10 );
    Ht1621_CS_1();
    delay_nop( 10 );
}

/**
***Ht1621写全屏数据
***
**/
void Ht1621WriteAllSegVal( uint8_t* p, uint8_t cnt )
{
    uint8_t i = 0;

    Ht1621_CS_0();

    Ht1621Wr_Data( 0xa0, 3 );    //写入数据标志101
    Ht1621Wr_Data( 0 << 2, 6 );  //写入地址数据Seg0地址
    for ( i = 0; i < cnt; i++ ) {
        Ht1621Wr_Data( *p, 8 );  //写入数据
        p++;
    }

    Ht1621_CS_1();
    __NOP();
}

void LcdTurnOnLed( void )
{
    printf( "turn on led.\n" );
    Ht1621LightOn();
}

void LcdTurnOffLed( void )
{
    printf( "turn off led.\n" );
    Ht1621LightOff();
}

void LcdAllOn( void )
{
    uint8_t i = 0;

    for ( i = 0; i <= MaxSegNum; i++ ) {
        Ht1621WriteSegVal( i, 0xFF );
        //rt_thread_mdelay( 3 * 1000 );
    }
}

void LcdAllOff( void )
{
    uint8_t i = 0;

    for ( i = 0; i <= MaxSegNum; i++ ) {
        Ht1621WriteSegVal( i, 0x00 );
    }
}

void LcdDisplay( uint8_t seg, uint8_t segVal, LCD_Display_TypeDef displayFlg )
{
    uint8_t lcdBuf = 0;
    lcdBuf         = HT_LCD_Read( seg );
    if ( LCD_DISPLAY == displayFlg ) {
        lcdBuf |= segVal;
    } else {
        lcdBuf &= ~segVal;
    }
    // HT_LCD_Write(seg,lcdBuf);
}

//显示与后台连接标志
void LcdDisplayBackStageConnect( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 11, SEG11_BACKSTAGE_CONNECT, displayFlg );
}

//显示信号
void LcdDisplaySingnal( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 11, SEG11_SIGNAL, displayFlg );
}

//显示无信号
void LcdDisplayNoSingnal( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 11, SEG11_NOSIGNAL, displayFlg );
}

//显示蓝牙
void LcdDisplayBlutooth( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 22, SEG22_BLUTOOTH, displayFlg );
}

//显示充电时长
void LcdDisplayChargingTime( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 14, SEG14_CHARGING_TIME, displayFlg );
}

//显示时钟表盘
void LcdDisplayClockDial( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 12, SEG12_CLOCK_DIAL, displayFlg );
}

//显示数卡或扫码开启充电
void LcdDisplayChgMethod( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 11, SEG11_CHARGE_METHOD, displayFlg );
}

//显示卡余额
void LcdDisplayCardBalance( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 25, SEG25_CARD_BALANCE, displayFlg );
}

//显示实时信息
void LcdDisplayRealInformation( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 25, SEG25_REAL_INFORMATION, displayFlg );
}

//显示充电金额
void LcdDisplayChargeingMoney( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 25, SEG25_CHARING_MONEY, displayFlg );
}

//显示所有的边缘
void LcdDisplayAllEdge( void )
{
    LcdDisplayEdge( LCD_DISPLAY );
    LcdDisplayTab1Edge( LCD_DISPLAY );
    LcdDisplayTab2Edge( LCD_DISPLAY );
    LcdDisplayTab3Edge( LCD_DISPLAY );
}
//显示边缘
void LcdDisplayEdge( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 24, SEG24_EDGE, displayFlg );
}
//显示Tab1边缘
void LcdDisplayTab1Edge( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 24, SEG24_TAB1_EDGE, displayFlg );
}
//显示Tab2边缘
void LcdDisplayTab2Edge( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 24, SEG24_TAB2_EDGE, displayFlg );
}
//显示Tab3边缘
void LcdDisplayTab3Edge( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 24, SEG24_TAB3_EDGE, displayFlg );
}

//显示预充
void LcdDisplayPredictCharging( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 10, SEG10_PREDICT_CHARGING, displayFlg );
}

//显示已充
void LcdDisplayAlreadyCharging( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 10, SEG10_ALREADY_CHARGING, displayFlg );
}

//显示KW
void LcdDisplayKw( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 10, SEG10_KW, displayFlg );
}

//显示 .h
void LcdDisplayHour( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 25, SEG25_HOUR, displayFlg );
}

//显示元
void LcdDisplayYuan( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 2, SEG2_YUAN, displayFlg );
}

//显示A
void LcdDisplayA( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 10, SEG10_A, displayFlg );
}

//显示V
void LcdDisplayV( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 18, SEG18_V, displayFlg );
}

//显示无效
void LcdDisplayInvalid( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 0, SEG0_INVALID, displayFlg );
}

//显示故障
void LcdDisplayFault( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 0, SEG0_FAULT, displayFlg );
}

//显示成功
void LcdDisplaySuccess( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 0, SEG0_SUCCESS, displayFlg );
}

//显示失败
void LcdDisplayFailure( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 0, SEG0_FAILURE, displayFlg );
}

//显示进度条1
void LcdDisplayProgressBar1( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 1, SEG1_PROGRESS_BAR1, displayFlg );
}
//显示进度条2
void LcdDisplayProgressBar2( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 1, SEG1_PROGRESS_BAR2, displayFlg );
}
//显示进度条3
void LcdDisplayProgressBar3( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 1, SEG1_PROGRESS_BAR3, displayFlg );
}

//显示充电中
void LcdDisplayChargeing( LCD_Display_TypeDef displayFlg )
{
    LcdDisplay( 1, SEG1_CHARGING, displayFlg );
}

//清除相对应位置数字
void LcdClrData( DATA_POS_t pos )
{
    uint8_t segValue = 0;
    switch ( pos ) {
    case FIRST:
        HT_LCD->LCDBUF[ 13 ] = 0;
        segValue             = SEG12_1E | SEG12_1F | SEG12_1G;
        LcdDisplay( 12, segValue, LCD_CLEAR );
        break;

    case SECOND:
        HT_LCD->LCDBUF[ 15 ] = 0;
        segValue             = SEG14_2E | SEG14_2G | SEG14_2F;
        LcdDisplay( 14, segValue, LCD_CLEAR );
        break;

    case THIRD:
        HT_LCD->LCDBUF[ 17 ] = 0;
        segValue             = SEG16_3E | SEG16_3G | SEG16_3F;
        LcdDisplay( 16, segValue, LCD_CLEAR );
        break;

    case FOURTH:
        HT_LCD->LCDBUF[ 19 ] = 0;
        segValue             = SEG18_4E | SEG18_4G | SEG18_4F;
        LcdDisplay( 18, segValue, LCD_CLEAR );
        break;

    case FIFTH:
        HT_LCD->LCDBUF[ 21 ] = 0;
        segValue             = SEG20_5E | SEG20_5G | SEG20_5F;
        LcdDisplay( 20, segValue, LCD_CLEAR );
        break;

    case SIXTH:
        HT_LCD->LCDBUF[ 23 ] = 0;
        segValue             = SEG22_6E | SEG22_6G | SEG22_6F;
        LcdDisplay( 22, segValue, LCD_CLEAR );
        break;

    case SEVENTH:
        segValue = HT_LCD_Read( 9 );
        segValue = SEG8_7C | SEG8_7G | SEG8_7B;
        LcdDisplay( 8, segValue, LCD_CLEAR );
        segValue = 0;
        segValue = SEG9_7D | SEG9_7E | SEG9_7F | SEG9_7A;
        LcdDisplay( 9, segValue, LCD_CLEAR );
        break;

    case EIGHTH:
        segValue = HT_LCD_Read( 7 );
        segValue = SEG6_8C | SEG6_8G | SEG6_8B;
        LcdDisplay( 6, segValue, LCD_CLEAR );
        segValue = 0;
        segValue = SEG7_8D | SEG7_8E | SEG7_8F | SEG7_8A;
        LcdDisplay( 7, segValue, LCD_CLEAR );
        break;

    case NINTH:
        segValue = HT_LCD_Read( 5 );
        segValue = SEG4_9C | SEG4_9G | SEG4_9B;
        LcdDisplay( 4, segValue, LCD_CLEAR );
        segValue = 0;
        segValue = SEG5_9D | SEG5_9E | SEG5_9F | SEG5_9A;
        LcdDisplay( 5, segValue, LCD_CLEAR );
        break;

    case TEN:
        segValue = HT_LCD_Read( 3 );
        segValue = SEG2_10C | SEG2_10G | SEG2_10B;
        LcdDisplay( 2, segValue, LCD_CLEAR );
        segValue = 0;
        segValue = SEG3_10D | SEG3_10E | SEG3_10F | SEG3_10A;
        LcdDisplay( 3, segValue, LCD_CLEAR );
        break;
    default:
        break;
    }
}

//显示数字0	\A\B\C\D\E\F
void LcdDisplayZero( DATA_POS_t pos )
{
    uint8_t lcdBuf = 0;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        // set
        lcdBuf = HT_LCD_Read( 12 );
        lcdBuf |= SEG12_1E | SEG12_1F;
        HT_LCD->LCDBUF[ 12 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG13_1A | SEG13_1B | SEG13_1C | SEG13_1D;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;

    case SECOND:
        // set
        lcdBuf = HT_LCD_Read( 14 );
        lcdBuf |= SEG14_2E | SEG14_2F;
        HT_LCD->LCDBUF[ 14 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG15_2A | SEG15_2B | SEG15_2C | SEG15_2D;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;

    case THIRD:
        // set
        lcdBuf = HT_LCD_Read( 16 );
        lcdBuf |= SEG16_3E | SEG16_3F;
        HT_LCD->LCDBUF[ 16 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG17_3A | SEG17_3B | SEG17_3C | SEG17_3D;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;

    case FOURTH:
        // set
        lcdBuf = HT_LCD_Read( 18 );
        lcdBuf |= SEG18_4E | SEG18_4F;
        HT_LCD->LCDBUF[ 18 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG19_4A | SEG19_4B | SEG19_4C | SEG19_4D;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;

    case FIFTH:
        // set
        lcdBuf = HT_LCD_Read( 20 );
        lcdBuf |= SEG20_5E | SEG20_5F;
        HT_LCD->LCDBUF[ 20 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG21_5A | SEG21_5B | SEG21_5C | SEG21_5D;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;

    case SIXTH:
        // set
        lcdBuf = HT_LCD_Read( 22 );
        lcdBuf |= SEG22_6E | SEG22_6F;
        HT_LCD->LCDBUF[ 22 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG23_6A | SEG23_6B | SEG23_6C | SEG23_6D;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;

    case SEVENTH:
        // set
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7B | SEG8_7C;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG9_7A | SEG9_7D | SEG9_7E | SEG9_7F;
        HT_LCD->LCDBUF[ 9 ] = lcdBuf;
        break;

    case EIGHTH:
        // set
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8B | SEG6_8C;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG7_8A | SEG7_8D | SEG7_8E | SEG7_8F;
        HT_LCD->LCDBUF[ 7 ] = lcdBuf;
        break;

    case NINTH:
        // set
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9B | SEG4_9C;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG5_9A | SEG5_9D | SEG5_9E | SEG5_9F;
        HT_LCD->LCDBUF[ 5 ] = lcdBuf;
        break;

    case TEN:
        // set
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10B | SEG2_10C;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG3_10A | SEG3_10D | SEG3_10E | SEG3_10F;
        HT_LCD->LCDBUF[ 3 ] = lcdBuf;
        break;

    default:
        break;
    }
}

//显示数字1	\B\C
void LcdDisplayOne( DATA_POS_t pos )
{
    uint8_t lcdBuf = 0;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        lcdBuf               = SEG13_1B | SEG13_1C;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;
    case SECOND:
        lcdBuf               = SEG15_2B | SEG15_2C;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;
    case THIRD:
        lcdBuf               = SEG17_3B | SEG17_3C;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;
    case FOURTH:
        lcdBuf               = SEG19_4B | SEG19_4C;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;
    case FIFTH:
        lcdBuf               = SEG21_5B | SEG21_5C;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;
    case SIXTH:
        lcdBuf               = SEG23_6B | SEG23_6C;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;
    case SEVENTH:
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7B | SEG8_7C;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        break;
    case EIGHTH:
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8B | SEG6_8C;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        break;
    case NINTH:
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9B | SEG4_9C;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        break;
    case TEN:
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10B | SEG2_10C;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        break;

    default:
        break;
    }
}

//显示数字2	\A\B\D\E\G
void LcdDisplayTwo( DATA_POS_t pos )
{
    uint8_t lcdBuf;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        // set
        lcdBuf = HT_LCD_Read( 12 );
        lcdBuf |= SEG12_1E | SEG12_1G;
        HT_LCD->LCDBUF[ 12 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG13_1A | SEG13_1B | SEG13_1D;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;

    case SECOND:
        // set
        lcdBuf = HT_LCD_Read( 14 );
        lcdBuf |= SEG14_2E | SEG14_2G;
        HT_LCD->LCDBUF[ 14 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG15_2A | SEG15_2B | SEG15_2D;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;

    case THIRD:
        // set
        lcdBuf = HT_LCD_Read( 16 );
        lcdBuf |= SEG16_3E | SEG16_3G;
        HT_LCD->LCDBUF[ 16 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG17_3A | SEG17_3B | SEG17_3D;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;

    case FOURTH:
        // set
        lcdBuf = HT_LCD_Read( 18 );
        lcdBuf |= SEG18_4E | SEG18_4G;
        HT_LCD->LCDBUF[ 18 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG19_4A | SEG19_4B | SEG19_4D;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;

    case FIFTH:
        // set
        lcdBuf = HT_LCD_Read( 20 );
        lcdBuf |= SEG20_5E | SEG20_5G;
        HT_LCD->LCDBUF[ 20 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG21_5A | SEG21_5B | SEG21_5D;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;

    case SIXTH:
        // set
        lcdBuf = HT_LCD_Read( 22 );
        lcdBuf |= SEG22_6E | SEG22_6G;
        HT_LCD->LCDBUF[ 22 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG23_6A | SEG23_6B | SEG23_6D;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;

    case SEVENTH:
        // set
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7B | SEG8_7G;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG9_7A | SEG9_7D | SEG9_7E;
        HT_LCD->LCDBUF[ 9 ] = lcdBuf;
        break;

    case EIGHTH:
        // set
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8B | SEG6_8G;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG7_8A | SEG7_8D | SEG7_8E;
        HT_LCD->LCDBUF[ 7 ] = lcdBuf;
        break;

    case NINTH:
        // set
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9B | SEG4_9G;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG5_9A | SEG5_9D | SEG5_9E;
        HT_LCD->LCDBUF[ 5 ] = lcdBuf;
        break;

    case TEN:
        // set
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10B | SEG2_10G;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG3_10A | SEG3_10D | SEG3_10E;
        HT_LCD->LCDBUF[ 3 ] = lcdBuf;
        break;
    default:
        break;
    }
}

//显示数字3	\A\B\C\D\G
void LcdDisplayThree( DATA_POS_t pos )
{
    uint8_t lcdBuf;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        // set
        lcdBuf = HT_LCD_Read( 12 );
        lcdBuf |= SEG12_1G;
        HT_LCD->LCDBUF[ 12 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG13_1A | SEG13_1B | SEG13_1C | SEG13_1D;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;

    case SECOND:
        // set
        lcdBuf = HT_LCD_Read( 14 );
        lcdBuf |= SEG14_2G;
        HT_LCD->LCDBUF[ 14 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG15_2A | SEG15_2B | SEG15_2C | SEG15_2D;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;

    case THIRD:
        // set
        lcdBuf = HT_LCD_Read( 16 );
        lcdBuf |= SEG16_3G;
        HT_LCD->LCDBUF[ 16 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG17_3A | SEG17_3B | SEG17_3C | SEG17_3D;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;

    case FOURTH:
        // set
        lcdBuf = HT_LCD_Read( 18 );
        lcdBuf |= SEG18_4G;
        HT_LCD->LCDBUF[ 18 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG19_4A | SEG19_4B | SEG19_4C | SEG19_4D;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;

    case FIFTH:
        // set
        lcdBuf = HT_LCD_Read( 20 );
        lcdBuf |= SEG20_5G;
        HT_LCD->LCDBUF[ 20 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG21_5A | SEG21_5B | SEG21_5C | SEG21_5D;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;

    case SIXTH:
        // set
        lcdBuf = HT_LCD_Read( 22 );
        lcdBuf |= SEG22_6G;
        HT_LCD->LCDBUF[ 22 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG23_6A | SEG23_6B | SEG23_6C | SEG23_6D;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;

    case SEVENTH:
        // set
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7B | SEG8_7C | SEG8_7G;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG9_7A | SEG9_7D;
        HT_LCD->LCDBUF[ 9 ] = lcdBuf;
        break;

    case EIGHTH:
        // set
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8B | SEG6_8C | SEG6_8G;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG7_8A | SEG7_8D;
        HT_LCD->LCDBUF[ 7 ] = lcdBuf;
        break;

    case NINTH:
        // set
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9B | SEG4_9C | SEG4_9G;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG5_9A | SEG5_9D;
        HT_LCD->LCDBUF[ 5 ] = lcdBuf;
        break;

    case TEN:
        // set
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10B | SEG2_10C | SEG2_10G;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG3_10A | SEG3_10D;
        HT_LCD->LCDBUF[ 3 ] = lcdBuf;
        break;
    default:
        break;
    }
}

//显示数字4	\B\C\F\G
void LcdDisplayFour( DATA_POS_t pos )
{
    uint8_t lcdBuf;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        // set
        lcdBuf = HT_LCD_Read( 12 );
        lcdBuf |= SEG12_1G | SEG12_1F;
        HT_LCD->LCDBUF[ 12 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG13_1B | SEG13_1C;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;

    case SECOND:
        // set
        lcdBuf = HT_LCD_Read( 14 );
        lcdBuf |= SEG14_2G | SEG14_2F;
        HT_LCD->LCDBUF[ 14 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG15_2B | SEG15_2C;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;

    case THIRD:
        // set
        lcdBuf = HT_LCD_Read( 16 );
        lcdBuf |= SEG16_3G | SEG16_3F;
        HT_LCD->LCDBUF[ 16 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG17_3B | SEG17_3C;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;

    case FOURTH:
        // set
        lcdBuf = HT_LCD_Read( 18 );
        lcdBuf |= SEG18_4G | SEG18_4F;
        HT_LCD->LCDBUF[ 18 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG19_4B | SEG19_4C;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;

    case FIFTH:
        // set
        lcdBuf = HT_LCD_Read( 20 );
        lcdBuf |= SEG20_5G | SEG20_5F;
        HT_LCD->LCDBUF[ 20 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG21_5B | SEG21_5C;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;

    case SIXTH:
        // set
        lcdBuf = HT_LCD_Read( 22 );
        lcdBuf |= SEG22_6G | SEG22_6F;
        HT_LCD->LCDBUF[ 22 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG23_6B | SEG23_6C;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;

    case SEVENTH:
        // set
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7B | SEG8_7C | SEG8_7G;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG9_7F;
        HT_LCD->LCDBUF[ 9 ] = lcdBuf;
        break;

    case EIGHTH:
        // set
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8B | SEG6_8C | SEG6_8G;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG7_8F;
        HT_LCD->LCDBUF[ 7 ] = lcdBuf;
        break;

    case NINTH:
        // set
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9B | SEG4_9C | SEG4_9G;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG5_9F;
        HT_LCD->LCDBUF[ 5 ] = lcdBuf;
        break;

    case TEN:
        // set
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10B | SEG2_10C | SEG2_10G;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG3_10F;
        HT_LCD->LCDBUF[ 3 ] = lcdBuf;
        break;

    default:
        break;
    }
}

//显示数字5	\A\C\D\F\G
void LcdDisplayFive( DATA_POS_t pos )
{
    uint8_t lcdBuf;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        // set
        lcdBuf = HT_LCD_Read( 12 );
        lcdBuf |= SEG12_1G | SEG12_1F;
        HT_LCD->LCDBUF[ 12 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG13_1A | SEG13_1C | SEG13_1D;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;

    case SECOND:
        // set
        lcdBuf = HT_LCD_Read( 14 );
        lcdBuf |= SEG14_2G | SEG14_2F;
        HT_LCD->LCDBUF[ 14 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG15_2A | SEG15_2C | SEG15_2D;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;

    case THIRD:
        // set
        lcdBuf = HT_LCD_Read( 16 );
        lcdBuf |= SEG16_3G | SEG16_3F;
        HT_LCD->LCDBUF[ 16 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG17_3A | SEG17_3C | SEG17_3D;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;

    case FOURTH:
        // set
        lcdBuf = HT_LCD_Read( 18 );
        lcdBuf |= SEG18_4G | SEG18_4F;
        HT_LCD->LCDBUF[ 18 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG19_4A | SEG19_4C | SEG19_4D;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;

    case FIFTH:
        // set
        lcdBuf = HT_LCD_Read( 20 );
        lcdBuf |= SEG20_5G | SEG20_5F;
        HT_LCD->LCDBUF[ 20 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG21_5A | SEG21_5C | SEG21_5D;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;

    case SIXTH:
        // set
        lcdBuf = HT_LCD_Read( 22 );
        lcdBuf |= SEG22_6G | SEG22_6F;
        HT_LCD->LCDBUF[ 22 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG23_6A | SEG23_6C | SEG23_6D;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;

    case SEVENTH:
        // set
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7C | SEG8_7G;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG9_7A | SEG9_7D | SEG9_7F;
        HT_LCD->LCDBUF[ 9 ] = lcdBuf;
        break;

    case EIGHTH:
        // set
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8C | SEG6_8G;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG7_8A | SEG7_8D | SEG7_8F;
        HT_LCD->LCDBUF[ 7 ] = lcdBuf;
        break;

    case NINTH:
        // set
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9C | SEG4_9G;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG5_9A | SEG5_9D | SEG5_9F;
        HT_LCD->LCDBUF[ 5 ] = lcdBuf;
        break;

    case TEN:
        // set
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10C | SEG2_10G;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG3_10A | SEG3_10D | SEG3_10F;
        HT_LCD->LCDBUF[ 3 ] = lcdBuf;
        break;
    default:
        break;
    }
}

//显示数字6	\A\C\D\E\F\G
void LcdDisplaySix( DATA_POS_t pos )
{
    uint8_t lcdBuf;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        // set
        lcdBuf = HT_LCD_Read( 12 );
        lcdBuf |= SEG12_1E | SEG12_1F | SEG12_1G;
        HT_LCD->LCDBUF[ 12 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG13_1A | SEG13_1C | SEG13_1D;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;

    case SECOND:
        // set
        lcdBuf = HT_LCD_Read( 14 );
        lcdBuf |= SEG14_2E | SEG14_2F | SEG14_2G;
        HT_LCD->LCDBUF[ 14 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG15_2A | SEG15_2C | SEG15_2D;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;

    case THIRD:
        // set
        lcdBuf = HT_LCD_Read( 16 );
        lcdBuf |= SEG16_3E | SEG16_3F | SEG16_3G;
        HT_LCD->LCDBUF[ 16 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG17_3A | SEG17_3C | SEG17_3D;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;

    case FOURTH:
        // set
        lcdBuf = HT_LCD_Read( 18 );
        lcdBuf |= SEG18_4E | SEG18_4F | SEG18_4G;
        HT_LCD->LCDBUF[ 18 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG19_4A | SEG19_4C | SEG19_4D;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;

    case FIFTH:
        // set
        lcdBuf = HT_LCD_Read( 20 );
        lcdBuf |= SEG20_5E | SEG20_5F | SEG20_5G;
        HT_LCD->LCDBUF[ 20 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG21_5A | SEG21_5C | SEG21_5D;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;

    case SIXTH:
        // set
        lcdBuf = HT_LCD_Read( 22 );
        lcdBuf |= SEG22_6E | SEG22_6F | SEG22_6G;
        HT_LCD->LCDBUF[ 22 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG23_6A | SEG23_6C | SEG23_6D;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;

    case SEVENTH:
        // set
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7C | SEG8_7G;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG9_7A | SEG9_7D | SEG9_7E | SEG9_7F;
        HT_LCD->LCDBUF[ 9 ] = lcdBuf;
        break;

    case EIGHTH:
        // set
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8C | SEG6_8G;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG7_8A | SEG7_8D | SEG7_8E | SEG7_8F;
        HT_LCD->LCDBUF[ 7 ] = lcdBuf;
        break;

    case NINTH:
        // set
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9C | SEG4_9G;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG5_9A | SEG5_9D | SEG5_9E | SEG5_9F;
        HT_LCD->LCDBUF[ 5 ] = lcdBuf;
        break;

    case TEN:
        // set
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10C | SEG2_10G;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG3_10A | SEG3_10D | SEG3_10E | SEG3_10F;
        HT_LCD->LCDBUF[ 3 ] = lcdBuf;
        break;
    default:
        break;
    }
}

//显示数字7	\A\B\C
void LcdDisplaySeven( DATA_POS_t pos )
{
    uint8_t lcdBuf;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        // set
        lcdBuf = 0x0;
        lcdBuf |= SEG13_1A | SEG13_1B | SEG13_1C;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;

    case SECOND:
        // set
        lcdBuf = 0x0;
        lcdBuf |= SEG15_2A | SEG15_2B | SEG15_2C;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;

    case THIRD:
        // set
        lcdBuf = 0x0;
        lcdBuf |= SEG17_3A | SEG17_3B | SEG17_3C;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;

    case FOURTH:
        // set
        lcdBuf = 0x0;
        lcdBuf |= SEG19_4A | SEG19_4B | SEG19_4C;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;

    case FIFTH:
        // set
        lcdBuf = 0x0;
        lcdBuf |= SEG21_5A | SEG21_5B | SEG21_5C;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;

    case SIXTH:
        // set
        lcdBuf = 0x0;
        lcdBuf |= SEG23_6A | SEG23_6B | SEG23_6C;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;

    case SEVENTH:
        // set
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7B | SEG8_7C;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG9_7A;
        HT_LCD->LCDBUF[ 9 ] = lcdBuf;
        break;

    case EIGHTH:
        // set
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8B | SEG6_8C;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG7_8A;
        HT_LCD->LCDBUF[ 7 ] = lcdBuf;
        break;

    case NINTH:
        // set
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9B | SEG4_9C;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG5_9A;
        HT_LCD->LCDBUF[ 5 ] = lcdBuf;
        break;

    case TEN:
        // set
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10B | SEG2_10C;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG3_10A;
        HT_LCD->LCDBUF[ 3 ] = lcdBuf;
        break;
    default:
        break;
    }
}

//显示数字8	\A\B\C\D\E\F\G
void LcdDisplayEight( DATA_POS_t pos )
{
    uint8_t lcdBuf;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        // set
        lcdBuf = HT_LCD_Read( 12 );
        lcdBuf |= SEG12_1E | SEG12_1F | SEG12_1G;
        HT_LCD->LCDBUF[ 12 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG13_1A | SEG13_1B | SEG13_1C | SEG13_1D;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;

    case SECOND:
        // set
        lcdBuf = HT_LCD_Read( 14 );
        lcdBuf |= SEG14_2E | SEG14_2F | SEG14_2G;
        HT_LCD->LCDBUF[ 14 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG15_2A | SEG15_2B | SEG15_2C | SEG15_2D;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;

    case THIRD:
        // set
        lcdBuf = HT_LCD_Read( 16 );
        lcdBuf |= SEG16_3E | SEG16_3F | SEG16_3G;
        HT_LCD->LCDBUF[ 16 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG17_3A | SEG17_3B | SEG17_3C | SEG17_3D;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;

    case FOURTH:
        // set
        lcdBuf = HT_LCD_Read( 18 );
        lcdBuf |= SEG18_4E | SEG18_4F | SEG18_4G;
        HT_LCD->LCDBUF[ 18 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG19_4A | SEG19_4B | SEG19_4C | SEG19_4D;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;

    case FIFTH:
        // set
        lcdBuf = HT_LCD_Read( 20 );
        lcdBuf |= SEG20_5E | SEG20_5F | SEG20_5G;
        HT_LCD->LCDBUF[ 20 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG21_5A | SEG21_5B | SEG21_5C | SEG21_5D;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;

    case SIXTH:
        // set
        lcdBuf = HT_LCD_Read( 22 );
        lcdBuf |= SEG22_6E | SEG22_6F | SEG22_6G;
        HT_LCD->LCDBUF[ 22 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG23_6A | SEG23_6B | SEG23_6C | SEG23_6D;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;

    case SEVENTH:
        // set
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7B | SEG8_7C | SEG8_7G;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG9_7A | SEG9_7D | SEG9_7E | SEG9_7F;
        HT_LCD->LCDBUF[ 9 ] = lcdBuf;
        break;

    case EIGHTH:
        // set
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8B | SEG6_8C | SEG6_8G;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG7_8A | SEG7_8D | SEG7_8E | SEG7_8F;
        HT_LCD->LCDBUF[ 7 ] = lcdBuf;
        break;

    case NINTH:
        // set
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9B | SEG4_9C | SEG4_9G;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG5_9A | SEG5_9D | SEG5_9E | SEG5_9F;
        HT_LCD->LCDBUF[ 5 ] = lcdBuf;
        break;

    case TEN:
        // set
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10B | SEG2_10C | SEG2_10G;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG3_10A | SEG3_10D | SEG3_10E | SEG3_10F;
        HT_LCD->LCDBUF[ 3 ] = lcdBuf;
        break;
    default:
        break;
    }
}
//显示数字9 \A\B\C\D\F\G
void LcdDisplayNine( DATA_POS_t pos )
{
    uint8_t lcdBuf;
    LcdClrData( pos );
    switch ( pos ) {
    case FIRST:
        // set
        lcdBuf = HT_LCD_Read( 12 );
        lcdBuf |= SEG12_1F | SEG12_1G;
        HT_LCD->LCDBUF[ 12 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG13_1A | SEG13_1B | SEG13_1C | SEG13_1D;
        HT_LCD->LCDBUF[ 13 ] = lcdBuf;
        break;

    case SECOND:
        // set
        lcdBuf = HT_LCD_Read( 14 );
        lcdBuf |= SEG14_2F | SEG14_2G;
        HT_LCD->LCDBUF[ 14 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG15_2A | SEG15_2B | SEG15_2C | SEG15_2D;
        HT_LCD->LCDBUF[ 15 ] = lcdBuf;
        break;

    case THIRD:
        // set
        lcdBuf = HT_LCD_Read( 16 );
        lcdBuf |= SEG16_3G | SEG16_3F;
        HT_LCD->LCDBUF[ 16 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG17_3A | SEG17_3B | SEG17_3C | SEG17_3D;
        HT_LCD->LCDBUF[ 17 ] = lcdBuf;
        break;

    case FOURTH:
        // set
        lcdBuf = HT_LCD_Read( 18 );
        lcdBuf |= SEG18_4G | SEG18_4F;
        HT_LCD->LCDBUF[ 18 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG19_4A | SEG19_4B | SEG19_4C | SEG19_4D;
        HT_LCD->LCDBUF[ 19 ] = lcdBuf;
        break;

    case FIFTH:
        // set
        lcdBuf = HT_LCD_Read( 20 );
        lcdBuf |= SEG20_5G | SEG20_5F;
        HT_LCD->LCDBUF[ 20 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG21_5A | SEG21_5B | SEG21_5C | SEG21_5D;
        HT_LCD->LCDBUF[ 21 ] = lcdBuf;
        break;

    case SIXTH:
        // set
        lcdBuf = HT_LCD_Read( 22 );
        lcdBuf |= SEG22_6G | SEG22_6F;
        HT_LCD->LCDBUF[ 22 ] = lcdBuf;
        lcdBuf               = 0x0;
        lcdBuf |= SEG23_6A | SEG23_6B | SEG23_6C | SEG23_6D;
        HT_LCD->LCDBUF[ 23 ] = lcdBuf;
        break;

    case SEVENTH:
        // set
        lcdBuf = HT_LCD_Read( 8 );
        lcdBuf |= SEG8_7B | SEG8_7C | SEG8_7G;
        HT_LCD->LCDBUF[ 8 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG9_7A | SEG9_7D | SEG9_7F;
        HT_LCD->LCDBUF[ 9 ] = lcdBuf;
        break;

    case EIGHTH:
        // set
        lcdBuf = HT_LCD_Read( 6 );
        lcdBuf |= SEG6_8B | SEG6_8C | SEG6_8G;
        HT_LCD->LCDBUF[ 6 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG7_8A | SEG7_8D | SEG7_8F;
        HT_LCD->LCDBUF[ 7 ] = lcdBuf;
        break;

    case NINTH:
        // set
        lcdBuf = HT_LCD_Read( 4 );
        lcdBuf |= SEG4_9B | SEG4_9C | SEG4_9G;
        HT_LCD->LCDBUF[ 4 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG5_9A | SEG5_9D | SEG5_9F;
        HT_LCD->LCDBUF[ 5 ] = lcdBuf;
        break;

    case TEN:
        // set
        lcdBuf = HT_LCD_Read( 2 );
        lcdBuf |= SEG2_10B | SEG2_10C | SEG2_10G;
        HT_LCD->LCDBUF[ 2 ] = lcdBuf;
        lcdBuf              = 0x0;
        lcdBuf |= SEG3_10A | SEG3_10D | SEG3_10F;
        HT_LCD->LCDBUF[ 3 ] = lcdBuf;
        break;
    default:
        break;
    }
}

////显示\清除符号小数点.
void LcdDisplayPoint( DATA_POS_t pos, LCD_Display_TypeDef displayFlg )
{
    switch ( pos ) {
    case FIRST:
        LcdDisplay( 8, SEG8_P3, displayFlg );
        break;

    case SECOND:
        LcdDisplay( 6, SEG6_P4, displayFlg );
        break;

    case THIRD:
        LcdDisplay( 4, SEG4_P5, displayFlg );
        break;
    default:
        break;
    }
}

//显示\清除符号冒号:
void LcdDisplayCol( DATA_POS_t pos, LCD_Display_TypeDef displayFlg )
{
    switch ( pos ) {
    case 1:
        LcdDisplay( 16, SEG16_COL1, displayFlg );
        break;
    case 2:
        LcdDisplay( 20, SEG20_COL2, displayFlg );
        break;
    default:
        break;
    }
}

static void gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    //CS
    memset( &GPIO_InitStruct, 0, sizeof( GPIO_InitTypeDef ) );
    GPIO_InitStruct.GPIO_Pin          = HT1621_CS_PIN;
    GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init( HT1621_CS_PORT, &GPIO_InitStruct );

    //WR
    memset( &GPIO_InitStruct, 0, sizeof( GPIO_InitTypeDef ) );
    GPIO_InitStruct.GPIO_Pin          = HT1621_WR_PIN;
    GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init( HT1621_WR_PORT, &GPIO_InitStruct );

    //RD
    memset( &GPIO_InitStruct, 0, sizeof( GPIO_InitTypeDef ) );
    GPIO_InitStruct.GPIO_Pin          = HT1621_RD_PIN;
    GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init( HT1621_RD_PORT, &GPIO_InitStruct );

    //DATA
    memset( &GPIO_InitStruct, 0, sizeof( GPIO_InitTypeDef ) );
    GPIO_InitStruct.GPIO_Pin          = HT1621_DATA_PIN;
    GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init( HT1621_DATA_PORT, &GPIO_InitStruct );
}

/**
***Ht1621芯片初始化
***
**/
void Ht1621Init( void )
{
    printf("Ht1621Init. \n");

    // rt_pin_mode(HT1621_CS, PIN_MODE_OUTPUT);//gpio_init(LCD_CS_GPIO, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LCD_CS_PIN);
    // rt_pin_mode(HT1621_RD, PIN_MODE_OUTPUT);//gpio_init(LCD_RDN_GPIO, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LCD_RDN_PIN);
    // rt_pin_mode(HT1621_WR, PIN_MODE_OUTPUT);//gpio_init(LCD_WRN_GPIO, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LCD_WRN_PIN);
    // //gpio_init(LCD_LIGHT_GPIO, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LCD_LIGHT_PIN);
    // rt_pin_mode(HT1621_DATA, PIN_MODE_OUTPUT);//gpio_init(LCD_DATA_GPIO, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, LCD_DATA_PIN);

    gpio_init();

    Ht1621_WR_1();
    Ht1621_CS_1();
    Ht1621_RD_1();
    Ht1621_DATA_1();

	extern void Delay_mSec(int mSec);
	Delay_mSec(1000);

    Ht1621WrCmd( BIAS );
    Ht1621WrCmd( SYSDIS );
    // Ht1621WrCmd(XTAL);			//使用外部晶振 32.768k
    Ht1621WrCmd( RC256 );  //使用内部晶振 32.768k
    Ht1621WrCmd( WDTDIS1 );
    Ht1621WrCmd( SYSEN );
    Ht1621WrCmd( LCDON );
    // Ht1621WrCmd(0x12);
    // Ht1621WrCmd(0x10);
}

void LcdInit(void)
{
    Ht1621Init();
    LcdAllOff();
    LcdTurnOnLed();
    LcdAllOn();
    printf("init ok.\n");
}

