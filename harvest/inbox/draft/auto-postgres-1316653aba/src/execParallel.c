// AUTO-DRAFT from postgres/postgres PR #675dd940bc459f3e1748ffcd82c48f24c29b2a80
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
	MemoryContextSwitchTo(oldcontext);

	planstate->worker_instrument->num_workers = instrumentation->num_workers;
	memcpy(&planstate->worker_instrument->instrument, instrument, ibytes);

	/* Perform any node-type-specific work that needs to be done. */
	switch (nodeTag(planstate))
