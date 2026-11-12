/*
 * 文件职责：硬件模块公开接口
 * 调用关系：main包含此文件调用初始化和提示。
 * 阅读重点：声明不是实现，不会因包含头文件自动执行。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#ifndef BOOT_BOARD_H
#define BOOT_BOARD_H

/* 上电调用一次，初始化板级资源和三颗LED并关闭LED。 */
void boot_board_init(void);

/* 初始化后调用，LED2闪三次，延时只作肉眼提示，非超时计时源。 */
void boot_board_show_startup(void);

#endif
