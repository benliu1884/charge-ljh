#ifndef __LCD_H__
#define __LCD_H__

#include "includes.h"


#define HT1621_CS_PORT      HT_GPIOD
#define HT1621_CS_PIN       GPIO_Pin_11

#define HT1621_RD_PORT      HT_GPIOD
#define HT1621_RD_PIN       GPIO_Pin_11

#define HT1621_WR_PORT      HT_GPIOD
#define HT1621_WR_PIN       GPIO_Pin_11

#define HT1621_DATA_PORT    HT_GPIOD
#define HT1621_DATA_PIN     GPIO_Pin_11


#define MaxSegNum       31 //25
#define COMMAND         0
#define DAT             1
//HT1621指令
#define BIAS		0x52		//1000 0101 0010
#define SYSDIS		0x00		//1000 0000 0000
#define SYSEN		0x02		//1000 0000 0010
#define LCDOFF		0x04		//1000 0000 0100
#define LCDON		0x06		//1000 0000 0110
#define XTAL		0x28		//1000 0010 1000
#define RC256		0x30		//1000 0011 0000
#define	WDTDIS1		0x0A		//1000 0000 1010


typedef enum
{
	VRsel_OneFourth_P99= 0x0000,     /*!< 99.1%  */
	VRsel_OneFourth_P93= 0x0001,     /*!< 94.0%  */
	VRsel_OneFourth_P88= 0x0010,     /*!< 88.8%  */
	VRsel_OneFourth_P83= 0x0011,     /*!< 84.2%  */
	VRsel_OneFourth_P78= 0x0100,     /*!< 80.0%  */
	VRsel_OneFourth_P74= 0x0101,     /*!< 76.2%  */
	VRsel_OneFourth_P71= 0x0110,     /*!< 72.8%  */
	VRsel_OneFourth_P68= 0x0111,     /*!< 69.6%  */
	VRsel_OneFourth_P65= 0x1000,     /*!< 66.6%  */
	VRsel_OneFourth_P64= 0x1001,     /*!< 64.0%  */
	VRsel_OneFourth_P60= 0x1010,     /*!< 60.0%  */
	VRsel_OneFourth_P59= 0x1011,     /*!< 59.2%  */
	VRsel_OneFourth_P57= 0x1100,     /*!< 57.2%  */
	VRsel_OneFourth_P55= 0x1101,     /*!< 55.2%  */
	VRsel_OneFourth_P53= 0x1110,     /*!< 53.4%  */
	VRsel_OneFourth_P51= 0x1111,     /*!< 51.6%  */

}LCD_VRsel_OneFourth_TypeDef;



typedef enum{
	LCD_CLEAR = 0,			//清除
	LCD_DISPLAY = 1,		//显示
}LCD_Display_TypeDef;


typedef enum{
	SEG0_INVALID 		    = 0X01,//无效
	SEG0_FAULT 			    = 0X02,//故障
	SEG0_SUCCESS			= 0x04,//成功
    SEG0_FAILURE			= 0x08,//失败
}SEG0_DISPLAY_t;

typedef enum{
	SEG1_CHARGING 		    = 0X01,//充电中
	SEG1_PROGRESS_BAR3   	= 0x02,	//进度条T3
	SEG1_PROGRESS_BAR2 		= 0x04,	//进度条T2
	SEG1_PROGRESS_BAR1		= 0x08,	//进度条T1
}SEG1_DISPLAY_t;


typedef enum{
	SEG2_YUAN				= 0x01,//元
	SEG2_10C   				= 0x02,//10C
	SEG2_10G 				= 0x04,//10G
	SEG2_10B  				= 0x08,//10B
}SEG2_DISPLAY_t;


typedef enum{
	SEG3_10D				= 0x01,//10D
	SEG3_10E   				= 0x02,//10E
	SEG3_10F 				= 0x04,//10F
	SEG3_10A  				= 0x08,//10A
}SEG3_DISPLAY_t;


typedef enum{
	SEG4_P5				    = 0x01,//小数点P5
	SEG4_9C   				= 0x02,//9C
	SEG4_9G 				= 0x04,//9G
	SEG4_9B  				= 0x08,//9B
}SEG4_DISPLAY_t;

typedef enum{
	SEG5_9D				    = 0x01,//9D
	SEG5_9E   				= 0x02,//9E
	SEG5_9F 				= 0x04,//9F
	SEG5_9A  				= 0x08,//9A
}SEG5_DISPLAY_t;


typedef enum{
	SEG6_P4 			    = 0x01,//小数点P4
	SEG6_8C   				= 0x02,//8C
	SEG6_8G 				= 0x04,//8G
	SEG6_8B  				= 0x08,//8B
}SEG6_DISPLAY_t;

