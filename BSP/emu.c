#include "emu.h"
#include "charger.h"
#include "cmu.h"
#include "delay.h"

EMUC emu_ecr_reg;

//交表寄存器的校验和
uint32_t emu_var_checksum;

uint32_t ep_old;


static uint32_t Read_EPADR( uint16_t address );
static uint16_t Read_ECADR( uint16_t address );
static void     Write_ECADR( uint16_t address, uint16_t data );
static void     emu_var_cal( void );
static void     get_emu_var_default( void );

extern uint8_t charger_init;
void           EMU_IRQHandler( void )
{
    if ( ( HT_EMUECA->EMUIF & 0x0100 ) != 0 ) {
        HT_EMUECA->EMUIF &= ~( 0x0100 );
        // HT_EMUECA->EMUIF = 0;
        if ( !charger_init )
            return;
        uint16_t Ims2 = ReadRMSI( 2 );
        if ( Ims2 > 27 ) {
            charger.gun[ 0 ].faultState.BIT.fault_leakage = 1;
            //停止充电
            StopCharger( 1, charger.gun[ 0 ].faultState.fault );
        }
    }
}

void EMU_Init( void )
{
    EnWr_WPREG();

    HT_GPIOF->IOCFG |= 0x0001;

    if ( ( HT_PMU->PMUSTA & 0x01 ) != 0 ) {
        HT_CMU->CLKCTRL0 |= 0x8000;  // enable EMU

        //加载默认参数
        get_emu_var_default();

        //写参数到寄存器
        emu_var_cal();

        HT_EMUECA->EMUIF = 0x0000;

        ep_old = HT_EMUEPA->ENERGYP;

        NVIC_ClearPendingIRQ( EMU_IRQn );
        NVIC_SetPriority( EMU_IRQn, 3 );
        NVIC_EnableIRQ( EMU_IRQn );
        emu_var_checksum = Read_EPADR( EPR_Checksum );
    }
}

//实时对比交表寄存器校验和
void EMU_Proc( void )
{
    if ( Read_EPADR( EPR_Checksum ) != emu_var_checksum ) {
        Feed_WDT();
        //写参数到寄存器
        emu_var_cal();
        emu_var_checksum = Read_EPADR( EPR_Checksum );
    }
}

