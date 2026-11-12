#ifndef BOOT_INSTALL_H
#define BOOT_INSTALL_H
#include <stdbool.h>
/* 启动阶段、跳转前单次调用；默认无请求则不操作。
 * 调试器在调用前设置g_install_request=1，重新校验W25Q后覆盖内部S4。
 * 成功或无请求返回true；失败返回false，必须停留Bootloader。
 * 请求仅在RAM，不等于持久READY/INSTALLING，不支持自动掉电恢复。 */
bool boot_install_process(void);
#endif
