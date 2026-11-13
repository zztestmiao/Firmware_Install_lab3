/*
 * 文件职责：程序入口与执行顺序
 * 调用关系：先初始化和探测，再首次迁移/可选暂存，最后状态驱动安装与校验、跳转App。
 * 阅读重点：只组织顺序，不实现Flash细节。暂存/安装失败停留本程序，不能继续启动可能无效的App。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#include "stm32f4xx.h"
#include "boot_board.h"
#include "boot_jump.h"
#include "boot_stage.h"
#include "boot_control.h"
#include "boot_state.h"
#include "boot_state_seed.h"

int main(void)
{
    boot_board_init();
    boot_board_show_startup();
    /* 只读探测状态候选区，不改变当前安装或跳转决策。 */
    boot_state_probe();
    /* 仅首次迁移手动请求，默认不擦状态区。 */
    if (!boot_state_seed_process())
    {
        while (1)
        {
            __NOP();
        }
    }

    /* 在本行设置断点，Watch手动写g_stage_request=1再运行。
     * 失败停留Bootloader，成功后继续处理独立安装请求，默认安装请求也为0。 */
    if (!boot_stage_process())
    {
        while (1)
        {
            __NOP();
        }
    }
    /* 状态控制入口：手动升级使用g_upgrade_request；READY/INSTALLING无需请求自动处理。 */
    if (!boot_control_process())
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
