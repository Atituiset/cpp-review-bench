// AUTO-DRAFT from postgres/postgres PR #44539786b8296b6e73035142670e0d1ff2639078
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
/*
 * Types related to shared memory storage of statistics.
 *
 * Per-object statistics are stored in the "shared stats" hashtable. That
 * table's entries (PgStatShared_HashEntry) contain a pointer to the actual stats
 * data for the object (the size of the stats data varies depending on the
 * kind of stats). The table is keyed by PgStat_HashKey.
 *
 * Once a backend has a reference to a shared stats entry, it increments the
 * entry's refcount. Even after stats data is dropped (e.g., due to a DROP
 * TABLE), the entry itself can only be deleted once all references have been
 * released.
 *
 * These refcounts, in combination with a backend local hashtable
 * (pgStatEntryRefHash, with entries pointing to PgStat_EntryRef) in front of
 * the shared hash table, mean that most stats work can happen without
 * touching the shared hash table, reducing contention.
 *
 * Once there are pending stats updates for a table PgStat_EntryRef->pending
 * is allocated to contain a working space for as-of-yet-unapplied stats
 * updates. Once the stats are flushed, PgStat_EntryRef->pending is freed.
 *
 * Each stat kind in the shared hash table has a fixed member
 * PgStatShared_Common as the first element.
 */

/*
/* …（同文件无关代码省略）… */
	 * stats eventually. Each stats kind utilizing pending data defines what
	 * format its pending data has and needs to provide a
	 * PgStat_KindInfo->flush_pending_cb callback to merge pending entries
	 * into the shared stats hash table.
	 */
	void	   *pending;
	dlist_node	pending_node;	/* membership in pgStatPending list */
/* …（同文件无关代码省略）… */
	bool		track_entry_count:1;

	/*
	 * The size of an entry in the shared stats hash table (pointed to by
	 * PgStatShared_HashEntry->body).  For fixed-numbered statistics, this is
	 * the size of an entry in PgStat_ShmemControl->custom_data.
	 */
	uint32		shared_size;

/* …（同文件无关代码省略）… */
#define SLRU_NUM_ELEMENTS	lengthof(slru_names)
/* …（同文件无关代码省略）… */
/*
 * Central shared memory entry for the cumulative stats system.
 *
 * Fixed amount stats, the dynamic shared memory hash table for
 * non-fixed-amount stats, as well as remaining bits and pieces are all
 * reached from here.
 */
typedef struct PgStat_ShmemControl
{
	void	   *raw_dsa_area;

	/*
	 * Stats for variable-numbered objects are kept in this shared hash table.
	 * See comment above PgStat_Kind for details.
	 */
	dshash_table_handle hash_handle;	/* shared dbstat hash */

	/* Has the stats system already been shut down? Just a debugging check. */
	bool		is_shutdown;
/* …（同文件无关代码省略）… */
	pg_atomic_uint64 gc_request_count;

	/*
	 * Counters for the number of entries associated to a single stats kind
	 * that uses variable-numbered objects stored in the shared hash table.
	 * These counters can be enabled on a per-kind basis, when
	 * track_entry_count is set.  This counter is incremented each time a new
	 * entry is created (not reused) in the shared hash table, and is
	 * decremented each time an entry is freed from the shared hash table.
	 */
	pg_atomic_uint64 entry_counts[PGSTAT_KIND_MAX];

/* …（同文件无关代码省略）… */
typedef struct PgStat_Snapshot
{
	PgStat_FetchConsistency mode;

	/* time at which snapshot was taken */
	TimestampTz snapshot_timestamp;

	bool		fixed_valid[PGSTAT_KIND_BUILTIN_SIZE];

	PgStat_ArchiverStats archiver;

	PgStat_BgWriterStats bgwriter;

	PgStat_CheckpointerStats checkpointer;

	PgStat_IO	io;

	PgStat_Lock lock;

	PgStat_SLRUStats slru[SLRU_NUM_ELEMENTS];

	PgStat_WalStats wal;

	/*
	 * Data in snapshot for custom fixed-numbered statistics, indexed by
	 * (PgStat_Kind - PGSTAT_KIND_CUSTOM_MIN).  Each entry is allocated in
	 * TopMemoryContext, for a size of PgStat_KindInfo->shared_data_len.
	 */
	bool		custom_valid[PGSTAT_KIND_CUSTOM_SIZE];
	void	   *custom_data[PGSTAT_KIND_CUSTOM_SIZE];

	/* to free snapshot in bulk */
	MemoryContext context;
	struct pgstat_snapshot_hash *stats;
} PgStat_Snapshot;
/* …（同文件无关代码省略）… */
typedef struct PgStat_LocalState
{
	PgStat_ShmemControl *shmem;
	dsa_area   *dsa;
	dshash_table *shared_hash;

	/* the current statistics snapshot */
	PgStat_Snapshot snapshot;
