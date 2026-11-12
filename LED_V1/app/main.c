#include "stm32f4xx.h"
#include "app_init.h"
#include "app_timer.h"

int main(void)
{
    app_hardware_init();
    app_timer_start();

    /* 主循环不再控制灯；LED1亮灭只能由TIM6中断触发。 */
    while (1)
    {
        __NOP();
    }
}
