// AUTO-DRAFT from postgres/postgres PR #31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct PgStat_HashKey
{
	PgStat_Kind kind;			/* statistics entry kind */
	Oid			dboid;			/* database ID. InvalidOid for shared objects. */
	uint64		objid;			/* object ID (table, function, etc.), or
								 * identifier. */
} PgStat_HashKey;
/* …（同文件无关代码省略）… */
typedef struct PgStatShared_HashEntry
{
	PgStat_HashKey key;			/* hash key */

	/*
	 * If dropped is set, backends need to release their references so that
	 * the memory for the entry can be freed. No new references may be made
	 * once marked as dropped.
	 */
	bool		dropped;

	/*
	 * Refcount managing lifetime of the entry itself (as opposed to the
	 * dshash entry pointing to it). The stats lifetime has to be separate
	 * from the hash table entry lifetime because we allow backends to point
	 * to a stats entry without holding a hash table lock (and some other
	 * reasons).
	 *
	 * As long as the entry is not dropped, 1 is added to the refcount
	 * representing that the entry should not be dropped. In addition each
	 * backend that has a reference to the entry needs to increment the
	 * refcount as long as it does.
	 *
	 * May only be incremented / decremented while holding at least a shared
	 * lock on the dshash partition containing the entry. It needs to be an
	 * atomic variable because multiple backends can increment the refcount
	 * with just a shared lock.
	 *
	 * When the refcount reaches 0 the entry needs to be freed.
	 */
	pg_atomic_uint32 refcount;

	/*
	 * Counter tracking the number of times the entry has been reused.
	 *
	 * Set to 0 when the entry is created, and incremented by one each time
	 * the shared entry is reinitialized with pgstat_reinit_entry().
	 *
	 * May only be incremented / decremented while holding at least a shared
	 * lock on the dshash partition containing the entry. Like refcount, it
	 * needs to be an atomic variable because multiple backends can increment
	 * the generation with just a shared lock.
	 */
	pg_atomic_uint32 generation;

	/*
	 * Pointer to shared stats. The stats entry always starts with
	 * PgStatShared_Common, embedded in a larger struct containing the
	 * PgStat_Kind specific stats fields.
	 */
	dsa_pointer body;
} PgStatShared_HashEntry;
/* …（同文件无关代码省略）… */
typedef struct PgStatShared_Common
{
	uint32		magic;			/* just a validity cross-check */
	/* lock protecting stats contents (i.e. data following the header) */
	LWLock		lock;
} PgStatShared_Common;
/* …（同文件无关代码省略）… */
typedef struct PgStat_EntryRef
{
	/*
	 * Pointer to the PgStatShared_HashEntry entry in the shared stats
	 * hashtable.
	 */
	PgStatShared_HashEntry *shared_entry;

	/*
	 * Pointer to the stats data (i.e. PgStatShared_HashEntry->body), resolved
	 * as a local pointer, to avoid repeated dsa_get_address() calls.
	 */
	PgStatShared_Common *shared_stats;

	/*
	 * Copy of PgStatShared_HashEntry->generation, keeping locally track of
	 * the shared stats entry "generation" retrieved (number of times reused).
	 */
	uint32		generation;

	/*
	 * Pending statistics data that will need to be flushed to shared memory
	 * stats eventually. Each stats kind utilizing pending data defines what
	 * format its pending data has and needs to provide a
	 * PgStat_KindInfo->flush_pending_cb callback to merge pending entries
	 * into shared stats hashtable entries.
	 */
	void	   *pending;
	dlist_node	pending_node;	/* membership in pgStatPending list */
} PgStat_EntryRef;
/* …（同文件无关代码省略）… */
extern bool pgstat_drop_entry(PgStat_Kind kind, Oid dboid, uint64 objid,
							  bool missing_ok);
extern void pgstat_drop_all_entries(void);
extern void pgstat_drop_matching_entries(bool (*do_drop) (PgStatShared_HashEntry *, Datum),
										 Datum match_data);
extern PgStat_EntryRef *pgstat_get_entry_ref_locked(PgStat_Kind kind, Oid dboid, uint64 objid,
													bool nowait);
