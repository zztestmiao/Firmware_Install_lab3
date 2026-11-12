/*
 * 文件职责：CRC32计算
 * 调用关系：暂存模块验证包头和载荷，安装模块验证内部App。
 * 阅读重点：纯软件按字节计算，不修改输入，不访问外设。参数中的长度不包含字符串结尾等额外字节。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#include "boot_crc.h"

/* 反射多项式EDB88320，初值FFFFFFFF，结束异或FFFFFFFF。
 * 按字节计算，不使用STM32默认硬件CRC配置，避免算法不一致。
 * 无外设副作用，可处理任意字节长度；调用者负责指针有效性。 */
uint32_t boot_crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i;
    uint32_t bit;
    for (i = 0u; i < length; ++i)
    {
        crc ^= data[i];
        for (bit = 0u; bit < 8u; ++bit)
        {
            crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}
