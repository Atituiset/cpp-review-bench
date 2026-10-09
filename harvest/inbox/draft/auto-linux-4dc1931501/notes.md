# auto-linux-4dc1931501

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#6b48ed85ae0ca37c7403ef5d512be75975844f2f](https://github.com/torvalds/linux/commit/6b48ed85ae0ca37c7403ef5d512be75975844f2f) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 103 |
| 编译错误数（gcc syntax-only） | 57（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6b48ed85ae0ca37c7403ef5d512be75975844f2f (https://github.com/torvalds/linux/commit/6b48ed85ae0ca37c7403ef5d512be75975844f2f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2434；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6b48ed85ae0ca37c7403ef5d512be75975844f2f net: macb: check TX ring before modifying skb :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2421,8 +2421,15 @@ static inline int macb_clear_csum(struct sk_buff *skb)
 	return 0;
 }
 
+static bool macb_needs_sw_fcs(struct sk_buff *skb, struct net_device *netdev)
+{
+	return netdev->features & NETIF_F_HW_CSUM &&
+	       skb->ip_summed != CHECKSUM_PARTIAL &&
+	       !skb_shinfo(skb)->gso_size && !ptp_one_step_sync(skb);
+}
+
 /* Returns a negative errno, or the FCS bytes appended (0 or ETH_FCS_LEN). */
