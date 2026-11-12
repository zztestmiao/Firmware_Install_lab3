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
