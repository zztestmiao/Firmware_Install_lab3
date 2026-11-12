#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H

/* 硬件初始化后调用，当前只做向量入口基本检查，不代替完整镜像校验。
 * 正常跳转后不返回；入口不合法时点亮LED3并返回。
 * 跳转前清理中断、切换时钟并设置VTOR，ARMCC5汇编负责切栈。
 * 后续增加Flash等外设后，须扩展外设清理；不能在ISR中调用。 */
void boot_jump_to_app(void);

#endif
