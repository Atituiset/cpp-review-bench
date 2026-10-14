// AUTO-DRAFT from postgres/postgres PR #f2dd2a3bd44eb1d158e6e507b86dc3df787043b1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
  // <<< BUG ANCHOR
#define WALSENDER_STATS_FLUSH_INTERVAL         1000
/* …（同文件无关代码省略）… */
	if (xlogreader != NULL && xlogreader->seg.ws_file >= 0)
		wal_segment_close(xlogreader);

	if (MyReplicationSlot != NULL)
		ReplicationSlotRelease();

/* …（同文件无关代码省略）… */
static void
WalSndShutdown(void)
{
	/*
	 * Reset whereToSendOutput to prevent ereport from attempting to send any
	 * more messages to the standby.
	 */
	if (whereToSendOutput == DestRemote)
		whereToSendOutput = DestNone;

	proc_exit(0);
}
/* …（同文件无关代码省略）… */
		Assert(streamingDoneSending && streamingDoneReceiving);
	}

	if (cmd->slotname)
		ReplicationSlotRelease();

/* …（同文件无关代码省略）… */
	WalSndLoop(XLogSendLogical);

	FreeDecodingContext(logical_decoding_ctx);
	ReplicationSlotRelease();

	replication_active = false;
/* …（同文件无关代码省略）… */
static void
WalSndHandleConfigReload(void)
{
	if (!ConfigReloadPending)
		return;

	ConfigReloadPending = false;
	ProcessConfigFile(PGC_SIGHUP);
	SyncRepInitConfig();

	/*
	 * Recheck and release any now-satisfied waiters after config reload
	 * changes synchronous replication requirements (e.g., reducing the number
	 * of sync standbys or changing the standby names).
	 */
	if (!am_cascading_walsender)
		SyncRepReleaseWaiters();
}
/* …（同文件无关代码省略）… */

	CHECK_FOR_INTERRUPTS();

	/*
	 * Prepare to parse and execute the command.
	 *
/* …（同文件无关代码省略）… */
				/* dupe, but necessary per libpqrcv_endstreaming */
				EndReplicationCommand(cmdtag);

				Assert(xlogreader != NULL);
				break;
			}

/* …（同文件无关代码省略）… */
static void
ProcessRepliesIfAny(void)
{
	unsigned char firstchar;
	int			maxmsglen;
	int			r;
	bool		received = false;

	last_processing = GetCurrentTimestamp();

	/*
	 * If we already received a CopyDone from the frontend, any subsequent
	 * message is the beginning of a new command, and should be processed in
	 * the main processing loop.
	 */
	while (!streamingDoneReceiving)
	{
		pq_startmsgread();
		r = pq_getbyte_if_available(&firstchar);
		if (r < 0)
		{
			/* unexpected error or EOF */
			ereport(COMMERROR,
					(errcode(ERRCODE_PROTOCOL_VIOLATION),
					 errmsg("unexpected EOF on standby connection")));
			proc_exit(0);
		}
		if (r == 0)
		{
			/* no data available without blocking */
			pq_endmsgread();
			break;
		}

		/* Validate message type and set packet size limit */
		switch (firstchar)
		{
			case PqMsg_CopyData:
				maxmsglen = PQ_LARGE_MESSAGE_LIMIT;
				break;
			case PqMsg_CopyDone:
			case PqMsg_Terminate:
				maxmsglen = PQ_SMALL_MESSAGE_LIMIT;
				break;
			default:
				ereport(FATAL,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("invalid standby message type \"%c\"",
								firstchar)));
				maxmsglen = 0;	/* keep compiler quiet */
				break;
		}

		/* Read the message contents */
		resetStringInfo(&reply_message);
		if (pq_getmessage(&reply_message, maxmsglen))
		{
			ereport(COMMERROR,
					(errcode(ERRCODE_PROTOCOL_VIOLATION),
					 errmsg("unexpected EOF on standby connection")));
			proc_exit(0);
		}

		/* ... and process it */
		switch (firstchar)
		{
				/*
				 * PqMsg_CopyData means a standby reply wrapped in a CopyData
				 * packet.
				 */
			case PqMsg_CopyData:
				ProcessStandbyMessage();
				received = true;
				break;

				/*
				 * PqMsg_CopyDone means the standby requested to finish
				 * streaming.  Reply with CopyDone, if we had not sent that
				 * already.
				 */
			case PqMsg_CopyDone:
				if (!streamingDoneSending)
				{
					pq_putmessage_noblock(PqMsg_CopyDone, NULL, 0);
					streamingDoneSending = true;
				}

				streamingDoneReceiving = true;
				received = true;
				break;

				/*
				 * PqMsg_Terminate means that the standby is closing down the
				 * socket.
				 */
			case PqMsg_Terminate:
				proc_exit(0);

			default:
				Assert(false);	/* NOT REACHED */
		}
	}

	/*
	 * Save the last reply timestamp if we've received at least one reply.
	 */
	if (received)
	{
		last_reply_timestamp = last_processing;
		waiting_for_ping_response = false;
	}
}
/* …（同文件无关代码省略）… */
static void
ProcessStandbyMessage(void)
{
	char		msgtype;

	/*
	 * Check message type from the first byte.
	 */
	msgtype = pq_getmsgbyte(&reply_message);

	switch (msgtype)
	{
		case PqReplMsg_StandbyStatusUpdate:
			ProcessStandbyReplyMessage();
			break;

		case PqReplMsg_HotStandbyFeedback:
			ProcessStandbyHSFeedbackMessage();
			break;

		case PqReplMsg_PrimaryStatusRequest:
			ProcessStandbyPSRequestMessage();
			break;

		default:
			ereport(COMMERROR,
					(errcode(ERRCODE_PROTOCOL_VIOLATION),
					 errmsg("unexpected message type \"%c\"", msgtype)));
			proc_exit(0);
	}
}
/* …（同文件无关代码省略）… */
static void
PhysicalConfirmReceivedLocation(XLogRecPtr lsn)
{
	bool		changed = false;
	ReplicationSlot *slot = MyReplicationSlot;

	Assert(XLogRecPtrIsValid(lsn));
	SpinLockAcquire(&slot->mutex);
	if (slot->data.restart_lsn != lsn)
	{
		changed = true;
		slot->data.restart_lsn = lsn;
	}
	SpinLockRelease(&slot->mutex);

	if (changed)
	{
		ReplicationSlotMarkDirty();
		ReplicationSlotsComputeRequiredLSN();
		PhysicalWakeupLogicalWalSnd();
	}

	/*
	 * One could argue that the slot should be saved to disk now, but that'd
	 * be energy wasted - the worst thing lost information could cause here is
	 * to give wrong information in a statistics view - we'll just potentially
	 * be more conservative in removing files.
	 */
}
/* …（同文件无关代码省略）… */
static void
ProcessStandbyReplyMessage(void)
{
	XLogRecPtr	writePtr,
				flushPtr,
				applyPtr;
	bool		replyRequested;
	TimeOffset	writeLag,
				flushLag,
				applyLag;
	bool		clearLagTimes;
	TimestampTz now;
	TimestampTz replyTime;

	static XLogRecPtr prevWritePtr = InvalidXLogRecPtr;
	static XLogRecPtr prevFlushPtr = InvalidXLogRecPtr;
	static XLogRecPtr prevApplyPtr = InvalidXLogRecPtr;

	/* the caller already consumed the msgtype byte */
	writePtr = pq_getmsgint64(&reply_message);
	flushPtr = pq_getmsgint64(&reply_messa
