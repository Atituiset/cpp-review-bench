// AUTO-DRAFT from torvalds/linux PR #8ebbaf8835c62d7541a6226672a6c2e8e6a5428a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
kmemleak_free(page_address(page));
		__free_pages(page, page->private);
	}
	kfree(tags);
}

void blk_mq_free_tags(struct blk_mq_tags *tags)
{
	sbitmap_queue_free(&tags->bitmap_tags);
	sbitmap_queue_free(&tags->breserved_tags);

	/* if tags pages is not allocated yet, free tags directly */
	if (list_empty(&tags->page_list)) {
		kfree(tags);
		return;
	}
