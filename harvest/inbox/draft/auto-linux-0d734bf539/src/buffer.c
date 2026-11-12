// AUTO-DRAFT from torvalds/linux PR #9d66f26fefacebfeed15de31c1123451a17ff119
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>

 * @bh: The buffer to mark dirty.
 * @mmb: The list of buffers to add the buffer to.
 *
 * Mark the buffer dirty and add it to the list if it is not already on
 * a list.
 */
void mmb_mark_buffer_dirty(struct buffer_head *bh,
			   struct mapping_metadata_bhs *mmb)
{
	mark_buffer_dirty(bh);
	if (!bh->b_mmb) {
		spin_lock(&mmb->lock);
/* …（同文件无关代码省略）… */
		list_move_tail(&bh->b_assoc_buffers, &mmb->list);
		spin_unlock(&mmb->lock);
	}
}
EXPORT_SYMBOL(mmb_mark_buffer_dirty);

/* …（同文件无关代码省略）… */
void mark_buffer_dirty(struct buffer_head *bh)
{
	WARN_ON_ONCE(!buffer_uptodate(bh));

	trace_block_dirty_buffer(bh);

	/*
	 * Very *carefully* optimize the it-is-already-dirty case.
	 *
	 * Don't let the final "is it dirty" escape to before we
	 * perhaps modified the buffer.
	 */
	if (buffer_dirty(bh)) {
		smp_mb();
		if (buffer_dirty(bh))
			return;
	}

	if (!test_set_buffer_dirty(bh)) {
		struct folio *folio = bh->b_folio;
		struct address_space *mapping = NULL;

		if (!folio_test_set_dirty(folio)) {
			mapping = folio->mapping;
			if (mapping)
				__folio_mark_dirty(folio, mapping, 0);
		}
		if (mapping)
			__mark_inode_dirty(mapping->host, I_DIRTY_PAGES);
	}
}
