// AUTO-DRAFT from torvalds/linux PR #f11fe7603df07fbb9fa02b0d385c1696c87a6dab
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
{
	struct f_ncm *ncm = container_of(data, struct f_ncm, task_timer);
	struct net_device *netdev = READ_ONCE(ncm->netdev);

	if (netdev) {
		/* XXX This allowance of a NULL skb argument to ndo_start_xmit
/* …（同文件无关代码省略）… */
		 *
		 * This will call directly into u_ether's eth_start_xmit()
		 */
		netdev->netdev_ops->ndo_start_xmit(NULL, netdev);
	}
	return HRTIMER_NORESTART;
}
