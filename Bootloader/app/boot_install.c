#include "boot_install.h"
#include "boot_stage.h"
#include "boot_crc.h"
#include "boot_layout.h"
#include "internal_flash.h"

#if BOOT_APP_BASE != IFLASH_DEMO_START
#error App_layout_mismatch
#endif

/* 0无请求/1安装校验通过/2外部包加载或校验失败/3超S4容量/
 * 4内部擦写失败/5内部CRC不匹配/6进行中。复位后清零，不作持久状态。
 * CRC及长度在失败前也可能被填写，必须以result=1判断完成。 */
volatile uint32_t g_install_request;
volatile uint32_t g_install_result;
volatile uint32_t g_install_bytes;
volatile uint32_t g_install_expected_crc;
volatile uint32_t g_install_actual_crc;

bool boot_install_process(void)
{
    const uint8_t *image;
    uint32_t length;
    uint32_t crc;
    if (g_install_request != 1u)
    {
        return true;
    }
    g_install_request = 0u;
    g_install_result = 6u;
    /* 只从W25Q加载，绝不在失败时偷偷改用内嵌演示包。 */
    if (!boot_stage_load(&image, &length, &crc))
    {
        g_install_result = 2u;
        return false;
    }
    if (length == 0u || length > IFLASH_DEMO_END - IFLASH_DEMO_START)
    {
        g_install_result = 3u;
        return false;
    }
    g_install_bytes = length;
    g_install_expected_crc = crc;
    if (!internal_flash_install_s4(image, length))
    {
        g_install_result = 4u;
        return false;
    }
    g_install_actual_crc = boot_crc32((const uint8_t *)BOOT_APP_BASE, length);
    if (g_install_actual_crc != crc)
    {
        g_install_result = 5u;
        return false;
    }
    g_install_result = 1u;
    return true;
}
