// AUTO-DRAFT from torvalds/linux PR #bdeb84ed24e543930487f03f5243bbf83aebaf31
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define DATA_SIZE(data_ring)		_DATA_SIZE((data_ring)->size_bits)
#define DATA_SIZE_MASK(data_ring)	(DATA_SIZE(data_ring) - 1)
/* …（同文件无关代码省略）… */
#define DATA_INDEX(data_ring, lpos)	((lpos) & DATA_SIZE_MASK(data_ring))
/* …（同文件无关代码省略）… */
#define DATA_WRAPS(data_ring, lpos)	((lpos) >> (data_ring)->size_bits)
/* …（同文件无关代码省略）… */
#define LPOS_DATALESS(lpos)		((lpos) & 1UL)
#define BLK_DATALESS(blk)		(LPOS_DATALESS((blk)->begin) && \
					 LPOS_DATALESS((blk)->next))
/* …（同文件无关代码省略）… */
static bool is_blk_wrapped(struct prb_data_ring *data_ring,
			   unsigned long begin_lpos, unsigned long next_lpos)
{
	/*
	 * Subtract one from next_lpos since it's not actually part of this data
	 * block. This allows perfectly fitting records to not wrap.
	 */
	return DATA_WRAPS(data_ring, begin_lpos) !=
	       DATA_WRAPS(data_ring, next_lpos - 1);
}
/* …（同文件无关代码省略）… */
	if (BLK_DATALESS(blk_lpos))
		return 0;

	if (!is_blk_wrapped(data_ring, blk_lpos->begin, blk_lpos->next)) {
		/* Data block does not wrap. */
		return (DATA_INDEX(data_ring, blk_lpos->next) -
			DATA_INDEX(data_ring, blk_lpos->begin));
	}

	/*
	 * For wrapping data blocks, the trailing (wasted) space is
