// AUTO-DRAFT from torvalds/linux PR #af32da41b0327b9c6a37856ba82b6760d6c8d10e
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
static struct mld2_grec mldv2_zero_grec;
#endif

static struct amt_skb_cb *amt_skb_cb(struct sk_buff *skb)
{
	BUILD_BUG_ON(sizeof(struct amt_skb_cb) + sizeof(struct tc_skb_cb) >
		     sizeof_field(struct sk_buff, cb));

	return (struct amt_skb_cb *)((void *)skb->cb +
		sizeof(struct tc_skb_cb));
}

static void __amt_source_gc_work(void)
{
	struct amt_source_node *snode;
/* …（同文件无关代码省略）… */
static void __amt_update_relay_status(struct amt_tunnel_list *tunnel,
				      enum amt_status status,
				      bool validate)
{
	if (validate && tunnel->status >= status)
		return;
	netdev_dbg(tunnel->amt->dev,
		   "Update Tunnel(IP = %pI4, PORT = %u) status %s -> %s",
		   &tunnel->ip4, ntohs(tunnel->source_port),
		   status_str[tunnel->status], status_str[status]);
	tunnel->status = status;
}
/* …（同文件无关代码省略）… */
static void amt_update_relay_status(struct amt_tunnel_list *tunnel,
				    enum amt_status status, bool validate)
{
	spin_lock_bh(&tunnel->lock);
	__amt_update_relay_status(tunnel, status, validate);
	spin_unlock_bh(&tunnel->lock);
}
/* …（同文件无关代码省略）… */
	rcu_read_unlock();
}

static void amt_send_igmp_gq(struct amt_dev *amt,
			     struct amt_tunnel_list *tunnel)
{
/* …（同文件无关代码省略）… */
	if (!skb)
		return;

	amt_skb_cb(skb)->tunnel = tunnel;
	dev_queue_xmit(skb);
}

#if IS_ENABLED(CONFIG_IPV6)
static struct sk_buff *amt_build_mld_gq(struct amt_dev *amt)
{
	u8 ra[AMT_IP6HDR_OPTS] = { IPPROTO_ICMPV6, 0, IPV6_TLV_ROUTERALERT,
				   2, 0, 0, IPV6_TLV_PAD1, IPV6_TLV_PAD1 };
	int hlen = LL_RESERVED_SPACE(amt->dev);
	int tlen = amt->dev->needed_tailroom;
	struct mld2_query *mld2q;
	void *csum_start = NULL;
	struct ipv6hdr *ip6h;
	struct sk_buff *skb;
	struct ethhdr *eth;
	u32 len;

	len = hlen + tlen + sizeof(*ip6h) + sizeof(ra) + sizeof(*mld2q);
	skb = netdev_alloc_skb_ip_align(amt->dev, len);
	if (!skb)
		return NULL;

	skb_reserve(skb, hlen);
	skb_push(skb, sizeof(*eth));
	skb_reset_mac_header(skb);
	eth = eth_hdr(skb);
	skb->priority = TC_PRIO_CONTROL;
	skb->protocol = htons(ETH_P_IPV6);
	skb_put_zero(skb, sizeof(*ip6h));
	skb_put_data(skb, ra, sizeof(ra));
	skb_put_zero(skb, sizeof(*mld2q));
	skb_pull(skb, sizeof(*eth));
	skb_reset_network_header(skb);
	ip6h			= ipv6_hdr(skb);
	ip6h->payload_len	= htons(sizeof(ra) + sizeof(*mld2q));
	ip6h->nexthdr		= NEXTHDR_HOP;
	ip6h->hop_limit		= 1;
	ip6h->daddr		= mld2_all_node;
	ip6_flow_hdr(ip6h, 0, 0);

	if (ipv6_dev_get_saddr(amt->net, amt->dev, &ip6h->daddr, 0,
			       &ip6h->saddr)) {
		amt->dev->stats.tx_errors++;
		kfree_skb(skb);
		return NULL;
	}

	eth->h_proto = htons(ETH_P_IPV6);
	ether_addr_copy(eth->h_source, amt->dev->dev_addr);
	ipv6_eth_mc_map(&mld2_all_node, eth->h_dest);

	skb_pull(skb, sizeof(*ip6h) + sizeof(ra));
	skb_reset_transport_header(skb);
	mld2q			= (struct mld2_query *)icmp6_hdr(skb);
	mld2q->mld2q_mrc	= htons(1);
	mld2q->mld2q_type	= ICMPV6_MGM_QUERY;
	mld2q->mld2q_code	= 0;
	mld2q->mld2q_cksum	= 0;
	mld2q->mld2q_resv1	= 0;
	mld2q->mld2q_resv2	= 0;
	mld2q->mld2q_suppress	= 0;
	mld2q->mld2q_qrv	= amt->qrv;
	mld2q->mld2q_nsrcs	= 0;
	mld2q->mld2q_qqic	= amt->qi;
	csum_start		= (void *)mld2q;
	mld2q->mld2q_cksum = csum_ipv6_magic(&ip6h->saddr, &ip6h->daddr,
					     sizeof(*mld2q),
					     IPPROTO_ICMPV6,
					     csum_partial(csum_start,
							  sizeof(*mld2q), 0));

