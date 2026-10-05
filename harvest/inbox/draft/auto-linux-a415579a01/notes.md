# auto-linux-a415579a01

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7704c4c5bb127673b4f0ead839919db573559e38](https://github.com/torvalds/linux/commit/7704c4c5bb127673b4f0ead839919db573559e38) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 37 |
| 编译错误数（gcc syntax-only） | 52（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7704c4c5bb127673b4f0ead839919db573559e38 (https://github.com/torvalds/linux/commit/7704c4c5bb127673b4f0ead839919db573559e38)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 924；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7704c4c5bb127673b4f0ead839919db573559e38 Merge tag 'i2c-fixes-7.3-rc6' of git://git.kernel.org/pub/scm/linux/kernel/git/andi.shyti/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -921,8 +921,14 @@ int at91_twi_probe_master(struct platform_device *pdev,
 	at91_calc_twi_clock(dev);
 
 	rc = at91_init_twi_recovery_info(pdev, dev);
-	if (rc == -EPROBE_DEFER)
+	if (rc == -EPROBE_DEFER) {
+		/*
+		 * The DMA channels were already set up above, release them
+		 * before deferring the probe so they are not leaked.
+		 */
+		at91_twi_dma_release(dev);
 		return rc;
+	}
 
 	dev->adapter.algo = &at91_twi_algorithm;
 	dev->adapter.quirks = &at91_twi_quirks;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`at91_twi_read`
- 外部函数：`at91_twi_write`
- 外部函数：`clk_get_rate`
- 外部函数：`d`
- 外部函数：`dev_dbg`
- 外部函数：`dev_info`
- 外部函数：`dev_warn`
- 外部函数：`devm_pinctrl_get`
- 外部函数：`fls`
- 外部函数：`i2c_get_adapdata`
- 外部函数：`i2c_parse_fw_timings`
- 外部函数：`max`
- 外部函数：`value`
- 大写宏：`AT91_TWI_ACR`
- 大写宏：`AT91_TWI_ACR_DATAL_MASK`
- 大写宏：`AT91_TWI_CLEAR`
- 大写宏：`AT91_TWI_CR`
- 大写宏：`AT91_TWI_CWGR_HOLD`
- 大写宏：`AT91_TWI_CWGR_HOLD_MAX`
- 大写宏：`AT91_TWI_FILTR_THRES_MAX`
- 大写宏：`AT91_TWI_SDA`
- 大写宏：`AT91_TWI_SR`
- 大写宏：`DIV_ROUND_UP`
- 大写宏：`EPROBE_DEFER`
- 大写宏：`HOLD`
- 大写宏：`IS_ERR`
- 大写宏：`PTR_ERR`
- 大写宏：`SDA`
- 外部类型：`Filter`
- 外部类型：`T_peripheral_clock`
- 外部类型：`Use`
- 外部类型：`at91_twi_dev`
- 外部类型：`at91_twi_pdata`
- 外部类型：`i2c_adapter`
- 外部类型：`i2c_bus_recovery_info`
- 外部类型：`i2c_timings`
- 外部类型：`platform_device`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a415579a01` → 本草稿移入 `cases/defect/auto-linux-a415579a01/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