-static int macb_pad_and_fcs(struct sk_buff **skb, struct net_device *netdev)
+static int macb_pad_and_fcs(struct sk_buff **skb, bool add_fcs)
 {
 	bool cloned = skb_cloned(*skb) || skb_header_cloned(*skb) ||
 		      skb_is_nonlinear(*skb);
@@ -2431,18 +2438,15 @@ static int macb_pad_and_fcs(struct sk_buff **skb, struct net_device *netdev)
 	struct sk_buff *nskb;
 	u32 fcs;
 
-	if (!(netdev->features & NETIF_F_HW_CSUM) ||
-	    !((*skb)->ip_summed != CHECKSUM_PARTIAL) ||
-	    skb_shinfo(*skb)->gso_size || ptp_one_step_sync(*skb))
+	if (!add_fcs)
 		return 0;
 
 	if (padlen <= 0) {
-		/* FCS could be appeded to tailroom. */
-		if (tailroom >= ETH_FCS_LEN)
+		/* FCS could be appended to tailroom. */
+		if (!skb_is_nonlinear(*skb) && tailroom >= ETH_FCS_LEN)
 			goto add_fcs;
-		/* No room for FCS, need to reallocate skb. */
-		else
-			padlen = ETH_FCS_LEN;
+		/* Reallocate with room for the FCS. */
+		padlen = ETH_FCS_LEN;
 	} else {
 		/* Add room for FCS. */
 		padlen += ETH_FCS_LEN;
@@ -2481,27 +2485,15 @@ static netdev_tx_t macb_start_xmit(struct sk_buff *skb,
 	unsigned int desc_cnt, nr_frags, frag_size, f;
 	struct macb_queue *queue = &bp->queues[q];
 	netdev_tx_t ret = NETDEV_TX_OK;
-	unsigned int hdrlen;
+	unsigned int hdrlen, tx_len;
+	bool add_fcs, is_lso;
 	unsigned long flags;
 	int fcs_len;
-	bool is_lso;
-
-	if (macb_clear_csum(skb)) {
-		dev_kfree_skb_any(skb);
-		return ret;
-	}
-
-	fcs_len = macb_pad_and_fcs(&skb, netdev);
-	if (fcs_len < 0) {
-		dev_kfree_skb_any(skb);
-		return ret;
-	}
-
-	if (macb_dma_ptp(bp) &&
-	    (skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP))
-		skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
 
+	add_fcs = macb_needs_sw_fcs(skb, netdev);
 	is_lso = (skb_shinfo(skb)->gso_size != 0);
+	tx_len = add_fcs ? max_t(unsigned int, skb->len, ETH_ZLEN) +
+			   ETH_FCS_LEN : skb->len;
 
 	if (is_lso) {
 		/* length of headers */
@@ -2515,8 +2507,11 @@ static netdev_tx_t macb_start_xmit(struct sk_buff *skb,
 			/* if this is required, would need to copy to single buffer */
 			return NETDEV_TX_BUSY;
 		}
-	} else
+	} else if (add_fcs) {
+		hdrlen = umin(tx_len, bp->max_tx_length);
+	} else {
 		hdrlen = umin(skb_headlen(skb), bp->max_tx_length);
+	}
 
 #if defined(DEBUG) && defined(VERBOSE_DEBUG)
 	netdev_vdbg(bp->netdev,
@@ -2531,12 +2526,18 @@ static netdev_tx_t macb_start_xmit(struct sk_buff *skb,
 	 * socket buffer: skb fragments of jumbo frames may need to be
 	 * split into many buffer descriptors.
 	 */
-	if (is_lso && (skb_headlen(skb) > hdrlen))
+	if (add_fcs) {
+		/* macb_pad_and_fcs() linearizes the skb before adding the FCS. */
+		desc_cnt = DIV_ROUND_UP(tx_len, bp->max_tx_length);
+		nr_frags = 0;
+	} else if (is_lso && (skb_headlen(skb) > hdrlen)) {
 		/* extra header descriptor if also payload in first buffer */
 		desc_cnt = DIV_ROUND_UP((skb_headlen(skb) - hdrlen), bp->max_tx_length) + 1;
-	else
+		nr_frags = skb_shinfo(skb)->nr_frags;
+	} else {
 		desc_cnt = DIV_ROUND_UP(skb_headlen(skb), bp->max_tx_length);
-	nr_frags = skb_shinfo(skb)->nr_frags;
+		nr_frags = skb_shinfo(skb)->nr_frags;
+	}
 	for (f = 0; f < nr_frags; f++) {
 		frag_size = skb_frag_size(&skb_shinfo(skb)->frags[f]);
 		desc_cnt += DIV_ROUND_UP(frag_size, bp->max_tx_length);
@@ -2554,6 +2555,21 @@ static netdev_tx_t macb_start_xmit(struct sk_buff *skb,
 		goto unlock;
 	}
 
+	if (macb_clear_csum(skb)) {
+		dev_kfree_skb_any(skb);
+		goto unlock;
+	}
+
+	fcs_len = macb_pad_and_fcs(&skb, add_fcs);
+	if (fcs_len < 0) {
+		dev_kfree_skb_any(skb);
+		goto unlock;
+	}
+
+	if (macb_dma_ptp(bp) &&
+	    (skb
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`defined`
- 外部函数：`dev_consume_skb_any`
- 外部函数：`dev_kfree_skb_any`
- 外部函数：`dma_map_single`
- 外部函数：`dma_mapping_error`
- 外部函数：`dma_unmap_page`
- 外部函数：`dma_unmap_single`
- 外部函数：`dma_wmb`
- 外部函数：`ip_hdr`
- 外部函数：`likely`
- 外部函数：`lower_32_bits`
- 外部函数：`macb_dma64`
- 外部函数：`macb_dma_ptp`
- 外部函数：`macb_pad_and_fcs`
- 外部函数：`netdev_err`
- 外部函数：`netdev_vdbg`
- 外部函数：`ptp_classify_raw`
- 外部函数：`ptp_get_msgtype`
- 外部函数：`ptp_parse_header`
- 外部函数：`skb_checksum_start`
- 外部函数：`skb_cloned`
- 外部函数：`skb_cow_head`
- 外部函数：`skb_frag_dma_map`
- 外部函数：`skb_frag_size`
- 外部函数：`skb_header_cloned`
- 外部函数：`skb_headlen`
- 外部函数：`skb_is_nonlinear`
- 外部函数：`skb_shinfo`
- 外部函数：`skb_transport_offset`
- 外部函数：`umin`
- 外部函数：`unlikely`
- 外部函数：`upper_32_bits`
- 外部函数：`wmb`
- 大写宏：`BUG`
- 大写宏：`CHECKSUM_PARTIAL`
- 大写宏：`DEBUG`
- 大写宏：`DIV_ROUND_UP`
- 大写宏：`DMA`
- 大写宏：`DMA_TO_DEVICE`
- 大写宏：`ENOMEM`
- 大写宏：`ETH_FCS_LEN`
- 大写宏：`FCS`
- 大写宏：`IPPROTO_UDP`
- 大写宏：`LSO`
- 大写宏：`MACB_BF`
- 大写宏：`MACB_BIT`
- 大写宏：`MACB_LSO_TSO_ENABLE`
- 大写宏：`MACB_LSO_UFO_ENABLE`
- 大写宏：`MFS`
- 大写宏：`MSS`
- 大写宏：`MSS_MFS`
- 大写宏：`NETDEV_TX_BUSY`
- 大写宏：`NETDEV_TX_OK`
- 大写宏：`NETIF_F_HW_CSUM`
- 大写宏：`NULL`
- 大写宏：`PTP`
- 大写宏：`PTP_CLASS_NONE`
- 大写宏：`PTP_FLAG_TWOSTEP`
- 大写宏：`PTP_MSGTYPE_SYNC`
- 大写宏：`RX_USED`
- 大写宏：`SKBTX_HW_TSTAMP`
- 大写宏：`SKBTX_IN_PROGRESS`
- 大写宏：`TCP`
- 大写宏：`TSO`
- 大写宏：`TX_LAST`
- 大写宏：`TX_LSO`
- 大写宏：`TX_NOCRC`
- 大写宏：`TX_TCP_SEQ_SRC`
- 大写宏：`TX_USED`
- 大写宏：`TX_WRAP`
- 大写宏：`UDP`
- 大写宏：`UFO`
- 大写宏：`VERBOSE_DEBUG`
- 外部类型：`Add`
- 外部类型：`First`
- 外部类型：`Identify`
- 外部类型：`Make`
- 外部类型：`Map`
- 外部类型：`No`
- 外部类型：`Number`
- 外部类型：`Only`
- 外部类型：`Returns`
- 外部类型：`Save`
- 外部类型：`Select`
- 外部类型：`Sequence`
- 外部类型：`Set`
- 外部类型：`Should`
- 外部类型：`Source`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`This`
- 外部类型：`Update`
- 外部类型：`Zynq`
- 外部类型：`dma_addr_t`
- 外部类型：`macb`
- 外部类型：`macb_dma_desc`
- 外部类型：`macb_dma_desc_64`
- 外部类型：`macb_queue`
- 外部类型：`macb_tx_skb`
- 外部类型：`net_device`
- 外部类型：`netdev_tx_t`
- 外部类型：`ptp_header`
- 外部类型：`sk_buff`
- 外部类型：`skb_frag_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-4dc1931501` → 本草稿移入 `cases/defect/auto-linux-4dc1931501/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
