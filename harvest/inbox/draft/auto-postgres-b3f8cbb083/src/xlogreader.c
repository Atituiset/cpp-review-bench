// AUTO-DRAFT from postgres/postgres PR #ce3387b74b708a679282ba5c72990b532d661bb8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#ifndef FRONTEND
#include "pgstat.h"
#include "storage/bufmgr.h"
#include "utils/wait_event.h"
#else
#include "common/logging.h"
/* …（同文件无关代码省略）… */
static void ResetDecoder(XLogReaderState *state);
static void WALOpenSegmentInit(WALOpenSegment *seg, WALSegmentContext *segcxt,
							   int segsize, const char *waldir);

/* size of the buffer allocated for error message. */
#define MAX_ERRORMSG_LEN 1000
/* …（同文件无关代码省略）… */
	pfree(state->errormsg_buf);
	if (state->readRecordBuf)
		pfree(state->readRecordBuf);
	pfree(state->readBuf);
	pfree(state);
}

/*
 * Allocate readRecordBuf to fit a record of at least the given length.
 *
/* …（同文件无关代码省略）… */
static void
WALOpenSegmentInit(WALOpenSegment *seg, WALSegmentContext *segcxt,
				   int segsize, const char *waldir)
{
	seg->ws_file = -1;
	seg->ws_segno = 0;
	seg->ws_tli = 0;

	segcxt->ws_segsize = segsize;
	if (waldir)
		snprintf(segcxt->ws_dir, MAXPGPATH, "%s", waldir);
}
/* …（同文件无关代码省略）… */
static void
ResetDecoder(XLogReaderState *state)
{
	DecodedXLogRecord *r;

	/* Reset the decoded record queue, freeing any oversized records. */
	while ((r = state->decode_queue_head) != NULL)
	{
		state->decode_queue_head = r->next;
		if (r->oversized)
			pfree(r);
	}
	state->decode_queue_tail = NULL;
	state->decode_queue_head = NULL;
	state->record = NULL;

	/* Reset the decode buffer to empty. */
	state->decode_buffer_tail = state->decode_buffer;
	state->decode_buffer_head = state->decode_buffer;

	/* Clear error state. */
	state->errormsg_buf[0] = '\0';
	state->errormsg_deferred = false;
}
/* …（同文件无关代码省略）… */
		else if ((bkpb->bimg_info & BKPIMAGE_COMPRESS_ZSTD) != 0)
		{
#ifdef USE_ZSTD
			size_t		decomp_result = ZSTD_decompress(tmp.data,
														BLCKSZ - bkpb->hole_length,
														ptr, bkpb->bimg_len);

			if (ZSTD_isError(decomp_result))
				decomp_success = false;
