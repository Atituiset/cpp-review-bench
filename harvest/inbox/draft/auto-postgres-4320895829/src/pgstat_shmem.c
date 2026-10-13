// AUTO-DRAFT from postgres/postgres PR #44539786b8296b6e73035142670e0d1ff2639078
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct PgStat_EntryRefHashEntry
{
	PgStat_HashKey key;			/* hash key */
	char		status;			/* for simplehash use */
	PgStat_EntryRef *entry_ref;
} PgStat_EntryRefHashEntry;
/* …（同文件无关代码省略）… */
	.init_fn = StatsShmemInit,
};

/* parameter for the shared hash */
static const dshash_parameters dsh_params = {
	sizeof(PgStat_HashKey),
	sizeof(PgStatShared_HashEntry),
/* …（同文件无关代码省略）… */
 */

/*
 * The size of the shared memory allocation for stats stored in the shared
 * stats hash table. This allocation will be done as part of the main shared
 * memory, rather than dynamic shared memory, allowing it to be initialized in
 * postmaster.
 */
static Size
pgstat_dsa_init_size(void)
{
	Size		sz;

	/*
	 * The dshash header / initial buckets array needs to fit into "plain"
	 * shared memory, but it's beneficial to not need dsm segments
	 * immediately. A size of 256kB seems works well and is not
	 * disproportional compared to other constant sized shared memory
	 * allocations. NB: To avoid DSMs further, the user can configure
	 * min_dynamic_shared_memory.
	 */
	sz = 256 * 1024;
	Assert(dsa_minimum_size() <= sz);
	return MAXALIGN(sz);
}
/* …（同文件无关代码省略）… */
	sz = MAXALIGN(sizeof(PgStat_ShmemControl));
	sz = add_size(sz, pgstat_dsa_init_size());

	/* Add shared memory for all the custom fixed-numbered statistics */
	for (PgStat_Kind kind = PGSTAT_KIND_CUSTOM_MIN; kind <= PGSTAT_KIND_CUSTOM_MAX; kind++)
	{
/* …（同文件无关代码省略）… */
static void
StatsShmemInit(void *arg)
{
	dsa_area   *dsa;
	dshash_table *dsh;
	PgStat_ShmemControl *ctl = pgStatLocal.shmem;
	char	   *p = (char *) ctl;

	/* the allocation of pgStatLocal.shmem itself */
	p += MAXALIGN(sizeof(PgStat_ShmemControl));

	/*
	 * Create a small dsa allocation in plain shared memory. This is required
	 * because postmaster cannot use dsm segments. It also provides a small
	 * efficiency win.
	 */
	ctl->raw_dsa_area = p;
	p += pgstat_dsa_init_size();
	dsa = dsa_create_in_place(ctl->raw_dsa_area,
							  pgstat_dsa_init_size(),
							  LWTRANCHE_PGSTATS_DSA, NULL);
	dsa_pin(dsa);

	/*
	 * To ensure dshash is created in "plain" shared memory, temporarily limit
	 * size of dsa to the initial size of the dsa.
	 */
	dsa_set_size_limit(dsa, pgstat_dsa_init_size());

	/*
	 * With the limit in place, create the dshash table. XXX: It'd be nice if
	 * there were dshash_create_in_place().
	 */
	dsh = dshash_create(dsa, &dsh_params, NULL);
	ctl->hash_handle = dshash_get_hash_table_handle(dsh);

	/* lift limit set above */
	dsa_set_size_limit(dsa, -1);

	/*
	 * Postmaster will never access these again, thus free the local
	 * dsa/dshash references.
	 */
	dshash_detach(dsh);
	dsa_detach(dsa);

	pg_atomic_init_u64(&ctl->gc_request_count, 1);

	/* Do the per-kind initialization */
	for (PgStat_Kind kind = PGSTAT_KIND_MIN; kind <= PGSTAT_KIND_MAX; kind++)
	{
		const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);
		char	   *ptr;

		if (!kind_info)
			continue;

		/* initialize entry count tracking */
		if (kind_info->track_entry_count)
			pg_atomic_init_u64(&ctl->entry_counts[kind - 1], 0);

		/* initialize fixed-numbered stats */
		if (kind_info->fixed_amount)
		{
			if (pgstat_is_kind_builtin(kind))
				ptr = ((char *) ctl) + kind_info->shared_ctl_off;
			else
			{
				int			idx = kind - PGSTAT_KIND_CUSTOM_MIN;

				Assert(kind_info->shared_size != 0);
				ctl->custom_data[idx] = p;
				p += MAXALIGN(kind_info->shared_size);
				ptr = ctl->custom_data[idx];
			}

			kind_info->init_shmem_cb(ptr);
		}
	}
}
/* …（同文件无关代码省略）… */
void
pgstat_attach_shmem(void)
{
	MemoryContext oldcontext;

	Assert(pgStatLocal.dsa == NULL);

	/* stats shared memory persists for the backend lifetime */
	oldcontext = MemoryContextSwitchTo(TopMemoryContext);

	pgStatLocal.dsa = dsa_attach_in_place(pgStatLocal.shmem->raw_dsa_area,
										  NULL);
	dsa_pin_mapping(pgStatLocal.dsa);

	pgStatLocal.shared_hash = dshash_attach(pgStatLocal.dsa, &dsh_params,
											pgStatLocal.shmem->hash_handle,
											NULL);

	MemoryContextSwitchTo(oldcontext);
}

void
pgstat_detach_shmem(void)
{
	Assert(pgStatLocal.dsa);

	/* we shouldn't leave references to shared stats */
	pgstat_release_all_entry_refs(false);

	dshash_detach(pgStatLocal.shared_hash);
	pgStatLocal.shared_hash = NULL;

	dsa_detach(pgStatLocal.dsa);

	/*
	 * dsa_detach() does not decrement the DSA reference count as no segment
/* …（同文件无关代码省略）… */
	 */
	dsa_release_in_place(pgStatLocal.shmem->raw_dsa_area);

	pgStatLocal.dsa = NULL;
}


/* …（同文件无关代码省略）… */
{
	const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);

	return dsa_allocate_extended(pgStatLocal.dsa,
								 kind_info->shared_size,
								 DSA_ALLOC_ZERO | DSA_ALLOC_NO_OOM);
}
/* …（同文件无关代码省略）… */
PgStatShared_Common *
pgstat_init_entry(PgStat_Kind kind,
				  PgStatShared_HashEntry *shhashent,
				  dsa_pointer chunk)
{
	PgStatShared_Common *shheader;
	const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);

	Assert(DsaPointerIsValid(chunk));

	/*
	 * Initialize refcount to 1, marking it as valid / not dropped. The entry
	 * can't be freed before the initialization because it can't be found as
	 * long as we hold the dshash partition lock. Caller needs to increase
	 * further if a longer lived reference is needed.
	 */
	pg_atomic_init_u32(&shhashent->refcount, 1);

	/*
	 * Initialize "generation" to 0, as freshly created.
	 */
	pg_atomic_init_u32(&shhashent->generation, 0);
	shhashent->dropped = false;

	shheader = dsa_get_address(pgStatLocal.dsa, chunk);
	shheader->magic = 0xdeadbeef;

	/* Link the new entry from the hash entry. */
	shhashent->body = chunk;

	/* Increment entry count, if required. */
	if (kind_info->track_entry_count)
		pg_atomic_fetch_add_u64(&pgStatLocal.shmem->entry_counts[kind - 1], 1);

	LWLockInitialize(&shheader->lock, LWTRANCHE_PGSTATS_DATA);

	return shheader;
}
/* …（同文件无关代码省略）… */
static PgStatShared_Common *
pgstat_reinit_entry(PgStat_Kind kind, PgStatShared_HashEntry *shhashent)
{
	PgStatShared_Common *shheader;

	shheader = dsa_get_address(pgStatLocal.dsa, shhashent->body);

	/* mark as not dropped anymore */
	pg_atomic_fetch_add_u32(&shhashent->refcount, 1);

	/*
	 * Increment "generation", to let any backend with local references know
	 * that what they point to is outdated.
	 */
	pg_atomic_fetch_add_u32(&shhashent->generation, 1);
	shhashent->dropped = false;

	/* reinitialize content */
	Assert(shheader->magic == 0xdeadbeef);
	memset(pgstat_get_entry_data(kind, shheader), 0,
		   pgstat_get_entry_len(kind));

	return shheader;
}
/* …（同文件无关代码省略）… */
 */
