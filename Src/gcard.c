/**
 * @file gcard.c
 * @japy (you@domain.com)
 * @brief goiot M1卡读写操作
 * @version 0.1
 * @date 2019-01-03
 *
 * @copyright Copyright (c) 2019
 *
 */
#include "gcard.h"
#include "tiny_aes.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// M1卡操作接口
#define M1_SERIALID_SIZE 4
#define M1_BLOCK_SIZE 16
#define M1_KEY_SIZE 6

int m1_find_card( void );
int m1_read_serial_id( char* data );
int m1_auth_keya( int section, const char* pwd );
int m1_auth_keyb( int section, const char* pwd );
int m1_read_block( int section, int block, char* data );
int m1_write_block( int section, int block, const char* data );

//铭特626读卡器

/******************************************************
 * 铭特MT626
 * 读卡器数据帧格式
 * STX 长度(2字节) 命令字(CMD) 命令参数 数据包 ETX BCC
 *
 * 读卡器返回信息帧格式
 * STX 长度（2字节） 命令字 命令参数 状态字 数据包 ETX BCC
 *
 * 长度计算范围：命令字 ~ 数据包
 * BCC计算范围：STX~ETX，异或运算
 * 状态字：'Y'(0x59) - 操作成功
 *        'N'(0x4E) - 操作失败
 ******************************************************/
#define STX 0x02  //起始字节
#define ETX 0x03  //结束字节
#define CMD_BOARD 0x31  //卡机命令
#define BOARD_VERSION 0x40  //卡机读版本命令
#define BOARD_BEEP 0x3E  //蜂鸣器控制
#define CMD_CARD 0x34  //卡命令
#define M1_FIND_CARD 0x30  //寻射频卡
#define M1_SERIAL_ID 0x31  //获取Mefare1卡序列号
#define M1_AUTHENTICATE_KEYA 0x32  //校验密码A
#define M1_AUTHENTICATE_KEYB 0x39  //校验密码B
#define M1_READ_BLOCK 0x33  //读扇区块
#define M1_WRITE_BLOCK 0x34  //写扇区块
#define M1_UPDATE_KEY 0x35  //更新密码A
#define M1_INC_VALUE 0x37  //增值操作
#define M1_DEC_VALUE 0x38  //减值操作

#define MT_STX_OFFSET 0
#define MT_LENGTH_OFFSET 1
#define MT_CMD_OFFSET 3
#define MT_CMDP_OFFSET 4
#define MT_DATA_OFFSET 5
#define MT_RES_STATUS_OFFSET 5
#define MT_READ_ID_OFFSET 6
#define MT_READ_BLOCK_DATA_OFFSET 8

#define MT_DELAY_TIME 40

#define MT_BUFFER_SIZE 64
static unsigned char io_buffer[ MT_BUFFER_SIZE ];  //发送和接收缓冲区

