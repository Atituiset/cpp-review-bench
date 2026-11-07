// AUTO-DRAFT from torvalds/linux PR #c53d8fc9a7571b63482548a7e5284ef9f025fd3f
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

bool blk_mq_sched_try_insert_merge(struct request_queue *q, struct request *rq,
				   struct list_head *free)
{
	return rq_mergeable(rq) && elv_attempt_insert_merge(q, rq, free);
}
EXPORT_SYMBOL_GPL(blk_mq_sched_try_insert_merge);

/* called in queue's release handler, tagset has gone away */
static void blk_mq_sched_tags_teardown(struct request_queue *q, unsigned int flags)
{
	struct blk_mq_hw_ctx *hctx;
/* …（同文件无关代码省略）… */

	if (blk_mq_is_shared_tags(flags))
		q->sched_shared_tags = NULL;
}

void blk_mq_sched_reg_debugfs(struct request_queue *q)
{
	struct blk_mq_hw_ctx *hctx;
	unsigned int memflags;
	unsigned long i;

	memflags = blk_debugfs_lock(q);
	blk_mq_debugfs_register_sched(q);
	queue_for_each_hw_ctx(q, hctx, i)
		blk_mq_debugfs_register_sched_hctx(q, hctx);
	blk_debugfs_unlock(q, memflags);
}
/* …（同文件无关代码省略）… */
	queue_for_each_hw_ctx(q, hctx, i) {
		if (blk_mq_is_shared_tags(flags))
			hctx->sched_tags = q->sched_shared_tags;
		else
			hctx->sched_tags = et->tags[i];
	}
