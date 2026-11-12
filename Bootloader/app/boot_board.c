#include "boot_board.h"
#include "stm32f4xx.h"
#include "board.h"
#include "led.h"

/* 用于观察灯光的粗略延时，不作为精确计时依据。 */
static void boot_delay(void)
{
    for (volatile uint32_t i = 0; i < 8000000u; ++i)
    {
        __NOP();
    }
}

/* 上电首先准备硬件，不擦写Flash、不创建中断。 */
void boot_board_init(void)
{
    board_lowlevel_init();
    led_init(&led1);
    led_init(&led2);
    led_init(&led3);
    led_off(&led1);
    led_off(&led2);
    led_off(&led3);

}

/* 硬件初始化后显示启动提示，使用粗略阻塞延时。 */
void boot_board_show_startup(void)
{
    /* 引导程序启动提示：LED2 闪烁三次。 */
    for (uint32_t n = 0; n < 3u; ++n)
    {
        led_on(&led2);
        boot_delay();
        led_off(&led2);
        boot_delay();
    }

}
