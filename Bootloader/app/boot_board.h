#ifndef BOOT_BOARD_H
#define BOOT_BOARD_H

/* 上电调用一次，初始化板级资源和三颗LED并关闭LED。 */
void boot_board_init(void);

/* 初始化后调用，LED2闪三次，延时只作肉眼提示，非超时计时源。 */
void boot_board_show_startup(void);

#endif