static void
pgstat_acquire_entry_ref(PgStat_EntryRef *entry_ref,
						 PgStatShared_HashEntry *shhashent,
						 PgStatShared_Common *shheader)
{
/* …（同文件无关代码省略）… */
	 * LWLock can process a pending interrupt, and callers may catch the
	 * resulting error and continue using the backend-local cache.
	 */
	dshash_release_lock(pgStatLocal.shared_hash, shhashent);
}

/*
/* …（同文件无关代码省略）… */
	PgStatShared_HashEntry *shhashent;
	PgStatShared_Common *shheader = NULL;
	PgStat_EntryRef *entry_ref;

	key.kind = kind;
	key.dboid = dboid;
/* …（同文件无关代码省略）… */
	 */
	Assert(create || created_entry == NULL);
	pgstat_assert_is_up();
	Assert(pgStatLocal.shared_hash != NULL);
	Assert(!pgStatLocal.shmem->is_shutdown);

	pgstat_setup_memcxt();
/* …（同文件无关代码省略）… */

	Assert(entry_ref != NULL);

	/*
	 * Do a lookup in the hash table first - it's quite likely that the entry
	 * already exists, and that way we only need a shared lock.
	 */
	shhashent = dshash_find(pgStatLocal.shared_hash, &key, false);

	if (create && !shhashent)
	{
/* …（同文件无关代码省略）… */
		 * lookup. If so, fall through to the same path as if we'd have if it
		 * already had been created before the dshash_find() calls.
		 */
		shhashent = dshash_find_or_insert_extended(pgStatLocal.shared_hash,
												   &key, &shfound,
												   DSHASH_INSERT_NO_OOM);
		if (!shhashent)
		{
			dsa_free(pgStatLocal.dsa, chunk);

			/*
			 * Clean up the local reference when failing insert into the
			 * shared hashtable.
			 */
			pgstat_release_entry_ref(key, entry_ref, false);
			ereport(ERROR,
/* …（同文件无关代码省略）… */
		if (!shfound)
		{
			shheader = pgstat_init_entry(kind, shhashent, chunk);
			pgstat_acquire_entry_ref(entry_ref, shhashent, shheader);

			if (created_entry != NULL)
				*created_entry = true;
/* …（同文件无关代码省略）… */
		}

		/* Concurrent insert won; drop the unused body. */
		dsa_free(pgStatLocal.dsa, chunk);
	}

	if (!shhashent)
/* …（同文件无关代码省略）… */
			 * in the shared entry for any remaining local references.
			 */
			shheader = pgstat_reinit_entry(kind, shhashent);
			pgstat_acquire_entry_ref(entry_ref, shhashent, shheader);

			if (created_entry != NULL)
				*created_entry = true;
/* …（同文件无关代码省略）… */
		}
		else if (shhashent->dropped)
		{
			dshash_release_lock(pgStatLocal.shared_hash, shhashent);
			pgstat_release_entry_ref(key, entry_ref, false);

			return NULL;
		}
		else
		{
			shheader = dsa_get_address(pgStatLocal.dsa, shhashent->body);
			pgstat_acquire_entry_ref(entry_ref, shhashent, shheader);

			return entry_ref;
		}
/* …（同文件无关代码省略）… */
static void
pgstat_release_entry_ref(PgStat_HashKey key, PgStat_EntryRef *entry_ref,
						 bool discard_pending)
{
	if (entry_ref && entry_ref->pending)
	{
		if (discard_pending)
			pgstat_delete_pending_entry(entry_ref);
		else
			elog(ERROR, "releasing ref with pending data");
	}

	if (entry_ref && entry_ref->shared_stats)
	{
		Assert(entry_ref->shared_stats->magic == 0xdeadbeef);
		Assert(entry_ref->pending == NULL);

		/*
		 * This can't race with another backend looking up the stats entry and
		 * increasing the refcount because it is not "legal" to create
		 * additional references to dropped entries.
		 */
		if (pg_atomic_fetch_sub_u32(&entry_ref->shared_entry->refcount, 1) == 1)
		{
			PgStatShared_HashEntry *shent;

			/*
			 * We're the last referrer to this entry, try to drop the shared
			 * entry.
			 */

			/* only dropped entries can reach a 0 refcount */
			Assert(entry_ref->shared_entry->dropped);

			shent = dshash_find(pgStatLocal.shared_hash,
								&entry_ref->shared_entry->key,
								true);
			if (!shent)
				elog(ERROR, "could not find just referenced shared stats entry");

			/*
			 * This entry may have been reinitialized while trying to release
			 * it, so double-check that it has not been reused while holding a
			 * lock on its shared entry.
			 */
			if (pg_atomic_read_u32(&entry_ref->shared_entry->generation) ==
				entry_ref->generation)
			{
				/* Same "generation", so we're OK with the removal */
				Assert(pg_atomic_read_u32(&entry_ref->shared_entry->refcount) == 0);
				Assert(entry_ref->shared_entry == shent);
				pgstat_free_entry(shent, NULL);
			}
			else
			{
				/*
				 * Shared stats entry has been reinitialized, so do not drop
				 * its shared entry, only release its lock.
				 */
				dshash_release_lock(pgStatLocal.shared_hash, shent);
			}
		}
	}

	if (!pgstat_entry_ref_hash_delete(pgStatEntryRefHash, key))
		elog(ERROR, "entry ref vanished before deletion");

	if (entry_ref)
		pfree(entry_ref);
}
/* …（同文件无关代码省略）… */
void
pgstat_request_entry_refs_gc(void)
{
	pg_atomic_fetch_add_u64(&pgStatLocal.shmem->gc_request_count, 1);
}
/* …（同文件无关代码省略）… */
static void
pgstat_release_matching_entry_refs(bool discard_pending, ReleaseMatchCB match,
								   Datum match_data)
{
	pgstat_entry_ref_hash_iterator i;
	PgStat_EntryRefHashEntry *ent;

	if (pgStatEntryRefHash == NULL)
		return;

	pgstat_entry_ref_hash_start_iterate(pgStatEntryRefHash, &i);

	while ((ent = pgstat_entry_ref_hash_iterate(pgStatEntryRefHash, &i))
		   != NULL)
	{
		Assert(ent->entry_ref != NULL);

		if (match && !match(ent, match_data))
			continue;

		pgstat_release_entry_ref(ent->key, ent->entry_ref, discard_pending);
	}
}
/* …（同文件无关代码省略）… */
static void
pgstat_release_all_entry_refs(bool discard_pending)
{
	if (pgStatEntryRefHash == NULL)
		return;

	pgstat_release_matching_entry_refs(discard_pending, NULL, 0);
	Assert(pgStatEntryRefHash->members == 0);
	pgstat_entry_ref_hash_destroy(pgStatEntryRefHash);
	pgStatEntryRefHash = NULL;
}
/* …（同文件无关代码省略）… */
static bool
match_db(PgStat_EntryRefHashEntry *ent, Datum match_data)
{
	Oid			dboid = DatumGetObjectId(match_data);

	return ent->key.dboid == dboid;
}
/* …（同文件无关代码省略）… */
static void
pgstat_release_db_entry_refs(Oid dboid)
{
	pgstat_release_matching_entry_refs( /* discard pending = */ true,
									   match_db,
									   ObjectIdGetDatum(dboid));
}
/* …（同文件无关代码省略）… */
static void
pgstat_free_entry(PgStatShared_HashEntry *shent, dshash_seq_status *hstat)
{
	dsa_pointer pdsa;
	PgStat_Kind kind = shent->key.kind;

	/*
	 * Fetch dsa pointer before deleting entry - that way we can free the
	 * memory after releasing the lock.
	 */
	pdsa = shent->body;

	if (!hstat)
		dshash_delete_entry(pgStatLocal.shared_hash, shent);
	else
		dshash_delete_current(hstat);

	dsa_free(pgStatLocal.dsa, pdsa);

	/* Decrement entry count, if required. */
	if (pgstat_get_kind_info(kind)->track_entry_count)
		pg_atomic_sub_fetch_u64(&pgStatLocal.shmem->entry_counts[kind - 1], 1);
}
/* …（同文件无关代码省略）… */
static bool
pgstat_drop_entry_internal(PgStatShared_HashEntry *shent,
						   dshash_seq_status *hstat)
{
	Assert(shent->body != InvalidDsaPointer);

	/* should already have released local reference */
	if (pgStatEntryRefHash)
		Assert(!pgstat_entry_ref_hash_lookup(pgStatEntryRefHash, shent->key));

	/*
	 * Signal that the entry is dropped - this will eventually cause other
	 * backends to release their references.
	 */
	Assert(!shent->dropped);
	shent->dropped = true;

	/* release refcount marking entry as not dropped */
	if (pg_atomic_sub_fetch_u32(&shent->refcount, 1) == 0)
	{
		pgstat_free_entry(shent, hstat);
		return true;
	}
	else
	{
		if (!hstat)
			dshash_release_lock(pgStatLocal.shared_hash, shent);
		return false;
	}
}
/* …（同文件无关代码省略）… */

	Assert(OidIsValid(dboid));

	Assert(pgStatLocal.shared_hash != NULL);

	/*
	 * This backend might very well be the only backend holding a reference to
	 * about-to-be-dropped entries. Ensure that we're not preventing it from
	 * being cleaned up till later.
	 *
	 * Doing this separately from the dshash iteration below avoids having to
	 * do so while holding a partition lock on the shared hashtable.
	 */
	pgstat_release_db_entry_refs(dboid);

	/* some of the dshash entries are to be removed, take exclusive lock. */
	dshash_seq_init(&hstat, pgStatLocal.shared_hash, true);
	while ((p = dshash_seq_next(&hstat)) != NULL)
	{
		if (p->dropped)
			continue;

		if (p->key.dboid != dboid)
			continue;

		if (!pgstat_drop_entry_internal(p, &hstat))
		{
			/*
			 * Even statistics for a dropped database might currently be
			 * accessed (consider e.g. database stats for pg_stat_database).
			 */
			not_freed_count++;
		}
	}
	dshash_seq_term(&hstat);

	/*
	 * If some of the stats data could not be freed, signal the reference
/* …（同文件无关代码省略）… */
	PgStatShared_HashEntry *shent;
	bool		freed = true;

	key.kind = kind;
	key.dboid = dboid;
	key.objid = objid;
/* …（同文件无关代码省略）… */
									 true);
	}

	/* mark entry in shared hashtable as deleted, drop if possible */
	shent = dshash_find(pgStatLocal.shared_hash, &key, true);
	if (shent)
	{
		if (shent->dropped)
/* …（同文件无关代码省略）… */
					 shent->key.objid,
					 pg_atomic_read_u32(&shent->refcount),
					 pg_atomic_read_u32(&shent->generation));
			dshash_release_lock(pgStatLocal.shared_hash, shent);
			return true;
		}

