// AUTO-DRAFT from torvalds/linux PR #df37e3924292fb41d325721008520ba93deced42
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
  // <<< BUG ANCHOR
	unsigned long e820_checksum;
};

/**
 * compute_e820_crc32 - calculate crc32 of a given e820 table
 *
 * @table: the e820 table to be calculated
 *
 * Return: the resulting checksum
 */
static inline u32 compute_e820_crc32(struct e820_table *table)
{
	int size = offsetof(struct e820_table, entries) +
		sizeof(struct e820_entry) * table->nr_entries;

	return ~crc32_le(~0, (unsigned char const *)table, size);
}

#ifdef CONFIG_X86_64
#define RESTORE_MAGIC	0x23456789ABCDEF02UL
#else
#define RESTORE_MAGIC	0x12345679UL
#endif

/**
