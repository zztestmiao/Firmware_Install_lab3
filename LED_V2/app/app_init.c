#include "app_init.h"
#include "board.h"
#include "led.h"

/* 仅准备板级和LED，先于定时器启动。 */
void app_hardware_init(void)
{
    board_lowlevel_init();
    led_init(&led1);
    led_init(&led2);
    led_init(&led3);
    led_off(&led1);
    led_off(&led2);
    led_off(&led3);
}
