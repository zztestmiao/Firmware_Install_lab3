/*
 * 文件职责：安装模块接口
 * 调用关系：main只需包含此头文件，不需知道擦写实现。
 * 阅读重点：true可表示没有请求，也可表示安装成功，实际完成须看g_install_result=1。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#ifndef BOOT_INSTALL_H
#define BOOT_INSTALL_H
#include <stdbool.h>
/* 启动阶段、跳转前单次调用；默认无请求则不操作。
 * 调试器在调用前设置g_install_request=1，重新校验W25Q后覆盖内部S4。
 * 成功或无请求返回true；失败返回false，必须停留Bootloader。
 * 请求仅在RAM，不等于持久READY/INSTALLING，不支持自动掉电恢复。 */
bool boot_install_process(void);
#endif
