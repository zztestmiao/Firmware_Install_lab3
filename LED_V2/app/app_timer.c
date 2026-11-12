#include "app_timer.h"
#include "stm32f4xx.h"
#include "board.h"
#include "led.h"
#include "misc.h"

void app_timer_start(void)
{
    RCC_ClocksTypeDef clocks;
    TIM_TimeBaseInitTypeDef timer;
    NVIC_InitTypeDef interrupt;
    uint32_t timer_clock;

    RCC_GetClocksFreq(&clocks);
    timer_clock = clocks.PCLK1_Frequency;
    /* F407：APB1分频不为1时，定时器时钟是APB1时钟的两倍。 */
    if (clocks.PCLK1_Frequency != clocks.HCLK_Frequency)
    {
        timer_clock *= 2u;
    }

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
    TIM_TimeBaseStructInit(&timer);
	
    /* 先分频到每秒10000次计数，再每1000次产生更新事件，约0.1秒。 */
    timer.TIM_Prescaler = (uint16_t)(timer_clock / 10000u - 1u);
    timer.TIM_Period = 1000u - 1u;
    timer.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM6, &timer);

    /* 初始化可能产生更新标志，开启中断前先清除。 */
    TIM_ClearITPendingBit(TIM6, TIM_IT_Update);//清除 TIM6 外设内部的更新中断标志
    NVIC_ClearPendingIRQ(TIM6_DAC_IRQn);//清除 NVIC 中该通道等待处理的请求
	
    interrupt.NVIC_IRQChannel = TIM6_DAC_IRQn;
    interrupt.NVIC_IRQChannelPreemptionPriority = 1;
    interrupt.NVIC_IRQChannelSubPriority = 0;
    interrupt.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&interrupt);
	
    TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM6, ENABLE);
}

/* 名字必须与启动文件中的向量表项一致，由硬件中断进入。 */
void TIM6_DAC_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM6, TIM_IT_Update);
        GPIO_ToggleBits(led1.Port, led1.Pin);
    }
}

