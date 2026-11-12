#include "stm32f4xx.h"
#include "boot_board.h"
#include "boot_jump.h"
#include "boot_stage.h"

int main(void)
{
    boot_board_init();
    boot_board_show_startup();

    /* 在本行设置断点，Watch手动写g_stage_request=1再运行。
     * 失败停留Bootloader，成功后仍启动内部V1，不执行安装。 */
    if (!boot_stage_process())
    {
        while (1)
        {
            __NOP();
        }
    }
    boot_jump_to_app();

    /* 无有效App时跳转函数返回，保留错误提示并等待处理。 */
    while (1)
    {
        __NOP();
    }
}
