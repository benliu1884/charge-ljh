Readme

##充电桩号
Flash的地址：0x1FFF0 ，最大16个字节
代码中默认是关闭刷卡桩号检测的，如需打开，请在mainTask.c文件中，取消451行的注释
450    if ( ret == CR_SUCESS ) {
451        // if (strcmp(dev_sn, card.userid) == 0)
452        if (1)
453        {




