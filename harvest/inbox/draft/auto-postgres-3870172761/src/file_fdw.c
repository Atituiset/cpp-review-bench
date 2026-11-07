// AUTO-DRAFT from postgres/postgres PR #72b9498df9a369e747f8d96efb5a6cd33c2451e2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
	CopyFromState cstate;
	ErrorContextCallback errcallback;
	MemoryContext oldcontext = CurrentMemoryContext;
	MemoryContext tupcontext;

	Assert(onerel);
	Assert(targrows > 0);
/* …（同文件无关代码省略）… */
						   options);

	/*
	 * Use per-tuple memory context to prevent leak of memory used to read
	 * rows from the file with Copy routines.
	 */
	tupcontext = AllocSetContextCreate(CurrentMemoryContext,
									   "file_fdw temporary context",
									   ALLOCSET_DEFAULT_SIZES);

	/* Prepare for sampling rows */
	reservoir_init_selection_state(&rstate, targrows);
/* …（同文件无关代码省略）… */
		/* Check for user-requested abort or sleep */
		vacuum_delay_point(true);

		/* Fetch next row */
		MemoryContextReset(tupcontext);
		MemoryContextSwitchTo(tupcontext);

		found = NextCopyFrom(cstate, NULL, values, nulls);

		MemoryContextSwitchTo(oldcontext);

/* …（同文件无关代码省略）… */
	error_context_stack = errcallback.previous;

	/* Clean up. */
	MemoryContextDelete(tupcontext);

	if (cstate->opts.on_error == COPY_ON_ERROR_IGNORE &&
		cstate->num_errors > 0 &&
