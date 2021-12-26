Readme

##充电桩号
Flash的地址：0x1FFF0 ，最大16个字节
代码中默认是关闭刷卡桩号检测的，如需打开，请在mainTask.c文件中，取消451行的注释
450    if ( ret == CR_SUCESS ) {
451        // if (strcmp(dev_sn, card.userid) == 0)
452        if (1)
453        {


##读卡器
默认的aed_seed是：hellowGoiot2018
默认的M1起始删除扇区是：6
需要和发卡器的配置一样，可以根据实际情况修改：在gcard.c文件中，#261和262行
261 static unsigned char       aes_seed_keya[ 16 ]     = "hellowGoiot2018";  // KEY_A seed key
262 static unsigned char       base_sector             = 6;


#电流电压
为了方便测试，目前的电流、电压是模式的，可以在请在mainTask.c #162行注销
// TODO zhoumin - 测试
162 #if 1
163 charger.gun[ 0 ].meter.voltage_an = 2192;
164 charger.gun[ 0 ].meter.current_an = 12860;
165 charger.gun[ 0 ].meter.electricity = ReadEnergy();
166 #endif
