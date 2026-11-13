// AUTO-DRAFT from torvalds/linux PR #dee3429e94d8089713645706df678ec54a049e13
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <net/gso.h>
#include <net/nsh.h>
#include <net/tun_proto.h>

/* …（同文件无关代码省略）… */

	skb_reset_network_header(skb);

	outer_proto = skb->protocol;
	outer_hlen = skb_mac_header_len(skb);
	mac_len = skb->mac_len;
