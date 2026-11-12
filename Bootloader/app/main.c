#include "stm32f4xx.h"
#include "board.h"
#include "led.h"
#include "misc.h"

#define APP_BASE  0x08010000u
#define APP_END   0x08040000u

/* ARMCC5 汇编函数：切换栈后直接跳转，避免再访问旧的函数栈。 */
__asm void boot_enter_app(uint32_t stack_top, uint32_t reset_entry)
{
    MSR MSP, r0
    MOVS r2, #0
    MSR CONTROL, r2
    ISB
    CPSIE i
    BX r1
}

static void boot_jump_to_app(void)
{
    /* 向量表第一个字是初始栈顶，第二个字是复位处理函数入口。 */
    uint32_t stack_top = *(volatile uint32_t *)APP_BASE;
    uint32_t reset_entry = *(volatile uint32_t *)(APP_BASE + 4u);
    uint32_t code_address = reset_entry & ~1u;

    /* 最小合法性检查：本实验只使用主 SRAM，不包含完整固件校验。 */
    if (stack_top <= 0x20000000u || stack_top > 0x20020000u ||
        (stack_top & 7u) != 0u || (reset_entry & 1u) == 0u ||
        code_address < APP_BASE || code_address >= APP_END)
    {
        led_on(&led3);  /* 应用入口无效：LED3 常亮，不执行跳转。 */
        return;
    }
	
	//暂时屏蔽通常的可屏蔽中断
    __disable_irq();
	
	//停止并清理 SysTick 计数器
    SysTick->CTRL = 0u;
    SysTick->LOAD = 0u;
    SysTick->VAL = 0u;
	
    /* 使用 CMSIS 接口关闭并清除本芯片的外部中断。 */
    for (uint32_t i = 0; i <= (uint32_t)FPU_IRQn; ++i)
    {
        NVIC_DisableIRQ((IRQn_Type)i);//关闭该中断通道的响应许可
        NVIC_ClearPendingIRQ((IRQn_Type)i);//清除该通道已经挂起、等待处理的请求
    }
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;//清除 SysTick 和 PendSV 的挂起请求

    /* 先切回内部时钟，再停用 PLL，便于应用重新初始化时钟。 */
	/* 开启内部高速时钟，等待稳定 */
    RCC_HSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET) {}
	/* 将系统时钟切换到内部高速时钟 */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    while (RCC_GetSYSCLKSource() != 0x00u) {}
	/* 关闭PLL，等待关闭完成 */
    RCC_PLLCmd(DISABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) != RESET) {}

    /* 设置app应用的向量表；仅设置此寄存器不会自动启动应用。 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, APP_BASE - FLASH_BASE);
    __DSB();//确保前面的相关内存访问完成。
    __ISB();//同步后续指令执行，使处理器按更新后的环境继续运行。
    boot_enter_app(stack_top, reset_entry);
}

/* 用于观察灯光的粗略延时，不作为精确计时依据。 */
static void boot_delay(void)
{
    for (volatile uint32_t i = 0; i < 8000000u; ++i)
    {
        __NOP();
    }
}

int main(void)
{
    board_lowlevel_init();
    led_init(&led1);
    led_init(&led2);
    led_init(&led3);
    led_off(&led1);
    led_off(&led2);
    led_off(&led3);

    /* 引导程序启动提示：LED2 闪烁三次。 */
    for (uint32_t n = 0; n < 3u; ++n)
    {
        led_on(&led2);
        boot_delay();
        led_off(&led2);
        boot_delay();
    }

    /* 提示结束后启动闪灯应用；若入口无效，则返回并停在下面。 */
    boot_jump_to_app();
    while (1)
    {
        __NOP();
    }
}
