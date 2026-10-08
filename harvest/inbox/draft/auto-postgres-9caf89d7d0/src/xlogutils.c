// AUTO-DRAFT from postgres/postgres PR #cb1eef81c9bc8d1a2adc1e3a63669e315b6232bd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <errno.h>

	XLogFilePath(path, tli, nextSegNo, state->segcxt.ws_segsize);
	state->seg.ws_file = BasicOpenFile(path, O_RDONLY | PG_BINARY);
	if (state->seg.ws_file >= 0)
		return;

	if (errno == ENOENT)
		ereport(ERROR,
