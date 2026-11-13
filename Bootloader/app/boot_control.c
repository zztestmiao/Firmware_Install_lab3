/* 状态控制层：只使用逻辑记录，不接触S2/S3物理地址。
 * 擦App前必须确认INSTALLING提交成功；正常完成后再提交NORMAL。
 * 提交失败即停止，不假定未生效、不盲目重试。 */
#include "boot_control.h"
#include "boot_state.h"
#include "boot_stage.h"
#include "boot_install.h"
#include "boot_crc.h"
#include "boot_layout.h"

volatile uint32_t g_upgrade_request;
/* result：0未完成/1 NORMAL检查通过无需安装/2安装及NORMAL提交完成/
 * 3无有效状态/4内部App CRC不符/5外部包失败或与记录不符/
 * 6状态提交或读回失败/7安装失败/8未知状态。 */
volatile uint32_t g_boot_result;
volatile uint32_t g_boot_phase;
volatile uint32_t g_boot_start_sequence;
volatile uint32_t g_boot_end_sequence;
/* 下轮中断恢复实验使用，默认0。1时在INSTALLING已提交、擦App前等待。
 * 这是教学暂停点，不是状态本身，不在当前正常实验中设置。 */
volatile uint32_t g_boot_hold_after_installing;

/* 原子状态提交后立即重读最新记录，后续只基于持久内容决策。 */
static bool control_commit(boot_state_info *info, boot_state_value state)
{
    info->state = state;
    if (!boot_state_commit(info, false) || boot_state_read(info) != BOOT_STATE_VALID ||
        info->state != state)
    {
        return false;
    }
    g_boot_phase = (uint32_t)info->state;
    g_boot_end_sequence = info->sequence;
    boot_state_probe();
    return true;
}

bool boot_control_process(void)
{
    boot_state_info info;
    const uint8_t *image;
    uint32_t length;
    uint32_t crc;
    uint32_t package_crc;
    bool requested = (g_upgrade_request == 1u);
    g_upgrade_request = 0u;
    if (boot_state_read(&info) != BOOT_STATE_VALID)
    {
        g_boot_result = 3u;
        return false;
    }
    g_boot_phase = (uint32_t)info.state;
    g_boot_start_sequence = info.sequence;
    g_boot_end_sequence = info.sequence;
    if (info.state == BOOT_STATE_NORMAL)
    {
        /* 正常启动不依赖W25Q镜像存在，但内部内容必须与NORMAL记录一致。 */
        if (info.image_size < 8u || info.image_size > BOOT_APP_END - BOOT_APP_BASE ||
            boot_crc32((const uint8_t *)BOOT_APP_BASE, info.image_size) != info.image_crc)
        {
            g_boot_result = 4u;
            return false;
        }
        if (!requested)
        {
            g_boot_result = 1u;
            return true;
        }
        if (!boot_stage_load(&image, &length, &crc))
        {
            g_boot_result = 5u;
            return false;
        }
        /* 当前load只接受教学V2，将来版本信息须由包解析接口输出。 */
        info.version = 2u;
        info.image_size = length;
        info.image_crc = crc;
        info.package_size = length + 32u;
        info.package_crc = boot_crc32(image - 32u, info.package_size);
        if (!control_commit(&info, BOOT_STATE_READY))
        {
            g_boot_result = 6u;
            return false;
        }
    }
    if (info.state != BOOT_STATE_READY && info.state != BOOT_STATE_INSTALLING)
    {
        g_boot_result = 8u;
        return false;
    }
    /* 无论本次刚提交READY，还是复位读到READY/INSTALLING，都重新验源。
     * INSTALLING期间内部App不可信，绝不因入口看似合法而直接启动。 */
    if (!boot_stage_load(&image, &length, &crc))
    {
        g_boot_result = 5u;
        return false;
    }
    package_crc = boot_crc32(image - 32u, length + 32u);
    if (info.version != 2u || length != info.image_size || crc != info.image_crc ||
        info.package_size != length + 32u || package_crc != info.package_crc)
    {
        g_boot_result = 5u;
        return false;
    }
    if (info.state == BOOT_STATE_READY && !control_commit(&info, BOOT_STATE_INSTALLING))
    {
        g_boot_result = 6u;
        return false;
    }
    while (g_boot_hold_after_installing != 0u)
    {
        /* 可控暂停点：此时INSTALLING在Flash，尚未擦App。
         * 下次复位变量清零，将从完整外部包重新安装。 */
    }
    if (!boot_install_verified(image, length, crc))
    {
        g_boot_result = 7u;
        return false;
    }
    if (!control_commit(&info, BOOT_STATE_NORMAL))
    {
        g_boot_result = 6u;
        return false;
    }
    g_boot_result = 2u;
    return true;
}
