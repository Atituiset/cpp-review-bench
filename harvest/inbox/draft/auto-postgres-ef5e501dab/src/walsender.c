// AUTO-DRAFT from postgres/postgres PR #bc85373ff1344c03abf0d74e698b3631936ca3c7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define LAG_TRACKER_BUFFER_SIZE 8192
/* …（同文件无关代码省略）… */
static void WalSndCheckTimeOut(void);
static void WalSndCheckShutdownTimeout(void);
static long WalSndComputeSleeptime(TimestampTz now);
static void WalSndWait(uint32 socket_events, long timeout, uint32 wait_event);
static void WalSndPrepareWrite(LogicalDecodingContext *ctx, XLogRecPtr lsn, TransactionId xid, bool last_write);
static void WalSndWriteData(LogicalDecodingContext *ctx, XLogRecPtr lsn, TransactionId xid, bool last_write);
static void WalSndUpdateProgress(LogicalDecodingContext *ctx, XLogRecPtr lsn, TransactionId xid,
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
static void
WalSndPrepareWrite(LogicalDecodingContext *ctx, XLogRecPtr lsn, TransactionId xid, bool last_write)
{
	/* can't have sync rep confused by sending the same LSN several times */
	if (!last_write)
		lsn = InvalidXLogRecPtr;

	resetStringInfo(ctx->out);

	pq_sendbyte(ctx->out, PqReplMsg_WALData);
	pq_sendint64(ctx->out, lsn);	/* dataStart */
	pq_sendint64(ctx->out, lsn);	/* walEnd */

	/*
	 * Fill out the sendtime later, just as it's done in XLogSendPhysical, but
	 * reserve space here.
	 */
	pq_sendint64(ctx->out, 0);	/* sendtime */
}
/* …（同文件无关代码省略）… */
static void
WalSndWriteData(LogicalDecodingContext *ctx, XLogRecPtr lsn, TransactionId xid,
				bool last_write)
{
	TimestampTz now;

	/*
	 * Fill the send timestamp last, so that it is taken as late as possible.
	 * This is somewhat ugly, but the protocol is set as it's already used for
	 * several releases by streaming physical replication.
	 */
	resetStringInfo(&tmpbuf);
	now = GetCurrentTimestamp();
	pq_sendint64(&tmpbuf, now);
	memcpy(&ctx->out->data[1 + sizeof(int64) + sizeof(int64)],
		   tmpbuf.data, sizeof(int64));

	/* output previously gathered data in a CopyData packet */
	pq_putmessage_noblock(PqMsg_CopyData, ctx->out->data, ctx->out->len);

	CHECK_FOR_INTERRUPTS();

	/* Try to flush pending output to the client */
	if (pq_flush_if_writable() != 0)
		WalSndShutdown();

	/* Try taking fast path unless we get too close to walsender timeout. */
	if (now < TimestampTzPlusMilliseconds(last_reply_timestamp,
										  wal_sender_timeout / 2) &&
		!pq_is_send_pending())
	{
		return;
	}

	/* If we have pending write here, go to slow path */
	ProcessPendingWrites();
}
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
static void
ProcessPendingWrites(void)
{
	for (;;)
	{
		long		sleeptime;

		/* Check for input from the client */
		ProcessRepliesIfAny();

		/* die if timeout was reached */
		WalSndCheckTimeOut();

		/*
		 * During shutdown, die if the shutdown timeout expires. Call this
		 * before WalSndComputeSleeptime() so the timeout is considered when
		 * computing sleep time.
		 */
		WalSndCheckShutdownTimeout();

		/* Send keepalive if the time has come */
		WalSndKeepaliveIfNecessary();

		if (!pq_is_send_pending())
			break;

		sleeptime = WalSndComputeSleeptime(GetCurrentTimestamp());

		/* Sleep until something happens or we time out */
		WalSndWait(WL_SOCKET_WRITEABLE | WL_SOCKET_READABLE, sleeptime,
				   WAIT_EVENT_WAL_SENDER_WRITE_DATA);

		/* Clear any already-pending wakeups */
		ResetLatch(MyLatch);

		CHECK_FOR_INTERRUPTS();

		/* Process any requests or signals received recently */
		WalSndHandleConfigReload();

		/* Try to flush pending output to the client */
		if (pq_flush_if_writable() != 0)
			WalSndShutdown();
	}

	/* reactivate latch so WalSndLoop knows to continue */
	SetLatch(MyLatch);
}
/* …（同文件无关代码省略）… */
static void
WalSndUpdateProgress(LogicalDecodingContext *ctx, XLogRecPtr lsn, TransactionId xid,
					 bool skipped_xact)
{
	static TimestampTz sendTime = 0;
	TimestampTz now = GetCurrentTimestamp();
	bool		pending_writes = false;
	bool		end_xact = ctx->end_xact;

	/*
	 * Track lag no more than once per WALSND_LOGICAL_LAG_TRACK_INTERVAL_MS to
	 * avoid flooding the lag tracker when we commit frequently.
	 *
	 * We don't have a mechanism to get the ack for any LSN other than end
	 * xact LSN from the downstream. So, we track lag only for end of
	 * transaction LSN.
	 */
#define WALSND_LOGICAL_LAG_TRACK_INTERVAL_MS	1000
	if (end_xact && TimestampDifferenceExceeds(sendTime, now,
											   WALSND_LOGICAL_LAG_TRACK_INTERVAL_MS))
	{
		LagTrackerWrite(lsn, now);
		sendTime = now;
	}

	/*
	 * When skipping empty transactions in synchronous replication, we send a
	 * keepalive message to avoid delaying such transactions.
	 *
	 * It is okay to check sync_standbys_status without lock here as in the
	 * worst case we will just send an extra keepalive message when it is
	 * really not required.
	 */
	if (skipped_xact &&
		SyncRepRequested() &&
		(((volatile WalSndCtlData *) WalSndCtl)->sync_standbys_status & SYNC_STANDBY_DEFINED))
	{
		WalSndKeepalive(false, lsn);

		/* Try to flush pending output to the client */
		if (pq_flush_if_writable() != 0)
			WalSndShutdown();

		/* If we have pending write here, make sure it's actually flushed */
		if (pq_is_send_pending())
			pending_writes = true;
	}

	/*
	 * Process pending writes if any or try to send a keepalive if required.
	 * We don't need to try sending keep alive messages at the transaction end
	 * as that will be done at a l
