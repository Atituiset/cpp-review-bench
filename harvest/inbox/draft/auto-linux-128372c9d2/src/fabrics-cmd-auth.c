// AUTO-DRAFT from torvalds/linux PR #8ebbaf8835c62d7541a6226672a6c2e8e6a5428a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
/* Restart negotiation */
			pr_debug("%s: ctrl %d qid %d reset negotiation\n",
				 __func__, ctrl->cntlid, req->sq->qid);
			if (!req->sq->qid) {
				dhchap_status = nvmet_setup_auth(ctrl, req->sq,
								 true);
			}
			req->sq->dhchap_step =
				NVME_AUTH_DHCHAP_MESSAGE_NEGOTIATE;
		} else if (data->auth_id != req->sq->dhchap_step)
			goto done_failure1;
		/* Validate negotiation parameters */
		dhchap_status = nvmet_auth_negotiate(req, d, tl);
		if (dhchap_status == 0)
	switch (req->sq->dhchap_step) {
	case NVME_AUTH_DHCHAP_MESSAGE_CHALLENGE:
		if (nvmet_auth_challenge(req, d, al) < 0) {
			pr_warn("ctrl %d qid %d: challenge error (%d)\n",
				ctrl->cntlid, req->sq->qid, status);
			status = NVME_SC_INTERNAL;
			break;
		}
		req->sq->dhchap_step = NVME_AUTH_DHCHAP_MESSAGE_REPLY;
		break;
