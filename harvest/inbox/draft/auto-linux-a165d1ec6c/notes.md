# auto-linux-a165d1ec6c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#ae3690eb2a994dd74a43639d6bf800b51f18b48c](https://github.com/torvalds/linux/commit/ae3690eb2a994dd74a43639d6bf800b51f18b48c) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 147 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ae3690eb2a994dd74a43639d6bf800b51f18b48c (https://github.com/torvalds/linux/commit/ae3690eb2a994dd74a43639d6bf800b51f18b48c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 208；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ae3690eb2a994dd74a43639d6bf800b51f18b48c Merge tag 'dmaengine-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/vkoul/dmaengine :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7,12 +7,14 @@
  */
 
 #include <linux/module.h>
+#include <linux/pci.h>
 #include <linux/delay.h>
 #include <linux/device.h>
 #include <linux/kernel.h>
 #include <linux/dmaengine.h>
 #include <linux/err.h>
 #include <linux/interrupt.h>
+#include <linux/io.h>
 #include <linux/irq.h>
 #include <linux/dma/edma.h>
 #include <linux/dma-mapping.h>
@@ -30,9 +32,9 @@ struct dw_edma_desc *vd2dw_edma_desc(struct virt_dma_desc *vd)
 	return container_of(vd, struct dw_edma_desc, vd);
 }
 
-enum dw_edma_irq_event {
-	DW_EDMA_IRQ_DONE	= BIT(0),
-	DW_EDMA_IRQ_ABORT	= BIT(1),
+enum dw_edma_deferred_event {
+	DW_EDMA_DEFERRED_DONE	= BIT(0),
+	DW_EDMA_DEFERRED_ABORT	= BIT(1),
 };
 
 static inline
@@ -51,13 +53,19 @@ dw_edma_alloc_desc(struct dw_edma_chan *chan, size_t nburst)
 {
 	struct dw_edma_desc *desc;
 
+	/*
+	 * For now, a descriptor that does not fit would stall the channel
+	 * forever: reject it up front.
+	 */
+	if (!chan->non_ll && nburst > chan->ll_max - 1)
+		return NULL;
+
 	desc = kzalloc_flex(*desc, burst, nburst, GFP_NOWAIT);
 	if (unlikely(!desc))
 		return NULL;
 
 	desc->chan = chan;
 	desc->nburst = nburst;
-	desc->cb = true;
 
 	return desc;
 }
@@ -67,38 +75,122 @@ static void vchan_free_desc(struct virt_dma_desc *vdesc)
 	kfree(vd2dw_edma_desc(vdesc));
 }
 
-static void dw_edma_core_start(struct dw_edma_desc *desc, bool first)
+/* Must be called with vc.lock held. */
+static void
+dw_edma_set_request(struct dw_edma_chan *chan, enum dw_edma_request request)
 {
-	struct dw_edma_chan *chan = desc->chan;
-	size_t i = 0;
+	chan->request = request;
+}
 