typedef enum{
	SEG7_8D				    = 0x01,//8D
	SEG7_8E   				= 0x02,//8E
	SEG7_8F 				= 0x04,//8F
	SEG7_8A  				= 0x08,//8A
}SEG7_DISPLAY_t;

typedef enum{
	SEG8_P3 			    = 0x01,//小数点P3
	SEG8_7C   				= 0x02,//7C
	SEG8_7G 				= 0x04,//7G
	SEG8_7B  				= 0x08,//7B
}SEG8_DISPLAY_t;

typedef enum{
	SEG9_7D				    = 0x01,//7D
	SEG9_7E   				= 0x02,//7E
	SEG9_7F 				= 0x04,//7F
	SEG9_7A  				= 0x08,//7A
}SEG9_DISPLAY_t;


typedef enum{
	SEG10_ALREADY_CHARGING  = 0x01,//已充
	SEG10_PREDICT_CHARGING  = 0x02,//预充 
	SEG10_KW 				= 0x04,//KW
	SEG10_A  				= 0x08,//A
}SEG10_DISPLAY_t;


typedef enum{
	SEG11_NOSIGNAL 			= 0x01,//无信号
	SEG11_SIGNAL   			= 0x02,//有信号
	SEG11_BACKSTAGE_CONNECT = 0x04,//后台连接符号
	SEG11_CHARGE_METHOD		= 0x08,//请刷卡或扫码开启充电
}SEG11_DISPLAY_t;


typedef enum{
	SEG12_CLOCK_DIAL		= 0x01,//时钟表盘
	SEG12_1E   				= 0x02,//1E
	SEG12_1G 				= 0x04,//1G
	SEG12_1F				= 0x08,//1F
}SEG12_DISPLAY_t;

typedef enum{
	SEG13_1D				= 0x01,//1D
	SEG13_1C   				= 0x02,//1C
	SEG13_1B 				= 0x04,//1B
	SEG13_1A				= 0x08,//1A
}SEG13_DISPLAY_t;

typedef enum{
	SEG14_CHARGING_TIME		= 0x01,//充电时长
	SEG14_2E   				= 0x02,//2E
	SEG14_2G 				= 0x04,//2G
	SEG14_2F				= 0x08,//1F
}SEG14_DISPLAY_t;

typedef enum{
	SEG15_2D				= 0x01,//2D
	SEG15_2C   				= 0x02,//2C
	SEG15_2B 				= 0x04,//2B
	SEG15_2A				= 0x08,//2A
}SEG15_DISPLAY_t;


typedef enum{
	SEG16_COL1				= 0x01,//P1冒号
	SEG16_3E   				= 0x02,//3E
	SEG16_3G 				= 0x04,//3G
	SEG16_3F				= 0x08,//3F
}SEG16_DISPLAY_t;

typedef enum{
	SEG17_3D				= 0x01,//3D
	SEG17_3C   				= 0x02,//3C
	SEG17_3B 				= 0x04,//3B
	SEG17_3A				= 0x08,//3A
}SEG17_DISPLAY_t;

typedef enum{
    SEG18_V                 = 0x01,//V
	SEG18_4E   				= 0x02,//4E
	SEG18_4G 				= 0x04,//4G
	SEG18_4F				= 0x08,//4F
}SEG18_DISPLAY_t;

typedef enum{
	SEG19_4D				= 0x01,//4D
	SEG19_4C   				= 0x02,//4C
	SEG19_4B 				= 0x04,//4B
	SEG19_4A				= 0x08,//4A
}SEG19_DISPLAY_t;

typedef enum{
	SEG20_COL2				= 0x01,//冒号2
	SEG20_5E   				= 0x02,//5E
	SEG20_5G 				= 0x04,//5G
	SEG20_5F				= 0x08,//5F
}SEG20_DISPLAY_t;

typedef enum{
	SEG21_5D				= 0x01,//5D
	SEG21_5C   				= 0x02,//5C
	SEG21_5B 				= 0x04,//5B
	SEG21_5A				= 0x08,//5A
}SEG21_DISPLAY_t;

typedef enum{
	SEG22_BLUTOOTH			= 0x01,//蓝牙符号
	SEG22_6E   				= 0x02,//6E
	SEG22_6G 				= 0x04,//6G
	SEG22_6F				= 0x08,//6F
}SEG22_DISPLAY_t;

typedef enum{
	SEG23_6D				= 0x01,//6D
	SEG23_6C   				= 0x02,//6C
	SEG23_6B 				= 0x04,//6B
	SEG23_6A				= 0x08,//6A
}SEG23_DISPLAY_t;

