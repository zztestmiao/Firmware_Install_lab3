#ifndef BOOT_CRC_H
#define BOOT_CRC_H
#include <stdint.h>
/* CRC-32/ISO-HDLC，与Python zlib.crc32一致。
 * data须指向至少length字节，length为0时允许空指针。
 * 仅校验数据损坏，不提供固件来源认证。 */
uint32_t boot_crc32(const uint8_t *data, uint32_t length);
#endif