-	if (chan->non_ll) {
-		chan->dw->core->non_ll_start(chan, &desc->burst[desc->start_burst]);
-		desc->done_burst = desc->start_burst;
-		desc->start_burst += 1;
+static void dw_hdma_set_callback_result(struct virt_dma_desc *vd,
+					enum dmaengine_tx_result result)
+{
+	u32 residue = 0;
+	struct dw_edma_desc *desc;
+	struct dmaengine_result *res;
+
+	if (!vd->tx.callback_result)
 		return;
+
+	desc = vd2dw_edma_desc(vd);
+	if (desc) {
+		residue = desc->alloc_sz;
+
+		if (result == DMA_TRANS_NOERROR)
+			residue -= desc->burst[desc->start_burst - 1].xfer_sz;
+		else if (desc->done_burst)
+			residue -= desc->burst[desc->done_burst - 1].xfer_sz;
 	}
 
-	for (i = 0; i + desc->start_burst < desc->nburst; i++) {
-		u32 idx = i + desc->start_burst;
+	res = &vd->tx_result;
+	res->result = result;
+	res->residue = residue;
+}
 
-		if (i == chan->ll_max)
-			break;
+static void dw_edma_core_reset_ll(struct dw_edma_chan *chan)
+{
+	u32 i;
+
+	chan->ll_head = 0;
+	chan->ll_done = 0;
+	/* Drop stale CB bits before reusing the circular LL ring. */
+	for (i = 0; i < chan->ll_max; i++)
+		dw_edma_core_ll_clear(chan, i);
+	chan->cb = true;
+
+	dw_edma_core_ll_link(chan, chan->ll_max, chan->cb,
+			     chan->ll_region.paddr);
+
+	dw_edma_core_ch_enable(chan);
+	chan->ll_valid = true;
+}
+
+static u32 dw_edma_core_get_ll_dist(struct dw_edma_chan *chan, u32 from, u32 to)
+{
+	return (to + chan->ll_max - from) % chan->ll_max;
+}
 
-		dw_edma_core_ll_data(chan, &desc->burst[idx],
-				     i, desc->cb,
-				     idx == desc->nburst - 1 || i == chan->ll_max - 1);
+static u32 dw_edma_core_get_used_num(struct dw_edma_chan *chan)
+{
+	return dw_edma_core_get_ll_dist(chan, chan->ll_done, chan->ll_head);
+}
+
+static u32 dw_edma_core_get_free_num(struct dw_edma_chan *chan)
+{
+	/* Keep one data entry free so equal indices mean an empty ring. */
+	return chan->ll_max - 1 - dw_edma_core_get_used_num(chan);
+}
+
+static bool dw_edma_ll_pending(struct dw_edma_chan *chan)
+{
+	return chan->ll_head != chan->ll_done;
+}
+
+static void dw_edma_core_ll_start(struct dw_edma_desc *desc)
+{
+	struct dw_edma_chan *chan = desc->chan;
+	size_t i;
+	u32 free;
+
+	free = dw_edma_core_get_free_num(chan);
+	for (i = desc->start_burst; i < desc->nburst && free; i++, free--) {
+		/*
+		 * Refresh the link element before filling the last data slot so
+		 * the next lap has the updated CB value.
+		 */
+		if (chan->ll_
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`container_of`
- 外部函数：`dchan2dw_edma_chan`
- 外部函数：`dev_err`
- 外部函数：`dma_cookie_complete`
- 外部函数：`dw_edma_core_ch_doorbell`
- 外部函数：`dw_edma_core_ch_enable`
- 外部函数：`dw_edma_core_ch_status`
- 外部函数：`dw_edma_core_ll_data`
- 外部函数：`dw_edma_core_ll_link`
- 外部函数：`guard`
- 外部函数：`kfree`
- 外部函数：`kzalloc_flex`
- 外部函数：`list_del`
- 外部函数：`list_for_each_entry_safe`
- 外部函数：`memcpy`
- 外部函数：`non_ll_start`
- 外部函数：`pci_address`
- 外部函数：`unlikely`
- 外部函数：`vchan_next_desc`
- 外部函数：`vchan_terminate_vdesc`
- 大写宏：`BIT`
- 大写宏：`BUSY`
- 大写宏：`DMA`
- 大写宏：`DMA_COMPLETE`
- 大写宏：`DW_EDMA_CH_IRQ_REMOTE`
- 大写宏：`DW_EDMA_IRQ_ABORT`
- 大写宏：`DW_EDMA_IRQ_DONE`
- 大写宏：`EDMA_MF_HDMA_NATIVE`
- 大写宏：`EDMA_REQ_NONE`
- 大写宏：`EDMA_REQ_PAUSE`
- 大写宏：`EDMA_REQ_STOP`
- 大写宏：`EDMA_ST_BUSY`
- 大写宏：`EDMA_ST_IDLE`
- 大写宏：`EDMA_ST_PAUSE`
- 大写宏：`EINVAL`
- 大写宏：`EPERM`
- 大写宏：`GFP_NOWAIT`
- 大写宏：`HDMA`
- 大写宏：`LLP`
- 大写宏：`NULL`
- 外部类型：`Cases`
- 外部类型：`Completing`
- 外部类型：`Cookies`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`When`
- 外部类型：`dma_chan`
- 外部类型：`dma_slave_caps`
- 外部类型：`dma_slave_config`
- 外部类型：`dw_edma_chan`
- 外部类型：`dw_edma_chip`
- 外部类型：`dw_edma_desc`
- 外部类型：`list_head`
- 外部类型：`phys_addr_t`
- 外部类型：`size_t`
- 外部类型：`virt_dma_desc`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a165d1ec6c` → 本草稿移入 `cases/defect/auto-linux-a165d1ec6c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
