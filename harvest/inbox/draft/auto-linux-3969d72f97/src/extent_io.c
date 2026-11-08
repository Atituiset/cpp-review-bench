// AUTO-DRAFT from torvalds/linux PR #fa4ad798f6f681031775266044ba1b0cf3c670bd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static bool btrfs_verify_folio(struct fsverity_info *vi, struct folio *folio,
			       u64 start, u32 len)
{
	struct btrfs_fs_info *fs_info = folio_to_fs_info(folio);

	if (!vi || btrfs_folio_test_uptodate(fs_info, folio, start, len))
		return true;
	return fsverity_verify_folio(vi, folio);
}

static void end_folio_read(struct fsverity_info *vi, struct folio *folio,
			   bool uptodate, u64 start, u32 len)
{
	struct btrfs_fs_info *fs_info = folio_to_fs_info(folio);

	ASSERT(folio_pos(folio) <= start &&
	       start + len <= folio_next_pos(folio));

	if (uptodate && btrfs_verify_folio(vi, folio, start, len))
		btrfs_folio_set_uptodate(fs_info, folio, start, len);
	else
		btrfs_folio_clear_uptodate(fs_info, folio, start, len);

	if (!btrfs_is_subpage(fs_info, folio))
		folio_unlock(folio);
	else
		btrfs_folio_end_lock(fs_info, folio, start, len);
}
