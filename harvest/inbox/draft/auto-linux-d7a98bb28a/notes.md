# auto-linux-d7a98bb28a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7683df331d296819a8f596dd752bbf95aa2e3478](https://github.com/torvalds/linux/commit/7683df331d296819a8f596dd752bbf95aa2e3478) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 76 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7683df331d296819a8f596dd752bbf95aa2e3478 (https://github.com/torvalds/linux/commit/7683df331d296819a8f596dd752bbf95aa2e3478)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 456；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7683df331d296819a8f596dd752bbf95aa2e3478 Merge branch 'net-virtio-validate-outer-protocol-of-udp-tunnel-gso-packets' :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5,7 +5,10 @@
 #include <linux/if_vlan.h>
 #include <linux/ip.h>
 #include <linux/ipv6.h>
+#include <linux/netdevice.h>
 #include <linux/udp.h>
+#include <net/ip.h>
+#include <net/ipv6.h>
 #include <net/tcp.h>
 #include <uapi/linux/tcp.h>
 #include <uapi/linux/virtio_net.h>
@@ -48,6 +51,145 @@ static inline int virtio_net_hdr_set_proto(struct sk_buff *skb,
 	return 0;
 }
 
+/* TODO: nothing virtio specific here, move to ip.h */
+
+/*
+ * Pull and minimally validate the IPv4 header at @nhoff.
+ * Returns the header and stores the transport header offset in
+ * @thoff, or NULL if truncated or malformed.  The whole header,
+ * options included, is guaranteed linear on return.
+ */
+static inline struct iphdr *virtio_net_ip4_parse(struct sk_buff *skb,
+						  unsigned int nhoff,
+						  unsigned int *thoff)
+{
+	unsigned int ihl;
+	struct iphdr *iph;
+
+	if (!pskb_may_pull(skb, nhoff + sizeof(struct iphdr)))
+		return NULL;
+
+	iph = (struct iphdr *)(skb->data + nhoff);
+	if (iph->version != 4 ||
+	    iph->ihl < sizeof(struct iphdr) / sizeof(u32))
+		return NULL;
+
+	ihl = iph->ihl * sizeof(u32);
+	*thoff = nhoff + ihl;
+	/* Pull the options as well: the transport header offset
+	 * returned above must be within the linear data.  iph goes
+	 * stale across the pull.
+	 */
+	if (!pskb_may_pull(skb, nhoff + ihl))
+		return NULL;
+
+	return (struct iphdr *)(skb->data + nhoff);
+}
+
+/* Where does the L3 header start, and what protocol is it?
+ * With @has_eth, the frame starts with an Ethernet header, possibly
+ * followed by VLAN tags. Otherwise the L3 header starts at the network
+ * header offset the caller set up.
+ *
+ * The caller knows how the frame will be parsed later: the device
+ * type does not tell, as tun can change it with TUNSETLINK.
+ */
+static inline bool
+virtio_net_l3_start(struct sk_buff *skb, bool has_eth, int *nhoff,
+		    __be16 *proto)
+{
+	if (has_eth) {
+		__be16 h_proto;
+
+		if (!pskb_may_pull(skb, ETH_HLEN))
+			return false;
+		h_proto = eth_hdr(skb)->h_proto;
+		/* skb->protocol is a sender claim, and parts of the
+		 * stack parse the packet per it: when it disagrees with
+		 * the frame we do not know which view the packet will
+		 * be parsed as, so do not validate.
+		 */
+		if (skb->protocol && skb->protocol != h_proto)
+			return false;
+		*nhoff = ETH_HLEN;
+		*proto = h_proto;
+		if (eth_type_vlan(*proto)) {
+			*proto = __vlan_get_protocol(skb, *proto, nhoff);
+			if (!*proto)
+				return false;
+		}
+	} else {
+		const struct iphdr *iph;
+
+		*nhoff = skb_network_offset(skb);
+		/* An unset network header shows up as an offset
+		 * outside of the packet.
+		 */
+		if (*nhoff < 0 || (unsigned int)*nhoff >= skb->len)
+			return false;
+		/* No L2 header to cross-check with: take the protocol
+		 * from the IP version at the network header.
+		 */
+		if (!pskb_may_pull(skb, *nhoff + 1))
+			return false;
+		iph = (const struct iphdr *)(skb->data + *nhoff);
+		if (iph->version == 4)
+			*proto = htons(ETH_P_IP);
+		else if (iph->version == 6)
+			*proto = htons(ETH_P_IPV6);
+		else
+			return false;
+		if (skb->protocol && skb->protocol != *proto)
+			return false;
+	}
+	return true;
+}
+
+/* Walk the L3 header at @nhoff: where does its transport header
+ * start, and which protocol is it?  Returns false if the header
+ * is truncated, malformed or fragmented.
+ */
+static inline bool
+virtio_net_l4_start(struct sk_buff *skb, unsigned int nhoff, __be16 proto,
+		    unsigned int *thoff, int *ip_proto)
+{
+	if (proto == htons(ETH_P_IP)) {
+		const struct iphdr *iph = virtio_net_ip4_parse(skb, nhoff, thoff);
+
+		if (!iph || ip_is_fragment(iph))
+			return false;
+		*ip_proto = iph->protocol;
+		return true;
+	}
+
+	if (proto == htons(ETH_P_IPV6)) {
+		const struct ipv6hdr *iph;
+		int flags = IP6_FH_F_AUTH;
+
+		if (!pskb_may_pull(skb, nhoff + sizeof(*iph)))
+			return false;
+		iph = (const void *)(skb->data + nhoff);
+		if (iph->version != 6)
+			return false;
+		*thoff = nhoff + sizeof
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__cpu_to_virtio16`
- 外部函数：`__virtio16_to_cpu`
- 外部函数：`cpu_to_be16`
- 外部函数：`cpu_to_le16`
- 外部函数：`dev_parse_header_protocol`
- 外部函数：`htons`
- 外部函数：`inner_tcp_hdrlen`
- 外部函数：`le16_to_cpu`
- 外部函数：`max`
- 外部函数：`max_t`
- 外部函数：`offsetof`
- 外部函数：`pskb_may_pull`
- 外部函数：`skb_flow_dissect_flow_keys_basic`
- 外部函数：`skb_headroom`
- 外部函数：`skb_inner_transport_offset`
- 外部函数：`skb_partial_csum_set`
- 外部函数：`skb_reset_mac_header`
- 外部函数：`skb_set_inner_protocol`
- 外部函数：`skb_set_transport_header`
- 外部函数：`skb_shinfo`
- 外部函数：`skb_transport_offset`
- 大写宏：`BUILD_BUG_ON`
- 大写宏：`CHECKSUM_PARTIAL`
- 大写宏：`EINVAL`
- 大写宏：`ETH_HLEN`
- 大写宏：`ETH_P_IP`
- 大写宏：`ETH_P_IPV6`
- 大写宏：`GSO`
- 大写宏：`GSO_BY_FRAGS`
- 大写宏：`GSO_MAX_SEGS`
- 大写宏：`GSO_MAX_SIZE`
- 大写宏：`IPPROTO_TCP`
- 大写宏：`IPPROTO_UDP`
- 大写宏：`NEEDS_CSUM`
- 大写宏：`NULL`
- 大写宏：`SKB_GSO_DODGY`
- 大写宏：`SKB_GSO_TCPV4`
- 大写宏：`SKB_GSO_TCPV6`
- 大写宏：`SKB_GSO_TCP_ECN`
- 大写宏：`SKB_GSO_UDP`
- 大写宏：`SKB_GSO_UDP_L4`
- 大写宏：`SKB_GSO_UDP_TUNNEL`
- 大写宏：`SKB_GSO_UDP_TUNNEL_CSUM`
- 大写宏：`TCP_MIN_GSO_SIZE`
- 大写宏：`UDP`
- 大写宏：`UDP_MAX_SEGMENTS`
- 大写宏：`UFO`
- 大写宏：`USO`
- 大写宏：`VIRTIO_NET_HDR_F_NEEDS_CSUM`
- 大写宏：`VIRTIO_NET_HDR_F_UDP_TUNNEL_CSUM`
- 大写宏：`VIRTIO_NET_HDR_GSO_ECN`
- 大写宏：`VIRTIO_NET_HDR_GSO_NONE`
- 大写宏：`VIRTIO_NET_HDR_GSO_TCPV4`
- 大写宏：`VIRTIO_NET_HDR_GSO_TCPV6`
- 大写宏：`VIRTIO_NET_HDR_GSO_UDP`
- 大写宏：`VIRTIO_NET_HDR_GSO_UDP_L4`
- 大写宏：`VIRTIO_NET_HDR_GSO_UDP_TUNNEL`
- 大写宏：`VIRTIO_NET_HDR_GSO_UDP_TUNNEL_IPV4`
- 大写宏：`VIRTIO_NET_HDR_GSO_UDP_TUNNEL_IPV6`
- 外部类型：`Either`
- 外部类型：`Header`
- 外部类型：`In`
- 外部类型：`Kernel`
- 外部类型：`Let`
- 外部类型：`Rely`
- 外部类型：`The`
- 外部类型：`Too`
- 外部类型：`Tunnel`
- 外部类型：`Validate`
- 外部类型：`flow_keys_basic`
- 外部类型：`iphdr`
- 外部类型：`ipv6hdr`
- 外部类型：`sk_buff`
- 外部类型：`skb_shared_info`
- 外部类型：`tcphdr`
- 外部类型：`udphdr`
- 外部类型：`virtio_net_hdr`
- 外部类型：`virtio_net_hdr_v1_hash_tunnel`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-d7a98bb28a` → 本草稿移入 `cases/defect/auto-linux-d7a98bb28a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
