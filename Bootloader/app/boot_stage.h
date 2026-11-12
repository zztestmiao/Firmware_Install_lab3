#ifndef BOOT_STAGE_H
#define BOOT_STAGE_H
#include <stdbool.h>
/* 启动阶段单次调用，默认不操作。调试器在调用前设置g_stage_request=1。
 * 将擦除第一颗最后一个扇区、写入本地V2包并读回校验。
 * true表示无请求或暂存成功，false表示失败应停留Bootloader。
 * 不修改内部App、不持久化READY；不能在中断或并发SPI环境调用。 */
bool boot_stage_process(void);
#endif
