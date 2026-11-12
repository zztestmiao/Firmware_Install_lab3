#ifndef BOOT_LAYOUT_H
#define BOOT_LAYOUT_H

/* LED教学内部布局，结束地址不包含；必须匹配App链接地址及向量偏移。
 * 只集中定义现有边界，不表示已完成镜像检查或正式天气时钟分区。
 * 后续安装驱动及校验共用本文件，避免各自硬编码地址。 */
#define BOOT_APP_BASE 0x08010000u
#define BOOT_APP_END  0x08040000u

#endif