void emu_var_cal( void )
{
    Write_ECADR( VAR_EMUSR, emu_ecr_reg.ECR.EMUSR );
    Write_ECADR( VAR_EMUIE, emu_ecr_reg.ECR.EMUIE );
    Write_ECADR( VAR_EMUIF, emu_ecr_reg.ECR.EMUIF );
    Write_ECADR( VAR_GP1, emu_ecr_reg.ECR.GP1 );
    Write_ECADR( VAR_GQ1, emu_ecr_reg.ECR.GQ1 );
    Write_ECADR( VAR_GS1, emu_ecr_reg.ECR.GS1 );
    Write_ECADR( VAR_GPhs1, emu_ecr_reg.ECR.GPhs1 );
    Write_ECADR( VAR_GP2, emu_ecr_reg.ECR.GP2 );
    Write_ECADR( VAR_GQ2, emu_ecr_reg.ECR.GQ2 );
    Write_ECADR( VAR_GS2, emu_ecr_reg.ECR.GS2 );
    Write_ECADR( VAR_GPhs2, emu_ecr_reg.ECR.GPhs2 );
    Write_ECADR( VAR_QPhsCal, emu_ecr_reg.ECR.QPhsCal );
    Write_ECADR( VAR_I2Gain, emu_ecr_reg.ECR.I2Gain );
    Write_ECADR( VAR_I1Off, emu_ecr_reg.ECR.I1Off );
    Write_ECADR( VAR_I2Off, emu_ecr_reg.ECR.I2Off );
    Write_ECADR( VAR_UOff, emu_ecr_reg.ECR.UOff );
    Write_ECADR( VAR_PStart, emu_ecr_reg.ECR.PStart );
    Write_ECADR( VAR_QStart, emu_ecr_reg.ECR.QStart );
    Write_ECADR( VAR_SStart, emu_ecr_reg.ECR.SStart );
    Write_ECADR( VAR_HFconst, emu_ecr_reg.ECR.HFconst );
    Write_ECADR( VAR_ADCCFG, emu_ecr_reg.ECR.ADCCFG );
    Write_ECADR( VAR_CHNLCR, emu_ecr_reg.ECR.CHNLCR );
    Write_ECADR( VAR_EMCON, emu_ecr_reg.ECR.EMCON );
    Write_ECADR( VAR_PFCnt, emu_ecr_reg.ECR.PFCnt );
    Write_ECADR( VAR_QFCnt, emu_ecr_reg.ECR.QFCnt );
    Write_ECADR( VAR_SFCnt, emu_ecr_reg.ECR.SFCnt );
    Write_ECADR( VAR_ADCCON, emu_ecr_reg.ECR.ADCCON );
    Write_ECADR( VAR_IPTAMP, emu_ecr_reg.ECR.IPTAMP );
    Write_ECADR( VAR_ICHK, emu_ecr_reg.ECR.ICHK );
    Write_ECADR( VAR_EMUCTRL, emu_ecr_reg.ECR.EMUCTRL );
    Write_ECADR( VAR_P1OFFSET, emu_ecr_reg.ECR.P1OFFSET );
    Write_ECADR( VAR_P2OFFSET, emu_ecr_reg.ECR.P2OFFSET );
    Write_ECADR( VAR_Q1OFFSET, emu_ecr_reg.ECR.Q1OFFSET );
    Write_ECADR( VAR_Q2OFFSET, emu_ecr_reg.ECR.Q2OFFSET );
    Write_ECADR( VAR_I1RMSOFFSET, emu_ecr_reg.ECR.I1RMSOFFSET );
    Write_ECADR( VAR_I2RMSOFFSET, emu_ecr_reg.ECR.I2RMSOFFSET );
    Write_ECADR( VAR_URMSOFFSET, emu_ecr_reg.ECR.URMSOFFSET );
    Write_ECADR( VAR_RosiCtrl, emu_ecr_reg.ECR.RosiCtrl );
    Write_ECADR( VAR_ANA_control, emu_ecr_reg.ECR.ANA_control );
    Write_ECADR( VAR_UCONST, emu_ecr_reg.ECR.UCONST );
    Write_ECADR( VAR_LpIdleTime, emu_ecr_reg.ECR.LpIdleTime );
    Write_ECADR( VAR_USAGLVL, emu_ecr_reg.ECR.USAGLVL );
    Write_ECADR( VAR_USagCyc, emu_ecr_reg.ECR.USagCyc );
    Write_ECADR( VAR_UOVLVL, emu_ecr_reg.ECR.UOVLVL );
    Write_ECADR( VAR_OvCyc, emu_ecr_reg.ECR.OvCyc );
    Write_ECADR( VAR_IOVLVL, emu_ecr_reg.ECR.IOVLVL );
    Write_ECADR( VAR_ZXILVL, emu_ecr_reg.ECR.ZXILVL );
    Write_ECADR( VAR_PDataCpH, emu_ecr_reg.ECR.PDataCpH );
    Write_ECADR( VAR_PDataCpL, emu_ecr_reg.ECR.PDataCpL );
    Write_ECADR( VAR_QDataCpH, emu_ecr_reg.ECR.QDataCpH );
    Write_ECADR( VAR_QDataCpL, emu_ecr_reg.ECR.QDataCpL );
    Write_ECADR( VAR_SDataCpH, emu_ecr_reg.ECR.SDataCpH );
    Write_ECADR( VAR_SDataCpL, emu_ecr_reg.ECR.SDataCpL );
    Write_ECADR( VAR_FilterCtrl, emu_ecr_reg.ECR.FilterCtrl );
    Write_ECADR( VAR_TUgain, emu_ecr_reg.ECR.TUgain );
    Write_ECADR( VAR_TI1gain, emu_ecr_reg.ECR.TI1gain );
    Write_ECADR( VAR_TI2gain, emu_ecr_reg.ECR.TI2gain );
    Write_ECADR( VAR_UTCcoffA, emu_ecr_reg.ECR.UTCcoffA );
    Write_ECADR( VAR_UTCcoffB, emu_ecr_reg.ECR.UTCcoffB );
    Write_ECADR( VAR_UTCcoffC, emu_ecr_reg.ECR.UTCcoffC );
    Write_ECADR( VAR_I1TCcoffA, emu_ecr_reg.ECR.I1TCcoffA );
    Write_ECADR( VAR_I1TCcoffB, emu_ecr_reg.ECR.I1TCcoffB );
    Write_ECADR( VAR_I1TCcoffC, emu_ecr_reg.ECR.I1TCcoffC );
    Write_ECADR( VAR_I2TCcoffA, emu_ecr_reg.ECR.I2TCcoffA );
    Write_ECADR( VAR_I2TCcoffB, emu_ecr_reg.ECR.I2TCcoffB );
    Write_ECADR( VAR_I2TCcoffC, emu_ecr_reg.ECR.I2TCcoffC );
    Write_ECADR( VAR_LoadDataCp, emu_ecr_reg.ECR.LoadDataCp );
}

