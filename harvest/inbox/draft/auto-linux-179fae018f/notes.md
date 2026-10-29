# auto-linux-179fae018f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e191b05f321b064a973399b7cdc003c0de89e5b9](https://github.com/torvalds/linux/commit/e191b05f321b064a973399b7cdc003c0de89e5b9) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 50 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e191b05f321b064a973399b7cdc003c0de89e5b9 (https://github.com/torvalds/linux/commit/e191b05f321b064a973399b7cdc003c0de89e5b9)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 21（原始 PR diff 行 497；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e191b05f321b064a973399b7cdc003c0de89e5b9 Merge tag 'phy-for-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/phy/linux-phy :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -44,6 +44,8 @@
 #define MHZ	(1000UL * 1000UL)
 #endif
 
+#define PHY_DEFAULT_RATE	74250000
+
 #define PHY_PLL_DIV_REGS_NUM 7
 
 struct phy_config {
@@ -489,17 +491,6 @@ static int fsl_samsung_hdmi_phy_configure(struct fsl_samsung_hdmi_phy *phy,
 	return ret;
 }
 
-static unsigned long phy_clk_recalc_rate(struct clk_hw *hw,
-					 unsigned long parent_rate)
-{
-	struct fsl_samsung_hdmi_phy *phy = to_fsl_samsung_hdmi_phy(hw);
-
-	if (!phy->cur_cfg)
-		return 74250000;
-
-	return phy->cur_cfg->pixclk;
-}
-
 /* Helper function to lookup the available fractional-divider rate */
 static const struct phy_config *fsl_samsung_hdmi_phy_lookup_rate(unsigned long rate)
 {
@@ -520,6 +511,14 @@ static const struct phy_config *fsl_samsung_hdmi_phy_lookup_rate(unsigned long r
 		&phy_pll_cfg[i] : &phy_pll_cfg[i+1]);
 }
 
+static unsigned long phy_clk_recalc_rate(struct clk_hw *hw,
+					 unsigned long parent_rate)
+{
+	struct fsl_samsung_hdmi_phy *phy = to_fsl_samsung_hdmi_phy(hw);
+
+	return phy->cur_cfg->pixclk;
+}
+
 static void fsl_samsung_hdmi_calculate_phy(struct phy_config *cal_phy, unsigned long rate,
 				    u8 p, u16 m, u8 s)
 {
@@ -610,7 +609,7 @@ static int phy_clk_register(struct fsl_samsung_hdmi_phy *phy)
 {
 	struct device *dev = phy->dev;
 	struct device_node *np = dev->of_node;
-	struct clk_init_data init;
+	struct clk_init_data init = {};
 	const char *parent_name;
 	struct clk *phyclk;
 	int ret;
@@ -640,6 +639,7 @@ static int phy_clk_register(struct fsl_samsung_hdmi_phy *phy)
 
 static int fsl_samsung_hdmi_phy_probe(struct platform_device *pdev)
 {
+	const struct phy_config *default_cfg;
 	struct fsl_samsung_hdmi_phy *phy;
 	int ret;
 
@@ -664,6 +664,23 @@ static int fsl_samsung_hdmi_phy_probe(struct platform_device *pdev)
 		return dev_err_probe(phy->dev, PTR_ERR(phy->refclk),
 				     "failed to get ref clk\n");
 
+	/*
+	 * Establish the rate reported by recalc_rate() before registering the
+	 * clock. Otherwise CCF may skip the first set_rate() when it requests
+	 * the assumed default rate, leaving the PHY unconfigured.
+	 */
+	ret = clk_prepare_enable(phy->refclk);
+	if (ret)
+		return dev_err_probe(phy->dev, ret,
+				     "failed to enable ref clk\n");
+
+	default_cfg = fsl_samsung_hdmi_phy_lookup_rate(PHY_DEFAULT_RATE);
+	ret = fsl_samsung_hdmi_phy_configure(phy, default_cfg);
+	clk_disable_unprepare(phy->refclk);
+	if (ret)
+		return dev_err_probe(phy->dev, ret,
+				     "failed to configure default rate\n");
+
 	pm_runtime_get_noresume(phy->dev);
 	pm_runtime_set_active(phy->dev);
 	pm_runtime_enable(phy->dev);
@@ -702,19 +719,31 @@ static int __maybe_unused fsl_samsung_hdmi_phy_suspend(struct device *dev)
 static int __maybe_unused fsl_samsung_hdmi_phy_resume(struct device *dev)
 {
 	struct fsl_samsung_hdmi_phy *phy = dev_get_drvdata(dev);
-	int ret = 0;
+	int ret;
 
 	ret = clk_prepare_enable(phy->apbclk);
 	if (ret) {
 		dev_err(phy->dev, "failed to enable apbclk\n");
 		return ret;
 	}
 
-	if (phy->cur_cfg)
-		ret = fsl_samsung_hdmi_phy_configure(phy, phy->cur_cfg);
+	if (!phy->cur_cfg)
+		return 0;
 
-	return ret;
+	ret = clk_prepare_enable(phy->refclk);
+	if (ret) {
+		dev_err(phy->dev, "failed to enable ref clk\n");
+		goto disable_apbclk;
+	}
+
+	ret = fsl_samsung_hdmi_phy_configure(phy, phy->cur_cfg);
+	clk_disable_unprepare(phy->refclk);
+	if (!ret)
+		return 0;
 
+disable_apbclk:
+	clk_disable_unprepare(phy->apbclk);
+	return ret;
 }
 
 static DEFINE_RUNTIME_DEV_PM_OPS(fsl_samsung_hdmi_phy_pm_ops,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clk_prepare_enable`
- 外部函数：`code`
- 外部函数：`container_of`
- 外部函数：`dev_err`
- 外部函数：`dev_err_probe`
- 外部函数：`dev_get_drvdata`
- 外部函数：`div`
- 外部函数：`pm_runtime_enable`
- 外部函数：`pm_runtime_get_noresume`
- 外部函数：`pm_runtime_set_active`
- 外部函数：`readb_poll_timeout`
- 外部函数：`unlikely`
- 外部函数：`writeb`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`BIT`
- 大写宏：`DEFINE_RUNTIME_DEV_PM_OPS`
- 大写宏：`DIV_ROUND_UP`
- 大写宏：`EINVAL`
- 大写宏：`FIELD_PREP`
- 大写宏：`FLD_CK_DIV`
- 大写宏：`FLD_RP_CODE`
- 大写宏：`FLD_TOL`
- 大写宏：`GENMASK`
- 大写宏：`HDMI`
- 大写宏：`NXP`
- 大写宏：`PHY`
- 大写宏：`PHY_REG1`
- 大写宏：`PHY_REG13`
- 大写宏：`PHY_REG21`
- 大写宏：`PHY_REG3`
- 大写宏：`PHY_REG7`
- 大写宏：`PLL`
- 大写宏：`PTR_ERR`
- 大写宏：`TODO`
- 外部类型：`Calculate`
- 外部类型：`Calculation`
- 外部类型：`Check`
- 外部类型：`Do`
- 外部类型：`Find`
- 外部类型：`Freq`
- 外部类型：`Helper`
- 外部类型：`High`
- 外部类型：`Increase`
- 外部类型：`Reduce`
- 外部类型：`clk`
- 外部类型：`clk_hw`
- 外部类型：`clk_init_data`
- 外部类型：`device`
- 外部类型：`device_node`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-179fae018f` → 本草稿移入 `cases/defect/auto-linux-179fae018f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
