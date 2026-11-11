// AUTO-DRAFT from torvalds/linux PR #5f5be6584cf5af0d70ee2fd40aa6264da18617ec
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static int blkdev_iomap_begin(struct inode *inode, loff_t offset, loff_t length,
		unsigned int flags, struct iomap *iomap, struct iomap *srcmap)
{
	struct block_device *bdev = I_BDEV(inode);
	loff_t isize = i_size_read(inode);

	if (offset >= isize)
		return -EIO;

	iomap->bdev = bdev;
	iomap->offset = ALIGN_DOWN(offset, bdev_logical_block_size(bdev));
	iomap->type = IOMAP_MAPPED;
	iomap->addr = iomap->offset;
	iomap->length = isize - iomap->offset;
	iomap->flags |= IOMAP_F_BUFFER_HEAD; /* noop for !CONFIG_BUFFER_HEAD */
	return 0;
}

static DEFINE_IOMAP_ITER_NEXT(blkdev_iomap_next, blkdev_iomap_begin);

static const struct iomap_ops blkdev_iomap_ops = {
	.iomap_next		= blkdev_iomap_next,
};

#ifdef CONFIG_BUFFER_HEAD
static int blkdev_get_block(struct inode *inode, sector_t iblock,
		struct buffer_head *bh, int create)
/* …（同文件无关代码省略）… */
static int blkdev_read_folio(struct file *file, struct folio *folio)
{
	return block_read_full_folio(folio, blkdev_get_block);
}
/* …（同文件无关代码省略）… */
#else /* CONFIG_BUFFER_HEAD */
static int blkdev_read_folio(struct file *file, struct folio *folio)
{
	iomap_bio_read_folio(folio, &blkdev_iomap_ops);
	return 0;
}

static void blkdev_readahead(struct readahead_control *rac)
{
	iomap_bio_readahead(rac, &blkdev_iomap_ops);
}

static ssize_t blkdev_writeback_range(struct iomap_writepage_ctx *wpc,
		struct folio *folio, u64 offset, unsigned int len, u64 end_pos)
{
	loff_t isize = i_size_read(wpc->inode);

	if (WARN_ON_ONCE(offset >= isize))
		return -EIO;

	if (offset < wpc->iomap.offset ||
	    offset >= wpc->iomap.offset + wpc->iomap.length) {
		int error;

		error = blkdev_iomap_begin(wpc->inode, offset, isize - offset,
				IOMAP_WRITE, &wpc->iomap, NULL);
		if (error)
			return error;
	}

	return iomap_add_to_ioend(wpc, folio, offset, end_pos, len);
}
/* …（同文件无关代码省略）… */
	 * i_blkbits/folio order and truncating the page cache.
	 */
	inode_lock_shared(inode);
	ret = iomap_file_buffered_write(iocb, from, &blkdev_iomap_ops, NULL,
					NULL);
	inode_unlock_shared(inode);
	return ret;
