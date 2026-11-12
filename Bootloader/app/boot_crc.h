/*
 * 文件职责：CRC算法接口及约定
 * 调用关系：计算缓冲区指定长度的CRC，算法须与电脑打包工具一致。
 * 阅读重点：不是加密、签名或版本防回退。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#ifndef BOOT_CRC_H
#define BOOT_CRC_H
#include <stdint.h>
/* CRC-32/ISO-HDLC，与Python zlib.crc32一致。
 * data须指向至少length字节，length为0时允许空指针。
 * 仅校验数据损坏，不提供固件来源认证。 */
uint32_t boot_crc32(const uint8_t *data, uint32_t length);
#endif
