/* 内部状态只读后端。S2/S3地址只在此文件出现，未来替换后端不泄露给main。
 * 槽内布局为12个小端uint32：magic/格式/序号/状态/版本/镜像长度/
 * 镜像CRC/包长度/包CRC/保留0/前40字节CRC/最后提交字。
 * 支持交替提交，但安装状态机尚未接入，不能宣称已具备自动掉电恢复。 */
#include "boot_state.h"
#include "boot_crc.h"
#include <stdbool.h>
#include "stm32f4xx.h"

#define STATE_MAGIC 0x31545342u
#define STATE_COMMIT 0xA55A3CC3u

volatile uint32_t g_state_probe_result;
volatile uint32_t g_state_value;
volatile uint32_t g_state_sequence;
volatile uint32_t g_state_slot;

static bool state_valid(const uint32_t *words)
{
    return words[0] == STATE_MAGIC && words[1] == 1u &&
        words[2] != 0u && words[2] != 0xFFFFFFFFu &&
        words[3] >= BOOT_STATE_NORMAL && words[3] <= BOOT_STATE_INSTALLING &&
        words[5] >= 8u && words[5] <= 0x10000u &&
        words[7] == words[5] + 32u && words[7] <= 4096u &&
        words[9] == 0u && words[11] == STATE_COMMIT &&
        boot_crc32((const uint8_t *)words, 40u) == words[10];
}

boot_state_scan_result boot_state_read(boot_state_info *out)
{
    uint32_t records[2][12];
    const uint32_t addresses[2] = {0x08008000u, 0x0800C000u};
    bool valid[2];
    bool all_blank = true;
    uint32_t slot;
    uint32_t i;
    uint32_t selected;
    if (out == 0)
        return BOOT_STATE_INVALID;
    for (slot = 0u; slot < 2u; ++slot)
    {
        for (i = 0u; i < 12u; ++i)
            records[slot][i] = *(volatile uint32_t *)(addresses[slot] + i * 4u);
        valid[slot] = state_valid(records[slot]);
        /* EMPTY表示两个完整扇区都FF，不能只看记录头推断无其他数据。 */
        for (i = 0u; i < 0x4000u; i += 4u)
        {
            if (*(volatile uint32_t *)(addresses[slot] + i) != 0xFFFFFFFFu)
                all_blank = false;
        }
    }
    if (!valid[0] && !valid[1])
        return all_blank ? BOOT_STATE_EMPTY : BOOT_STATE_INVALID;
    if (valid[0] && valid[1] && records[0][2] == records[1][2])
    {
        for (i = 0u; i < 12u; ++i)
        {
            if (records[0][i] != records[1][i])
                return BOOT_STATE_CONFLICT;
        }
    }
    /* 教学协议不允许序号回绕，提交模块未来到达上限必须拒绝。 */
    selected = (!valid[0] || (valid[1] && records[1][2] > records[0][2])) ? 1u : 0u;
    out->sequence = records[selected][2];
    out->state = (boot_state_value)records[selected][3];
    out->version = records[selected][4];
    out->image_size = records[selected][5];
    out->image_crc = records[selected][6];
    out->package_size = records[selected][7];
    out->package_crc = records[selected][8];
    out->slot = selected;
    return BOOT_STATE_VALID;
}

void boot_state_probe(void)
{
    boot_state_info info;
    boot_state_scan_result result = boot_state_read(&info);
    if (result == BOOT_STATE_VALID)
    {
        g_state_value = (uint32_t)info.state;
        g_state_sequence = info.sequence;
        g_state_slot = info.slot;
    }
    g_state_probe_result = (uint32_t)result;
}

/* 具体Flash地址和擦除实现不公开给上层，便于未来换成外部后端。 */
bool boot_state_commit(const boot_state_info *info, bool first_init)
{
    boot_state_info current;
    boot_state_info readback;
    boot_state_scan_result scan;
    uint32_t words[12];
    uint32_t slot;
    uint32_t address;
    uint32_t sector;
    uint32_t sequence;
    uint32_t i;
    uint32_t mask;
    uint32_t cache;
    bool success = false;

    if (info == 0)
        return false;
    scan = boot_state_read(&current);
    if (scan == BOOT_STATE_CONFLICT)
        return false;
    if (scan == BOOT_STATE_VALID)
    {
        if (current.sequence >= 0xFFFFFFFEu)
            return false;
        slot = 1u - current.slot;
        sequence = current.sequence + 1u;
    }
    else
    {
        if (!first_init)
            return false;
        slot = 0u;
        sequence = 1u;
    }
    words[0] = STATE_MAGIC;
    words[1] = 1u;
    words[2] = sequence;
    words[3] = (uint32_t)info->state;
    words[4] = info->version;
    words[5] = info->image_size;
    words[6] = info->image_crc;
    words[7] = info->package_size;
    words[8] = info->package_crc;
    words[9] = 0u;
    words[10] = boot_crc32((const uint8_t *)words, 40u);
    words[11] = STATE_COMMIT;
    if (!state_valid(words))
        return false;
    address = (slot == 0u) ? 0x08008000u : 0x0800C000u;
    sector = (slot == 0u) ? FLASH_Sector_2 : FLASH_Sector_3;
    mask = __get_PRIMASK();
    __disable_irq();
    cache = FLASH->ACR;
    FLASH_InstructionCacheCmd(DISABLE);
    FLASH_DataCacheCmd(DISABLE);
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    if (FLASH_EraseSector(sector, VoltageRange_3) != FLASH_COMPLETE)
        goto finish;
    /* 先写并读回正文和CRC，提交字保持擦除态。 */
    for (i = 0u; i < 11u; ++i)
    {
        if (FLASH_ProgramWord(address + i * 4u, words[i]) != FLASH_COMPLETE ||
            *(volatile uint32_t *)(address + i * 4u) != words[i])
            goto finish;
    }
    /* 最后一个独立字表示记录可使用，中途断电的未提交记录会被忽略。 */
    if (FLASH_ProgramWord(address + 44u, STATE_COMMIT) != FLASH_COMPLETE ||
        *(volatile uint32_t *)(address + 44u) != STATE_COMMIT)
        goto finish;
    success = true;
finish:
    FLASH_Lock();
    FLASH_InstructionCacheReset();
    FLASH_DataCacheReset();
    if ((cache & FLASH_ACR_ICEN) != 0u)
        FLASH_InstructionCacheCmd(ENABLE);
    if ((cache & FLASH_ACR_DCEN) != 0u)
        FLASH_DataCacheCmd(ENABLE);
    __DSB();
    __ISB();
    __set_PRIMASK(mask);
    if (!success || boot_state_read(&readback) != BOOT_STATE_VALID)
        return false;
    return readback.sequence == sequence && readback.slot == slot &&
        readback.state == info->state && readback.version == info->version &&
        readback.image_size == info->image_size && readback.image_crc == info->image_crc &&
        readback.package_size == info->package_size && readback.package_crc == info->package_crc;
}