#define MT_LOG( format, ... )                   \
    {                                           \
        printf( "[%s]", __FUNCTION__ );         \
        printf( format "\r\n", ##__VA_ARGS__ ); \
    }

#define MT_LOG_ARRAY( format, buffer, len )

// #define MT_LOG_ARRAY(format, buffer, len)            \
// {                                                   \
//     printf(format);                                 \
//     for(int i=0;i<len;i++)                          \
//         printf("%02X ", (unsigned char)buffer[i]);   \
//     printf("\r\n");                                 \
// }

static int  io_write( unsigned char* data, int size );
static int  io_read( unsigned char* data, int size );
static void io_delay( int ms );

static int check_res( unsigned char cmd, const unsigned char* data, int size )
{
    unsigned char bcc = 0;
    unsigned char i;
    //检查帧长度
    if ( size < MT_DATA_OFFSET + 2 )
        return CR_FRAME_ERROR;
    //检查帧标志
    if ( data[ MT_STX_OFFSET ] != STX )
        return CR_FRAME_ERROR;
    //检查帧尾标志
    if ( data[ size - 2 ] != ETX )
        return CR_FRAME_ERROR;
    //检查命令字
    if ( data[ MT_CMDP_OFFSET ] != cmd )
        return CR_FRAME_ERROR;
    //检查bcc
    for ( int i = 0; i < size - 1; i++ )
        bcc ^= data[ i ];
    if ( data[ size - 1 ] != bcc )
        return CR_FRAME_ERROR;

    switch ( cmd ) {
    case M1_FIND_CARD:  //寻射频卡
    case M1_SERIAL_ID:  //获取Mefare1卡序列号
        i = 5;
        break;
    case M1_AUTHENTICATE_KEYA:  //校验密码A
    case M1_AUTHENTICATE_KEYB:  //校验密码B
    case M1_UPDATE_KEY:         //更新密码A
        i = 6;
        break;
    case M1_READ_BLOCK:   //读扇区块
    case M1_WRITE_BLOCK:  //写扇区块
    case M1_INC_VALUE:    //增值操作
    case M1_DEC_VALUE:    //减值操作
        i = 7;
        break;
    }
    //检查返回状态字
    if ( io_buffer[ i ] == 'Y' )
        return CR_SUCESS;
    else
        return CR_FAILED;
}

/**
 * @brief
 * 发送卡操作命令，组装读卡器数据帧，并通过io_write发送
 * @param cmd 卡操作命令参数，寻射频卡，获取Mefare1卡序列号，校验密码A，读扇区块，写扇区块等
 * @param data 读卡器数据帧的数据区域，
 * @param size data的长度
 * @return int 返回发送的数据长度
 */
static int send_card_cmd( unsigned char cmd, unsigned char* data, int size )
{
    unsigned char bcc                 = 0;
    int           len                 = 0;
    io_buffer[ MT_STX_OFFSET ]        = STX;
    io_buffer[ MT_LENGTH_OFFSET ]     = ( 2 + size ) >> 8;    //
    io_buffer[ MT_LENGTH_OFFSET + 1 ] = ( 2 + size ) & 0xff;  //长度为 2 + data长度
    io_buffer[ MT_CMD_OFFSET ]        = CMD_CARD;
    io_buffer[ MT_CMDP_OFFSET ]       = cmd;
    len                               = MT_DATA_OFFSET;
    if ( data != 0 && size > 0 ) {
        memcpy( &io_buffer[ MT_DATA_OFFSET ], data, size );
        len += size;
    }
    io_buffer[ len++ ] = ETX;
    for ( int i = 0; i < len; i++ ) {
        bcc ^= io_buffer[ i ];
    }
    io_buffer[ len++ ] = bcc;
    return io_write( io_buffer, len );
}

int m1_find_card()
{
    int len;
    send_card_cmd( M1_FIND_CARD, 0, 0 );
    io_delay( MT_DELAY_TIME );
    len = io_read( io_buffer, MT_BUFFER_SIZE );
    if ( len > 0 ) {
        return check_res( M1_FIND_CARD, io_buffer, len );
    } else
        return CR_TIMEOUT;
}

int m1_read_serial_id( char* data )
{
    int len, ret;
    send_card_cmd( M1_SERIAL_ID, 0, 0 );
    io_delay( MT_DELAY_TIME );
    io_delay( MT_DELAY_TIME );
    len = io_read( io_buffer, MT_BUFFER_SIZE );
    if ( len > 0 ) {
        ret = check_res( M1_SERIAL_ID, io_buffer, len );
        if ( ret == CR_SUCESS )
            memcpy( data, &io_buffer[ MT_READ_ID_OFFSET ], M1_SERIALID_SIZE );
        return ret;
    } else
        return CR_TIMEOUT;
}

int m1_auth_keya( int section, const char* pwd )
{
    int           len;
    unsigned char data[ 1 + M1_KEY_SIZE ];

    data[ 0 ] = section;
    memcpy( data + 1, pwd, M1_KEY_SIZE );
    send_card_cmd( M1_AUTHENTICATE_KEYA, data, 1 + M1_KEY_SIZE );

    io_delay( MT_DELAY_TIME );
    len = io_read( io_buffer, MT_BUFFER_SIZE );
    if ( len > 0 ) {
        return check_res( M1_AUTHENTICATE_KEYA, io_buffer, len );
    } else
        return CR_TIMEOUT;
}

int m1_auth_keyb( int section, const char* pwd )
{
    int           len;
    unsigned char data[ 1 + M1_KEY_SIZE ];

    data[ 0 ] = section;
    memcpy( data + 1, pwd, M1_KEY_SIZE );
    send_card_cmd( M1_AUTHENTICATE_KEYB, data, 1 + M1_KEY_SIZE );

    io_delay( MT_DELAY_TIME );
    len = io_read( io_buffer, MT_BUFFER_SIZE );
    if ( len > 0 ) {
        return check_res( M1_AUTHENTICATE_KEYB, io_buffer, len );
    } else
        return CR_TIMEOUT;
}

int m1_read_block( int section, int block, char* data )
{
    int           len, ret;
    unsigned char udata[ 2 ];

    udata[ 0 ] = section;
    udata[ 1 ] = block;
    send_card_cmd( M1_READ_BLOCK, udata, 2 );

    io_delay( MT_DELAY_TIME );
    len = io_read( io_buffer, MT_BUFFER_SIZE );
    if ( len > 0 ) {
        ret = check_res( M1_READ_BLOCK, io_buffer, len );
        if ( ret == CR_SUCESS )
            memcpy( data, &io_buffer[ MT_READ_BLOCK_DATA_OFFSET ], M1_BLOCK_SIZE );
        return ret;
    } else
        return CR_TIMEOUT;
}

int m1_write_block( int section, int block, const char* data )
{
    int           len;
    unsigned char udata[ 2 + M1_BLOCK_SIZE ];

    udata[ 0 ] = section;
    udata[ 1 ] = block;
    memcpy( udata + 2, data, M1_BLOCK_SIZE );
    send_card_cmd( M1_WRITE_BLOCK, udata, 2 + M1_BLOCK_SIZE );

    io_delay( MT_DELAY_TIME );
    len = io_read( io_buffer, MT_BUFFER_SIZE );
    if ( len > 0 ) {
        return check_res( M1_WRITE_BLOCK, io_buffer, len );
    } else
        return CR_TIMEOUT;
}

static unsigned char       aes_seed_keya[ 16 ]     = "hellowGoiot2018";  // KEY_A seed key
static unsigned char       base_sector             = 6;
const static unsigned char keya_default[ 6 ]       = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };  //默认密码
const static unsigned char default_ctrl_block[ 4 ] = { 0xFF, 0x07, 0x80, 0x69 };              //默认控制块
static unsigned char       current_keya[ 6 ];                                                 //当前正在处理的卡片keya

