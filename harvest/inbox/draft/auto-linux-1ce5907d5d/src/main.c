// AUTO-DRAFT from torvalds/linux PR #ca3e345926b00bd59106c1bf2022b1daf4164a67
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdint.h>
#include <string.h>
  // <<< BUG ANCHOR
#define BOOTCONFIG_FOOTER_SIZE	\
	(sizeof(uint32_t) * 2 + BOOTCONFIG_MAGIC_LEN)
/* …（同文件无关代码省略）… */
	footer.size = htole32(size);
	footer.csum = htole32(csum);
	memcpy(footer.magic, BOOTCONFIG_MAGIC, BOOTCONFIG_MAGIC_LEN);
	static_assert(sizeof(footer) == BOOTCONFIG_FOOTER_SIZE);
	memcpy(data + size, &footer, BOOTCONFIG_FOOTER_SIZE);

	total_size = size + BOOTCONFIG_FOOTER_SIZE;
