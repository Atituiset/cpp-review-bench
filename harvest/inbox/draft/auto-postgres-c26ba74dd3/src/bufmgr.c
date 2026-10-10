// AUTO-DRAFT from postgres/postgres PR #15a947f30726cadff695b7601329d90b6703e268
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct PrivateRefCountData
{
	/*
	 * How many times has the buffer been pinned by this backend.
	 */
	int32		refcount;

	/*
	 * Is the buffer locked by this backend? BUFFER_LOCK_UNLOCK indicates that
	 * the buffer is not locked.
	 */
	BufferLockMode lockmode;
} PrivateRefCountData;
/* …（同文件无关代码省略）… */
typedef struct PrivateRefCountEntry
{
	/*
	 * Note that this needs to be same as the entry's corresponding
	 * PrivateRefCountArrayKeys[i], if the entry is stored in the array. We
	 * store it in both places as this is used for the hashtable key and
	 * because it is more convenient (passing around a PrivateRefCountEntry
	 * suffices to identify the buffer) and faster (checking the keys array is
	 * faster when checking many entries, checking the entry is faster if just
	 * checking a single entry).
	 */
	Buffer		buffer;

	char		status;

	PrivateRefCountData data;
} PrivateRefCountEntry;
/* …（同文件无关代码省略）… */
#define REFCOUNT_ARRAY_ENTRIES 8
/* …（同文件无关代码省略）… */

static uint32 MaxProportionalPins;

static void ReservePrivateRefCountEntry(void);
static PrivateRefCountEntry *NewPrivateRefCountEntry(Buffer buffer);
static PrivateRefCountEntry *GetPrivateRefCountEntry(Buffer buffer, bool do_move);
/* …（同文件无关代码省略）… */
static void
ReservePrivateRefCountEntry(void)
{
	/* Already reserved (or freed), nothing to do */
	if (ReservedRefCountSlot != -1)
		return;

	/*
	 * First search for a free entry the array, that'll be sufficient in the
	 * majority of cases.
	 */
	{
		int			i;

		for (i = 0; i < REFCOUNT_ARRAY_ENTRIES; i++)
		{
			if (PrivateRefCountArrayKeys[i] == InvalidBuffer)
			{
				ReservedRefCountSlot = i;

				/*
				 * We could return immediately, but iterating till the end of
				 * the array allows compiler-autovectorization.
				 */
			}
		}

		if (ReservedRefCountSlot != -1)
			return;
	}

	/*
	 * No luck. All array entries are full. Move one array entry into the hash
	 * table.
	 */
	{
		/*
		 * Move entry from the current clock position in the array into the
		 * hashtable. Use that slot.
		 */
		int			victim_slot;
		PrivateRefCountEntry *victim_entry;
		PrivateRefCountEntry *hashent;
		bool		found;

		/* select victim slot */
		victim_slot = PrivateRefCountClock++ % REFCOUNT_ARRAY_ENTRIES;
		victim_entry = &PrivateRefCountArray[victim_slot];
		ReservedRefCountSlot = victim_slot;

		/* Better be used, otherwise we shouldn't get here. */
		Assert(PrivateRefCountArrayKeys[victim_slot] != InvalidBuffer);
		Assert(PrivateRefCountArray[victim_slot].buffer != InvalidBuffer);
		Assert(PrivateRefCountArrayKeys[victim_slot] == PrivateRefCountArray[victim_slot].buffer);

		/* enter victim array entry into hashtable */
		hashent = refcount_insert(PrivateRefCountHash,
								  PrivateRefCountArrayKeys[victim_slot],
								  &found);
		Assert(!found);
		/* move data from the entry in the array to the hash entry */
		hashent->data = victim_entry->data;

		/* clear the now free array slot */
		PrivateRefCountArrayKeys[victim_slot] = InvalidBuffer;
		victim_entry->buffer = InvalidBuffer;

		/* clear the whole data member, just for future proofing */
		memset(&victim_entry->data, 0, sizeof(victim_entry->data));
		victim_entry->data.refcount = 0;
		victim_entry->data.lockmode = BUFFER_LOCK_UNLOCK;

		PrivateRefCountOverflowed++;
	}
}
/* …（同文件无关代码省略）… */
static PrivateRefCountEntry *
NewPrivateRefCountEntry(Buffer buffer)
{
	PrivateRefCountEntry *res;

	/* only allowed to be called when a reservation has been made */
	Assert(ReservedRefCountSlot != -1);

	/* use up the reserved entry */
	res = &PrivateRefCountArray[ReservedRefCountSlot];

	/* and fill it */
	PrivateRefCountArrayKeys[ReservedRefCountSlot] = buffer;
	res->buffer = buffer;
	res->data.refcount = 0;
	res->data.lockmode = BUFFER_LOCK_UNLOCK;

	/* update cache for the next lookup */
	PrivateRefCountEntryLast = ReservedRefCountSlot;

	ReservedRefCountSlot = -1;

	return res;
}
/* …（同文件无关代码省略）… */
static pg_noinline PrivateRefCountEntry *
GetPrivateRefCountEntrySlow(Buffer buffer, bool do_move)
{
	PrivateRefCountEntry *res;
	int			match = -1;
	int			i;

	/*
	 * First search for references in the array, that'll be sufficient in the
	 * majority of cases.
	 */
	for (i = 0; i < REFCOUNT_ARRAY_ENTRIES; i++)
	{
		if (PrivateRefCountArrayKeys[i] == buffer)
		{
			match = i;
			/* see ReservePrivateRefCountEntry() for why we don't return */
		}
	}

	if (likely(match != -1))
	{
		/* update cache for the next lookup */
		PrivateRefCountEntryLast = match;

		return &PrivateRefCountArray[match];
	}

	/*
	 * By here we know that the buffer, if already pinned, isn't residing in
	 * the array.
	 *
	 * Only look up the buffer in the hashtable if we've previously overflowed
	 * into it.
	 */
	if (PrivateRefCountOverflowed == 0)
		return NULL;

	res = refcount_lookup(PrivateRefCountHash, buffer);

	if (res == NULL)
		return NULL;
	else if (!do_move)
	{
		/* caller doesn't want us to move the hash entry into the array */
		return res;
	}
	else
	{
		/* move buffer from hashtable into the free array slot */
		PrivateRefCountEntry *free;
		PrivateRefCountData data;

		/* Save data and delete from hashtable while res is still valid */
		data = res->data;
		refcount_delete_item(PrivateRefCountHash, res);
		Assert(PrivateRefCountOverflowed > 0);
		PrivateRefCountOverflowed--;

		/* Ensure there's a free array slot */
		ReservePrivateRefCountEntry();

		/* Use up the reserved slot */
		Assert(ReservedRefCountSlot != -1);
		free = &PrivateRefCountArray[ReservedRefCountSlot];
		Assert(PrivateRefCountArrayKeys[ReservedRefCountSlot] == free->buffer);
		Assert(free->buffer == InvalidBuffer);

		/* and fill it */
		free->buffer = buffer;
		free->data = data;
		PrivateRefCountArrayKeys[ReservedRefCountSlot] = buffer;
		/* update cache for the next lookup */
		PrivateRefCountEntryLast = ReservedRefCountSlot;

		ReservedRefCountSlot = -1;
