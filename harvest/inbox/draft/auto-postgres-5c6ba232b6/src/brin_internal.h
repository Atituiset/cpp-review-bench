// AUTO-DRAFT from postgres/postgres PR #633dee1bd1b41145f81619423d573754c7ad2b9e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
typedef struct BrinOpcInfo
{
	/* Number of columns stored in an index column of this opclass */
	uint16		oi_nstored;

	/* Regular processing of NULLs in BrinValues? */
	bool		oi_regular_nulls;

	/* Opaque pointer for the opclass' private use */
	void	   *oi_opaque;

	/* Type cache entries of the stored columns */
	TypeCacheEntry *oi_typcache[FLEXIBLE_ARRAY_MEMBER];
} BrinOpcInfo;

/* the size of a BrinOpcInfo for the given number of columns */
