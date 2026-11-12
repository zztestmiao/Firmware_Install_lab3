#ifndef INTERNAL_FLASH_H
#define INTERNAL_FLASH_H
#include <stdint.h>
#include <stdbool.h>

/* 第一版仅允许覆盖S4，内部固定地址，绝不提供整片擦除接口。
 * Bootloader在S0/S1，S5及其他区不在本驱动可擦写范围。 */
#define IFLASH_DEMO_START 0x08010000u
#define IFLASH_DEMO_END   0x08020000u

/* 从有效主SRAM镜像擦写S4：调用者须完成源镜像校验及授权。
 * 长度1～64KiB，源RAM连续且在20000000～2001FFFF，独占Flash。
 * 固定擦整个64KiB S4，不保留旧App。2.7～3.6V稳定VDD下使用32位编程。
 * false可能已擦坏/部分写入，必须停留Bootloader，禁止启动残缺App。
 * 阻塞标准库等待，无精确软件超时，不在ISR使用，不保证掉电恢复。 */
bool internal_flash_install_s4(const uint8_t *image, uint32_t length);
#endif
