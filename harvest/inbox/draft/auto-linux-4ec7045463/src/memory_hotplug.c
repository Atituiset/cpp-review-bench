// AUTO-DRAFT from torvalds/linux PR #58e81bb7de5aac8ec5e4aa21d401e7dafbd4058b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
struct page *pfn_to_online_page(unsigned long pfn)
{
	unsigned long nr = pfn_to_section_nr(pfn);
	struct dev_pagemap *pgmap;
	struct mem_section *ms;

	BUILD_BUG_ON(IS_ENABLED(CONFIG_HAVE_ARCH_PFN_VALID));

	if (nr >= NR_MEM_SECTIONS)
		return NULL;

	ms = __nr_to_section(nr);
	if (!online_section(ms))
		return NULL;

	if (!pfn_section_valid(ms, pfn))
		return NULL;

	if (!online_device_section(ms))
		return pfn_to_page(pfn);

	/*
	 * Slowpath: when ZONE_DEVICE collides with
	 * ZONE_{NORMAL,MOVABLE} within the same section some pfns in
	 * the section may be 'offline' but 'valid'. Only
	 * get_dev_pagemap() can determine sub-section online status.
	 */
	pgmap = get_dev_pagemap(pfn);
	put_dev_pagemap(pgmap);

	/* The presence of a pgmap indicates ZONE_DEVICE offline pfn */
	if (pgmap)
		return NULL;

	return pfn_to_page(pfn);
}
/* …（同文件无关代码省略）… */
static bool subsection_overlaps_zone(unsigned long pfn, struct zone *zone)
{
	const unsigned long start_pfn = ALIGN_DOWN(pfn, PAGES_PER_SUBSECTION);
	const unsigned long end_pfn = start_pfn + PAGES_PER_SUBSECTION - 1;

	/* All pages in a subsection are either online or offline. */
	if (unlikely(!pfn_to_online_page(start_pfn)))
		return false;

	/* Checking start+end is sufficient. */
	return zone == page_zone(pfn_to_page(start_pfn)) ||
	       zone == page_zone(pfn_to_page(end_pfn));
}
/* …（同文件无关代码省略）… */
static unsigned long find_smallest_section_pfn(struct zone *zone,
		unsigned long start_pfn, unsigned long end_pfn)
{
	for (; start_pfn < end_pfn; start_pfn += PAGES_PER_SUBSECTION) {
		if (subsection_overlaps_zone(start_pfn, zone))
			return start_pfn;
	}
	return 0;
}
/* …（同文件无关代码省略）… */
static unsigned long find_biggest_section_pfn(struct zone *zone,
		unsigned long start_pfn, unsigned long end_pfn)
{
	for (; end_pfn > start_pfn; end_pfn -= PAGES_PER_SUBSECTION) {
		if (subsection_overlaps_zone(end_pfn - 1, zone))
			return end_pfn - 1;
	}
	return 0;
}

static void shrink_zone_span(struct zone *zone, unsigned long start_pfn,
			     unsigned long end_pfn)
{
/* …（同文件无关代码省略）… */
		 */
		pfn = find_smallest_section_pfn(zone, end_pfn,
						zone_end_pfn(zone));
		if (pfn) {
			zone->spanned_pages = zone_end_pfn(zone) - pfn;
			zone->zone_start_pfn = pfn;
		} else {
			zone->zone_start_pfn = 0;
			zone->spanned_pages = 0;
		}
	} else if (zone_end_pfn(zone) == end_pfn) {
		/*
		 * If the section is biggest section in the zone, it need
/* …（同文件无关代码省略）… */
		pfn = find_biggest_section_pfn(zone, zone->zone_start_pfn,
					       start_pfn);
		if (pfn)
			zone->spanned_pages = pfn - zone->zone_start_pfn + 1;
		else {
			zone->zone_start_pfn = 0;
			zone->spanned_pages = 0;
		}
	}
}

/* …（同文件无关代码省略）… */
static void __meminit resize_zone_range(struct zone *zone, unsigned long start_pfn,
		unsigned long nr_pages)
{
	unsigned long old_end_pfn = zone_end_pfn(zone);

	if (zone_is_empty(zone) || start_pfn < zone->zone_start_pfn)
		zone->zone_start_pfn = start_pfn;

	zone->spanned_pages = max(start_pfn + nr_pages, old_end_pfn) - zone->zone_start_pfn;
}

static void __meminit resize_pgdat_range(struct pglist_data *pgdat, unsigned long start_pfn,
                                     unsigned long nr_pages)
{
	unsigned long old_end_pfn = pgdat_end_pfn(pgdat);

	if (!pgdat->node_spanned_pages || start_pfn < pgdat->node_start_pfn)
		pgdat->node_start_pfn = start_pfn;

	pgdat->node_spanned_pages = max(start_pfn + nr_pages, old_end_pfn) - pgdat->node_start_pfn;

}
