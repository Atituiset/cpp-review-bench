# auto-linux-be250fd000

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#af32da41b0327b9c6a37856ba82b6760d6c8d10e](https://github.com/torvalds/linux/commit/af32da41b0327b9c6a37856ba82b6760d6c8d10e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 81 |
| 编译错误数（gcc syntax-only） | 44（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #af32da41b0327b9c6a37856ba82b6760d6c8d10e (https://github.com/torvalds/linux/commit/af32da41b0327b9c6a37856ba82b6760d6c8d10e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1266；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT af32da41b0327b9c6a37856ba82b6760d6c8d10e Merge tag 'net-7.3-rc7' of git://git.kernel.org/pub/scm/linux/kernel/git/netdev/net :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -80,15 +80,6 @@ static struct in6_addr mld2_all_node = MLD2_ALL_NODE_INIT;
 static struct mld2_grec mldv2_zero_grec;
 #endif
 
-static struct amt_skb_cb *amt_skb_cb(struct sk_buff *skb)
-{
-	BUILD_BUG_ON(sizeof(struct amt_skb_cb) + sizeof(struct tc_skb_cb) >
-		     sizeof_field(struct sk_buff, cb));
-
-	return (struct amt_skb_cb *)((void *)skb->cb +
-		sizeof(struct tc_skb_cb));
-}
-
 static void __amt_source_gc_work(void)
 {
 	struct amt_source_node *snode;
@@ -791,6 +782,11 @@ static void amt_send_request(struct amt_dev *amt, bool v6)
 	rcu_read_unlock();
 }
 
+static bool amt_send_membership_query(struct amt_dev *amt,
+				      struct sk_buff *skb,
+				      struct amt_tunnel_list *tunnel,
+				      bool v6);
+
 static void amt_send_igmp_gq(struct amt_dev *amt,
 			     struct amt_tunnel_list *tunnel)
 {
@@ -800,8 +796,11 @@ static void amt_send_igmp_gq(struct amt_dev *amt,
 	if (!skb)
 		return;
 
-	amt_skb_cb(skb)->tunnel = tunnel;
-	dev_queue_xmit(skb);
+	skb_pull(skb, sizeof(struct ethhdr));
+	if (amt_send_membership_query(amt, skb, tunnel, false)) {
+		amt->dev->stats.tx_dropped++;
+		kfree_skb(skb);
+	}
 }
 
 #if IS_ENABLED(CONFIG_IPV6)
@@ -885,8 +884,11 @@ static void amt_send_mld_gq(struct amt_dev *amt, struct amt_tunnel_list *tunnel)
 	if (!skb)
 		return;
 
-	amt_skb_cb(skb)->tunnel = tunnel;
-	dev_queue_xmit(skb);
+	skb_pull(skb, sizeof(struct ethhdr));
+	if (amt_send_membership_query(amt, skb, tunnel, true)) {
+		amt->dev->stats.tx_dropped++;
+		kfree_skb(skb);
+	}
 }
 #else
 static void amt_send_mld_gq(struct amt_dev *amt, struct amt_tunnel_list *tunnel)
@@ -1186,7 +1188,6 @@ static netdev_tx_t amt_dev_xmit(struct sk_buff *skb, struct net_device *dev)
 #endif
 	bool report = false;
 	struct igmphdr *ih;
-	bool query = false;
 	struct iphdr *iph;
 	bool data = false;
 	bool v6 = false;
@@ -1204,9 +1205,6 @@ static netdev_tx_t amt_dev_xmit(struct sk_buff *skb, struct net_device *dev)
 			case IGMP_HOST_MEMBERSHIP_REPORT:
 				report = true;
 				break;
-			case IGMP_HOST_MEMBERSHIP_QUERY:
-				query = true;
-				break;
 			default:
 				goto free;
 			}
@@ -1228,9 +1226,6 @@ static netdev_tx_t amt_dev_xmit(struct sk_buff *skb, struct net_device *dev)
 			case ICMPV6_MLD2_REPORT:
 				report = true;
 				break;
-			case ICMPV6_MGM_QUERY:
-				query = true;
-				break;
 			default:
 				goto free;
 			}
