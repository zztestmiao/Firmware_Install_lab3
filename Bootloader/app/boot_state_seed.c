/* 首次迁移流程独立于存储后端。会擦S2旧内容，保留S3及App/W25Q。
 * 仅适用于已安装V2的当前教学基线，不能拿它给未知App强行标有效。 */
#include "boot_state_seed.h"
#include "boot_state.h"
#include "boot_stage.h"
#include "boot_crc.h"
#include "boot_layout.h"

volatile uint32_t g_state_init_request;
/* 0未请求/1成功/2已有有效记录或冲突/3源包或内部CRC不符/4提交失败/5进行中。 */
volatile uint32_t g_state_init_result;

bool boot_state_seed_process(void)
{
    boot_state_info info;
    boot_state_scan_result scan;
    const uint8_t *image;
    uint32_t length;
    uint32_t crc;
    if (g_state_init_request != 1u)
        return true;
    g_state_init_request = 0u;
    g_state_init_result = 5u;
    scan = boot_state_read(&info);
    if (scan == BOOT_STATE_VALID || scan == BOOT_STATE_CONFLICT)
    {
        g_state_init_result = 2u;
        return false;
    }
    if (!boot_stage_load(&image, &length, &crc) ||
        boot_crc32((const uint8_t *)BOOT_APP_BASE, length) != crc)
    {
        g_state_init_result = 3u;
        return false;
    }
    info.state = BOOT_STATE_NORMAL;
    info.version = 2u;
    info.image_size = length;
    info.image_crc = crc;
    info.package_size = length + 32u;
    info.package_crc = boot_crc32(image - 32u, length + 32u);
    /* first_init是显式首次迁移许可，未来日常状态更新必须传false。 */
    if (!boot_state_commit(&info, true))
    {
        g_state_init_result = 4u;
        return false;
    }
    boot_state_probe();
    g_state_init_result = 1u;
    return true;
}
