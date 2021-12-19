#ifndef __UART_H__
#define __UART_H__

#include "FIFO.h"
#include "includes.h"

#define UART0_EN 1
#define UART3_EN 1
#define UART4_EN 0

#define UART0_BUF_SIZE 128
#define UART3_BUF_SIZE 128
#define UART4_BUF_SIZE 128

//校验位
#define USART_Parity_NO_1 ( ( uint16_t )0x0070 )  //固定位1
#define USART_Parity_No_0 ( ( uint16_t )0x0010 )  //固定0
#define USART_Parity_Even ( ( uint16_t )0x0050 )  //偶校验
#define USART_Parity_Odd ( ( uint16_t )0x0030 )   //奇校验
#define USART_Parity_NONE ( ( uint16_t )0x0000 )  //无校验

//停止位
#define USART_StopBits_1 ( ( uint16_t )0x0000 )  // 1位停止位
#define USART_StopBits_2 ( ( uint16_t )0x0100 )  // 2位停止位

#define USART_WordLength_8b ( ( uint16_t )0x0000 )  //
#define USART_WordLength_7b ( ( uint16_t )0x0080 )  //

typedef struct
{
    FIFO_S_t         rxFIFO;
    HT_UART_TypeDef* sysAddr;
    uint8_t          SendState;
} UART_INFO_STR;

extern UART_INFO_STR gUart0Handle;
extern UART_INFO_STR gUart4Handle;
extern UART_INFO_STR gUart3Handle;

void Uart_Send( UART_INFO_STR* USARTx, char* data, uint16_t len );
void UART_init( void );

#endif
