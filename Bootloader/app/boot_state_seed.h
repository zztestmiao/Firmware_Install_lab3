#ifndef BOOT_STATE_SEED_H
#define BOOT_STATE_SEED_H
#include <stdbool.h>
/* 首次迁移初始化，默认不执行。调用前已备份S2/S3。
 * request=1时验证外部包及当前内部V2，再在S2提交NORMAL。
 * false停止启动，true表示无请求或已成功，不表示自动恢复已实现。 */
bool boot_state_seed_process(void);
#endif