/* …（同文件无关代码省略）… */
}

/*
 * Scan through the shared hashtable of stats, dropping statistics if
 * approved by the optional do_drop() function.
 */
void
pgstat_drop_matching_entries(bool (*do_drop) (PgStatShared_HashEntry *, Datum),
							 Datum match_data)
{
	dshash_seq_status hstat;
	PgStatShared_HashEntry *ps;
	uint64		not_freed_count = 0;

	/* entries are removed, take an exclusive lock */
	dshash_seq_init(&hstat, pgStatLocal.shared_hash, true);
	while ((ps = dshash_seq_next(&hstat)) != NULL)
	{
		if (ps->dropped)
			continue;

		if (do_drop != NULL && !do_drop(ps, match_data))
			continue;

		/* delete local reference */
		if (pgStatEntryRefHash)
		{
			PgStat_EntryRefHashEntry *lohashent =
				pgstat_entry_ref_hash_lookup(pgStatEntryRefHash, ps->key);

			if (lohashent)
				pgstat_release_entry_ref(lohashent->key, lohashent->entry_ref,
										 true);
		}

		if (!pgstat_drop_entry_internal(ps, &hstat))
			not_freed_count++;
	}
	dshash_seq_term(&hstat);

	if (not_freed_count > 0)
		pgstat_request_entry_refs_gc();
}

