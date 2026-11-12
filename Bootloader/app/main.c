/*
 * 文件职责：程序入口与执行顺序
 * 调用关系：先初始化及灯光提示，再处理可选暂存、可选安装，最后跳转App。两个请求默认0。
 * 阅读重点：只组织顺序，不实现Flash细节。暂存/安装失败停留本程序，不能继续启动可能无效的App。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#include "stm32f4xx.h"
#include "boot_board.h"
#include "boot_jump.h"
#include "boot_stage.h"
#include "boot_install.h"

int main(void)
{
    boot_board_init();
    boot_board_show_startup();

    /* 在本行设置断点，Watch手动写g_stage_request=1再运行。
     * 失败停留Bootloader，成功后继续处理独立安装请求，默认安装请求也为0。 */
    if (!boot_stage_process())
    {
        while (1)
        {
            __NOP();
        }
    }
    /* 保持stage请求为0；在此断点手动触发安装。失败禁止跳入残缺App。 */
    if (!boot_install_process())
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
