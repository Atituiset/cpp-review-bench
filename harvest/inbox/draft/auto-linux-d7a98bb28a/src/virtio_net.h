// AUTO-DRAFT from torvalds/linux PR #7683df331d296819a8f596dd752bbf95aa2e3478
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/udp.h>
#include <net/tcp.h>
#include <uapi/linux/tcp.h>
#include <uapi/linux/virtio_net.h>
/* …（同文件无关代码省略）… */
static inline bool virtio_net_hdr_match_proto(__be16 protocol, __u8 gso_type)
{
	switch (gso_type & ~VIRTIO_NET_HDR_GSO_ECN) {
	case VIRTIO_NET_HDR_GSO_TCPV4:
		return protocol == cpu_to_be16(ETH_P_IP);
	case VIRTIO_NET_HDR_GSO_TCPV6:
		return protocol == cpu_to_be16(ETH_P_IPV6);
	case VIRTIO_NET_HDR_GSO_UDP:
	case VIRTIO_NET_HDR_GSO_UDP_L4:
		return protocol == cpu_to_be16(ETH_P_IP) ||
		       protocol == cpu_to_be16(ETH_P_IPV6);
	default:
		return false;
	}
}
/* …（同文件无关代码省略）… */
static inline int virtio_net_hdr_set_proto(struct sk_buff *skb,
					   const struct virtio_net_hdr *hdr)
{
	if (skb->protocol)
		return 0;

	switch (hdr->gso_type & ~VIRTIO_NET_HDR_GSO_ECN) {
	case VIRTIO_NET_HDR_GSO_TCPV4:
	case VIRTIO_NET_HDR_GSO_UDP:
	case VIRTIO_NET_HDR_GSO_UDP_L4:
		skb->protocol = cpu_to_be16(ETH_P_IP);
		break;
	case VIRTIO_NET_HDR_GSO_TCPV6:
		skb->protocol = cpu_to_be16(ETH_P_IPV6);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static inline int __virtio_net_hdr_to_skb(struct sk_buff *skb,
					  const struct virtio_net_hdr *hdr,
					  bool little_endian, u8 hdr_gso_type)
{
	unsigned int nh_min_len = sizeof(struct iphdr);
	unsigned int gso_type = 0;
	unsigned int thlen = 0;
	unsigned int p_off = 0;
	unsigned int ip_proto;

	if (hdr_gso_type != VIRTIO_NET_HDR_GSO_NONE) {
		switch (hdr_gso_type & ~VIRTIO_NET_HDR_GSO_ECN) {
		case VIRTIO_NET_HDR_GSO_TCPV4:
			gso_type = SKB_GSO_TCPV4;
			ip_proto = IPPROTO_TCP;
			thlen = sizeof(struct tcphdr);
			break;
		case VIRTIO_NET_HDR_GSO_TCPV6:
			gso_type = SKB_GSO_TCPV6;
			ip_proto = IPPROTO_TCP;
			thlen = sizeof(struct tcphdr);
			nh_min_len = sizeof(struct ipv6hdr);
			break;
		case VIRTIO_NET_HDR_GSO_UDP:
			gso_type = SKB_GSO_UDP;
			ip_proto = IPPROTO_UDP;
			thlen = sizeof(struct udphdr);
			break;
		case VIRTIO_NET_HDR_GSO_UDP_L4:
			gso_type = SKB_GSO_UDP_L4;
			ip_proto = IPPROTO_UDP;
			thlen = sizeof(struct udphdr);
			break;
		default:
			return -EINVAL;
		}

		if (hdr_gso_type & VIRTIO_NET_HDR_GSO_ECN)
			gso_type |= SKB_GSO_TCP_ECN;

		if (hdr->gso_size == 0)
			return -EINVAL;
	}

	skb_reset_mac_header(skb);

	if (hdr->flags & VIRTIO_NET_HDR_F_NEEDS_CSUM) {
		u32 start = __virtio16_to_cpu(little_endian, hdr->csum_start);
		u32 off = __virtio16_to_cpu(little_endian, hdr->csum_offset);
		u32 needed = start + max_t(u32, thlen, off + sizeof(__sum16));

		if (!pskb_may_pull(skb, needed))
			return -EINVAL;

		if (!skb_partial_csum_set(skb, start, off))
			return -EINVAL;
		if (skb_transport_offset(skb) < nh_min_len)
			return -EINVAL;

		nh_min_len = skb_transport_offset(skb);
		p_off = nh_min_len + thlen;
		if (!pskb_may_pull(skb, p_off))
			return -EINVAL;
	} else if (gso_type) {
		/* gso packets without NEEDS_CSUM do not set transport_offset.
		 * probe and drop if does not match one of the above types.
		 */
		struct flow_keys_basic keys;

		if (!skb->protocol) {
			skb->protocol = dev_parse_header_protocol(skb);
			if (!skb->protocol)
				virtio_net_hdr_set_proto(skb, hdr);
		}
retry:
		if (!skb_flow_dissect_flow_keys_basic(NULL, skb, &keys,
						      NULL, 0, 0, 0,
						      0)) {
			/* UFO does not specify ipv4 or 6: try both */
			if (gso_type & SKB_GSO_UDP &&
			    skb->protocol == htons(ETH_P_IP)) {
				skb->protocol = htons(ETH_P_IPV6);
				goto retry;
			}
			return -EINVAL;
		}

		p_off = keys.control.thoff + thlen;
		if (!pskb_may_pull(skb, p_off) ||
		    keys.basic.ip_proto != ip_proto ||
		    !virtio_net_hdr_match_proto(keys.basic.n_proto,
						hdr_gso_type))
			return -EINVAL;

		skb_set_transport_header(skb, keys.control.thoff);
	}

	if (hdr_gso_type != VIRTIO_NET_HDR_GSO_NONE) {
		u16 gso_size = __virtio16_to_cpu(little_endian, hdr->gso_size);
		unsigned int nh_off = p_off;
		struct skb_shared_info *shinfo = skb_shinfo(skb);

		switch (gso_type & ~SKB_GSO_TCP_ECN) {
		case SKB_GSO_UDP:
			/* UFO may not include transport header in gso_size. */
			nh_off -= thlen;
			break;
		case SKB_GSO_UDP_L4:
			if (!(hdr->flags & VIRTIO_NET_HDR_F_NEEDS_CSUM))
				return -EINVAL;
			if (skb->csum_offset != offsetof(struct udphdr, check))
				return -EINVAL;
			if (skb->len - p_off > gso_size * UDP_MAX_SEGMENTS)
				return -EINVAL;
			if (gso_type != SKB_GSO_UDP_L4)
				return -EINVAL;
			break;
		case SKB_GSO_TCPV4:
		case SKB_GSO_TCPV6:
			if (skb->ip_summed == CHECKSUM_PARTIAL &&
			    skb->csum_offset != offsetof(struct tcphdr, check))
				return -EINVAL;

			BUILD_BUG_ON(TCP_MIN_GSO_SIZE * GSO_MAX_SEGS < GSO_MAX_SIZE);
			gso_size = max(gso_size, TCP_MIN_GSO_SIZE);
			break;
		}

		/* Kernel has a special handling for GSO_BY_FRAGS. */
		if (gso_size == GSO_BY_FRAGS)
			return -EINVAL;

		/* Too small packets are not really GSO ones. */
		if (skb->len - nh_off > gso_size) {
			shinfo->gso_size = gso_size;
			shinfo->gso_type = gso_type;

			/* Header must be checked, and gso_segs computed. */
			shinfo->gso_type |= SKB_GSO_DODGY;
			shinfo->gso_segs = 0;
		}
	}

	return 0;
}
/* …（同文件无关代码省略）… */
static inline int virtio_net_hdr_to_skb(struct sk_buff *skb,
					const struct virtio_net_hdr *hdr,
					bool little_endian)
{
	return __virtio_net_hdr_to_skb(skb, hdr, little_endian, hdr->gso_type);
}
/* …（同文件无关代码省略）… */
static inline void __virtio_net_set_tnl_hdrlen(const struct sk_buff *skb,
					       struct virtio_net_hdr *hdr)
{
	u16 hdr_len;

	hdr_len = skb_inner_transport_offset(skb);

	if (hdr->gso_type == VIRTIO_NET_HDR_GSO_UDP_L4)
		hdr_len += sizeof(struct udphdr);
	else
		hdr_len += inner_tcp_hdrlen(skb);

	hdr->hdr_len = __cpu_to_virtio16(true, hdr_len);
}
/* …（同文件无关代码省略）… */
static inline unsigned int virtio_l3min(bool is_ipv6)
{
	return is_ipv6 ? sizeof(struct ipv6hdr) : sizeof(struct iphdr);
}

static inline int
virtio_net_hdr_tnl_to_skb(struct sk_buff *skb,
			  const struct virtio_net_hdr_v1_hash_tunnel *vhdr,
			  bool tnl_hdr_negotiated,
			  bool tnl_csum_negotiated,
			  bool little_endian)
{
	const struct virtio_net_hdr *hdr = (const struct virtio_net_hdr *)vhdr;
	unsigned int inner_nh, outer_th, inner_th;
	unsigned int inner_l3min, outer_l3min;
	u8 gso_inner_type, gso_tunnel_type;
	bool outer_isv6, inner_isv6;
	int ret;

	gso_tunnel_type = hdr->gso_type & VIRTIO_NET_HDR_GSO_UDP_TUNNEL;
	if (!gso_tunnel_type)
		return virtio_net_hdr_to_skb(skb, hdr, little_endian);

	/* Tunnel not supported/negotiated, but the hdr asks for it. */
	if (!tnl_hdr_negotiated)
		return -EINVAL;

	/* Either ipv4 or ipv6. */
	if (gso_tunnel_type == VIRTIO_NET_HDR_GSO_UDP_TUNNEL)
		return -EINVAL;

	/* The UDP tunnel must carry a GSO packet, but no UFO. */
	gso_inner_type = hdr->gso_type & ~(VIRTIO_NET_HDR_GSO_ECN |
					   VIRTIO_NET_HDR_GSO_UDP_TUNNEL);
	if (!gso_inner_type || gso_inner_type == VIRTIO_NET_HDR_GSO_UDP)
		return -EINVAL;

	/* Rely on csum being present. */
	if (!(hdr->flags & VIRTIO_NET_HDR_F_NEEDS_CSUM))
		return -EINVAL;

	/* Validate offsets. */
	outer_isv6 = gso_tunnel_type & VIRTIO_NET_HDR_GSO_UDP_TUNNEL_IPV6;
	inner_isv6 = gso_inner_type == VIRTIO_NET_HDR_GSO_TCPV6;
	inner_l3min = virtio_l3min(inner_isv6);
	outer_l3min = ETH_HLEN + virtio_l3min(outer_isv6);

	inner_th = __virtio16_to_cpu(little_endian, hdr->csum_start);
	inner_nh = le16_to_cpu(vhdr->inner_nh_offset);
	outer_th = le16_to_cpu(vhdr->outer_th_offset);
	if (outer_th < outer_l3min ||
	    inner_nh < outer_th + sizeof(struct udphdr) ||
	    inner_th < inner_nh + inner_l3min)
		return -EINVAL;

	/* Let the basic parsing deal with plain GSO features. */
	ret = __virtio_net_hdr_to_skb(skb, hdr, true,
				      hdr->gso_type & ~gso_tunnel_type);
	if (ret)
		return ret;

	/* In case of USO, the inner protocol is still unknown and
	 * `inner_isv6` is just a guess, additional parsing is needed.
	 * The previous validation ensures that accessing an ipv4 inner
	 * network header is safe.
	 */
	if (gso_inner_type == VIRTIO_NET_HDR_GSO_UDP_L4) {
		struct iphdr *iphdr = (struct iphdr *)(skb->data + inner_nh);

		inner_isv6 = iphdr->version == 6;
		inner_l3min = virtio_l3min(inner_isv6);
		if (inner_th < inner_nh + inner_l3min)
			return -EINVAL;
	}

	skb_set_inner_protocol(skb, inner_isv6 ? htons(ETH_P_IPV6) :
						 htons(ETH_P_IP));
	if (hdr->flags & VIRTIO_NET_HDR_F_UDP_TUNNEL_CSUM) {
		if (!tnl_csum_negotiated)
			return -EINVAL;

		skb_shinfo(skb)->gso_type |= SKB_GSO_UDP_TUNNEL_CSUM;
	} else {
		skb_shinfo(skb)->gso_type |= SKB_GSO_UDP_TUNNEL;
	}

	skb->inner_transport_header = inner_th + skb_headroom(skb);
	skb->inner_network_header = inner_nh + skb_headroom(skb);
	skb->inner_mac_header = inner_nh + skb_headroom(skb);
	skb->transport_header = outer_th + skb_headroom(skb);
	skb->encapsulation = 1;
	return 0;
}
/* …（同文件无关代码省略）… */
	if (feature_hdrlen && hdr->hdr_len)
		__virtio_net_set_tnl_hdrlen(skb, hdr);

	if (skb->protocol == htons(ETH_P_IPV6))
		hdr->gso_type |= VIRTIO_NET_HDR_GSO_UDP_TUNNEL_IPV6;
	else
		hdr->gso_type |= VIRTIO_NET_HDR_GSO_UDP_TUNNEL_IPV4;

	if (skb_shinfo(skb)->gso_type & SKB_GSO_UDP_TUNNEL_CSUM)
		hdr->flags |= VIRTIO_NET_HDR_F_UDP_TUNNEL_CSUM;

	inner_nh = skb->inner_network_header - skb_headroom(skb);
	outer_th = skb->transport_header - skb_headroom(skb);
	vhdr->inner_nh_offset = cpu_to_le16(inner_nh);
	vhdr->outer_th_offset = cpu_to_le16(outer_th);
	return 0;
