// AUTO-DRAFT from postgres/postgres PR #6296e78c3d2c7406bb450c5ae477b2fef58f155e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
Datum
pg_stat_get_kind_info(PG_FUNCTION_ARGS)
{
#define PG_STAT_KIND_INFO_COLS	7
	ReturnSetInfo *rsinfo;

	InitMaterializedSRF(fcinfo, 0);
/* …（同文件无关代码省略）… */
		values[3] = BoolGetDatum(info->fixed_amount);
		values[4] = BoolGetDatum(info->accessed_across_databases);
		values[5] = BoolGetDatum(info->write_to_file);

		/*
		 * When track_entry_count is disabled, use NULL.  Fixed-sized stats
		 * kinds report NULL here.
		 */
		if (info->track_entry_count)
			values[6] = Int64GetDatum(pgstat_get_entry_count(kind));
		else
			nulls[6] = true;

		tuplestore_putvalues(rsinfo->setResult, rsinfo->setDesc, values, nulls);
	}