	skb->ip_summed = CHECKSUM_NONE;
	skb_push(skb, sizeof(*eth) + sizeof(*ip6h) + sizeof(ra));
	return skb;
}
/* …（同文件无关代码省略）… */
static void amt_send_mld_gq(struct amt_dev *amt, struct amt_tunnel_list *tunnel)
{
	struct sk_buff *skb;

	skb = amt_build_mld_gq(amt);
	if (!skb)
		return;

	amt_skb_cb(skb)->tunnel = tunnel;
	dev_queue_xmit(skb);
}
#else
static void amt_send_mld_gq(struct amt_dev *amt, struct amt_tunnel_list *tunnel)
/* …（同文件无关代码省略）… */
static bool amt_send_membership_query(struct amt_dev *amt,
				      struct sk_buff *skb,
				      struct amt_tunnel_list *tunnel,
				      bool v6)
{
	struct amt_header_membership_query *amtmq;
	struct rtable *rt;
	struct flowi4 fl4;
	struct sock *sk;
	int err;

	sk = rcu_dereference_bh(amt->sk);
	if (!sk)
		return true;

	err = skb_cow_head(skb, LL_RESERVED_SPACE(amt->dev) + sizeof(*amtmq) +
			   sizeof(struct iphdr) + sizeof(struct udphdr));
	if (err)
		return true;

	skb_reset_inner_headers(skb);
	memset(&fl4, 0, sizeof(struct flowi4));
	fl4.flowi4_oif         = amt->stream_dev->ifindex;
	fl4.daddr              = tunnel->ip4;
	fl4.saddr              = amt->local_ip;
	fl4.flowi4_dscp        = inet_dsfield_to_dscp(AMT_TOS);
	fl4.flowi4_proto       = IPPROTO_UDP;
	rt = ip_route_output_key(amt->net, &fl4);
	if (IS_ERR(rt)) {
		netdev_dbg(amt->dev, "no route to %pI4\n", &tunnel->ip4);
		return true;
	}

	amtmq		= skb_push(skb, sizeof(*amtmq));
	amtmq->version	= 0;
	amtmq->type	= AMT_MSG_MEMBERSHIP_QUERY;
	amtmq->reserved = 0;
	amtmq->l	= 0;
	amtmq->g	= 0;
	amtmq->nonce	= tunnel->nonce;
	amtmq->response_mac = tunnel->mac;

	if (!v6)
		skb_set_inner_protocol(skb, htons(ETH_P_IP));
	else
		skb_set_inner_protocol(skb, htons(ETH_P_IPV6));
	udp_tunnel_xmit_skb(rt, sk, skb,
			    fl4.saddr,
			    fl4.daddr,
			    AMT_TOS,
			    ip4_dst_hoplimit(&rt->dst),
			    0,
			    amt->relay_port,
			    tunnel->source_port,
			    false,
			    false,
			    0);
	amt_update_relay_status(tunnel, AMT_STATUS_SENT_QUERY, true);
	return false;
}
/* …（同文件无关代码省略）… */
#endif
	bool report = false;
	struct igmphdr *ih;
	bool query = false;
	struct iphdr *iph;
	bool data = false;
	bool v6 = false;
/* …（同文件无关代码省略）… */
			case IGMP_HOST_MEMBERSHIP_REPORT:
				report = true;
				break;
			case IGMP_HOST_MEMBERSHIP_QUERY:
				query = true;
				break;
			default:
				goto free;
			}
/* …（同文件无关代码省略）… */
			case ICMPV6_MLD2_REPORT:
				report = true;
				break;
			case ICMPV6_MGM_QUERY:
				query = true;
				break;
			default:
				goto free;
			}
/* …（同文件无关代码省略）… */
			goto free;
		goto unlock;
	} else if (amt->mode == AMT_MODE_RELAY) {
		if (query) {
			tunnel = amt_skb_cb(skb)->tunnel;
			if (!tunnel) {
				WARN_ON(1);
				goto free;
			}

			/* Do not forward unexpected query */
			if (amt_send_membership_query(amt, skb, tunnel, v6))
				goto free;
			goto unlock;
		}

		if (!data)
			goto free;
		list_for_each_entry_rcu(tunnel, &amt->tunnel_list, list) {
