// AUTO-DRAFT from torvalds/linux PR #59e72644a7e697fdaf3494e8df8af9ab2b2ba31d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>


		if (xdp_do_redirect(bp->dev, xdp, xdp_prog)) {
			trace_xdp_exception(bp->dev, xdp_prog, act);
			page_pool_recycle_direct(rxr->page_pool, page);
			return true;
		}
