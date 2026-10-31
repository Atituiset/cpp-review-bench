// AUTO-DRAFT from postgres/postgres PR #5e4966e9d23931e49f914770e663dbc316ed568f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
 * during normal 2PC COMMIT/ABORT PREPARED processing.
 */
void
pgstat_execute_transactional_drops(int ndrops, struct xl_xact_stats_item *items, bool is_redo)
{
	int			not_freed_count = 0;
