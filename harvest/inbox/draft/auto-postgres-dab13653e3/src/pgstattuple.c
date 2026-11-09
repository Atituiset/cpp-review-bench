// AUTO-DRAFT from postgres/postgres PR #0091c241f6cffb2b163f18dea54c10d0b45090aa
#include "access/nbtree.h"
#include "access/relscan.h"
#include "access/tableam.h"
#include "catalog/namespace.h"
#include "catalog/pg_am_d.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "storage/bufmgr.h"
#include "storage/lmgr.h"
#include "utils/varlena.h"
/* …（同文件无关代码省略）… */
}
  // <<< BUG ANCHOR
/*
 * pgstat_relation
 */
static Datum
pgstat_relation(Relation rel, FunctionCallInfo fcinfo)
{
	const char *err;

	/*
	 * Reject attempts to read non-local temporary relations; we would be
	 * likely to get wrong data since we have no visibility into the owning
	 * session's local buffers.
	 */
	if (RELATION_IS_OTHER_TEMP(rel))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot access temporary tables of other sessions")));

	if (RELKIND_HAS_TABLE_AM(rel->rd_rel->relkind) ||
		rel->rd_rel->relkind == RELKIND_SEQUENCE)
