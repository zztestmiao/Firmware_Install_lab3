/*
 * 文件职责：跳转模块公开接口
 * 调用关系：提供给main，具体清理及汇编放在boot_jump.c。
 * 阅读重点：成功通常不返回，失败时返回并由main停留。
 * 教学阶段：实验三本地安装，尚未完成网络OTA及自动安装中断恢复。
 */

#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H

/* 硬件初始化后调用，当前只做向量入口基本检查，不代替完整镜像校验。
 * 正常跳转后不返回；入口不合法时点亮LED3并返回。
 * 跳转前清理中断、切换时钟并设置VTOR，ARMCC5汇编负责切栈。
 * 后续增加Flash等外设后，须扩展外设清理；不能在ISR中调用。 */
void boot_jump_to_app(void);

#endif
