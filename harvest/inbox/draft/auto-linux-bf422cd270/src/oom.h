// AUTO-DRAFT from torvalds/linux PR #067c5c24cef23d913b67eaaf71ae67e7169a6620
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR

TRACE_EVENT(reclaim_retry_zone,

	TP_PROTO(struct zoneref *zoneref,
		int order,
		unsigned long reclaimable,
		unsigned long available,
		unsigned long min_wmark,
		int no_progress_loops,
		bool wmark_check),

	TP_ARGS(zoneref, order, reclaimable, available, min_wmark, no_progress_loops, wmark_check),

	TP_STRUCT__entry(
		__field(	int, node)
/* …（同文件无关代码省略）… */
	),

	TP_fast_assign(
		__entry->node = zonelist_node_idx(zoneref);
		__entry->zone_idx = zonelist_zone_idx(zoneref);
		__entry->order = order;
		__entry->reclaimable = reclaimable;
		__entry->available = available;
