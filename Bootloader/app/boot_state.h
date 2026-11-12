/* 状态模块只公开逻辑状态与读取接口，不公开内部扇区地址。
 * 提供状态读取与交替提交，尚未驱动自动安装。
 * 后续可替换内部存储后端为W25Q，保留上层逻辑状态。 */
#ifndef BOOT_STATE_H
#define BOOT_STATE_H
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    BOOT_STATE_NORMAL = 1,
    BOOT_STATE_READY = 2,
    BOOT_STATE_INSTALLING = 3
} boot_state_value;

/* 返回EMPTY不等于App有效，也不允许直接擦除；INVALID需保留现场。
 * VALID时才可使用输出，slot仅是逻辑槽编号，非物理地址。 */
typedef enum
{
    BOOT_STATE_EMPTY = 0,
    BOOT_STATE_VALID = 1,
    BOOT_STATE_INVALID = 2,
    BOOT_STATE_CONFLICT = 3
} boot_state_scan_result;

typedef struct
{
    uint32_t sequence;
    boot_state_value state;
    uint32_t version;
    uint32_t image_size;
    uint32_t image_crc;
    uint32_t package_size;
    uint32_t package_crc;
    uint32_t slot;
} boot_state_info;

/* 只读两个状态槽，out须有效，仅VALID时覆盖输出，无擦写副作用。 */
boot_state_scan_result boot_state_read(boot_state_info *out);
/* 启动时调用，发布Watch探测结果，不改变当前启动决策。 */
void boot_state_probe(void);
/* 可靠提交新逻辑记录：序号由模块分配，写另一个槽并最后提交。
 * 无有效记录时只有first_init=true允许从头建立，调用前必须备份旧区。
 * true须经过完整读回确认，false时新记录可能已提交，禁止盲目重试。
 * 独占内部Flash，稳定2.7～3.6V供电，不在中断中调用。
 * 旧有效槽在新槽提交前不擦，序号禁止回绕。 */
bool boot_state_commit(const boot_state_info *info, bool first_init);
#endif