typedef enum{
	SEG24_EDGE				= 0x01,//边缘
	SEG24_TAB1_EDGE   		= 0x02,//TAB1边缘
	SEG24_TAB2_EDGE			= 0x04,//TAB2边缘
	SEG24_TAB3_EDGE			= 0x08,//TAB3边缘
}SEG24_DISPLAY_t;

typedef enum{
	SEG25_HOUR				= 0x01,//H
	SEG25_CARD_BALANCE   	= 0x02,//卡片余额
	SEG25_REAL_INFORMATION 	= 0x04,//实时信息
	SEG25_CHARING_MONEY		= 0x08,//充电金额
}SEG25_DISPLAY_t;


typedef enum{
	FIRST = 1,
	SECOND = 2,
	THIRD = 3,
	FOURTH = 4,
	FIFTH = 5,
	SIXTH = 6,
	SEVENTH = 7,
	EIGHTH = 8,
	NINTH = 9,
	TEN = 10,
	ELEVENTH = 11,
}DATA_POS_t;




void LcdGpioAFInit(HT_GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint16_t GPIO_AFMode, GPIOInput_TypeDef GPIO_InputStruct,GPIOOutput_TypeDef GPIO_OutputStruct);
void LcdSegAndComGpioConfig(void);
void LcdLedInit(void);
void LcdInit(void);
void LcdAllOn(void);
void LcdAllOff(void);
void LcdDisplay(uint8_t seg, uint8_t segVal, LCD_Display_TypeDef displayFlg);
void LcdDisplayBackStageConnect(LCD_Display_TypeDef displayFlg);
void LcdDisplaySingnal(LCD_Display_TypeDef displayFlg);
void LcdDisplayNoSingnal(LCD_Display_TypeDef displayFlg);
void LcdDisplayBlutooth(LCD_Display_TypeDef displayFlg);
void LcdDisplayChargingTime(LCD_Display_TypeDef displayFlg);
void LcdDisplayClockDial(LCD_Display_TypeDef displayFlg);;
void LcdDisplayChgMethod(LCD_Display_TypeDef displayFlg);
void LcdDisplayCardBalance(LCD_Display_TypeDef displayFlg);
void LcdDisplayRealInformation(LCD_Display_TypeDef displayFlg);
void LcdDisplayChargeingMoney(LCD_Display_TypeDef displayFlg);
void LcdDisplayAllEdge(void);
void LcdDisplayEdge(LCD_Display_TypeDef displayFlg);
void LcdDisplayTab1Edge(LCD_Display_TypeDef displayFlg);
void LcdDisplayTab2Edge(LCD_Display_TypeDef displayFlg);
void LcdDisplayTab3Edge(LCD_Display_TypeDef displayFlg);
void LcdDisplayPredictCharging(LCD_Display_TypeDef displayFlg);
void LcdDisplayAlreadyCharging(LCD_Display_TypeDef displayFlg);
void LcdDisplayKw(LCD_Display_TypeDef displayFlg);
void LcdDisplayHour(LCD_Display_TypeDef displayFlg);
void LcdDisplayYuan(LCD_Display_TypeDef displayFlg);
void LcdDisplayA(LCD_Display_TypeDef displayFlg);
void LcdDisplayV(LCD_Display_TypeDef displayFlg);
void LcdDisplayInvalid(LCD_Display_TypeDef displayFlg);
void LcdDisplayFault(LCD_Display_TypeDef displayFlg);
void LcdDisplaySuccess(LCD_Display_TypeDef displayFlg);
void LcdDisplayFailure(LCD_Display_TypeDef displayFlg);
void LcdDisplayProgressBar1(LCD_Display_TypeDef displayFlg);
void LcdDisplayProgressBar2(LCD_Display_TypeDef displayFlg);
void LcdDisplayProgressBar3(LCD_Display_TypeDef displayFlg);
void LcdDisplayChargeing(LCD_Display_TypeDef displayFlg);
    
void LcdClrData(DATA_POS_t pos);
void LcdDisplayZero(DATA_POS_t pos);
void LcdDisplayOne(DATA_POS_t pos);
void LcdDisplayTwo(DATA_POS_t pos);
void LcdDisplayThree(DATA_POS_t pos);
void LcdDisplayFour(DATA_POS_t pos);
void LcdDisplayFive(DATA_POS_t pos);
void LcdDisplaySix(DATA_POS_t pos);;
void LcdDisplaySeven(DATA_POS_t pos);
void LcdDisplayEight(DATA_POS_t pos);
void LcdDisplayNine(DATA_POS_t pos);
void LcdDisplayPoint( DATA_POS_t pos, LCD_Display_TypeDef displayFlg);
void LcdDisplayCol(DATA_POS_t pos, LCD_Display_TypeDef displayFlg);
void LcdDisplayTest(void);

#endif
