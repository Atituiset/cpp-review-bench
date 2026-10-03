// AUTO-DRAFT from torvalds/linux PR #ff47652a4b66c067c765a7ad464d930b5a9367cc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static struct folio *netfs_grab_folio_for_write(struct address_space *mapping,
						loff_t pos, size_t part)
{
	pgoff_t index = pos / PAGE_SIZE;
	fgf_t fgp_flags = FGP_WRITEBEGIN;

	if (mapping_large_folio_support(mapping))
		fgp_flags |= fgf_set_order(pos % PAGE_SIZE + part);

	return __filemap_get_folio(mapping, index, fgp_flags,
				   mapping_gfp_mask(mapping));
}
/* …（同文件无关代码省略）… */
void netfs_update_i_size(struct netfs_inode *ctx, struct inode *inode,
			 loff_t pos, size_t copied)
{
	loff_t i_size, end = pos + copied;
	blkcnt_t add;
	size_t gap;

	if (end <= i_size_read(inode))
		return;

	if (ctx->ops->update_i_size) {
		ctx->ops->update_i_size(inode, end);
		return;
	}

	spin_lock(&inode->i_lock);

	i_size = i_size_read(inode);
	if (end > i_size) {
		i_size_write(inode, end);
#if IS_ENABLED(CONFIG_FSCACHE)
		fscache_update_cookie(ctx->cache, NULL, &end);
#endif

		gap = SECTOR_SIZE - (i_size & (SECTOR_SIZE - 1));
		if (copied > gap) {
			add = DIV_ROUND_UP(copied - gap, SECTOR_SIZE);

			inode->i_blocks = min_t(blkcnt_t,
						DIV_ROUND_UP(end, SECTOR_SIZE),
						inode->i_blocks + add);
		}
	}
	spin_unlock(&inode->i_lock);
}
/* …（同文件无关代码省略）… */
ssize_t netfs_perform_write(struct kiocb *iocb, struct iov_iter *iter,
			    struct netfs_group *netfs_group)
{
	struct file *file = iocb->ki_filp;
	struct inode *inode = file_inode(file);
	struct address_space *mapping = inode->i_mapping;
	struct netfs_inode *ctx = netfs_inode(inode);
	struct writeback_control wbc = {
		.sync_mode	= WB_SYNC_NONE,
		.for_sync	= true,
		.nr_to_write	= LONG_MAX,
		.range_start	= iocb->ki_pos,
		.range_end	= iocb->ki_pos + iter->count,
	};
	struct netfs_io_request *wreq = NULL;
	struct folio *folio = NULL, *writethrough = NULL;
	unsigned int bdp_flags = (iocb->ki_flags & IOCB_NOWAIT) ? BDP_ASYNC : 0;
	ssize_t written = 0, ret, ret2;
	loff_t pos = iocb->ki_pos;
	size_t max_chunk = mapping_max_folio_size(mapping);
	bool maybe_trouble = false;

	if (unlikely(iocb->ki_flags & (IOCB_DSYNC | IOCB_SYNC))
	    ) {
		wbc_attach_fdatawrite_inode(&wbc, mapping->host);

		ret = filemap_write_and_wait_range(mapping, pos, pos + iter->count);
		if (ret < 0) {
			wbc_detach_inode(&wbc);
			goto out;
		}

		wreq = netfs_begin_writethrough(iocb, iter->count);
		if (IS_ERR(wreq)) {
			wbc_detach_inode(&wbc);
			ret = PTR_ERR(wreq);
			wreq = NULL;
			goto out;
		}
		if (!is_sync_kiocb(iocb))
			wreq->iocb = iocb;
		netfs_stat(&netfs_n_wh_writethrough);
	} else {
		netfs_stat(&netfs_n_wh_buffered_write);
	}

	do {
		enum netfs_folio_trace trace;
		struct netfs_folio *finfo;
		struct netfs_group *group;
		unsigned long long fpos;
		size_t flen;
		size_t offset;	/* Offset into pagecache folio */
		size_t part;	/* Bytes to write to folio */
		size_t copied;	/* Bytes copied from user */
		void *priv;

		offset = pos & (max_chunk - 1);
		part = min(max_chunk - offset, iov_iter_count(iter));

		/* Bring in the user pages that we will copy from _first_ lest
		 * we hit a nasty deadlock on copying from the same page as
		 * we're writing to, without it being marked uptodate.
		 *
		 * Not only is this an optimisation, but it is also required to
		 * check that the address is actually valid, when atomic
		 * usercopies are used below.
		 *
		 * We rely on the page being held onto long enough by the LRU
		 * that we can grab it below if this causes it to be read.
		 */
		ret = -EFAULT;
		if (unlikely(fault_in_iov_iter_readable(iter, part) == part))
			break;

		folio = netfs_grab_folio_for_write(mapping, pos, part);
		if (IS_ERR(folio)) {
			ret = PTR_ERR(folio);
			break;
		}

		flen = folio_size(folio);
		fpos = folio_pos(folio);
		offset = pos - fpos;
		part = min_t(size_t, flen - offset, part);

		/* Wait for writeback to complete.  The writeback engine owns
		 * the info in folio->private and may change it until it
		 * removes the WB mark.
		 */
		if (folio_get_private(folio) &&
		    folio_wait_writeback_killable(folio)) {
			ret = written ? -EINTR : -ERESTARTSYS;
			goto error_folio_unlock;
		}

		if (signal_pending(current)) {
			ret = written ? -EINTR : -ERESTARTSYS;
			goto error_folio_unlock;
		}

		finfo = netfs_folio_info(folio);
		group = netfs_folio_group(folio);

		/* If the requested group differs from the group set on the
		 * page, then we need to flush out the folio if it has a group
		 * set (ie. is non-NULL).  Note that COPY_TO_CACHE is a special
		 * case, being a netfs annotation rather than an actual group.
		 *
		 * The filesystem isn't permitted to mix writes with groups and
		 * writes without groups as the NULL group is used to indicate
		 * that no group is set.
		 */
		if (unlikely(group != netfs_group) &&
		    group != NETFS_FOLIO_COPY_TO_CACHE &&
		    group) {
			WARN_ON_ONCE(!netfs_group);
			goto flush_content;
		}

		/* Decide how we should modify a folio.  We might be attempting
		 * to do write-streaming, as we don't want to a local RMW cycle
		 * if we can avoid it.  If we're doing local caching or content
		 * crypto, we award that priority over avoiding RMW.  If the
		 * file is open readably, then we let ->read_folio() fill in
		 * the gaps.
		 */
		if (folio_test_uptodate(folio)) {
			if (mapping_writably_mapped(mapping))
				flush_dcache_folio(folio);
			copied = copy_folio_from_iter_atomic(folio, offset, part, iter);
			if (unlikely(copied == 0))
				goto copy_failed;
			trace = netfs_folio_is_uptodate;
			goto copied_uptodate;
		}

		/* If the page is above the zero-point then we assume that the
		 * server would just return a block of zeros or a short read if
		 * we try to read it.
		 */
		if (fpos >= netfs_read_zero_point(inode)) {
			folio_zero_segment(folio, 0, offset);
			copied = copy_folio_from_iter_atomic(folio, offset, part, iter);
			if (unlikely(copied == 0))
				goto copy_failed;
			folio_zero_segment(folio, offset + copied, flen);
			if (finfo)
				trace = netfs
