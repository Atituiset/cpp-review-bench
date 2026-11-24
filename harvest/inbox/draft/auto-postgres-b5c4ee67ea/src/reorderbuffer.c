// AUTO-DRAFT from postgres/postgres PR #7bb8cc295b538df21d854e55e12a32dbbd8609b1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
		SetupHistoricSnapshot(snapshot_now, NULL);
		PG_TRY();
		{
			rb->message(rb, txn, lsn, false, prefix, message_size, message);

			TeardownHistoricSnapshot(false);
		}
/* …（同文件无关代码省略）… */
						  ReorderBufferChange *change, bool streaming)
{
	if (streaming)
		rb->stream_message(rb, txn, change->lsn, true,
						   change->data.msg.prefix,
						   change->data.msg.message_size,
						   change->data.msg.message);
	else
		rb->message(rb, txn, change->lsn, true,
					change->data.msg.prefix,
					change->data.msg.message_size,
					change->data.msg.message);
}
