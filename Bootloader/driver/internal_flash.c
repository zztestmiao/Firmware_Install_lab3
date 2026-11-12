/*
 * 文件职责：内部S4受限擦写驱动
 * 调用关系：仅由安装模块在校验源固件后调用，输入来自主SRAM的BIN。
 * 阅读重点：固定擦整个64KiB S4，按4字节写入并读回。必须稳定供电，当前标准库等待无精确超时。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#include "internal_flash.h"
#include "stm32f4xx.h"

bool internal_flash_install_s4(const uint8_t *image, uint32_t length)
{
    uint32_t source = (uint32_t)image;
    uint32_t offset;
    uint32_t i;
    uint32_t word;
    uint32_t mask;
    uint32_t cache;
    bool success = false;

    /* 所有检查在解锁和擦除前完成，容量减法避免加法回绕。
     * 强制源在主SRAM，不能从待擦App中取数据。 */
    if (image == 0 || length == 0u || length > IFLASH_DEMO_END - IFLASH_DEMO_START ||
        source < 0x20000000u || source >= 0x20020000u || length > 0x20020000u - source)
    {
        return false;
    }
    /* 保存原中断屏蔽状态，结束时原样恢复，而非无条件开中断。 */
    mask = __get_PRIMASK();
    __disable_irq();
    /* 保存缓存使能状态，避免擦写后读到旧App缓存；它不是数据备份。 */
    cache = FLASH->ACR;
    FLASH_InstructionCacheCmd(DISABLE);
    FLASH_DataCacheCmd(DISABLE);
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    /* F407VE同Bank操作时取指可能等待硬件，调用位置必须在Bootloader。
     * 标准库内部等待不带时间期限，本轮不宣称超时或看门狗恢复。 */
    if (FLASH_EraseSector(FLASH_Sector_4, VoltageRange_3) != FLASH_COMPLETE)
    {
        goto finish;
    }
    /* 每轮构造一个小端32位字并写到目的地址，offset按4字节递增。 */
    for (offset = 0u; offset < length; offset += 4u)
    {
        word = 0xFFFFFFFFu;
        /* 最后一字不足4字节时只从源取有效数据，剩余位保持FF。 */
        for (i = 0u; i < 4u && i < length - offset; ++i)
        {
            /* 先清除当前字节槽，再把源字节放入对应8位，其他字节不变。 */
            word &= ~(0xFFu << (i * 8u));
            word |= (uint32_t)image[offset + i] << (i * 8u);
        }
        if (FLASH_ProgramWord(IFLASH_DEMO_START + offset, word) != FLASH_COMPLETE ||
            *(volatile uint32_t *)(IFLASH_DEMO_START + offset) != word)
        {
            goto finish;
        }
    }
    success = true;
/* goto统一收尾：只在解锁后的路径使用，所有成功/失败都重新上锁。 */
finish:
    FLASH_Lock();
    /* 清除旧App可能残留的Flash指令/数据缓存，再恢复原来的使能状态。 */
    FLASH_InstructionCacheReset();
    FLASH_DataCacheReset();
    if ((cache & FLASH_ACR_ICEN) != 0u)
        FLASH_InstructionCacheCmd(ENABLE);
    if ((cache & FLASH_ACR_DCEN) != 0u)
        FLASH_DataCacheCmd(ENABLE);
    __DSB();
    __ISB();
    __set_PRIMASK(mask);
    return success;
}