void get_emu_var_default( void )
{
    emu_ecr_reg.ECR.EMUSR       = Read_ECADR( VAR_EMUSR );
    emu_ecr_reg.ECR.EMUIE       = Read_ECADR( VAR_EMUIE );
    emu_ecr_reg.ECR.EMUIF       = Read_ECADR( VAR_EMUIF );
    emu_ecr_reg.ECR.GP1         = Read_ECADR( VAR_GP1 );
    emu_ecr_reg.ECR.GQ1         = Read_ECADR( VAR_GQ1 );
    emu_ecr_reg.ECR.GS1         = Read_ECADR( VAR_GS1 );
    emu_ecr_reg.ECR.GPhs1       = Read_ECADR( VAR_GPhs1 );
    emu_ecr_reg.ECR.GP2         = Read_ECADR( VAR_GP2 );
    emu_ecr_reg.ECR.GQ2         = Read_ECADR( VAR_GQ2 );
    emu_ecr_reg.ECR.GS2         = Read_ECADR( VAR_GS2 );
    emu_ecr_reg.ECR.GPhs2       = Read_ECADR( VAR_GPhs2 );
    emu_ecr_reg.ECR.QPhsCal     = Read_ECADR( VAR_QPhsCal );
    emu_ecr_reg.ECR.I2Gain      = Read_ECADR( VAR_I2Gain );
    emu_ecr_reg.ECR.I1Off       = Read_ECADR( VAR_I1Off );
    emu_ecr_reg.ECR.I2Off       = Read_ECADR( VAR_I2Off );
    emu_ecr_reg.ECR.UOff        = Read_ECADR( VAR_UOff );
    emu_ecr_reg.ECR.PStart      = Read_ECADR( VAR_PStart );
    emu_ecr_reg.ECR.QStart      = Read_ECADR( VAR_QStart );
    emu_ecr_reg.ECR.SStart      = Read_ECADR( VAR_SStart );
    emu_ecr_reg.ECR.HFconst     = Read_ECADR( VAR_HFconst );
    emu_ecr_reg.ECR.ADCCFG      = Read_ECADR( VAR_ADCCFG );
    emu_ecr_reg.ECR.CHNLCR      = Read_ECADR( VAR_CHNLCR );
    emu_ecr_reg.ECR.EMCON       = Read_ECADR( VAR_EMCON );
    emu_ecr_reg.ECR.PFCnt       = Read_ECADR( VAR_PFCnt );
    emu_ecr_reg.ECR.QFCnt       = Read_ECADR( VAR_QFCnt );
    emu_ecr_reg.ECR.SFCnt       = Read_ECADR( VAR_SFCnt );
    emu_ecr_reg.ECR.ADCCON      = Read_ECADR( VAR_ADCCON );
    emu_ecr_reg.ECR.IPTAMP      = Read_ECADR( VAR_IPTAMP );
    emu_ecr_reg.ECR.ICHK        = Read_ECADR( VAR_ICHK );
    emu_ecr_reg.ECR.EMUCTRL     = Read_ECADR( VAR_EMUCTRL );
    emu_ecr_reg.ECR.P1OFFSET    = Read_ECADR( VAR_P1OFFSET );
    emu_ecr_reg.ECR.P2OFFSET    = Read_ECADR( VAR_P2OFFSET );
    emu_ecr_reg.ECR.Q1OFFSET    = Read_ECADR( VAR_Q1OFFSET );
    emu_ecr_reg.ECR.Q2OFFSET    = Read_ECADR( VAR_Q2OFFSET );
    emu_ecr_reg.ECR.I1RMSOFFSET = Read_ECADR( VAR_I1RMSOFFSET );
    emu_ecr_reg.ECR.I2RMSOFFSET = Read_ECADR( VAR_I2RMSOFFSET );
    emu_ecr_reg.ECR.URMSOFFSET  = Read_ECADR( VAR_URMSOFFSET );
    emu_ecr_reg.ECR.RosiCtrl    = Read_ECADR( VAR_RosiCtrl );
    emu_ecr_reg.ECR.ANA_control = Read_ECADR( VAR_ANA_control );
    emu_ecr_reg.ECR.UCONST      = Read_ECADR( VAR_UCONST );
    emu_ecr_reg.ECR.LpIdleTime  = Read_ECADR( VAR_LpIdleTime );
    emu_ecr_reg.ECR.USAGLVL     = Read_ECADR( VAR_USAGLVL );
    emu_ecr_reg.ECR.USagCyc     = Read_ECADR( VAR_USagCyc );
    emu_ecr_reg.ECR.UOVLVL      = Read_ECADR( VAR_UOVLVL );
    emu_ecr_reg.ECR.OvCyc       = Read_ECADR( VAR_OvCyc );
    emu_ecr_reg.ECR.IOVLVL      = Read_ECADR( VAR_IOVLVL );
    emu_ecr_reg.ECR.ZXILVL      = Read_ECADR( VAR_ZXILVL );
    emu_ecr_reg.ECR.PDataCpH    = Read_ECADR( VAR_PDataCpH );
    emu_ecr_reg.ECR.PDataCpL    = Read_ECADR( VAR_PDataCpL );
    emu_ecr_reg.ECR.QDataCpH    = Read_ECADR( VAR_QDataCpH );
    emu_ecr_reg.ECR.QDataCpL    = Read_ECADR( VAR_QDataCpL );
    emu_ecr_reg.ECR.SDataCpH    = Read_ECADR( VAR_SDataCpH );
    emu_ecr_reg.ECR.SDataCpL    = Read_ECADR( VAR_SDataCpL );
    emu_ecr_reg.ECR.FilterCtrl  = Read_ECADR( VAR_FilterCtrl );
    emu_ecr_reg.ECR.TUgain      = Read_ECADR( VAR_TUgain );
    emu_ecr_reg.ECR.TI1gain     = Read_ECADR( VAR_TI1gain );
    emu_ecr_reg.ECR.TI2gain     = Read_ECADR( VAR_TI2gain );
    emu_ecr_reg.ECR.UTCcoffA    = Read_ECADR( VAR_UTCcoffA );
    emu_ecr_reg.ECR.UTCcoffB    = Read_ECADR( VAR_UTCcoffB );
    emu_ecr_reg.ECR.UTCcoffC    = Read_ECADR( VAR_UTCcoffC );
    emu_ecr_reg.ECR.I1TCcoffA   = Read_ECADR( VAR_I1TCcoffA );
    emu_ecr_reg.ECR.I1TCcoffB   = Read_ECADR( VAR_I1TCcoffB );
    emu_ecr_reg.ECR.I1TCcoffC   = Read_ECADR( VAR_I1TCcoffC );
    emu_ecr_reg.ECR.I2TCcoffA   = Read_ECADR( VAR_I2TCcoffA );
    emu_ecr_reg.ECR.I2TCcoffB   = Read_ECADR( VAR_I2TCcoffB );
    emu_ecr_reg.ECR.I2TCcoffC   = Read_ECADR( VAR_I2TCcoffC );
    emu_ecr_reg.ECR.LoadDataCp  = Read_ECADR( VAR_LoadDataCp );

    //	emu_ecr_reg.ECR.CHNLCR = 0x0607;
    emu_ecr_reg.ECR.CHNLCR      = 0x0E07;
    emu_ecr_reg.ECR.ADCCON      = 0x0024;
    emu_ecr_reg.ECR.EMUCTRL     = 0x03F2;
    emu_ecr_reg.ECR.ANA_control = 0xA7A6;
    emu_ecr_reg.ECR.ADCCFG      = 0x2003;  //通道1计量
    // emu_ecr_reg.ECR.ADCCFG = 0x2013;	//通道2计量
    emu_ecr_reg.ECR.RosiCtrl = 0x4000;
    emu_ecr_reg.ECR.I2Gain   = 0xff62;

    emu_ecr_reg.ECR.HFconst = 0x46;  //脉频输出设置

    emu_ecr_reg.ECR.OvCyc     = 0x04;
    emu_ecr_reg.ECR.IOVLVL    = 0x3C2;  // 0x03D;
    emu_ecr_reg.ECR.EMUIE     = 0x0100;
    emu_ecr_reg.ECR.PStart    = 0x168;
    emu_ecr_reg.ECR.QStart    = 0x168;
    emu_ecr_reg.ECR.SStart    = 0x168;
    emu_ecr_reg.ECR.Emu_Krms  = 0.123892777;     //电压系数 0.1V
    emu_ecr_reg.ECR.Emu_Kp    = 0.000478965871;  //功率系数初始化
    emu_ecr_reg.ECR.Emu_Kims  = 0.0117838653;    //电流系数  1mA
    emu_ecr_reg.ECR.Emu_Kims2 = 1.4927e-4;       //通道2电流系数

    //通道1
    emu_ecr_reg.ECR.GP1      = 0xFFB8;
    emu_ecr_reg.ECR.GQ1      = 0xFFB8;
    emu_ecr_reg.ECR.GS1      = 0xFFB8;
    emu_ecr_reg.ECR.GPhs1    = 0x96;
    emu_ecr_reg.ECR.P1OFFSET = 6;

    emu_ecr_reg.ECR.GP2      = 0x2b7;
    emu_ecr_reg.ECR.GQ2      = 0x2b7;
    emu_ecr_reg.ECR.GS2      = 0x2b7;
    emu_ecr_reg.ECR.GPhs2    = 0;
    emu_ecr_reg.ECR.P2OFFSET = 0;
}

