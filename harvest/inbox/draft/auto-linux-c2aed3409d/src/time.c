// AUTO-DRAFT from torvalds/linux PR #65ba8f34ec886ec72b2c07ddc8cf3d815c32869d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <time.h>

		of_node_put(cpu);
		if (!IS_ERR(clk)) {
			ccount_freq = clk_get_rate(clk);
			return;
		} else {
			pr_warn("%s: CPU input clock not found\n",
