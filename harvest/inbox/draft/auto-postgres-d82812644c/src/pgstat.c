// AUTO-DRAFT from postgres/postgres PR #44539786b8296b6e73035142670e0d1ff2639078
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
 * Fixed-numbered stats are stored in plain (non-dynamic) shared memory.
 *
 * Statistics for variable-numbered objects are stored in dynamic shared
 * memory and can be found via a dshash hashtable. The statistics counters are
 * not part of the dshash entry (PgStatShared_HashEntry) directly, but are
 * separately allocated (PgStatShared_HashEntry->body). The separate
 * allocation allows different kinds of statistics to be stored in the same
 * hashtable without wasting space in PgStatShared_HashEntry.
 *
 * Variable-numbered stats are addressed by PgStat_HashKey while running.  It
 * is not possible to have statistics for an object that cannot be addressed
 * that way at runtime. A wider identifier can be used when serializing to
 * disk (used for replication slot stats).
 *
 * To avoid contention on the shared hashtable, each backend has a
 * backend-local hashtable (pgStatEntryRefHash) in front of the shared
 * hashtable, containing references (PgStat_EntryRef) to shared hashtable
 * entries. The shared hashtable only needs to be accessed when no prior
 * reference is found in the local hashtable. Besides pointing to the
 * shared hashtable entry (PgStatShared_HashEntry) PgStat_EntryRef also
 * contains a pointer to the shared statistics data, as a process-local
 * address, to reduce access costs.
 *
 * The names for structs stored in shared memory are prefixed with
 * PgStatShared instead of PgStat. Each stats entry in shared memory is
/* …（同文件无关代码省略）… */
#define PGSTAT_FILE_ENTRY_NAME	'N' /* stats entry identified by name */
#define PGSTAT_FILE_ENTRY_HASH	'S' /* stats entry identified by
/* …（同文件无关代码省略）… */
typedef struct PgStat_SnapshotEntry
{
	PgStat_HashKey key;
	char		status;			/* for simplehash use */
	void	   *data;			/* the stats data itself */
} PgStat_SnapshotEntry;
/* …（同文件无关代码省略）… */
	/*
	 * Snapshot all variable stats.
	 */
	dshash_seq_init(&hstat, pgStatLocal.shared_hash, false);
	while ((p = dshash_seq_next(&hstat)) != NULL)
	{
		PgStat_Kind kind = p->key.kind;
		const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);
		bool		found;
		PgStat_SnapshotEntry *entry;
		PgStatShared_Common *stats_data;

		/*
		 * Check if the stats object should be included in the snapshot.
		 * Unless the stats kind can be accessed from all databases (e.g.,
		 * database stats themselves), we only include stats for the current
		 * database or objects not associated with a database (e.g. shared
		 * relations).
		 */
		if (p->key.dboid != MyDatabaseId &&
			p->key.dboid != InvalidOid &&
			!kind_info->accessed_across_databases)
			continue;

		if (p->dropped)
			continue;

		Assert(pg_atomic_read_u32(&p->refcount) > 0);

		stats_data = dsa_get_address(pgStatLocal.dsa, p->body);
		Assert(stats_data);

		entry = pgstat_snapshot_insert(pgStatLocal.snapshot.stats, p->key, &found);
		Assert(!found);

		entry->data = MemoryContextAlloc(pgStatLocal.snapshot.context,
										 pgstat_get_entry_len(kind));

		/*
		 * Acquire the LWLock directly instead of using
		 * pg_stat_lock_entry_shared() which requires a reference.
		 */
		LWLockAcquire(&stats_data->lock, LW_SHARED);
		memcpy(entry->data,
			   pgstat_get_entry_data(kind, stats_data),
			   pgstat_get_entry_len(kind));
		LWLockRelease(&stats_data->lock);
	}
	dshash_seq_term(&hstat);

	/*
	 * Build snapshot of all fixed-numbered stats.
/* …（同文件无关代码省略）… */
static inline bool
pgstat_is_kind_valid(PgStat_Kind kind)
{
	return pgstat_is_kind_builtin(kind) || pgstat_is_kind_custom(kind);
}
/* …（同文件无关代码省略）… */
const PgStat_KindInfo *
pgstat_get_kind_info(PgStat_Kind kind)
{
	if (pgstat_is_kind_builtin(kind))
		return &pgstat_kind_builtin_infos[kind];

	if (pgstat_is_kind_custom(kind))
	{
		uint32		idx = kind - PGSTAT_KIND_CUSTOM_MIN;

		if (pgstat_kind_custom_infos == NULL ||
			pgstat_kind_custom_infos[idx] == NULL)
			return NULL;
		return pgstat_kind_custom_infos[idx];
	}

	return NULL;
}
/* …（同文件无关代码省略）… */
			ereport(ERROR,
					(errmsg("failed to register custom cumulative statistics \"%s\" with ID %u", kind_info->name, kind),
					 errhint("Custom cumulative statistics cannot use entry count tracking for fixed-numbered objects.")));
	}
	else
	{
/* …（同文件无关代码省略）… */
#define write_chunk_s(fpout, ptr) write_chunk(fpout, ptr, sizeof(*ptr))
/* …（同文件无关代码省略）… */
static void
write_chunk(FILE *fpout, void *ptr, size_t len)
{
	int			rc;

	rc = fwrite(ptr, len, 1, fpout);

	/* we'll check for errors with ferror once at the end */
	(void) rc;
}
/* …（同文件无关代码省略）… */
	}

	/*
	 * Walk through the stats entries
	 */
	dshash_seq_init(&hstat, pgStatLocal.shared_hash, false);
	while ((ps = dshash_seq_next(&hstat)) != NULL)
	{
		PgStatShared_Common *shstats;
		const PgStat_KindInfo *kind_info = NULL;

		/*
		 * We should not see any "dropped" entries when writing the stats
		 * file, as all backends and auxiliary processes should have cleaned
		 * up their references before they terminated.
		 *
		 * However, since we are already shutting down, it is not worth
		 * crashing the server over any potential cleanup issues, so we simply
		 * skip such entries if encountered.
		 */
		Assert(!ps->dropped);
		if (ps->dropped)
			continue;

		/*
		 * This discards data related to custom stats kinds that are unknown
		 * to this process.
		 */
		if (!pgstat_is_kind_valid(ps->key.kind))
		{
			elog(WARNING, "found unknown stats entry %u/%u/%" PRIu64,
				 ps->key.kind, ps->key.dboid,
				 ps->key.objid);
			continue;
		}

		shstats = (PgStatShared_Common *) dsa_get_address(pgStatLocal.dsa, ps->body);

		kind_info = pgstat_get_kind_info(ps->key.kind);

		/* if not dropped the valid-entry refcount should exist */
		Assert(pg_atomic_read_u32(&ps->refcount) > 0);

		/* skip if no need to write to file */
		if (!kind_info->write_to_file)
			continue;

		if (!kind_info->to_serialized_name)
		{
			/* normal stats entry, identified by PgStat_H
