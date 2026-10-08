// AUTO-DRAFT from postgres/postgres PR #cb1eef81c9bc8d1a2adc1e3a63669e315b6232bd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static void ResetDecoder(XLogReaderState *state);
static void WALOpenSegmentInit(WALOpenSegment *seg, WALSegmentContext *segcxt,
							   int segsize, const char *waldir);
#if defined(USE_ZSTD) && !defined(FRONTEND)
static void XLogReaderFreeZstdContext(void *arg);
#endif

/* size of the buffer allocated for error message. */
#define MAX_ERRORMSG_LEN 1000
/* …（同文件无关代码省略）… */
void
XLogReaderFree(XLogReaderState *state)
{
	if (state->seg.ws_file != -1)
		state->routine.segment_close(state);

/* …（同文件无关代码省略）… */
	pfree(state);
}

#if defined(USE_ZSTD) && !defined(FRONTEND)
/* Release the zstd decomoression context. */
static void
XLogReaderFreeZstdContext(void *arg)
{
	ZSTD_freeDCtx(arg);
}
#endif

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
