// AUTO-DRAFT from postgres/postgres PR #871dee0ccbe292cf4c2874aa7b460d242262d242
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct GlobalChannelKey
{
	Oid			dboid;
	char		channel[NAMEDATALEN];
} GlobalChannelKey;
/* …（同文件无关代码省略）… */
typedef struct ListenerEntry
{
	ProcNumber	procNo;			/* listener's ProcNumber */
	bool		removeOnAbort;	/* remove entry if current xact aborts */
} ListenerEntry;
/* …（同文件无关代码省略）… */
typedef struct GlobalChannelEntry
{
	GlobalChannelKey key;		/* hash key */
	dsa_pointer listenersArray; /* DSA pointer to ListenerEntry array */
	int			numListeners;	/* Number of listeners currently stored */
	int			allocatedListeners; /* Allocated size of array */
} GlobalChannelEntry;
/* …（同文件无关代码省略）… */
typedef enum
{
	PENDING_LISTEN,
	PENDING_UNLISTEN,
} PendingListenAction;
/* …（同文件无关代码省略）… */
typedef struct PendingListenEntry
{
	char		channel[NAMEDATALEN];	/* hash key */
	PendingListenAction action; /* which action should we perform? */
} PendingListenEntry;
/* …（同文件无关代码省略）… */
static inline void
GlobalChannelKeyInit(GlobalChannelKey *key, Oid dboid, const char *channel)
{
	memset(key, 0, sizeof(GlobalChannelKey));
	key->dboid = dboid;
	strlcpy(key->channel, channel, NAMEDATALEN);
}
/* …（同文件无关代码省略）… */
static void
RemoveListenerFromChannel(GlobalChannelEntry **entry_ptr,
						  ListenerEntry *listeners,
						  int idx)
{
	GlobalChannelEntry *entry = *entry_ptr;

	entry->numListeners--;
	if (idx < entry->numListeners)
		memmove(&listeners[idx], &listeners[idx + 1],
				sizeof(ListenerEntry) * (entry->numListeners - idx));

	if (entry->numListeners == 0)
	{
		dsa_free(globalChannelDSA, entry->listenersArray);
		dshash_delete_entry(globalChannelTable, entry);
		/* tells caller not to release the entry's lock: */
		*entry_ptr = NULL;
	}
}
/* …（同文件无关代码省略）… */
static void
ApplyPendingListenActions(bool isCommit)
{
	HASH_SEQ_STATUS seq;
	PendingListenEntry *pending;

	/* Quick exit if nothing to do */
	if (pendingListenActions == NULL)
		return;

	/* We made a globalChannelTable before building pendingListenActions */
	if (globalChannelTable == NULL)
		elog(PANIC, "global channel table missing post-commit/abort");

	/* For each staged action ... */
	hash_seq_init(&seq, pendingListenActions);
	while ((pending = (PendingListenEntry *) hash_seq_search(&seq)) != NULL)
	{
		GlobalChannelKey key;
		GlobalChannelEntry *entry;
		bool		removeLocal = true;
		bool		foundListener = false;

		/*
		 * Find the global entry for this channel.  If isCommit, it had better
		 * exist (it was created in PreCommit).  In an abort, it might not
		 * exist, in which case we are not listening and should discard any
		 * local entry that PreCommit may have managed to create.
		 */
		GlobalChannelKeyInit(&key, MyDatabaseId, pending->channel);
		entry = dshash_find(globalChannelTable, &key, true);
		if (entry != NULL)
		{
			/* Scan entry to find the ListenerEntry for this backend */
			ListenerEntry *listeners;

			listeners = (ListenerEntry *)
				dsa_get_address(globalChannelDSA, entry->listenersArray);

			for (int i = 0; i < entry->numListeners; i++)
			{
				if (listeners[i].procNo != MyProcNumber)
					continue;
				foundListener = true;
				if (isCommit)
				{
					if (pending->action == PENDING_LISTEN)
					{
						/*
						 * LISTEN being committed: entry is now permanent.
						 * localChannelTable entry was created during
						 * PreCommit and should be kept.
						 */
						listeners[i].removeOnAbort = false;
						removeLocal = false;
					}
					else
					{
						/*
						 * UNLISTEN being committed: remove pre-allocated
						 * entries from both tables.
						 */
						RemoveListenerFromChannel(&entry, listeners, i);
					}
				}
				else
				{
					/*
					 * Note: this part is reachable only if the transaction
					 * aborts after PreCommit_Notify() has made some
					 * pendingListenActions entries, so it's pretty hard to
					 * test.
					 */
					if (listeners[i].removeOnAbort)
					{
						/*
						 * Staged LISTEN (or LISTEN+UNLISTEN) being aborted,
						 * so remove pre-allocated entries from both tables.
						 */
						RemoveListenerFromChannel(&entry, listeners, i);
					}
					else
					{
						/*
						 * Entry predates this transaction, so keep the
						 * localChannelTable entry.
						 */
						removeLocal = false;
					}
				}
				break;			/* there shouldn't be another match */
			}

			/* We might have already released the entry by removing it */
			if (entry != NULL)
				dshash_release_lock(globalChannelTable, entry);
		}

		/*
		 * If we're committing a LISTEN action, we should have found a
		 * matching ListenerEntry, but otherwise it's okay if we didn't.
		 */
		if (isCommit && pending->action == PENDING_LISTEN && !foundListener)
			elog(PANIC, "could not find listener entry for channel \"%s\" backend %d",
				 pending->channel, MyProcNumber);

		/*
		 * If we did not find a globalChannelTable entry for our backend, or
		 * if we are unlistening, remove any localChannelTable entry that may
		 * exist.  (Note in particular that this cleans up if we created a
		 * localChannelTable entry and then failed while trying to create a
		 * globalChannelTable entry.)
		 */
		if (removeLocal && localChannelTable != NULL)
			(void) hash_search(localChannelTable, pending->channel,
							   HASH_REMOVE, NULL);
	}
}
/* …（同文件无关代码省略）… */
 *	This is called at transaction abort.
 *
 *	Revert any staged listen/unlisten changes and clean up transaction state.
 *	This only does anything if we abort after PreCommit_Notify has staged
 *	some entries.
 */
void
AtAbort_Notify(void)
{
	/* Revert staged listen/unlisten changes */
	ApplyPendingListenActions(false);

	/* If we're no longer listening on anything, unregister */
