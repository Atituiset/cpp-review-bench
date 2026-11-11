// AUTO-DRAFT from torvalds/linux PR #067c5c24cef23d913b67eaaf71ae67e7169a6620
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static struct folio *dequeue_hugetlb_folio_nodemask(struct hstate *h, gfp_t gfp_mask,
							int nid, nodemask_t *nmask)
{
	unsigned int cpuset_mems_cookie;
	struct zonelist *zonelist;
	struct zone *zone;
	struct zoneref *z;
	int node = NUMA_NO_NODE;

	/* 'nid' should not be NUMA_NO_NODE. Try to catch any misuse of it and rectifiy. */
	if (nid == NUMA_NO_NODE)
/* …（同文件无关代码省略）… */
	zonelist = node_zonelist(nid, gfp_mask);

retry_cpuset:
	cpuset_mems_cookie = read_mems_allowed_begin();
	for_each_zone_zonelist_nodemask(zone, z, zonelist, gfp_zone(gfp_mask), nmask) {
		struct folio *folio;

		if (!cpuset_zone_allowed(zone, gfp_mask))
			continue;
		/*
/* …（同文件无关代码省略）… */
		if (folio)
			return folio;
	}
	if (unlikely(read_mems_allowed_retry(cpuset_mems_cookie)))
		goto retry_cpuset;

	return NULL;
