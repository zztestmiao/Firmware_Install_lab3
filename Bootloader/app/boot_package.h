/*
 * 文件职责：固件包字节格式
 * 调用关系：包头32字节，后面才是原始BIN；整数按小端序解码。
 * 阅读重点：包头不是App代码，安装时必须跳过。CRC只检测损坏，不认证来源。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#ifndef BOOT_PACKAGE_H
#define BOOT_PACKAGE_H
#include <stdint.h>

/* 当前LED教学包格式，逐字段解码，不直接把文件强转为结构体。
 * 后续W25Q读取和安装模块共用；包头CRC覆盖前28字节。
 * payload_crc覆盖完整原始BIN，不含32字节包头。 */
#define BOOT_PACKAGE_MAGIC 0x3141544Fu
#define BOOT_PACKAGE_FORMAT 1u
#define BOOT_PACKAGE_HEADER_SIZE 32u
#define BOOT_PACKAGE_LENGTH_OFFSET 16u
#define BOOT_PACKAGE_IMAGE_CRC_OFFSET 20u
#define BOOT_PACKAGE_HEADER_CRC_OFFSET 28u

#endif
