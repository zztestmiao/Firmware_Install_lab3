/*
 * 文件职责：安装流程组织
 * 调用关系：main在跳转前调用：重新加载校验外部包，调用内部驱动，再计算内部CRC。
 * 阅读重点：request=1会覆盖旧App；不清除W25Q副本。默认0不安装，尚无持久INSTALLING或自动掉电恢复。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

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
    /* image指向RAM中的纯BIN（已跳过包头），length是BIN字节数，crc来自已验证包头。 */
    const uint8_t *image;
    uint32_t length;
    uint32_t crc;
    /* 只在调用前由调试器请求时安装。返回true不代表本次一定执行过擦写。 */
    if (g_install_request != 1u)
    {
        return true;
    }
    /* 先消费请求，再标记进行中，避免本次流程重复使用同一个请求。 */
    g_install_request = 0u;
    g_install_result = 6u;
    /* 只从W25Q加载，绝不在失败时偷偷改用内嵌演示包。 */
    if (!boot_stage_load(&image, &length, &crc))
    {
        g_install_result = 2u;
        return false;
    }
    /* 源包虽然允许192KiB App，本版驱动仅支持S4的64KiB，擦除前再收紧范围。 */
    if (length == 0u || length > IFLASH_DEMO_END - IFLASH_DEMO_START)
    {
        g_install_result = 3u;
        return false;
    }
    g_install_bytes = length;
    g_install_expected_crc = crc;
    /* 从这一调用开始会破坏旧V1。失败必须返回false，让main停留Bootloader。 */
    if (!internal_flash_install_s4(image, length))
    {
        g_install_result = 4u;
        return false;
    }
    /* 对内部Flash的实际字节重新计算CRC，不能使用RAM源镜像冒充安装结果。 */
    g_install_actual_crc = boot_crc32((const uint8_t *)BOOT_APP_BASE, length);
    if (g_install_actual_crc != crc)
    {
        g_install_result = 5u;
        return false;
    }
    g_install_result = 1u;
    return true;
}
