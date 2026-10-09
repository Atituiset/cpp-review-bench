// AUTO-DRAFT from torvalds/linux PR #fe4167bf53b61405f97614f644f6a63a5d6ad034
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <string.h>

	u8 l4_hash = skb->l4_hash;
	u8 sw_hash = skb->sw_hash;
	u32 hash = skb->hash;
	skb_scrub_packet(skb, true);
	memset(&skb->headers, 0, sizeof(skb->headers));
	if (encapsulating) {
		skb->l4_hash = l4_hash;
		skb->sw_hash = sw_hash;
		skb->hash = hash;
	}
	skb->queue_mapping = 0;
	skb->nohdr = 0;