uint8_t Get_TAMPState( void )
{
    uint32_t data = Read_EPADR( VAR_EMUSR );
    return ( uint8_t )( ( data >> 7 ) & 0x01 );
}

uint32_t Read_EPADR( uint16_t address )
{
    uint32_t* ptr;
    ptr = ( uint32_t* )( HT_EMU_EPA_BASE + address );
    return ( ( *ptr ) & 0xffffffff );
}

uint16_t Read_ECADR( uint16_t address )
{
    uint16_t* ptr;
    ptr = ( uint16_t* )( HT_EMU_ECA_BASE + address );
    return ( ( *ptr ) & 0xffff );
}

void Write_ECADR( uint16_t address, uint16_t data )
{
    uint16_t* ptr;
    ptr  = ( uint16_t* )( HT_EMU_ECA_BASE + address );
    *ptr = data;
}

//读取电压 分辨率：0.1V
uint16_t ReadRMSU( void )
{
    uint32_t temp_urms = Read_EPADR( EPR_FastRMSU );

    return temp_urms * ( emu_ecr_reg.ECR.Emu_Krms ) / 100;
}

//读取电流：分辨率1mA
uint16_t ReadRMSI( uint8_t channl )
{
    uint32_t temp_i1rms = 0;
    if ( channl == 1 ) {
        temp_i1rms = Read_EPADR( EPR_FastRMSI1 );
        temp_i1rms = temp_i1rms * ( emu_ecr_reg.ECR.Emu_Kims );
    } else if ( channl == 2 ) {
        temp_i1rms = Read_EPADR( EPR_FastRMSI2 );
        temp_i1rms = temp_i1rms * ( emu_ecr_reg.ECR.Emu_Kims2 );
    }

    return temp_i1rms;
}

