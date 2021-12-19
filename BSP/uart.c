#include "uart.h"
#include "cmu.h"
#include "delay.h"

#if UART0_EN
static void         UART0_Init( uint32_t rate );
// static __IO uint8_t UART0_RxBuff[ UART0_BUF_SIZE ] = { 0 };
UART_INFO_STR       gUart0Handle;
#endif

#if UART3_EN
static void UART3_Init( uint32_t rate );
UART_INFO_STR gUart3Handle;
#endif

#if UART4_EN
static void         UART4_Init( uint32_t rate );
static __IO uint8_t UART4_RxBuff[ UART4_BUF_SIZE ] = { 0 };
UART_INFO_STR       gUart4Handle;
#endif

static void USART_SendData( UART_INFO_STR* USARTx, uint8_t Data );
static void IRQHandler( UART_INFO_STR* USARTx );

#if UART0_EN

void UART0_Init( uint32_t rate )
{
    GPIO_InitAFTypeDef GPIO_InitAFStruct;
    memset( &GPIO_InitAFStruct, 0, sizeof( GPIO_InitAFTypeDef ) );

    // PC2 --RX0
    GPIO_InitAFStruct.GPIO_Pin         = GPIO_Pin_2;
    GPIO_InitAFStruct.GPIO_AFMode      = PC2_RX0;
    GPIO_InitAFStruct.GPIO_InputStruct = GPIO_Input_Up;
    HT_GPIO_AFInit( HT_GPIOC, &GPIO_InitAFStruct );

    // PC3--TX0
    GPIO_InitAFStruct.GPIO_Pin          = GPIO_Pin_3;
    GPIO_InitAFStruct.GPIO_AFMode       = PC3_TX0;
    GPIO_InitAFStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_AFInit( HT_GPIOC, &GPIO_InitAFStruct );

    UART_InitTypeDef UART_InitStructure;
    HT_CMU_ClkCtrl1Config( CMU_CLKCTRL1_UART0EN, ENABLE );
    UART_InitStructure.UART_Logic      = UartLogicPositive;
    UART_InitStructure.UART_StopBits   = OneStopBits;
    UART_InitStructure.UART_WordLength = WordLength_8Bits;
    UART_InitStructure.UART_Parity     = UartParity_EVEN;
    UART_InitStructure.UART_BaudRate   = rate;
    UART_InitStructure.ReceiveEN       = ENABLE;
    UART_InitStructure.SendEN          = ENABLE;

    HT_UART_Init( HT_UART0, &UART_InitStructure );

    // HT_UART_ITConfig( HT_UART0, UART_UARTCON_TXIE | UART_UARTCON_RXIE, ENABLE );
    HT_UART_ITConfig( HT_UART0, UART_UARTCON_TXIE, ENABLE );

    NVIC_ClearPendingIRQ( UART0_IRQn );
    NVIC_SetPriority( UART0_IRQn, 0 );
    NVIC_EnableIRQ( UART0_IRQn );

    //接收缓存区初始化
    gUart0Handle.sysAddr   = HT_UART0;
    gUart0Handle.SendState = 0;
    // FIFO_S_Init( &gUart0Handle.rxFIFO, ( void* )UART0_RxBuff, sizeof( UART0_RxBuff ) );
    // FIFO_S_Flush( &gUart0Handle.rxFIFO );
}

#endif

#if UART3_EN

void UART3_Init( uint32_t rate )
{
    GPIO_InitAFTypeDef GPIO_InitAFStruct;
    memset( &GPIO_InitAFStruct, 0, sizeof( GPIO_InitAFTypeDef ) );

    // PE4 --RX3
    GPIO_InitAFStruct.GPIO_Pin         = GPIO_Pin_4;
    GPIO_InitAFStruct.GPIO_AFMode      = PE4_RX3;
    GPIO_InitAFStruct.GPIO_InputStruct = GPIO_Input_Up;
    HT_GPIO_AFInit( HT_GPIOE, &GPIO_InitAFStruct );

    // PE5--TX3
    GPIO_InitAFStruct.GPIO_Pin          = GPIO_Pin_5;
    GPIO_InitAFStruct.GPIO_AFMode       = PE5_TX3;
    GPIO_InitAFStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_AFInit( HT_GPIOE, &GPIO_InitAFStruct );

    UART_InitTypeDef UART_InitStructure;
    HT_CMU_ClkCtrl1Config( CMU_CLKCTRL1_UART3_7816_1EN, ENABLE );
    UART_InitStructure.UART_Logic      = UartLogicPositive;
    UART_InitStructure.UART_StopBits   = OneStopBits;
    UART_InitStructure.UART_WordLength = WordLength_8Bits;
    UART_InitStructure.UART_Parity     = UartParity_Disable;
    UART_InitStructure.UART_BaudRate   = rate;
    UART_InitStructure.ReceiveEN       = ENABLE;
    UART_InitStructure.SendEN          = ENABLE;

    HT_UART_Init( HT_UART3, &UART_InitStructure );

    HT_UART_ITConfig( HT_UART3, UART_UARTCON_TXIE | UART_UARTCON_RXIE, ENABLE );

    NVIC_ClearPendingIRQ( UART3_IRQn );
    NVIC_SetPriority( UART3_IRQn, 0 );
    NVIC_EnableIRQ( UART3_IRQn );

    //接收缓存区初始化
    gUart3Handle.sysAddr   = HT_UART3;
    gUart3Handle.SendState = 0;
    static __IO uint8_t UART3_RxBuff[UART3_BUF_SIZE] = {0};
    FIFO_S_Init(&gUart3Handle.rxFIFO, (void*)UART3_RxBuff, sizeof(UART3_RxBuff));
    FIFO_S_Flush( &gUart3Handle.rxFIFO );
}

