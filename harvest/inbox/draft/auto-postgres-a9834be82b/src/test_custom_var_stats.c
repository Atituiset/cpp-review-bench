// AUTO-DRAFT from postgres/postgres PR #31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#define PGSTAT_KIND_TEST_CUSTOM_VAR_STATS 25
/* …（同文件无关代码省略）… */
	PG_RETURN_VOID();
}

/*
 * test_custom_stats_var_drop
 *		Remove custom statistic entry
/* …（同文件无关代码省略）… */
}

/*
 * Callback for pgstat_drop_matching_entries().
 */
static bool
test_custom_stats_var_match_kind(PgStatShared_HashEntry *entry,
								 Datum match_data)
{
	return entry->key.kind == PGSTAT_KIND_TEST_CUSTOM_VAR_STATS;
}
/* …（同文件无关代码省略）… */
Datum
test_custom_stats_var_drop_all(PG_FUNCTION_ARGS)
{
	pgstat_drop_matching_entries(test_custom_stats_var_match_kind, 0);

	PG_RETURN_VOID();
}