uint32_t ReadPowerQ1( void )
{
    uint32_t temp_powerq;

    temp_powerq = Read_EPADR( EPR_FastPowerQ1 );

    if ( temp_powerq >= 0x80000000 ) {
        temp_powerq = 0x100000000 - temp_powerq;
    }

    return temp_powerq * ( emu_ecr_reg.ECR.Emu_Kp );
}

//读取功率：分辨率0.1W
uint32_t ReadPower( uint8_t channl )
{
    uint32_t temp_powerp;

    if ( channl == 1 ) {
        temp_powerp = Read_EPADR( EPR_FastPowerP1 );
    } else if ( channl == 2 ) {
        temp_powerp = Read_EPADR( EPR_FastPowerP2 );
    }

    if ( temp_powerp >= 0x80000000 ) {
        temp_powerp = 0x100000000 - temp_powerp;
    }

    return temp_powerp * ( emu_ecr_reg.ECR.Emu_Kp );
}

uint32_t energy_all = 0;
//读取电量：分辨率0.01kwh
uint32_t ReadEnergy(void)
{
    uint32_t cur_ep = 0, ep_delta;

    cur_ep = HT_EMUEPA->ENERGYP;
    if (cur_ep < ep_old) {
        ep_delta = cur_ep + (0x00FFFFFF - ep_old);
    } else {
        ep_delta = cur_ep - ep_old;
    }

    energy_all += ep_delta;
    ep_old = cur_ep;

    return (energy_all*100/EC_CONST);
}

