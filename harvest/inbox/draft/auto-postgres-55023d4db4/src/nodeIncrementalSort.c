// AUTO-DRAFT from postgres/postgres PR #d9c6f22c3b2beaf34e47cf47fd21703886be8011
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	else
	{
		/* Next group of presorted data */
		tuplesort_reset(node->prefixsort_state);
	}

	/*
/* …（同文件无关代码省略）… */
		else
		{
			/* Reset sort for the next batch. */
			tuplesort_reset(fullsort_state);
		}

		/*
/* …（同文件无关代码省略）… */
	 * column functions by checking to see if the full sort state has been
	 * initialized yet, setting the sort states to null here might actually
	 * cause a leak.  The IncrementalSortState's bounded flag may change
	 * between scans, per ExecSetTupleBound(), so we must toggle that flag in
	 * the sort options before calling tuplesort_reset().
	 */
	if (node->fullsort_state != NULL)
	{
		tuplesort_set_allowbounded(node->fullsort_state, node->bounded);
		tuplesort_reset(node->fullsort_state);
	}
	if (node->prefixsort_state != NULL)
	{
		tuplesort_set_allowbounded(node->prefixsort_state, node->bounded);
		tuplesort_reset(node->prefixsort_state);
	}

	/*
	 * If chgParam of subnode is not null, then the plan will be re-scanned by
