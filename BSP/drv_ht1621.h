/**
 * @file drv_ht1621.h
 * @brief ht1621 断码屏控制驱动芯片
 * @date 2021-10-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef _DRV_HT1621_H
#define _DRV_HT1621_H

#include "includes.h"
#include <stdio.h>
#include <stdlib.h>

#define HT1621_CS_PORT      HT_GPIOD
#define HT1621_CS_PIN       GPIO_Pin_13

// #define HT1621_RD_PORT      HT_GPIOD
// #define HT1621_RD_PIN       GPIO_Pin_11

#define HT1621_WR_PORT      HT_GPIOE
#define HT1621_WR_PIN       GPIO_Pin_2

#define HT1621_DATA_PORT    HT_GPIOE
#define HT1621_DATA_PIN     GPIO_Pin_1

#define HT1621_BK_PORT       HT_GPIOD
#define HT1621_BK_PIN        GPIO_Pin_12


#define MaxSegNum 32

//HT1621指令
#define BIAS		0x52		//1000 0101 0010		1/3duty, 4 com		
#define SYSDIS		0x00		//1000 0000 0000		关闭系统振荡器和LCD偏压发生器
#define SYSEN		0x02		//1000 0000 0010		打开系统振荡器
#define LCDOFF		0x04		//1000 0000 0100        关闭LCD偏压
#define LCDON		0x06		//1000 0000 0110		打开LCD偏压
#define XTAL		0x28		//1000 0010 1000		外部接时钟
#define RC256		0x30		//1000 0011 0000		内部时钟
#define	WDTDIS1		0x0A		//1000 0000 1010		禁止看门狗

// void Ht1621_LightOn(void);
// void Ht1621_LightOff(void);

#define Ht1621_LightOn() HT_GPIO_BitsSet( HT1621_BK_PORT, HT1621_BK_PIN )
#define Ht1621_LightOff() HT_GPIO_BitsReset( HT1621_BK_PORT, HT1621_BK_PIN )


/**
 * @brief Ht1621读取段的值时序
 * 
 * @param segAddr 地址
 * @return uint8_t 读到的数据
 */
uint8_t Ht1621ReadSegVal(uint8_t segAddr);

/**
 * @brief Ht1621写一个数据
 * 
 * @param segAddr 地址
 * @param val 写入的数据
 */
void Ht1621WriteSegVal(uint8_t segAddr, uint8_t val);

/**
 * @brief Ht1621芯片初始化
 */
void Ht1621Init(void);


#endif
