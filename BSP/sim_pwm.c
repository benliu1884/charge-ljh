#include "sim_pwm.h"

static volatile uint8_t pwm_state = 1;

static void TIM_Cmd( uint8_t en );
static void Timer0_Init( void );
static void SetTIM_Prd( uint16_t prd );

void StartPwm( void )
{
    pwm_state = 1;
    TIM_Cmd( ENABLE );
}

void StopPwm( void )
{
    HT_GPIO_BitsSet( HT_GPIOB, GPIO_Pin_2 );
    TIM_Cmd( DISABLE );
}

void Timer0_Init( void )
{
    EnWr_WPREG();

    HT_CMU->CLKCTRL1 |= 0x0001;    //定时器0使能
    HT_TMR0->TMRCON |= 0x06;       //周期定时，计数器使能
    HT_TMR0->TMRDIV = 19 - 1;      //预分频器 1M
    HT_TMR0->TMRPRD = DUTY_CYCLE;  //重装值

    HT_TMR0->TMRIE = 1;

    NVIC_ClearPendingIRQ( TIMER_0_IRQn );
    NVIC_SetPriority( TIMER_0_IRQn, 0 );
    NVIC_EnableIRQ( TIMER_0_IRQn );
}

void TIM_Cmd( uint8_t en )
{
    EnWr_WPREG();

    if ( en == ENABLE ) {
        setbit( HT_TMR0->TMRCON, 0 );
    } else {
        clrbit( HT_TMR0->TMRCON, 0 );
    }
}

void SetTIM_Prd( uint16_t prd )
{
    EnWr_WPREG();
    HT_TMR0->TMRPRD = prd;
}

void SimPwm_Init( void )
{
    // PWM_OUT
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin          = GPIO_Pin_2;
    GPIO_InitStruct.GPIO_Mode         = GPIO_Mode_IOOUT;
    GPIO_InitStruct.GPIO_OutputStruct = GPIO_Output_PP;
    HT_GPIO_Init( HT_GPIOB, &GPIO_InitStruct );
    HT_GPIO_BitsSet( HT_GPIOB, GPIO_Pin_2 );

    Timer0_Init();
}

void TIMER_0_IRQHandler()
{
    if ( SET == HT_TMR_ITFlagStatusGet( HT_TMR0, TMR_TMRIF_PRDIF ) ) /*!< 周期中断           */
    {
        if ( pwm_state == 1 ) {
            pwm_state       = 0;
            HT_GPIOB->PTSET = ( uint32_t )GPIO_Pin_2;
            SetTIM_Prd( 1030 - DUTY_CYCLE );
        } else {
            pwm_state = 1;

            HT_GPIOB->PTCLR = ( uint32_t )GPIO_Pin_2;
            SetTIM_Prd( DUTY_CYCLE );
        }

        HT_TMR_ClearITPendingBit( HT_TMR0, TMR_TMRIF_PRDIF ); /*!< 清除中断标志       */
    }
}