/**
 * @brief 将bcd数组转成字符串输出
 *
 * @param str 输出的字符串指针
 * @param bcd 待转换的bcd数组
 * @param len 带转换的bcd数组长度
 */
static void bcd2str( char* str, const char* bcd, int len )
{
    char  temp[] = "0123456789ABCDEF";
    char* p      = str;
    for ( int i = 0; i < len; i++ ) {
        //逆反字节序
        // *p++ = temp[(bcd[len-1-i]>>4)&0x0F];
        // *str++ = temp[bcd[len-1-i]&0x0F];

        //正常字节序
        *p++ = temp[ ( bcd[ i ] >> 4 ) & 0x0F ];
        *p++ = temp[ bcd[ i ] & 0x0F ];
    }
    *p = '\0';
    //去掉字符串前面的 ‘0’ 字符
    p = str;
    while ( *p == '0' && *p != 0 ) {
        p++;
    }
    if ( p > str )
        memmove( str, p, strlen( p ) );
    str[ strlen( p ) ] = '\0';
}

static char atohex( char ch )
{
    char val = 0;

    if ( ( ch >= '0' ) && ( ch <= '9' ) )
        val = ch - '0';

    else if ( ( ch >= 'A' ) && ( ch <= 'F' ) )
        val = ch - 'A' + 10;

    else if ( ( ch >= 'a' ) && ( ch <= 'f' ) )
        val = ch - 'a' + 10;

    return val;
}

/**
 * @brief 格式化bcd字符串，对指定长度bcd字符串前面补0
 *
 * @param str bcd字符串
 * @param len bcd数组长度
 */
static void bcdstrformat( char* str, int len )
{
    int   f0 = 0;
    char* p  = str;
    if ( strlen( str ) < len * 2 ) {
        f0 = len * 2 - strlen( str );
        p  = str + f0;
        memmove( p, str, strlen( str ) );
        memset( str, '0', f0 );
        str[ len * 2 ] = 0;
    }
}

