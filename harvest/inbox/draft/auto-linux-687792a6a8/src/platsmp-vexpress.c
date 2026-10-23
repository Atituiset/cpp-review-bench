// AUTO-DRAFT from torvalds/linux PR #aad73b807cd1a5bff9c5277e8a42e75bbdc8adab
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void __init vexpress_smp_dt_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *scu = of_find_matching_node(NULL,
			vexpress_smp_dt_scu_match);

	if (scu)
		scu_enable(of_iomap(scu, 0));
