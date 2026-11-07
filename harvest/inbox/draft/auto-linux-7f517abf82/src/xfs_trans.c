// AUTO-DRAFT from torvalds/linux PR #0ccc72d4b3fcf8c79178b7024541e1aa694e08a7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
	xfs_extent_busy_clear(&tp->t_busy, false);

	trace_xfs_trans_free(tp, _RET_IP_);
	xfs_trans_clear_context(tp);
	if (!(tp->t_flags & XFS_TRANS_NO_WRITECOUNT))
		sb_end_intwrite(tp->t_mountp->m_super);
	xfs_trans_free_dqinfo(tp);
/* …（同文件无关代码省略）… */
	tp = kmem_cache_zalloc(xfs_trans_cache, GFP_KERNEL | __GFP_NOFAIL);
	if (!(flags & XFS_TRANS_NO_WRITECOUNT))
		sb_start_intwrite(mp->m_super);
	xfs_trans_set_context(tp);
	tp->t_flags = flags;
	tp->t_mountp = mp;
	INIT_LIST_HEAD(&tp->t_items);
/* …（同文件无关代码省略）… */
	 * xfs_trans_free.  Set it again here before doing memory
	 * allocations.
	 */
	xfs_trans_set_context(tp);

	if (error)
		return error;