/**
 * @brief
 * 字符串转指定长度bcd数组，字符串长度必须大于等于bcd数组长度的2倍
 * @param bcd 指定bcd数组输出缓冲区
 * @param str 待转换的字符串
 * @param len 指定bcd数组长度
 */
static void str2bcd( char* bcd, const char* str, int len )
{
    char chh, chl;
    // char temp;
    int i;
    // int f0=0;

    //判断str长度是否合法
    if ( strlen( str ) < len * 2 ) {
        return;
    }

    for ( i = 0; i < len; i++ ) {
        chh      = atohex( str[ 2 * i ] );
        chl      = atohex( str[ 2 * i + 1 ] );
        bcd[ i ] = ( chh << 4 ) | chl;
    }

    //逆反字节序
    // for(int j=0;j<i/2;j++) {
    //     temp=bcd[j];
    //     bcd[j]=bcd[i-1-j];
    //     bcd[i-1-j]=temp;
    // }
}

static void block_checksum( char* data )
{
    int i;
    for ( i = 0; i < 15; i++ ) {
        data[ 15 ] += data[ i ];
    }
}

int card_lock( const char* stationsn )
{
    // tiny_aes_context ctx;
    int  ret;
    char data[ M1_BLOCK_SIZE ];
    char sn[ 13 ];
    // unsigned char in_temp[16];
    // unsigned char keya_temp[16];
    // memset(in_temp, 0, 16);

    // ret = m1_read_serial_id(in_temp);
    // if(ret!=CR_SUCESS)
    //     return ret;

    // tiny_aes_setkey_enc(&ctx, aes_seed_keya, 128);
    // tiny_aes_crypt_ecb(&ctx, AES_ENCRYPT, in_temp, keya_temp);

    // ret = m1_auth_keya(2, keya_temp); //扇区2
    ret = m1_auth_keya( base_sector + 2, ( const char* )current_keya );
    if ( ret == CR_SUCESS ) {
        ret = m1_read_block( base_sector + 2, 0, data );
        if ( ret == CR_SUCESS ) {
            USERBLOCK_2_0* p = ( USERBLOCK_2_0* )data;
            p->status        = 1;
            strncpy( sn, stationsn, 13 );
            bcdstrformat( sn, 6 );
            str2bcd( ( char* )p->chargersn, ( const char* )sn, 6 );
            block_checksum( data );
            ret = m1_write_block( base_sector + 2, 0, data );
        }
    }
    return ret;
}

int card_unlock()
{
    // tiny_aes_context ctx;
    int  ret;
    char data[ M1_BLOCK_SIZE ];
    // unsigned char in_temp[16];
    // unsigned char keya_temp[16];
    // memset(in_temp, 0, 16);

    // ret = m1_read_serial_id(in_temp);
    // if(ret!=CR_SUCESS)
    //     return ret;

    // tiny_aes_setkey_enc(&ctx, aes_seed_keya, 128);
    // tiny_aes_crypt_ecb(&ctx, AES_ENCRYPT, in_temp, keya_temp);

    // ret = m1_auth_keya(2, keya_temp); //扇区2
    ret = m1_auth_keya( base_sector + 2, ( const char* )current_keya );
    if ( ret == CR_SUCESS ) {
        ret = m1_read_block( base_sector + 2, 0, data );
        if ( ret == CR_SUCESS ) {
            USERBLOCK_2_0* p = ( USERBLOCK_2_0* )data;
            p->status        = 0;
            block_checksum( data );
            ret = m1_write_block( base_sector + 2, 0, data );
        }
    }
    return ret;
}

int card_deduct( int payment )
{
    // tiny_aes_context ctx;
    int  ret;
    char data[ M1_BLOCK_SIZE ];
    // unsigned char in_temp[16];
    // unsigned char keya_temp[16];
    // memset(in_temp, 0, 16);

    // ret = m1_read_serial_id(in_temp);
    // if(ret!=CR_SUCESS)
    //     return ret;

    // tiny_aes_setkey_enc(&ctx, aes_seed_keya, 128);
    // tiny_aes_crypt_ecb(&ctx, AES_ENCRYPT, in_temp, keya_temp);

    // ret = m1_auth_keya(2, keya_temp); //扇区2
    ret = m1_auth_keya( base_sector + 2, ( const char* )current_keya );
    if ( ret == CR_SUCESS ) {
        ret = m1_read_block( base_sector + 2, 0, data );
        if ( ret == CR_SUCESS ) {
            USERBLOCK_2_0* p = ( USERBLOCK_2_0* )data;
            if ( p->balance < payment )
                return CR_INSUFFICIENT;
            p->balance -= payment;
            p->status = 0;
            block_checksum( data );
            ret = m1_write_block( base_sector + 2, 0, data );
        }
    }
    return ret;
}

