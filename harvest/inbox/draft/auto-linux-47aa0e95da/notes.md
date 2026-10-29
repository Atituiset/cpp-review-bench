# auto-linux-47aa0e95da

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
| 外部依赖数（dep_count） | 59 |
| 编译错误数（gcc syntax-only） | 66（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ae3690eb2a994dd74a43639d6bf800b51f18b48c (https://github.com/torvalds/linux/commit/ae3690eb2a994dd74a43639d6bf800b51f18b48c)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 850；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ae3690eb2a994dd74a43639d6bf800b51f18b48c Merge tag 'dmaengine-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/vkoul/dmaengine :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -847,11 +847,6 @@ static int msgdma_probe(struct platform_device *pdev)
 	if (mdev->irq < 0)
 		return -ENXIO;
 
-	ret = devm_request_irq(&pdev->dev, mdev->irq, msgdma_irq_handler,
-			       0, dev_name(&pdev->dev), mdev);
-	if (ret)
-		return ret;
-
 	tasklet_setup(&mdev->irq_tasklet, msgdma_tasklet);
 
 	dma_cookie_init(&mdev->dmachan);
@@ -863,6 +858,11 @@ static int msgdma_probe(struct platform_device *pdev)
 	INIT_LIST_HEAD(&mdev->done_list);
 	INIT_LIST_HEAD(&mdev->free_list);
 
+	ret = devm_request_irq(&pdev->dev, mdev->irq, msgdma_irq_handler,
+			       0, dev_name(&pdev->dev), mdev);
+	if (ret)
+		return ret;
+
 	dma_dev = &mdev->dmadev;
 
 	/* Set DMA capabilities */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_dbg`
- 外部函数：`dev_name`
- 外部函数：`devm_request_irq`
- 外部函数：`dma_cookie_complete`
- 外部函数：`dma_cookie_init`
- 外部函数：`dmaengine_desc_callback_invoke`
- 外部函数：`dmaengine_desc_callback_valid`
- 外部函数：`dmaengine_desc_get_callback`
- 外部函数：`from_tasklet`
- 外部函数：`ioread32`
- 外部函数：`iowrite32`
- 外部函数：`list_add_tail`
- 外部函数：`list_del`
- 外部函数：`list_first_entry_or_null`
- 外部函数：`list_move_tail`
- 外部函数：`list_splice_tail_init`
- 外部函数：`mdelay`
- 外部函数：`memcpy_toio`
- 外部函数：`offsetof`
- 外部函数：`s`
- 外部函数：`spin_lock`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`tasklet_schedule`
- 外部函数：`tasklet_setup`
- 外部函数：`wmb`
- 大写宏：`BIT`
- 大写宏：`DESC`
- 大写宏：`DMA`
- 大写宏：`ENXIO`
- 大写宏：`FIFO`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`IRQ_HANDLED`
- 大写宏：`NULL`
- 外部类型：`Avalon`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`Copy`
- 外部类型：`Ensure`
- 外部类型：`If`
- 外部类型：`On`
- 外部类型：`Read`
- 外部类型：`Run`
- 外部类型：`Set`
- 外部类型：`So`
- 外部类型：`Start`
- 外部类型：`The`
- 外部类型：`Write`
- 外部类型：`device`
- 外部类型：`dma_addr_t`
- 外部类型：`dma_chan`
- 外部类型：`dma_device`
- 外部类型：`dma_slave_config`
- 外部类型：`dmaengine_desc_callback`
- 外部类型：`irqreturn_t`
- 外部类型：`list_head`
- 外部类型：`msgdma_extended_desc`
- 外部类型：`msgdma_sw_desc`
- 外部类型：`spinlock_t`
- 外部类型：`tasklet_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-47aa0e95da` → 本草稿移入 `cases/defect/auto-linux-47aa0e95da/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
