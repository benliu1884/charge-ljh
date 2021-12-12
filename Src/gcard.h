/**
 * @file gcard.h
 * @japy (you@domain.com)
 * @brief goiot M1卡读写操作
 * @version 0.1
 * @date 2019-01-03
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#ifndef _GCARD_H
#define _GCARD_H

//返回值定义，在m1card.h中也有定义，防止重复定义
#ifndef CR_SUCESS
#define CR_SUCESS 0         //成功
#define CR_TIMEOUT -1       //读取超时
#define CR_FAILED -2        //操作失败
#define CR_FRAME_ERROR -3   //数据帧错误
#define CR_INSUFFICIENT -4  //余额不足
#define CR_INVALID -5       //无效卡
#endif

typedef enum {
    WALLET_CARD=0x11,
    ID_CARD=0x12,
    MANAGE_CARD=0x13,
} CARD_TYPE;

typedef struct {
    unsigned char serialid[4]; //卡物理序列号
    unsigned char type;     //卡类型，0x11-钱包卡，0x12-鉴权卡
    int expireddate;        //有效期，unix时间戳
    unsigned char status;   //卡状态，0x00-正常，0x01-锁定，0x02-冻结（黑名单，注销等情况）
    unsigned char pledge;   //是否有押金，0-无押金，1-有押金
    char userid[17];        //用户卡号，最大长度16字节
    char stationsn[17];     //上次消费充电站编号
    int timestamp;          //上次消费UNIX时间戳
    int balance;            //账户内余额，单位分
    char haspassword;       //是否启用卡内密码, 0-不启用，1-启用
    char password[6];       //卡内密码
    char customer[21];      //运营商代码
} CardInfo;

typedef struct {
    char userid[17];        //用户卡号，最大长度16字节
    unsigned char type;     //卡类型，0x11-钱包卡，0x12-鉴权卡
    int expireddate;        //有效期，unix时间戳
    unsigned char pledge;   //是否有押金，0-无押金，1-有押金
    char haspassword;       //是否启用卡内密码, 0-不启用，1-启用
    char password[6];       //卡内密码，ASCII
    char username[17];      //用户姓名，最大长度16字节，GBK编码
    char telphone[13];      //用户手机号
    char idcard[31];        //身份证号
} CardInitialisation;

#pragma pack(1)
typedef struct {
    unsigned char password[6];     //卡内密码
} USERBLOCK_0_1;

typedef struct {
    unsigned char username[16]; //用户姓名，GBK编码
} USERBLOCK_1_0;

typedef struct {
    unsigned char vendor[10];   //运营商代码
    unsigned char tel[6];       //用户手机号码
} USERBLOCK_1_1;

typedef struct {
    unsigned char idtype;       //证件类型，1-身份证，2-护照，3-其他
    unsigned char idnum[15];    //证件号
} USERBLOCK_1_2;

typedef struct {
    unsigned char status;           //卡使用标志，0：正常，1：锁定，2：冻结
    unsigned char chargersn[6];     //上次充电的充电桩号 
    unsigned int balance;           //卡内余额，分辨率分
    unsigned int lasttime;          //上次使用时间戳
    unsigned char checksum;         //校验
} USERBLOCK_2_0;

typedef struct {
    unsigned char type;         //卡类型，0x11-钱包卡，0x12-鉴权卡
    unsigned char userid[8];    //用户卡号，BCD
    unsigned char pledge;       //是否有押金，0-无押金，1-有押金
    unsigned int expireddate;   //有效期，unix时间戳
    unsigned char haspassword;  //用户密码标志，0-不启用，1-启用
    unsigned char checksum;     //校验
} USERBLOCK_2_1;
#pragma pack()

//typedef void (*CARD_READ)(CardInfo *card);
int init_gcard_device(int device, int baudrate);
int card_lock(const char *stationsn);
int card_unlock(void);
int card_deduct(int payment);
int card_recharge(int payment);
int card_create(CardInitialisation *card);
int card_read(CardInfo *card);
int card_clear(void);
int card_test(void);
#endif //_GCARD_H