int card_recharge( int payment )
{
    // tiny_aes_context ctx;
    int  ret;
    char data[ M1_BLOCK_SIZE ];
    // unsigned char in_temp[16];
    // unsigned char keya_temp[16];
    // memset(in_temp, 0, 16);

    // ret = m1_read_serial_id(in_temp);
    // if(ret!=CR_SUCESS)
    //     return ret;

    // tiny_aes_setkey_enc(&ctx, aes_seed_keya, 128);
    // tiny_aes_crypt_ecb(&ctx, AES_ENCRYPT, in_temp, keya_temp);

    // ret = m1_auth_keya(2, keya_temp); //扇区2
    ret = m1_auth_keya( base_sector + 2, ( const char* )current_keya );
    if ( ret == CR_SUCESS ) {
        ret = m1_read_block( base_sector + 2, 0, data );
        if ( ret == CR_SUCESS ) {
            USERBLOCK_2_0* p = ( USERBLOCK_2_0* )data;
            p->balance += payment;
            block_checksum( data );
            ret = m1_write_block( base_sector + 2, 0, data );
        }
    }
    return ret;
}

int card_clear()
{
    tiny_aes_context ctx;
    int              ret;
    char             data[ M1_BLOCK_SIZE ];
    unsigned char    in_temp[ 16 ];
    unsigned char    keya_temp[ 16 ];
    memset( in_temp, 0, 16 );

    ret = m1_read_serial_id( ( char* )in_temp );
    if ( ret != CR_SUCESS )
        return ret;

    tiny_aes_setkey_enc( &ctx, aes_seed_keya, 128 );
    tiny_aes_crypt_ecb( &ctx, AES_ENCRYPT, in_temp, keya_temp );

    //扇区0
    ret = m1_auth_keya( base_sector + 0, ( const char* )keya_temp );
    if ( ret != CR_SUCESS )
        return ret;
    // block 3
    memcpy( data, keya_default, 6 );
    memcpy( data + 6, default_ctrl_block, 4 );
    memcpy( data + 10, keya_default, 6 );
    ret = m1_write_block( base_sector + 0, 3, data );
    if ( ret != CR_SUCESS )
        return ret;

    //扇区1
    ret = m1_auth_keya( base_sector + 1, ( const char* )keya_temp );
    if ( ret != CR_SUCESS )
        return ret;
    // block 3, keya,keyb
    memcpy( data, keya_default, 6 );
    memcpy( data + 6, default_ctrl_block, 4 );
    memcpy( data + 10, keya_default, 6 );
    ret = m1_write_block( base_sector + 1, 3, data );
    if ( ret != CR_SUCESS )
        return ret;
    // block 0
    memset( data, 0, M1_BLOCK_SIZE );
    ret = m1_write_block( base_sector + 1, 0, data );
    if ( ret != CR_SUCESS )
        return ret;
    // block 1
    memset( data, 0, M1_BLOCK_SIZE );
    ret = m1_write_block( base_sector + 1, 1, data );
    if ( ret != CR_SUCESS )
        return ret;
    // block 2
    memset( data, 0, M1_BLOCK_SIZE );
    ret = m1_write_block( base_sector + 1, 2, data );
    if ( ret != CR_SUCESS )
        return ret;

    //扇区2
    ret = m1_auth_keya( base_sector + 2, ( const char* )keya_temp );
    if ( ret != CR_SUCESS )
        return ret;
    // block 3, keya,keyb
    memcpy( data, keya_default, 6 );
    memcpy( data + 6, default_ctrl_block, 4 );
    memcpy( data + 10, keya_default, 6 );
    ret = m1_write_block( base_sector + 2, 3, data );
    if ( ret != CR_SUCESS )
        return ret;
    memset( data, 0, M1_BLOCK_SIZE );
    ret = m1_write_block( base_sector + 2, 0, data );
    if ( ret != CR_SUCESS )
        return ret;
    // block 1
    memset( data, 0, M1_BLOCK_SIZE );
    ret = m1_write_block( base_sector + 2, 1, data );

    return ret;
}

