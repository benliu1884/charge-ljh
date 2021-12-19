/**
 * @file drv_ht1621.h
 * @brief ht1621 断码屏控制驱动芯片
 * @date 2021-10-18
 *
 * @copyright Copyright (c) 2021
 *
 * ht1621中 COM0-COM3为LCD公共端输出口
 *          SEG0-SEG31为LCD段输出端口
 * 点亮对应位置的图标需要先查找真值表找到对应的SEG
 * 写数据格式：101+6位地址(SEG0-SEG31)+4位数据(分别对应COM0-COM3,哪个位置1就点亮哪个位对应的图标)
 *
 */

#include "drv_ht1621.h"

/*******************************函数定义**************************/
#define Ht1621_CS_1() HT_GPIO_BitsSet( HT1621_CS_PORT, HT1621_CS_PIN )
#define Ht1621_CS_0() HT_GPIO_BitsReset( HT1621_CS_PORT, HT1621_CS_PIN )

#define Ht1621_WR_1() HT_GPIO_BitsSet( HT1621_WR_PORT, HT1621_WR_PIN )
#define Ht1621_WR_0() HT_GPIO_BitsReset( HT1621_WR_PORT, HT1621_WR_PIN )

#define Ht1621_RD_1() HT_GPIO_BitsSet( HT1621_WR_PORT, HT1621_WR_PIN )
#define Ht1621_RD_0() HT_GPIO_BitsReset( HT1621_WR_PORT, HT1621_WR_PIN )

#define Ht1621_DATA_1() HT_GPIO_BitsSet( HT1621_DATA_PORT, HT1621_DATA_PIN )
#define Ht1621_DATA_0() HT_GPIO_BitsReset( HT1621_DATA_PORT, HT1621_DATA_PIN )

#define Ht1621_GetData() HT_GPIO_BitsRead( HT1621_DATA_PORT, HT1621_DATA_PIN )

#define Ht1621_DataIn() HT1621_DATA_PORT->PTDIR &= ~( ( uint32_t )HT1621_DATA_PIN )
#define Ht1621_DataOut() HT1621_DATA_PORT->PTDIR |= ( uint32_t )HT1621_DATA_PIN


/**
 * @brief 写数据时序
 *
 * @param data 发送的数据
 * @param cnt  发送的数据位
 */
static void Ht1621Wr_Data( uint8_t data, uint8_t cnt )
{
    uint8_t i;
    // rt_pin_mode( Ht1621_DATA_PIN, PIN_MODE_OUTPUT );
    Ht1621_DataOut();

    for ( i = 0; i < cnt; i++ ) {
        Ht1621_WR_0();
        __NOP();
        if ( ( data & 0x80 ) == 0x80 )
            Ht1621_DATA_1();
        else
            Ht1621_DATA_0();
        Ht1621_WR_1();
        __NOP();
        data <<= 1;
    }
}

/**
 * @brief Ht1621读某段所映射地址存储数据内容的时序(D0-D3)
 */
static uint8_t Ht1621Rd_Data( void )
{
    uint8_t data = 0;
    uint8_t i;
    // rt_pin_mode( Ht1621_DATA_PIN, PIN_MODE_INPUT );
    Ht1621_DataIn();

    Ht1621_WR_1();
    __NOP();
    for ( i = 0; i < 4; i++ ) {
        Ht1621_RD_0();
        if ( 1 == Ht1621_GetData() )
            data |= ( 0x08 >> i );
        else
            data |= ( 0x0 >> i );
        __NOP();
        Ht1621_RD_1();
        __NOP();
    }

    return data;
}

/**
 * @brief 写指令
 *
 * @param cmd 命令
 */
static void Ht1621WrCmd( uint8_t cmd )
{
    Ht1621_CS_0();
    __NOP();
    Ht1621Wr_Data( 0x80, 4 );  //写入命令标志100
    Ht1621Wr_Data( cmd, 8 );   //写入命令数据
    Ht1621_CS_1();
    __NOP();
}

uint8_t segValue[MaxSegNum] = { 0 };

/**
 * @brief Ht1621读取段的值时序
 *
 * @param segAddr 地址
 * @return uint8_t 读到的数据
 */
uint8_t Ht1621ReadSegVal( uint8_t segAddr )
{
    // uint8_t segVal = 0;

    // Ht1621_CS_0();
    // __NOP();
    // Ht1621_RD_1();
    // Ht1621Wr_Data( 0xC0, 3 );          //写入命令标志110
    // Ht1621Wr_Data( segAddr << 2, 6 );  //写入地址(A5-A0)

    // segVal = Ht1621Rd_Data();
    // Ht1621_CS_1();
    // __NOP();
    // return segVal;
    if (segAddr >= MaxSegNum) {
        return 0;
    }
    return segValue[segAddr];
}

/**
 * @brief Ht1621写一个数据
 *
 * @param segAddr 地址
 * @param val 写入的数据
 */
void Ht1621WriteSegVal( uint8_t segAddr, uint8_t val )
{
    Ht1621_CS_0();
    __NOP();
    Ht1621Wr_Data( 0xa0, 3 );          //写入数据标志101
    Ht1621Wr_Data( segAddr << 2, 6 );  //写入地址数据(A5-A0)
    Ht1621Wr_Data( val << 4, 4 );      //写入数据的后4位
    Ht1621_CS_1();
    __NOP();
    if (segAddr < MaxSegNum) {
        segValue[segAddr] = val; 
    }
}

static void gpio_init( void )
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // CS
    memset( &GPIO_InitStruct, 0, sizeof( GPIO_InitTypeDef ) );
    GPIO_InitStruct.GPIO_Pin          = HT1621_CS_PIN;
    GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init( HT1621_CS_PORT, &GPIO_InitStruct );

    // WR
    memset( &GPIO_InitStruct, 0, sizeof( GPIO_InitTypeDef ) );
    GPIO_InitStruct.GPIO_Pin          = HT1621_WR_PIN;
    GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init( HT1621_WR_PORT, &GPIO_InitStruct );

    // RD
    // memset( &GPIO_InitStruct, 0, sizeof( GPIO_InitTypeDef ) );
    // GPIO_InitStruct.GPIO_Pin          = HT1621_RD_PIN;
    // GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    // GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    // HT_GPIO_Init( HT1621_RD_PORT, &GPIO_InitStruct );

    // DATA
    memset( &GPIO_InitStruct, 0, sizeof( GPIO_InitTypeDef ) );
    GPIO_InitStruct.GPIO_Pin          = HT1621_DATA_PIN;
    GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init( HT1621_DATA_PORT, &GPIO_InitStruct );

    // BK
    memset( &GPIO_InitStruct, 0, sizeof( GPIO_InitTypeDef ) );
    GPIO_InitStruct.GPIO_Pin          = HT1621_BK_PIN;
    GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init( HT1621_BK_PORT, &GPIO_InitStruct );
}

/**
 * @brief Ht1621芯片初始化
 */
void Ht1621Init( void )
{
    gpio_init();

    Ht1621_WR_1();
    Ht1621_CS_1();
    Ht1621_RD_1();
    Ht1621_DATA_1();
    Ht1621_LightOff();

    Ht1621WrCmd( BIAS );
    Ht1621WrCmd( SYSDIS );
    // Ht1621WrCmd(XTAL);			//使用外部晶振 32.768k
    Ht1621WrCmd( RC256 );
    Ht1621WrCmd( WDTDIS1 );
    Ht1621WrCmd( SYSEN );
    Ht1621WrCmd( LCDON );
}
