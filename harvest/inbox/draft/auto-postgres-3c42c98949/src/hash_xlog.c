// AUTO-DRAFT from postgres/postgres PR #8becd1123c98cd9338678c6c2d4ae6b4d56a6fb9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
#include "access/bufmask.h"
#include "access/hash.h"
#include "access/hash_xlog.h"
#include "access/xlogutils.h"
#include "storage/standby.h"

/* …（同文件无关代码省略）… */
	else
	{
		/*
		 * we don't care for return value as the purpose of reading bucketbuf
		 * is to ensure a cleanup lock on primary bucket page.
		 */
		(void) XLogReadBufferForRedoExtended(record, 0, RBM_NORMAL, true, &bucketbuf);

		action = XLogReadBufferForRedo(record, 1, &writebuf);
	}
/* …（同文件无关代码省略）… */
	else
	{
		/*
		 * we don't care for return value as the purpose of reading bucketbuf
		 * is to ensure a cleanup lock on primary bucket page.
		 */
		(void) XLogReadBufferForRedoExtended(record, 0, RBM_NORMAL, true, &bucketbuf);

		if (xldata->ntups > 0 || xldata->is_prev_bucket_same_wrt)
			action = XLogReadBufferForRedo(record, 1, &writebuf);
/* …（同文件无关代码省略）… */
	else
	{
		/*
		 * we don't care for return value as the purpose of reading bucketbuf
		 * is to ensure a cleanup lock on primary bucket page.
		 */
		(void) XLogReadBufferForRedoExtended(record, 0, RBM_NORMAL, true, &bucketbuf);

		action = XLogReadBufferForRedo(record, 1, &deletebuf);
	}
