// AUTO-DRAFT from postgres/postgres PR #31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR

/*
 * Scan through the stats hashtables, dropping statistics if approved by the
 * optional do_drop() function.
 */
void
pgstat_drop_matching_entries(bool (*do_drop) (PgStatShared_HashEntry *, Datum),
							 Datum match_data)
{
	dshash_seq_status hstat;
	PgStatShared_HashEntry *ps;
	uint64		not_freed_count = 0;

	for (int h = 0; h < pgStatLocal.num_var_hashes; h++)
	{
		/* entries are removed, take an exclusive lock */
		dshash_seq_init(&hstat, pgStatLocal.var_hashes[h], true);
		while ((ps = dshash_seq_next(&hstat)) != NULL)
		{
			if (ps->dropped)
				continue;

			if (do_drop != NULL && !do_drop(ps, match_data))
				continue;

/* …（同文件无关代码省略）… */
void
pgstat_drop_all_entries(void)
{
	pgstat_drop_matching_entries(NULL, 0);
}

static void
