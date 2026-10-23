// AUTO-DRAFT from torvalds/linux PR #aad73b807cd1a5bff9c5277e8a42e75bbdc8adab
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void __iomem *base;

	if (!base) {
		struct device_node *node = of_find_compatible_node(NULL, NULL,
				"arm,vexpress-sysreg");

		base = of_iomap(node, 0);
	}
