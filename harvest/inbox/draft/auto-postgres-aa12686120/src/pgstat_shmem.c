// AUTO-DRAFT from postgres/postgres PR #e9d0600aba5467b352eb67aaf6ad92a9a4776b0a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
			PgStatShared_HashEntry *shent;

			/*
			 * We're the last referrer to this entry, try to drop the shared
			 * entry.
			 */

			/* only dropped entries can reach a 0 refcount */
			Assert(entry_ref->shared_entry->dropped);

			shent = dshash_find(pgStatLocal.kind_hash[key.kind],
								&entry_ref->shared_entry->key, true);
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
				dshash_release_lock(pgStatLocal.kind_hash[key.kind], shent);
			}
		}
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
		dshash_delete_entry(pgStatLocal.kind_hash[kind], shent);
	else
		dshash_delete_current(hstat);

	dsa_free(pgStatLocal.kind_dsa[kind], pdsa);

	/* Decrement entry count, if required. */
	if (pgstat_get_kind_info(kind)->track_entry_count)
		pg_atomic_sub_fetch_u64(&pgStatLocal.shmem->entry_counts[kind - 1], 1);
}
