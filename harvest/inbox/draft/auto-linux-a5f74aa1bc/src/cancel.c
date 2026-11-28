// AUTO-DRAFT from torvalds/linux PR #7cd8495c0f16275c4f9abe1cef09a7b5fcd6d046
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
static bool io_match_linked(struct io_kiocb *head)
{
	struct io_kiocb *req;

	io_for_each_link(req, head) {
		if (req->flags & REQ_F_INFLIGHT)
			return true;
	}
	return false;
}
/* …（同文件无关代码省略）… */
bool io_match_task_safe(struct io_kiocb *head, struct io_uring_task *tctx,
			bool cancel_all)
{
	bool matched;

	if (tctx && head->tctx != tctx)
		return false;
	if (cancel_all)
		return true;

	if (head->flags & REQ_F_LINK_TIMEOUT) {
		struct io_ring_ctx *ctx = head->ctx;

		/* protect against races with linked timeouts */
		raw_spin_lock_irq(&ctx->timeout_lock);
		matched = io_match_linked(head);
		raw_spin_unlock_irq(&ctx->timeout_lock);
	} else {
		matched = io_match_linked(head);
	}
	return matched;
}
