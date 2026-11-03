// AUTO-DRAFT from postgres/postgres PR #51ae21aa313bb6b79ad914920279cd0d89e92220
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdio.h>
  // <<< BUG ANCHOR
#include "access/xlog_internal.h"
#include "access/xlogutils.h"
#include "access/xlogwait.h"
#include "commands/repack.h"
#include "commands/repack_internal.h"
#include "libpq/libpq.h"
/* …（同文件无关代码省略）… */
#define PGREPACK_PLUGIN   "pgrepack"

static void RepackWorkerShutdown(int code, Datum arg);
static LogicalDecodingContext *repack_setup_logical_decoding(Oid relid);
static void repack_cleanup_logical_decoding(LogicalDecodingContext *ctx);
static void export_initial_snapshot(Snapshot snapshot,
									DecodingWorkerShared *shared);
/* …（同文件无关代码省略）… */
	/*
	 * Prepare to capture the concurrent data changes ourselves.
	 */
	decoding_ctx = repack_setup_logical_decoding(shared->relid);

	/* Announce that we're ready. */
	SpinLockAcquire(&shared->mutex);
/* …（同文件无关代码省略）… */
static void
RepackWorkerShutdown(int code, Datum arg)
{
	dsm_segment *seg;
	DecodingWorkerShared *shared;
	pid_t		pid;
	ProcNumber	procno;

	seg = (dsm_segment *) DatumGetPointer(arg);
	shared = (DecodingWorkerShared *) dsm_segment_address(seg);
	pid = shared->backend_pid;
	procno = shared->backend_proc_number;

	/*
	 * Detach from the shared memory segment before we signal the backend.
	 * Detaching also detaches the error message queue, and the backend learns
	 * that we are gone by reading that queue when it handles our signal. If
	 * we signaled first, the backend could read the queue while it still
	 * looks attached, and nothing would make it read again.
	 */
	dsm_detach(seg);

	SendProcSignal(pid, PROCSIG_REPACK_MESSAGE, procno);
}
/* …（同文件无关代码省略）… */
	return am_repack_worker;
}

/*
 * This function is much like pg_create_logical_replication_slot() except that
 * the new slot is neither released (if anyone else could read changes from
/* …（同文件无关代码省略）… */
 * crash by restarting all the work from scratch).
 */
static LogicalDecodingContext *
repack_setup_logical_decoding(Oid relid)
{
	Relation	rel;
	Oid			toastrelid;
/* …（同文件无关代码省略）… */
	snprintf(slotname, NAMEDATALEN, "pg_repack_%d", MyProcPid);
	ReplicationSlotCreate(slotname, true, RS_TEMPORARY, false, true,
						  false, false);
	EnsureLogicalDecodingEnabled();

	/*
/* …（同文件无关代码省略）… */
	}
	table_close(rel, AccessShareLock);

	/*
	 * Set up our logical decoding context.  We initially use the blocking
	 * read_local_xlog_page until we find the start point, and switch to the
/* …（同文件无关代码省略）… */
static void
repack_cleanup_logical_decoding(LogicalDecodingContext *ctx)
{
	RepackDecodingState *dstate;

	dstate = (RepackDecodingState *) ctx->output_writer_private;
	if (dstate->slot)
		ExecDropSingleTupleTableSlot(dstate->slot);

	FreeDecodingContext(ctx);
	ReplicationSlotDropAcquired(true);
}
/* …（同文件无关代码省略）… */
static void
export_initial_snapshot(Snapshot snapshot, DecodingWorkerShared *shared)
{
	char		fname[MAXPGPATH];
	BufFile    *file;
	Size		snap_size;
	char	   *snap_space;

	snap_size = EstimateSnapshotSpace(snapshot);
	snap_space = (char *) palloc(snap_size);
	SerializeSnapshot(snapshot, snap_space);

	DecodingWorkerFileName(fname, shared->relid, shared->last_exported + 1);
	file = BufFileCreateFileSet(&shared->sfs.fs, fname);
	/* To make restoration easier, write the snapshot size first. */
	BufFileWrite(file, &snap_size, sizeof(snap_size));
	BufFileWrite(file, snap_space, snap_size);
	BufFileClose(file);
	pfree(snap_space);

	/* Increase the counter to tell the backend that the file is available. */
	SpinLockAcquire(&shared->mutex);
	shared->last_exported++;
	SpinLockRelease(&shared->mutex);
	ConditionVariableSignal(&shared->cv);
}