@@ -1261,19 +1256,6 @@ static netdev_tx_t amt_dev_xmit(struct sk_buff *skb, struct net_device *dev)
 			goto free;
 		goto unlock;
 	} else if (amt->mode == AMT_MODE_RELAY) {
-		if (query) {
-			tunnel = amt_skb_cb(skb)->tunnel;
-			if (!tunnel) {
-				WARN_ON(1);
-				goto free;
-			}
-
-			/* Do not forward unexpected query */
-			if (amt_send_membership_query(amt, skb, tunnel, v6))
-				goto free;
-			goto unlock;
-		}
-
 		if (!data)
 			goto free;
 		list_for_each_entry_rcu(tunnel, &amt->tunnel_list, list) {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`amt_send_membership_query`
- 外部函数：`csum_ipv6_magic`
- 外部函数：`csum_partial`
- 外部函数：`dev_queue_xmit`
- 外部函数：`eth_hdr`
- 外部函数：`ether_addr_copy`
- 外部函数：`htons`
- 外部函数：`icmp6_hdr`
- 外部函数：`inet_dsfield_to_dscp`
- 外部函数：`ip4_dst_hoplimit`
- 外部函数：`ip6_flow_hdr`
- 外部函数：`ip_route_output_key`
- 外部函数：`ipv6_dev_get_saddr`
- 外部函数：`ipv6_eth_mc_map`
- 外部函数：`ipv6_hdr`
- 外部函数：`kfree_skb`
- 外部函数：`memset`
- 外部函数：`netdev_alloc_skb_ip_align`
- 外部函数：`netdev_dbg`
- 外部函数：`ntohs`
- 外部函数：`rcu_dereference_bh`
- 外部函数：`rcu_read_unlock`
- 外部函数：`sizeof_field`
- 外部函数：`skb_cow_head`
- 外部函数：`skb_pull`
- 外部函数：`skb_push`
- 外部函数：`skb_put_data`
- 外部函数：`skb_put_zero`
- 外部函数：`skb_reserve`
- 外部函数：`skb_reset_inner_headers`
- 外部函数：`skb_reset_mac_header`
- 外部函数：`skb_reset_network_header`
- 外部函数：`skb_reset_transport_header`
- 外部函数：`skb_set_inner_protocol`
- 外部函数：`spin_lock_bh`
- 外部函数：`spin_unlock_bh`
- 外部函数：`udp_tunnel_xmit_skb`
- 大写宏：`AMT_IP6HDR_OPTS`
- 大写宏：`AMT_MODE_RELAY`
- 大写宏：`AMT_MSG_MEMBERSHIP_QUERY`
- 大写宏：`AMT_STATUS_SENT_QUERY`
- 大写宏：`AMT_TOS`
- 大写宏：`BUILD_BUG_ON`
- 大写宏：`CHECKSUM_NONE`
- 大写宏：`CONFIG_IPV6`
- 大写宏：`ETH_P_IP`
- 大写宏：`ETH_P_IPV6`
- 大写宏：`ICMPV6_MGM_QUERY`
- 大写宏：`ICMPV6_MLD2_REPORT`
- 大写宏：`IGMP_HOST_MEMBERSHIP_QUERY`
- 大写宏：`IGMP_HOST_MEMBERSHIP_REPORT`
- 大写宏：`IPPROTO_ICMPV6`
- 大写宏：`IPPROTO_UDP`
- 大写宏：`IPV6_TLV_PAD1`
- 大写宏：`IPV6_TLV_ROUTERALERT`
- 大写宏：`IS_ENABLED`
- 大写宏：`IS_ERR`
- 大写宏：`LL_RESERVED_SPACE`
- 大写宏：`NEXTHDR_HOP`
- 大写宏：`NULL`
- 大写宏：`PORT`
- 大写宏：`TC_PRIO_CONTROL`
- 大写宏：`WARN_ON`
- 外部类型：`Do`
- 外部类型：`Update`
- 外部类型：`amt_dev`
- 外部类型：`amt_header_membership_query`
- 外部类型：`amt_skb_cb`
- 外部类型：`amt_source_node`
- 外部类型：`amt_status`
- 外部类型：`amt_tunnel_list`
- 外部类型：`ethhdr`
- 外部类型：`flowi4`
- 外部类型：`igmphdr`
- 外部类型：`iphdr`
- 外部类型：`ipv6hdr`
- 外部类型：`mld2_grec`
- 外部类型：`mld2_query`
- 外部类型：`rtable`
- 外部类型：`sk_buff`
- 外部类型：`sock`
- 外部类型：`tc_skb_cb`
- 外部类型：`udphdr`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-be250fd000` → 本草稿移入 `cases/defect/auto-linux-be250fd000/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
