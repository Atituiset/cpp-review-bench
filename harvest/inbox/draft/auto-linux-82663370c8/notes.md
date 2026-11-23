# auto-linux-82663370c8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#fdcd45bc9cfee488b92efc4b7f43d6865ec82770](https://github.com/torvalds/linux/commit/fdcd45bc9cfee488b92efc4b7f43d6865ec82770) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 35 |
| 编译错误数（gcc syntax-only） | 22（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fdcd45bc9cfee488b92efc4b7f43d6865ec82770 (https://github.com/torvalds/linux/commit/fdcd45bc9cfee488b92efc4b7f43d6865ec82770)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1991；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fdcd45bc9cfee488b92efc4b7f43d6865ec82770 Merge tag 'spi-fix-v7.4-rc3' of git://git.kernel.org/pub/scm/linux/kernel/git/broonie/spi :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1967,7 +1967,6 @@ static int __spi_pump_transfer_message(struct spi_controller *ctlr,
 	if (!was_busy && ctlr->auto_runtime_pm) {
 		ret = pm_runtime_get_sync(ctlr->dev.parent);
 		if (ret < 0) {
-			pm_runtime_put_noidle(ctlr->dev.parent);
 			dev_err(&ctlr->dev, "Failed to power device: %d\n",
 				ret);
 
@@ -1988,9 +1987,6 @@ static int __spi_pump_transfer_message(struct spi_controller *ctlr,
 				"failed to prepare transfer hardware: %d\n",
 				ret);
 
-			if (ctlr->auto_runtime_pm)
-				pm_runtime_put(ctlr->dev.parent);
-
 			msg->status = ret;
 			spi_finalize_current_message(ctlr);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__spi_pump_transfer_message`
- 外部函数：`complete`
- 外部函数：`dev_err`
- 外部函数：`dma_unmap_sgtable`
- 外部函数：`kfree`
- 外部函数：`list_del`
- 外部函数：`pm_runtime_get_sync`
- 外部函数：`pm_runtime_put`
- 外部函数：`pm_runtime_put_noidle`
- 外部函数：`ptp_read_system_postts`
- 外部函数：`release`
- 外部函数：`sg_free_table`
- 外部函数：`smp_mb`
- 外部函数：`trace_spi_message_done`
- 外部函数：`unlikely`
- 外部函数：`unoptimize_message`
- 外部函数：`unprepare_message`
- 大写宏：`DMA_ATTR_SKIP_CPU_SYNC`
- 大写宏：`DMA_FROM_DEVICE`
- 大写宏：`DMA_TO_DEVICE`
- 大写宏：`NULL`
- 大写宏：`READ_ONCE`
- 大写宏：`VISIBLE_IF_KUNIT`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WRITE_ONCE`
- 外部类型：`Failed`
- 外部类型：`Restore`
- 外部类型：`See`
- 外部类型：`The`
- 外部类型：`device`
- 外部类型：`dma_data_direction`
- 外部类型：`sg_table`
- 外部类型：`spi_controller`
- 外部类型：`spi_message`
- 外部类型：`spi_res`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-82663370c8` → 本草稿移入 `cases/defect/auto-linux-82663370c8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
