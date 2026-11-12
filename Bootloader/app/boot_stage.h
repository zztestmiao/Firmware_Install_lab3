/*
 * 文件职责：暂存及加载接口
 * 调用关系：process由main调用，load由boot_install调用。
 * 阅读重点：load输出的是静态RAM缓冲区内的载荷指针，下一次加载会覆盖，不是W25Q可直接执行地址。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#ifndef BOOT_STAGE_H
#define BOOT_STAGE_H
#include <stdbool.h>
#include <stdint.h>
/* 启动阶段单次调用，默认不操作。调试器在调用前设置g_stage_request=1。
 * 将擦除第一颗最后一个扇区、写入本地V2包并读回校验。
 * true表示无请求或暂存成功，false表示失败应停留Bootloader。
 * 不修改内部App、不持久化READY；不能在中断或并发SPI环境调用。 */
bool boot_stage_process(void);
/* 只读第一颗W25Q暂存包并重新完整验证，不擦写。
 * 成功输出静态RAM载荷指针、长度、CRC，指针有效到下次暂存/加载。
 * 参数须有效，失败不保证旧输出可用；调用后关闭SPI，不提交READY。 */
bool boot_stage_load(const uint8_t **image, uint32_t *length, uint32_t *crc);

#endif
