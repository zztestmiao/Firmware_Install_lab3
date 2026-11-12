#ifndef APP_TIMER_H
#define APP_TIMER_H
/* 初始化LED后单次调用，开启TIM6心跳。该模块独占TIM6及其中断。 */
void app_timer_start(void);
#endif