int card_create( CardInitialisation* card )
{
    tiny_aes_context ctx;
    int              ret;
    char             data[ M1_BLOCK_SIZE ];
    unsigned char    in_temp[ 16 ];
    unsigned char    keya_temp[ 16 ];
    memset( in_temp, 0, 16 );

    ret = m1_read_serial_id( ( char* )in_temp );
    if ( ret != CR_SUCESS )
        return ret;

    tiny_aes_setkey_enc( &ctx, aes_seed_keya, 128 );
    tiny_aes_crypt_ecb( &ctx, AES_ENCRYPT, in_temp, keya_temp );

    //扇区0
    ret = m1_auth_keya( base_sector + 0, ( const char* )keya_default );
    if ( ret != CR_SUCESS )
        return ret;
    // block 3
    memcpy( data, keya_temp, 6 );
    memcpy( data + 6, default_ctrl_block, 4 );
    memcpy( data + 10, keya_temp, 6 );
    ret = m1_write_block( base_sector + 0, 3, data );
    if ( ret != CR_SUCESS )
        return ret;
    // block 1
    if ( card->haspassword ) {
        memset( data, 0, M1_BLOCK_SIZE );
        memcpy( data, card->password, 6 );
        ret = m1_write_block( base_sector + 0, 1, data );
        if ( ret != CR_SUCESS )
            return ret;
    }

    //扇区1
    ret = m1_auth_keya( base_sector + 1, ( const char* )keya_default );
    if ( ret != CR_SUCESS )
        return ret;
    // block 3, keya,keyb
    memcpy( data, keya_temp, 6 );
    memcpy( data + 6, default_ctrl_block, 4 );
    memcpy( data + 10, keya_temp, 6 );
    ret = m1_write_block( base_sector + 1, 3, data );
    if ( ret != CR_SUCESS )
        return ret;
    // block 0
    memset( data, 0, M1_BLOCK_SIZE );
    memcpy( ( ( USERBLOCK_1_0* )data )->username, card->username, 16 );
    ret = m1_write_block( base_sector + 1, 0, data );
    if ( ret != CR_SUCESS )
        return ret;
    // block 1
    memset( data, 0, M1_BLOCK_SIZE );
    bcdstrformat( card->telphone, 6 );
    str2bcd( ( char* )( ( USERBLOCK_1_1* )data )->tel, card->telphone, 6 );
    ret = m1_write_block( base_sector + 1, 1, data );
    if ( ret != CR_SUCESS )
        return ret;
    // block 2
    memset( data, 0, M1_BLOCK_SIZE );
    ( ( USERBLOCK_1_2* )data )->idtype = 1;
    bcdstrformat( card->idcard, 15 );
    str2bcd( ( char* )( ( USERBLOCK_1_2* )data )->idnum, card->idcard, 15 );
    ret = m1_write_block( base_sector + 1, 2, data );
    if ( ret != CR_SUCESS )
        return ret;

    //扇区2
    ret = m1_auth_keya( base_sector + 2, ( const char* )keya_default );
    if ( ret != CR_SUCESS )
        return ret;
    // block 3, keya,keyb
    memcpy( data, keya_temp, 6 );
    memcpy( data + 6, default_ctrl_block, 4 );
    memcpy( data + 10, keya_temp, 6 );
    ret = m1_write_block( base_sector + 2, 3, data );
    if ( ret != CR_SUCESS )
        return ret;
    // block 1
    memset( data, 0, M1_BLOCK_SIZE );
    ( ( USERBLOCK_2_1* )data )->haspassword = card->haspassword;
    ( ( USERBLOCK_2_1* )data )->type        = card->type;
    bcdstrformat( card->userid, 8 );
    str2bcd( ( char* )( ( USERBLOCK_2_1* )data )->userid, card->userid, 8 );
    block_checksum( data );
    ret = m1_write_block( base_sector + 2, 1, data );

    return ret;
}

int card_test()
{
    int ret;

    ret = m1_find_card();
    return ret == CR_TIMEOUT;
}