#endif

#if UART4_EN
void UART4_Init( uint32_t rate )
{
    GPIO_InitAFTypeDef GPIO_InitAFStruct;
    memset( &GPIO_InitAFStruct, 0, sizeof( GPIO_InitAFTypeDef ) );

    // PE2 --RX4
    GPIO_InitAFStruct.GPIO_Pin         = GPIO_Pin_2;
    GPIO_InitAFStruct.GPIO_AFMode      = PE2_RX4;
    GPIO_InitAFStruct.GPIO_InputStruct = GPIO_Input_Up;
    HT_GPIO_AFInit( HT_GPIOE, &GPIO_InitAFStruct );

    // PE1--TX4
    GPIO_InitAFStruct.GPIO_Pin          = GPIO_Pin_1;
    GPIO_InitAFStruct.GPIO_AFMode       = PE1_TX4;
    GPIO_InitAFStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_AFInit( HT_GPIOE, &GPIO_InitAFStruct );

    UART_InitTypeDef UART_InitStructure;
    HT_CMU_ClkCtrl1Config( CMU_CLKCTRL1_UART4_7816_0EN, ENABLE );
    UART_InitStructure.UART_Logic      = UartLogicPositive;
    UART_InitStructure.UART_StopBits   = OneStopBits;
    UART_InitStructure.UART_WordLength = WordLength_8Bits;
    UART_InitStructure.UART_Parity     = UartParity_Disable;
    UART_InitStructure.UART_BaudRate   = rate;
    UART_InitStructure.ReceiveEN       = ENABLE;
    UART_InitStructure.SendEN          = ENABLE;

    HT_UART_Init( HT_UART4, &UART_InitStructure );

    HT_UART_ITConfig( HT_UART4, UART_UARTCON_TXIE | UART_UARTCON_RXIE, ENABLE );

    NVIC_ClearPendingIRQ( UART4_IRQn );
    NVIC_SetPriority( UART4_IRQn, 3 );
    NVIC_EnableIRQ( UART4_IRQn );

    //接收缓存区初始化
    gUart4Handle.sysAddr   = HT_UART4;
    gUart4Handle.SendState = 0;
    FIFO_S_Init( &gUart4Handle.rxFIFO, ( void* )UART4_RxBuff, sizeof( UART4_RxBuff ) );
}
#endif

//轮询方式发送数据
void USART_SendData( UART_INFO_STR* USARTx, uint8_t Data )
{
    uint32_t SendTime = OSTimeGet();
    while ( USARTx->SendState != 0 ) {
        if ( OSTimeGet() - SendTime > 50 )
            break;
    }
    USARTx->SendState     = 1;
    USARTx->sysAddr->SBUF = Data;
}

void Uart_Send( UART_INFO_STR* USARTx, char* data, uint16_t len )
{
    for ( uint16_t i = 0; i < len; i++ ) {
        USART_SendData( USARTx, data[ i ] );
    }
}

void UART_init( void )
{
#if UART0_EN
    UART0_Init( 115200 );
#endif

#if UART3_EN
    UART3_Init( 115200 );
#endif

#if UART4_EN
    UART4_Init( 115200 );
#endif
}

void IRQHandler( UART_INFO_STR* USARTx )
{
    if ( SET == HT_UART_ITFlagStatusGet( USARTx->sysAddr, UART_UARTSTA_TXIF ) ) /*!< UART发送中断         */
    {
        HT_UART_ClearITPendingBit( USARTx->sysAddr, UART_UARTSTA_TXIF );
        USARTx->SendState = 0;
    }
    if ( SET == HT_UART_ITFlagStatusGet( USARTx->sysAddr, UART_UARTSTA_RXIF ) ) /*!< UART接收中断         */
    {
        uint8_t RxData = HT_UART_ReceiveData( USARTx->sysAddr );
        HT_UART_ClearITPendingBit( USARTx->sysAddr, UART_UARTSTA_RXIF );
        FIFO_S_Put( &USARTx->rxFIFO, RxData );
    }
}

#if UART0_EN
void UART0_IRQHandler()
{
    IRQHandler( &gUart0Handle );
}
#endif

#if UART3_EN
void UART3_IRQHandler()
{
    IRQHandler( &gUart3Handle );
}
#endif

#if UART4_EN
void UART4_IRQHandler()
{
    IRQHandler( &gUart4Handle );
}
#endif
