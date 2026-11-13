#ifndef BOOT_CONTROL_H
#define BOOT_CONTROL_H
#include <stdbool.h>
/* 上电在seed和可选暂存之后、跳转之前调用一次。
 * NORMAL校验内部App；升级请求提交READY再INSTALLING；READY/INSTALLING自动安装。
 * true允许main继续基本入口检查及跳转，false必须停留Bootloader。
 * 请求只在RAM，但READY/INSTALLING/NORMAL通过状态模块持久保存。
 * 本版固件仍固定V2小包，尚未接网络，不是正式时钟通用安装器。 */
bool boot_control_process(void);
#endif
