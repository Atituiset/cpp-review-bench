// AUTO-DRAFT from postgres/postgres PR #0091c241f6cffb2b163f18dea54c10d0b45090aa
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
#include "access/htup_details.h"
#include "access/nbtree.h"
#include "access/relation.h"
#include "catalog/namespace.h"
#include "catalog/pg_am.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "storage/bufmgr.h"
#include "storage/read_stream.h"
#include "utils/rel.h"
/* …（同文件无关代码省略）… */
				 errmsg("relation \"%s\" is not a btree index",
						RelationGetRelationName(rel))));

	/*
	 * Reject attempts to read non-local temporary relations; we would be
	 * likely to get wrong data since we have no visibility into the owning
	 * session's local buffers.
	 */
	if (RELATION_IS_OTHER_TEMP(rel))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot access temporary tables of other sessions")));

	/*
	 * A !indisready index could lead to ERRCODE_DATA_CORRUPTED later, so exit
/* …（同文件无关代码省略）… */
				 errdetail_relkind_not_supported(rel->rd_rel->relkind)));

	/* note: this will work OK on non-local temp tables */

	relpages = RelationGetNumberOfBlocks(rel);

/* …（同文件无关代码省略）… */
				 errmsg("relation \"%s\" is not a GIN index",
						RelationGetRelationName(rel))));

	/*
	 * Reject attempts to read non-local temporary relations; we would be
	 * likely to get wrong data since we have no visibility into the owning
	 * session's local buffers.
	 */
	if (RELATION_IS_OTHER_TEMP(rel))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot access temporary indexes of other sessions")));

	/* see pgstatindex_impl */
	if (!rel->rd_index->indisvalid)
/* …（同文件无关代码省略）… */
				 errmsg("relation \"%s\" is not a hash index",
						RelationGetRelationName(rel))));

	/*
	 * Reject attempts to read non-local temporary relations; we would be
	 * likely to get wrong data since we have no visibility into the owning
	 * session's local buffers.
	 */
	if (RELATION_IS_OTHER_TEMP(rel))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot access temporary indexes of other sessions")));

	/* see pgstatindex_impl */
	if (!rel->rd_index->indisvalid)
