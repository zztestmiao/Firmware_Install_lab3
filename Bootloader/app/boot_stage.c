/*
 * 文件职责：外部固件包暂存与验证
 * 调用关系：process可手动将内嵌包写入W25Q；load只读W25Q并校验，供安装调用。
 * 阅读重点：process会擦外部最后一个4KiB扇区，load不会擦。两者均不擦内部App。结果只在RAM，不是持久READY。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#include "boot_stage.h"
#include "boot_crc.h"
#include "boot_package.h"
#include "boot_layout.h"
#include "boot_demo_image.h"
#include "w25q.h"
#include "stm32f4xx.h"

/* 仅用于教学暂存，不是持久升级状态，复位会清零。
 * result：0未请求/1通过/2型号或通信失败/3 CRC自检失败/
 * 4擦除失败/5写失败/6读失败/7包校验失败/8进行中。
 * 触发覆盖第一颗FFF000～FFFFFF，原1000字节实验数据将被清除。 */
volatile uint32_t g_stage_request;
volatile uint32_t g_stage_result;
volatile uint32_t g_stage_image_size;
volatile uint32_t g_stage_image_crc;
static uint8_t stage_readback[sizeof(boot_demo_image)];

/* 按小端逐字节解码，避免非对齐访问和结构体填充依赖。 */
static uint32_t stage_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* 当前验证完整教学包，长度必须与本地演示包一致。
 * 将来网络固件改为读取头部声明长度并分块CRC，不能直接复用固定长度。 */
/* 验证包头和完整载荷，但不写任何Flash。成功更新的Watch信息不是持久升级标志。 */
static bool stage_validate(const uint8_t *package, uint32_t size)
{
    uint32_t length;
    uint32_t stack_top;
    uint32_t entry;
    if (size < BOOT_PACKAGE_HEADER_SIZE + 8u ||
        stage_u32(package) != BOOT_PACKAGE_MAGIC ||
        stage_u32(package + 4u) != BOOT_PACKAGE_FORMAT ||
        stage_u32(package + 8u) != 2u ||
        stage_u32(package + 12u) != BOOT_APP_BASE ||
        stage_u32(package + 24u) != 0u ||
        boot_crc32(package, 28u) != stage_u32(package + 28u))
    {
        return false;
    }
    /* 头CRC先通过，才使用长度；要求长度与实际读回包大小严格一致。 */
    length = stage_u32(package + 16u);
    if (length != size - BOOT_PACKAGE_HEADER_SIZE ||
        length > BOOT_APP_END - BOOT_APP_BASE ||
        boot_crc32(package + BOOT_PACKAGE_HEADER_SIZE, length) != stage_u32(package + 20u))
    {
        return false;
    }
    /* 固件前两个32位字是初始栈顶与Reset_Handler地址，不是待写数据长度。 */
    stack_top = stage_u32(package + BOOT_PACKAGE_HEADER_SIZE);
    entry = stage_u32(package + BOOT_PACKAGE_HEADER_SIZE + 4u);
    if (stack_top <= 0x20000000u || stack_top > 0x20020000u ||
        (stack_top & 7u) != 0u || (entry & 1u) == 0u ||
        (entry & ~1u) < BOOT_APP_BASE || (entry & ~1u) >= BOOT_APP_BASE + length)
    {
        return false;
    }
    g_stage_image_size = length;
    g_stage_image_crc = stage_u32(package + 20u);
    return true;
}

bool boot_stage_process(void)
{
    uint32_t id;
    bool success = false;
    static const uint8_t crc_sample[] = "123456789";

    if (g_stage_request != 1u)
    {
        return true;
    }
    g_stage_request = 0u;
    g_stage_result = 8u;
    /* 在任何擦写前验证CRC算法和本地源包。测试字符串有效长度为9，不含结尾零。 */
    if (boot_crc32(crc_sample, 9u) != 0xCBF43926u)
    {
        g_stage_result = 3u;
        return false;
    }
    if (sizeof(boot_demo_image) > 4096u ||
        !stage_validate(boot_demo_image, sizeof(boot_demo_image)))
    {
        g_stage_result = 7u;
        return false;
    }
    w25q_init();
    if (!w25q_read_jedec_id(1u, &id) || id != 0xEF4018u)
    {
        g_stage_result = 2u;
        goto finish;
    }
    if (!w25q_erase_sector(1u, 0xFFF000u, 100000u))
    {
        g_stage_result = 4u;
        goto finish;
    }
    if (!w25q_write_data(1u, 0xFFF000u, boot_demo_image,
                         sizeof(boot_demo_image), 100000u))
    {
        g_stage_result = 5u;
        goto finish;
    }
    if (!w25q_read_data(1u, 0xFFF000u, stage_readback, sizeof(stage_readback)))
    {
        g_stage_result = 6u;
        goto finish;
    }
    if (!stage_validate(stage_readback, sizeof(stage_readback)))
    {
        g_stage_result = 7u;
        goto finish;
    }
    g_stage_result = 1u;
    success = true;
finish:
    /* 结束SPI事务后停用本阶段使用的SPI，避免交给App时仍运行。
     * 失败可能已部分擦写W25Q，不重试；内部V1没有被修改。 */
    GPIO_SetBits(GPIOE, GPIO_Pin_13 | GPIO_Pin_14);
    SPI_Cmd(SPI1, DISABLE);
    return success;
}

/* 安装只信任本次从W25Q重新读出的完整包，不能沿用先前RAM结果。 */
bool boot_stage_load(const uint8_t **image, uint32_t *length, uint32_t *crc)
{
    uint32_t id;
    bool success = false;
    static const uint8_t sample[] = "123456789";
    if (image == 0 || length == 0 || crc == 0 ||
        boot_crc32(sample, 9u) != 0xCBF43926u)
    {
        return false;
    }
    w25q_init();
    if (!w25q_read_jedec_id(1u, &id) || id != 0xEF4018u ||
        !w25q_wait_ready(1u, 100000u) ||
        !w25q_read_data(1u, 0xFFF000u, stage_readback, sizeof(stage_readback)) ||
        !stage_validate(stage_readback, sizeof(stage_readback)))
    {
        goto finish;
    }
    /* 二级指针把载荷首地址交给调用者，数据仍由本模块静态数组持有。 */
    *image = stage_readback + BOOT_PACKAGE_HEADER_SIZE;
    *length = stage_u32(stage_readback + 16u);
    *crc = stage_u32(stage_readback + 20u);
    success = true;
finish:
    GPIO_SetBits(GPIOE, GPIO_Pin_13 | GPIO_Pin_14);
    SPI_Cmd(SPI1, DISABLE);
    return success;
}
