# auto-linux-f59dc2e6c7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#551c722f40809618230001baccf219193e22fc5a](https://github.com/torvalds/linux/commit/551c722f40809618230001baccf219193e22fc5a) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 58 |
| 编译错误数（gcc syntax-only） | 48（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #551c722f40809618230001baccf219193e22fc5a (https://github.com/torvalds/linux/commit/551c722f40809618230001baccf219193e22fc5a)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 320；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 551c722f40809618230001baccf219193e22fc5a Merge tag 'rtc-7.3-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/abelloni/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -317,10 +317,12 @@ static int ac100_rtc_register_clks(struct ac100_rtc_dev *chip)
 	if (!chip->clk_data)
 		return -ENOMEM;
 
-	chip->rtc_32k_clk = clk_hw_register_fixed_rate(chip->dev,
-						       AC100_RTC_32K_NAME,
-						       NULL, 0,
-						       AC100_RTC_32K_RATE);
+	chip->clk_data->num = AC100_CLKOUT_NUM;
+
+	chip->rtc_32k_clk = devm_clk_hw_register_fixed_rate(chip->dev,
+							    AC100_RTC_32K_NAME,
+							    NULL, 0,
+							    AC100_RTC_32K_RATE);
 	if (IS_ERR(chip->rtc_32k_clk)) {
 		ret = PTR_ERR(chip->rtc_32k_clk);
 		dev_err(chip->dev, "Failed to register RTC-32k clock: %d\n",
@@ -354,29 +356,14 @@ static int ac100_rtc_register_clks(struct ac100_rtc_dev *chip)
 		if (ret) {
 			dev_err(chip->dev, "Failed to register clk '%s': %d\n",
 				init.name, ret);
-			goto err_unregister_rtc_32k;
+			return ret;
 		}
 
 		chip->clk_data->hws[i] = &clk->hw;
 	}
 
-	chip->clk_data->num = i;
-	ret = of_clk_add_hw_provider(np, of_clk_hw_onecell_get, chip->clk_data);
-	if (ret)
-		goto err_unregister_rtc_32k;
-
-	return 0;
-
-err_unregister_rtc_32k:
-	clk_unregister_fixed_rate(chip->rtc_32k_clk->clk);
-
-	return ret;
-}
-
-static void ac100_rtc_unregister_clks(struct ac100_rtc_dev *chip)
-{
-	of_clk_del_provider(chip->dev->of_node);
-	clk_unregister_fixed_rate(chip->rtc_32k_clk->clk);
+	return devm_of_clk_add_hw_provider(chip->dev, of_clk_hw_onecell_get,
+					   chip->clk_data);
 }
 
 /*
@@ -614,13 +601,6 @@ static int ac100_rtc_probe(struct platform_device *pdev)
 	return devm_rtc_register_device(chip->rtc);
 }
 
-static void ac100_rtc_remove(struct platform_device *pdev)
-{
-	struct ac100_rtc_dev *chip = platform_get_drvdata(pdev);
-
-	ac100_rtc_unregister_clks(chip);
-}
-
 static const struct of_device_id ac100_rtc_match[] = {
 	{ .compatible = "x-powers,ac100-rtc" },
 	{ },
@@ -629,7 +609,6 @@ MODULE_DEVICE_TABLE(of, ac100_rtc_match);
 
 static struct platform_driver ac100_rtc_driver = {
 	.probe		= ac100_rtc_probe,
-	.remove		= ac100_rtc_remove,
 	.driver		= {
 		.name		= "ac100-rtc",
 		.of_match_table	= of_match_ptr(ac100_rtc_match),
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clk_hw_register_fixed_rate`
- 外部函数：`clk_unregister_fixed_rate`
- 外部函数：`dev_err`
- 外部函数：`dev_get_drvdata`
- 外部函数：`dev_name`
- 外部函数：`devm_clk_hw_register`
- 外部函数：`devm_kzalloc`
- 外部函数：`devm_request_threaded_irq`
- 外部函数：`devm_rtc_allocate_device`
- 外部函数：`devm_rtc_register_device`
- 外部函数：`of_clk_add_hw_provider`
- 外部函数：`of_clk_del_provider`
- 外部函数：`of_clk_get_parent_name`
- 外部函数：`of_match_ptr`
- 外部函数：`of_property_read_string_index`
- 外部函数：`platform_get_drvdata`
- 外部函数：`platform_get_irq`
- 外部函数：`platform_set_drvdata`
- 外部函数：`regmap_read`
- 外部函数：`regmap_write`
- 外部函数：`regmap_write_bits`
- 外部函数：`rtc_lock`
- 外部函数：`rtc_unlock`
- 外部函数：`rtc_update_irq`
- 外部函数：`struct_size`
- 大写宏：`AC100_ALM_INT_ENA`
- 大写宏：`AC100_ALM_INT_STA`
- 大写宏：`AC100_CLKOUT_CTRL1`
- 大写宏：`AC100_RTC_CTRL`
- 大写宏：`ADDA`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`BIT`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IRQ`
- 大写宏：`IRQF_ONESHOT`
- 大写宏：`IRQF_SHARED`
- 大写宏：`IRQ_HANDLED`
- 大写宏：`IS_ERR`
- 大写宏：`NULL`
- 大写宏：`PTR_ERR`
- 大写宏：`RTC`
- 大写宏：`RTC_AF`
- 大写宏：`RTC_IRQF`
- 外部类型：`Could`
- 外部类型：`Failed`
- 外部类型：`ac100_dev`
- 外部类型：`clk_hw`
- 外部类型：`clk_hw_onecell_data`
- 外部类型：`clk_init_data`
- 外部类型：`device`
- 外部类型：`device_node`
- 外部类型：`irqreturn_t`
- 外部类型：`of_device_id`
- 外部类型：`platform_device`
- 外部类型：`platform_driver`
- 外部类型：`regmap`
- 外部类型：`rtc_device`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-f59dc2e6c7` → 本草稿移入 `cases/defect/auto-linux-f59dc2e6c7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