/*
 * Scan through the shared hashtable of stats and drop all entries.
 */
void
pgstat_drop_all_entries(void)
{
	pgstat_drop_matching_entries(NULL, 0);
}
/* …（同文件无关代码省略）… */
static void
shared_stat_reset_contents(PgStat_Kind kind, PgStatShared_Common *header,
						   TimestampTz ts)
{
	const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);

	memset(pgstat_get_entry_data(kind, header), 0,
		   pgstat_get_entry_len(kind));

	if (kind_info->reset_timestamp_cb)
		kind_info->reset_timestamp_cb(header, ts);
}

/*
 * Reset one variable-numbered stats entry.
 */
/* …（同文件无关代码省略）… */
}

/*
 * Scan through the shared hashtable of stats, resetting statistics if
 * approved by the provided do_reset() function.
 */
void
pgstat_reset_matching_entries(bool (*do_reset) (PgStatShared_HashEntry *, Datum),
							  Datum match_data, TimestampTz ts)
{
	dshash_seq_status hstat;
	PgStatShared_HashEntry *p;

	/* dshash entry is not modified, take shared lock */
	dshash_seq_init(&hstat, pgStatLocal.shared_hash, false);
	while ((p = dshash_seq_next(&hstat)) != NULL)
	{
		PgStatShared_Common *header;

		if (p->dropped)
			continue;

		if (!do_reset(p, match_data))
			continue;

		header = dsa_get_address(pgStatLocal.dsa, p->body);

		LWLockAcquire(&header->lock, LW_EXCLUSIVE);

		shared_stat_reset_contents(p->key.kind, header, ts);

		LWLockRelease(&header->lock);
	}
	dshash_seq_term(&hstat);
}

static bool
match_kind(PgStatShared_HashEntry *p, Datum match_data)
{
	return p->key.kind == DatumGetInt32(match_data);
}
/* …（同文件无关代码省略）… */
void
pgstat_reset_entries_of_kind(PgStat_Kind kind, TimestampTz ts)
{
	pgstat_reset_matching_entries(match_kind, Int32GetDatum(kind), ts);
}

static void
pgstat_setup_memcxt(void)
{
	if (unlikely(!pgStatSharedRefContext))
		pgStatSharedRefContext =
			AllocSetContextCreate(TopMemoryContext,
								  "PgStat Shared Ref",
								  ALLOCSET_SMALL_SIZES);
	if (unlikely(!pgStatEntryRefHashContext))
		pgStatEntryRefHashContext =
			AllocSetContextCreate(TopMemoryContext,
								  "PgStat Shared Ref Hash",
								  ALLOCSET_SMALL_SIZES);
}
