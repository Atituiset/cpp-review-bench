// AUTO-DRAFT from postgres/postgres PR #67c10344ddbd1073f1b7aafb09d1b196a48735a1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
static inline MultiXactId
NextMultiXactId(MultiXactId multi)
{
	return multi == MaxMultiXactId ? FirstMultiXactId : multi + 1;
}
/* …（同文件无关代码省略）… */
#define MultiXactOffsetCtl	(&MultiXactOffsetSlruDesc)
#define MultiXactMemberCtl	(&MultiXactMemberSlruDesc)
/* …（同文件无关代码省略）… */
typedef struct mXactCacheEnt
{
	MultiXactId multi;
	int			nmembers;
	dlist_node	node;
	pg_attribute_counted_by(nmembers) MultiXactMember members[FLEXIBLE_ARRAY_MEMBER];
} mXactCacheEnt;
/* …（同文件无关代码省略）… */
#define MAX_CACHE_ENTRIES	256
/* …（同文件无关代码省略）… */
#define debug_elog2(a,b) elog(a,b)
#define debug_elog3(a,b,c) elog(a,b,c)
#define debug_elog4(a,b,c,d) elog(a,b,c,d)
/* …（同文件无关代码省略）… */
typedef struct MultiXactMemberSlruReadContext
{
	MultiXactId multi;
	MultiXactOffset offset;
} MultiXactMemberSlruReadContext;
/* …（同文件无关代码省略）… */
	int			nmembers;
	int			i;
	int			j;

	Assert(MultiXactIdIsValid(multi));
	Assert(TransactionIdIsValid(xid));
/* …（同文件无关代码省略）… */
		return newMulti;
	}

	/*
	 * If the TransactionId is already a member of the MultiXactId with the
	 * same status, just return the existing MultiXactId.
	 */
	for (i = 0; i < nmembers; i++)
	{
		if (TransactionIdEquals(members[i].xid, xid) &&
			(members[i].status == status))
		{
			debug_elog4(DEBUG2, "Expand: %u is already a member of %u",
						xid, multi);
			pfree(members);
			return multi;
		}
	}

	/*
	 * Determine which of the members of the MultiXactId are still of
	 * interest. This is any running transaction, and also any transaction
/* …（同文件无关代码省略）… */
	 * update Xid in a multixact would cause errors elsewhere.)
	 *
	 * Removing dead members is not just an optimization: freezing of tuples
	 * whose Xmax are multis depends on this behavior.
	 *
	 * Note we have the same race condition here as above: j could be 0 at the
	 * end of the loop.
	 */
	newMembers = palloc_array(MultiXactMember, nmembers + 1);

	for (i = 0, j = 0; i < nmembers; i++)
	{
		if (TransactionIdIsInProgress(members[i].xid) ||
			(ISUPDATE_from_mxstatus(members[i].status) &&
			 TransactionIdDidCommit(members[i].xid)))
		{
			newMembers[j].xid = members[i].xid;
			newMembers[j++].status = members[i].status;
		}
	}

	newMembers[j].xid = xid;
	newMembers[j++].status = status;
	newMulti = MultiXactIdCreateFromMembers(j, newMembers);

	pfree(members);
	pfree(newMembers);

	debug_elog3(DEBUG2, "Expand: returning new multi %u", newMulti);

	return newMulti;
}

/* …（同文件无关代码省略）… */
MultiXactId
MultiXactIdCreateFromMembers(int nmembers, MultiXactMember *members)
{
	MultiXactId multi;
	MultiXactOffset offset;
	xl_multixact_create xlrec;

	debug_elog3(DEBUG2, "Create: %s",
				mxid_to_string(InvalidMultiXactId, nmembers, members));

	/*
	 * See if the same set of members already exists in our cache; if so, just
	 * re-use that MultiXactId.  (Note: it might seem that looking in our
	 * cache is insufficient, and we ought to search disk to see if a
	 * duplicate definition already exists.  But since we only ever create
	 * MultiXacts containing our own XID, in most cases any such MultiXacts
	 * were in fact created by us, and so will be in our cache.  There are
	 * corner cases where someone else added us to a MultiXact without our
	 * knowledge, but it's not worth checking for.)
	 */
	multi = mXactCacheGetBySet(nmembers, members);
	if (MultiXactIdIsValid(multi))
	{
		debug_elog2(DEBUG2, "Create: in cache!");
		return multi;
	}

	/* Verify that there is a single update Xid among the given members. */
	{
		int			i;
		bool		has_update = false;

		for (i = 0; i < nmembers; i++)
		{
			if (ISUPDATE_from_mxstatus(members[i].status))
			{
				if (has_update)
					elog(ERROR, "new multixact has more than one updating member: %s",
						 mxid_to_string(InvalidMultiXactId, nmembers, members));
				has_update = true;
			}
		}
	}

	/* Load the injection point before entering the critical section */
	INJECTION_POINT_LOAD("multixact-create-from-members");

	/*
	 * Assign the MXID and offsets range to use, and make sure there is space
	 * in the OFFSETs and MEMBERs files.  NB: this routine does
	 * START_CRIT_SECTION().
	 *
	 * Note: unlike MultiXactIdCreate and MultiXactIdExpand, we do not check
	 * that we've called MultiXactIdSetOldestMember here.  This is because
	 * this routine is used in some places to create new MultiXactIds of which
	 * the current backend is not a member, notably during freezing of multis
	 * in vacuum.  During vacuum, in particular, it would be unacceptable to
	 * keep OldestMulti set, in case it runs for long.
	 */
	multi = GetNewMultiXactId(nmembers, &offset);

	INJECTION_POINT_CACHED("multixact-create-from-members", NULL);

	/*
	 * Prevent checkpoints from starting between the WAL record and the update
	 * of the OFFSETs and MEMBERs pages.  Otherwise the checkpoint could flush
	 * the pages before they hold the new multixact, while recovery after a
	 * crash would begin after the WAL record, and the multixact would be lost
	 * even though data pages might already refer to it.
	 */
	Assert((MyProc->delayChkptFlags & DELAY_CHKPT_START) == 0);
	MyProc->delayChkptFlags |= DELAY_CHKPT_START;

	/* Make an XLOG entry describing the new MXID. */
	xlrec.mid = multi;
	xlrec.moff = offset;
	xlrec.nmembers = nmembers;

	/*
	 * XXX Note: there's a lot of padding space in MultiXactMember.  We could
	 * find a more compact representation of this Xlog record -- perhaps all
	 * the status flags in one XLogRecData, then all the xids in another one?
	 * Not clear that it's worth the trouble though.
	 */
	XLogBeginInsert();
	XLogRegisterData(&xlrec, SizeOfMultiXactCreate);
	XLogRegisterData(members, nmembers * sizeof(MultiXactMember));

	(void) XLogInsert(RM_MULTIXACT_ID, XLOG_MULTIXACT_CREATE_ID);

	/* Now enter the information into the OFFSETs and MEMBERs logs */
	RecordNewMultiXact(multi, offset, nmembers, members);

	MyProc->delayChkptFlags &= ~DELAY_CHKPT_START