/***********************************************************************************************
//功率源输入电压220.000V,电流5.000A  0.5L 信号(功率源稳定即可，不需要要求功率源准确的 60 度夹角)
//标准表读取分相有功功率 Preal、无功功率值 Qreal 及电表常数 EC
//计量校表功率K值    Kp= 2.304*10^10/(HFConst*EC*2^31) 启动  SData_temp*0.2%
************************************************************************************************/
void Calibration_meter( void )
{
    float Data_PPtemp, Data_QQtemp, Data_I1temp, Data_Utemp;  //有功功率系数
                                                              //	float  Data_I2temp;

    float err1;
    float Preal = 5500.0f;
    float Qreal = 9526.0f;
    float Ireal = 500.0f;   // 5.00
    float Ureal = 2200.0f;  // 220.0
                            //	if(RenewStatus.ParaCalibMeters_flag)
                            //		{
                            //		Preal = RatedPara.RatedPowerP;
                            //		Qreal = RatedPara.RatedPowerQ;
                            //		Ireal = RatedPara.RatedCurrent;
                            //		Ureal = RatedPara.RatedVoltage;
                            //    }
    get_emu_var_default();  //设置校表参数为默认参数
    emu_ecr_reg.ECR.GP1      = 0;
    emu_ecr_reg.ECR.GQ1      = 0;
    emu_ecr_reg.ECR.GS1      = 0;
    emu_ecr_reg.ECR.GPhs1    = 0;
    emu_ecr_reg.ECR.GP2      = 0;
    emu_ecr_reg.ECR.GQ2      = 0;
    emu_ecr_reg.ECR.GS2      = 0;
    emu_ecr_reg.ECR.GPhs2    = 0;
    emu_ecr_reg.ECR.I2Gain   = 0;
    emu_ecr_reg.ECR.Emu_Krms = 1;
    emu_ecr_reg.ECR.Emu_Kims = 1;
    emu_ecr_reg.ECR.Emu_Kp   = 1;
    emu_var_cal();      //把校表参数写入到计量EMU
    Delay_mSec( 255 );  //等待1S使校表数据更新，计量参数稳定
    Delay_mSec( 255 );  //
    Delay_mSec( 255 );
    Delay_mSec( 255 );
    Feed_WDT();

    //	HFConst=2.332*Vu*Vi*10^10/(EC*Un*Ib)
    Data_Utemp  = ( ( float )Read_EPADR( EPR_RMSU ) );
    Data_I1temp = ( ( float )Read_EPADR( EPR_RMSI1 ) );
    //		Data_I2temp=((float)Read_EPADR(EPR_RMSI2));
    //		err1 = Data_I1temp/Data_I2temp - 1 ;
    //		if(err1 >=0) {emu_ecr_reg.ECR.I2Gain = (uint32_t)(err1*0x8000);}
    //		else {emu_ecr_reg.ECR.I2Gain =(uint32_t)(0x10000+err1*0x8000);}
    //		 Write_ECADR(VAR_I2Gain,emu_ecr_reg.ECR.I2Gain);
    emu_ecr_reg.ECR.HFconst = ( ( float )( 2.332 * Data_Utemp * Data_I1temp / ( 3200 * Ureal / 10 * Ireal / 100 ) ) * 1.4210854715202003717422485351563e-4 );
    //功率源输入电压220.000V,电流5.000A  0.5L 信号(功率源稳定即可，不需要要求功率源准确的 60 度夹角)
    //标准表读取分相有功功率 Preal、无功功率值 Qreal 及电表常数 EC
    //计量校表功率K值    Kp= 2.304*10^10/(HFConst*EC*2^31)
    //	  delay(0xB3f500);

    Delay_mSec( 255 );
    Delay_mSec( 255 );
    Delay_mSec( 255 );
    Delay_mSec( 255 );
    Delay_mSec( 255 );
    Delay_mSec( 255 );
    emu_ecr_reg.ECR.Emu_Kp   = ( float )( 0.03352761 / emu_ecr_reg.ECR.HFconst );   // 3位小数 kw
    emu_ecr_reg.ECR.Emu_Krms = ( float )( ( ( float )Ureal * 100 ) / Data_Utemp );  // 3位小数
    emu_ecr_reg.ECR.Emu_Kims = ( float )( ( ( float )Ireal * 10 ) / Data_I1temp );  // 3位小数
    // delay(0xf500);	//测试等待稳定
    /*1) 计算角差校正值
            θ = (realP*powQ-powP*realQ)/(realP*powP+powQ*realQ)
            如果θ ≥ 0，Gphs1 = INT[θ Χ 2 15]
            否则θ < 0，Gphs1 = INT[2 16 + θ Χ 2 15]
            将得到的 Gphs1 的值转成 HEX 值写入 Gphs1 寄存器即可		*/
    Data_PPtemp = ReadPower( 1 );
    Data_QQtemp = ReadPowerQ1();
    err1        = ( Preal * Data_QQtemp - Data_PPtemp * Qreal ) / ( Preal * Data_PPtemp + Data_QQtemp * Qreal );
    if ( err1 >= 0 ) {
        emu_ecr_reg.ECR.GPhs1 = ( uint32_t )( err1 * 0x8000 );
    } else {
        emu_ecr_reg.ECR.GPhs1 = ( uint32_t )( 0x10000 + err1 * 0x8000 );
    }

    /*	2) 计算校正角差后的功率值 P’
            PowerP ′ = PowerP + PowerQ × θ
            Pgain = Preal/PowerP'-1 如果Pgain ≥ 0，则Gp = INT[Pgain × 2 15]
                                                            否则Pgain < 0，则Gp = INT[2 16 + Pgain × 2 15]
            将得到的 GP 值转成 HEX 值写入 GP1、GQ1 和 GS1 即可。*/
    Data_PPtemp = Data_PPtemp + Data_QQtemp * err1;
    err1        = Preal / Data_PPtemp - 1;
    if ( err1 >= 0 ) {
        emu_ecr_reg.ECR.GP1 = ( uint32_t )( err1 * 0x8000 );
    } else {
        emu_ecr_reg.ECR.GP1 = ( uint32_t )( 0x10000 + err1 * 0x8000 );
    }
    emu_ecr_reg.ECR.GQ1 = emu_ecr_reg.ECR.GS1 = emu_ecr_reg.ECR.GP1;
    /*1) 计算角差校正值
            θ = (realP*powQ-powP*realQ)/(realP*powP+powQ*realQ)
            如果θ ≥ 0，Gphs1 = INT[θ Χ 2 15]
            否则θ < 0，Gphs1 = INT[2 16 + θ Χ 2 15]
            将得到的 Gphs1 的值转成 HEX 值写入 Gphs1 寄存器即可		*/
    //		Data_PPtemp = ReadPowerP2();
    //		Data_QQtemp = ReadPowerQ2();
    //		err1= (Preal*Data_QQtemp-Data_PPtemp*Qreal)/(Preal*Data_PPtemp+Data_QQtemp*Qreal);
    //		if(err1>=0){emu_ecr_reg.ECR.GPhs2 =(uint32_t)(err1*0x8000);}
    //		else {emu_ecr_reg.ECR.GPhs2 =(uint32_t)(0x10000+err1*0x8000);}

    /*	2) 计算校正角差后的功率值 P’
            PowerP ′ = PowerP + PowerQ × θ
            Pgain = Preal/PowerP'-1 如果Pgain ≥ 0，则Gp = INT[Pgain × 2 15]
                                                            否则Pgain < 0，则Gp = INT[2 16 + Pgain × 2 15]
            将得到的 GP 值转成 HEX 值写入 GP1、GQ1 和 GS1 即可。*/
    //		Data_PPtemp = Data_PPtemp+Data_QQtemp*err1;
    //		err1 = Preal/Data_PPtemp-1;
    //		if (err1>=0){emu_ecr_reg.ECR.GP2 =(uint32_t)(err1*0x8000);}
    //		else {emu_ecr_reg.ECR.GP2 =(uint32_t)(0x10000+err1*0x8000);}
    //		emu_ecr_reg.ECR.GQ2 = emu_ecr_reg.ECR.GS2 = emu_ecr_reg.ECR.GP2;

    //	1，校表结束后，输入 Ib，Un。
    //  2，读出 PowerP 的值为 32bit，取其高 24bit 为 x1，
    //  如果 x1 为正数，x2 = x1；
    //  如果 x1 为负数，取其原码为 x2；
    //  3，设写入 PStart 的值为 Y，假如要求 0.4%Ib 电表能够启动，则：
    //  Y = x 2 *0.2 %
    emu_ecr_reg.ECR.PStart      = ( ( float )Read_EPADR( EPR_PowerS1 ) / 256 * 0.002 );
    emu_ecr_reg.ECR.QStart      = emu_ecr_reg.ECR.PStart;
    emu_ecr_reg.ECR.SStart      = emu_ecr_reg.ECR.PStart;
    emu_ecr_reg.ECR.I1RMSOFFSET = 0x013;
    //		emu_ecr_reg.ECR.I2RMSOFFSET  = 0x010;
    emu_ecr_reg.ECR.URMSOFFSET = 0x012;
    emu_ecr_reg.ECR.P1OFFSET   = 0x006;
    //		emu_ecr_reg.ECR.P2OFFSET 	= 0x005;
    emu_var_cal();  //把校表参数写入到计量EMU
                    //		get_checksum((uint8_t*)&emu_ecr_reg,150);
                    //		HT_Info_PageErase(FLASH_EMU_ADDR);
                    //		HT_Info_ByteWrite((uint8_t*)&emu_ecr_reg,FLASH_EMU_ADDR,150);
    emu_var_checksum = Read_EPADR( EPR_Checksum );
}
