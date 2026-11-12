#include "stm32f4xx.h"
#include "boot_board.h"
#include "boot_jump.h"

int main(void)
{
    boot_board_init();
    boot_board_show_startup();

    /* 当前仍是启动基线。未来在此调用独立安装模块，成功校验后才跳转。
     * 不放置空壳安装接口，避免把尚未实现的能力误认为已经完成。 */
    boot_jump_to_app();

    /* 无有效App时跳转函数返回，保留错误提示并等待处理。 */
    while (1)
    {
        __NOP();
    }
}
