# auto-linux-66f7abedef

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#25faec9ce61c01fb45abf812a222c5b43c3cd5c5](https://github.com/torvalds/linux/commit/25faec9ce61c01fb45abf812a222c5b43c3cd5c5) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #25faec9ce61c01fb45abf812a222c5b43c3cd5c5 (https://github.com/torvalds/linux/commit/25faec9ce61c01fb45abf812a222c5b43c3cd5c5)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 1765；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 25faec9ce61c01fb45abf812a222c5b43c3cd5c5 net: dpaa2-eth: fix objtool warning and logic bug :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1681,11 +1681,9 @@ static int dpaa2_eth_add_bufs(struct dpaa2_eth_priv *priv,
 	struct device *dev = priv->net_dev->dev.parent;
 	u64 buf_array[DPAA2_ETH_BUFS_PER_CMD];
 	struct dpaa2_eth_swa *swa;
-	struct page *page;
 	dma_addr_t addr;
 	int retries = 0;
 	int i = 0, err;
-	u32 batch;
 
 	/* Allocate buffers visible to WRIOP */
 	if (!ch->xsk_zc) {
@@ -1695,14 +1693,16 @@ static int dpaa2_eth_add_bufs(struct dpaa2_eth_priv *priv,
 			 * the entire page except for a tailroom reserved for
 			 * skb shared info
 			 */
-			page = dev_alloc_pages(0);
+			struct page *page = dev_alloc_pages(0);
 			if (!page)
 				goto err_alloc;
 
 			addr = dma_map_page(dev, page, 0, priv->rx_buf_size,
 					    DMA_BIDIRECTIONAL);
-			if (unlikely(dma_mapping_error(dev, addr)))
-				goto err_map;
+			if (unlikely(dma_mapping_error(dev, addr))) {
+				__free_pages(page, 0);
+				goto err_alloc;
+			}
 
 			buf_array[i] = addr;
 
@@ -1718,8 +1718,8 @@ static int dpaa2_eth_add_bufs(struct dpaa2_eth_priv *priv,
 		 * of DPAA2_ETH_BUFS_PER_CMD. Bail out if the UMEM cannot
 		 * provide enough buffers at the moment
 		 */
-		batch = xsk_buff_alloc_batch(ch->xsk_pool, xdp_buffs,
-					     DPAA2_ETH_BUFS_PER_CMD);
+		u32 batch = xsk_buff_alloc_batch(ch->xsk_pool, xdp_buffs,
+						 DPAA2_ETH_BUFS_PER_CMD);
 		if (!batch)
 			goto err_alloc;
 
@@ -1729,8 +1729,11 @@ static int dpaa2_eth_add_bufs(struct dpaa2_eth_priv *priv,
 			swa->xsk.xdp_buff = xdp_buffs[i];
 
 			addr = xsk_buff_xdp_get_frame_dma(xdp_buffs[i]);
-			if (unlikely(dma_mapping_error(dev, addr)))
-				goto err_map;
+			if (unlikely(dma_mapping_error(dev, addr))) {
+				for (int j = i; j < batch; j++)
+					xsk_buff_free(xdp_buffs[j]);
+				goto err_alloc;
+			}
 
 			buf_array[i] = addr;
 
@@ -1761,13 +1764,6 @@ static int dpaa2_eth_add_bufs(struct dpaa2_eth_priv *priv,
 
 	return i;
 
-err_map:
-	if (!ch->xsk_zc) {
-		__free_pages(page, 0);
-	} else {
-		for (; i < batch; i++)
-			xsk_buff_free(xdp_buffs[i]);
-	}
 err_alloc:
 	/* If we managed to allocate at least some buffers,
 	 * release them to hardware
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__free_pages`
- 外部函数：`dev_alloc_pages`
- 外部函数：`dma_map_page`
- 外部函数：`dma_mapping_error`
- 外部函数：`unlikely`
- 外部函数：`xsk_buff_alloc_batch`
- 外部函数：`xsk_buff_free`
- 外部函数：`xsk_buff_xdp_get_frame_dma`
- 大写宏：`DMA_BIDIRECTIONAL`
- 大写宏：`DPAA2_ETH_BUFS_PER_CMD`
- 大写宏：`UMEM`
- 大写宏：`WRIOP`
- 外部类型：`Allocate`
- 外部类型：`Bail`
- 外部类型：`If`
- 外部类型：`device`
- 外部类型：`dma_addr_t`
- 外部类型：`dpaa2_eth_swa`
- 外部类型：`page`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-66f7abedef` → 本草稿移入 `cases/defect/auto-linux-66f7abedef/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