int card_read( CardInfo* card )
{
    // AES_KEY aes;
    tiny_aes_context ctx;
    int              ret;
    char             data[ M1_BLOCK_SIZE ];
    unsigned char    in_temp[ 16 ];
    unsigned char    out_temp[ 16 ];
    memset( in_temp, 0, 16 );

    // ret = m1_find_card();
    // if(ret!=CR_SUCESS)
    //     return ret;

    // io_read( io_buffer, MT_BUFFER_SIZE );
    FIFO_S_Flush( &gUart3Handle.rxFIFO );

    ret = m1_read_serial_id( ( char* )in_temp );
    if ( ret != CR_SUCESS )
        return ret;
    printf("read card success..\r\n");
    memcpy( card->serialid, in_temp, 4 );
    tiny_aes_setkey_enc( &ctx, aes_seed_keya, 128 );
    tiny_aes_crypt_ecb( &ctx, AES_ENCRYPT, in_temp, out_temp );
    memcpy( current_keya, out_temp, 6 );

    ret = m1_auth_keya( base_sector + 1, ( const char* )current_keya );  //扇区1
    if ( ret != CR_SUCESS )
        return CR_INVALID;
    ret = m1_read_block( base_sector + 1, 1, data );
    if ( ret == CR_SUCESS ) {
        USERBLOCK_1_1* p = ( USERBLOCK_1_1* )data;
        strncpy( card->customer, ( char* )p->vendor, sizeof( p->vendor ) );
    }

    ret = m1_auth_keya( base_sector + 2, ( const char* )current_keya );  //扇区2
    if ( ret != CR_SUCESS )
        return CR_INVALID;

    ret = m1_read_block( base_sector + 2, 0, data );
    if ( ret == CR_SUCESS ) {
        USERBLOCK_2_0* p = ( USERBLOCK_2_0* )data;
        card->status     = p->status;
        card->balance    = p->balance;
        bcd2str( card->stationsn, ( const char* )p->chargersn, 6 );
        card->timestamp = p->lasttime;
    }
    ret = m1_read_block( base_sector + 2, 1, data );
    if ( ret == CR_SUCESS ) {
        USERBLOCK_2_1* p = ( USERBLOCK_2_1* )data;
        card->type       = p->type;
        bcd2str( card->userid, ( const char* )p->userid, 8 );
        card->expireddate = p->expireddate;
        card->haspassword = p->haspassword;
    }
    if ( card->haspassword ) {                                               //如果卡设置了密码，读取卡密码
        ret = m1_auth_keya( base_sector + 0, ( const char* )current_keya );  //扇区0
        if ( ret == CR_SUCESS ) {
            ret = m1_read_block( base_sector + 0, 1, data );
            if ( ret == CR_SUCESS ) {
                // USERBLOCK_0_1* p = ( USERBLOCK_0_1* )data;
                memcpy( card->password, data, 6 );
            }
        }
    }
    return ret;
}

int io_write( unsigned char* data, int size )
{
    Uart_Send( &gUart3Handle, ( char* )data, size );
    return 0;
}

int io_read( unsigned char* data, int size )
{
    uint8_t get;
    int     read_index = 0;

    while ( 1 ) {
        if ( FIFO_S_Get( &gUart3Handle.rxFIFO, &get ) != CL_OK ) {
            break;
        }
        if ( read_index >= size ) {
            break;
        }

        data[ read_index++ ] = get;
    }

    return read_index;
}

void io_delay( int ms )
{
    extern void Delay_mSec( int mSec );
    Delay_mSec( ms );
    // usleep(ms*1000);
}

int init_gcard_device( int device, int baudrate )
{
    return 0;
}

// int init_gcard_device(const char *path, int baudrate)
//{
//    const char *val;

//    gcard_dev = open_uart_dev(path, baudrate, 0, 8, 0);

//    val = get_config("sector");
//    if(val) {
//        base_sector = atoi(val);
//    }
//    else base_sector = 6;
//    val = get_config("seedkey");
//    if(val) {
//        memcpy(aes_seed_keya,val,16);		//KEY_A seed key
//    }
//    else
//        memcpy(aes_seed_keya,"hellowGoiot2018",16);		//KEY_A seed key
//    MT_LOG("seedkey:%s, basesector:%d",aes_seed_keya,base_sector);
//    return 0;
//}
