// AUTO-DRAFT from torvalds/linux PR #f802b6558eb81bd5888d761a0cdc8afe5fd8c941
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
  // <<< BUG ANCHOR
struct xchk_stats {
	struct dentry		*cs_debugfs;
	struct xchk_scrub_stats	cs_stats[XFS_SCRUB_TYPE_NR];
};
/* …（同文件无关代码省略）… */
xchk_stats_estimate_bufsize(
	struct xchk_stats	*cs)
{
	struct xchk_scrub_stats	*css = &cs->cs_stats[0];
	unsigned int		i;
	size_t			field_width;
	size_t			ret = 0;
/* …（同文件无关代码省略）… */
			      offsetof(struct xchk_scrub_stats, checktime_us)) /
			     sizeof(uint64_t));

	for (i = 0; i < XFS_SCRUB_TYPE_NR; i++, css++) {
		if (!name_map[i])
			continue;
