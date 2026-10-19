# auto-linux-3542fe56f2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#8486d357129126e27fb5d4e02622566bafc77035](https://github.com/torvalds/linux/commit/8486d357129126e27fb5d4e02622566bafc77035) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 53 |
| 编译错误数（gcc syntax-only） | 114（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8486d357129126e27fb5d4e02622566bafc77035 (https://github.com/torvalds/linux/commit/8486d357129126e27fb5d4e02622566bafc77035)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 807；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8486d357129126e27fb5d4e02622566bafc77035 Merge tag 'spi-fix-v7.3-rc7' of git://git.kernel.org/pub/scm/linux/kernel/git/broonie/spi :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -550,11 +550,8 @@ static int fsl_lpspi_target_abort(struct spi_controller *controller)
 	return 0;
 }
 
-static int fsl_lpspi_wait_for_completion(struct spi_controller *controller)
+static int fsl_lpspi_wait_for_completion(struct fsl_lpspi_data *fsl_lpspi)
 {
-	struct fsl_lpspi_data *fsl_lpspi =
-				spi_controller_get_devdata(controller);
-
 	if (fsl_lpspi->is_target) {
 		if (wait_for_completion_interruptible(&fsl_lpspi->xfer_done) ||
 			fsl_lpspi->target_aborted) {
@@ -758,11 +755,9 @@ static int fsl_lpspi_dma_init(struct device *dev,
 	return ret;
 }
 
-static int fsl_lpspi_pio_transfer(struct spi_controller *controller,
+static int fsl_lpspi_pio_transfer(struct fsl_lpspi_data *fsl_lpspi,
 				  struct spi_transfer *t)
 {
-	struct fsl_lpspi_data *fsl_lpspi =
-				spi_controller_get_devdata(controller);
 	int ret;
 
 	fsl_lpspi->tx_buf = t->tx_buf;
@@ -774,7 +769,7 @@ static int fsl_lpspi_pio_transfer(struct spi_controller *controller,
 
 	fsl_lpspi_write_tx_fifo(fsl_lpspi);
 
-	ret = fsl_lpspi_wait_for_completion(controller);
+	ret = fsl_lpspi_wait_for_completion(fsl_lpspi);
 
 	fsl_lpspi_reset(fsl_lpspi);
 
@@ -803,11 +798,9 @@ static int fsl_lpspi_transfer_one(struct spi_controller *controller,
 	if (fsl_lpspi->usedma)
 		ret = fsl_lpspi_dma_transfer(controller, fsl_lpspi, t);
 	else
-		ret = fsl_lpspi_pio_transfer(controller, t);
-	if (ret < 0)
-		return ret;
+		ret = fsl_lpspi_pio_transfer(fsl_lpspi, t);
 
-	return 0;
+	return ret;
 }
 
 static irqreturn_t fsl_lpspi_isr(int irq, void *dev_id)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`complete`
- 外部函数：`dev_dbg`
- 外部函数：`dev_err`
- 外部函数：`dma_async_issue_pending`
- 外部函数：`dmaengine_prep_slave_sg`
- 外部函数：`dmaengine_slave_config`
- 外部函数：`dmaengine_submit`
- 外部函数：`dmaengine_terminate_sync`
- 外部函数：`readl`
- 外部函数：`reinit_completion`
- 外部函数：`rx`
- 外部函数：`secs_to_jiffies`
- 外部函数：`spi_controller_get_devdata`
- 外部函数：`tx`
- 外部函数：`void`
- 外部函数：`wait_for_completion_interruptible`
- 外部函数：`wait_for_completion_timeout`
- 外部函数：`writel`
- 大写宏：`BIT`
- 大写宏：`BITS_PER_BYTE`
- 大写宏：`DIV_ROUND_UP`
- 大写宏：`DMA`
- 大写宏：`DMA_CTRL_ACK`
- 大写宏：`DMA_DEV_TO_MEM`
- 大写宏：`DMA_MEM_TO_DEV`
- 大写宏：`DMA_PREP_INTERRUPT`
- 大写宏：`DMA_SLAVE_BUSWIDTH_1_BYTE`
- 大写宏：`DMA_SLAVE_BUSWIDTH_2_BYTES`
- 大写宏：`DMA_SLAVE_BUSWIDTH_4_BYTES`
- 大写宏：`EINTR`
- 大写宏：`EINVAL`
- 大写宏：`ETIMEDOUT`
- 大写宏：`FIFO`
- 大写宏：`GENMASK`
- 大写宏：`IRQ_HANDLED`
- 大写宏：`IRQ_NONE`
- 大写宏：`W1C`
- 外部类型：`Add`
- 外部类型：`Clear`
- 外部类型：`Disable`
- 外部类型：`Double`
- 外部类型：`Error`
- 外部类型：`Time`
- 外部类型：`Wait`
- 外部类型：`clk`
- 外部类型：`completion`
- 外部类型：`device`
- 外部类型：`dma_async_tx_descriptor`
- 外部类型：`dma_slave_buswidth`
- 外部类型：`dma_slave_config`
- 外部类型：`irqreturn_t`
- 外部类型：`sg_table`
- 外部类型：`spi_controller`
- 外部类型：`spi_transfer`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-3542fe56f2` → 本草稿移入 `cases/defect/auto-linux-3542fe56f2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
