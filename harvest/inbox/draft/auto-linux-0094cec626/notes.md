# auto-linux-0094cec626

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#0c2669a9f4a1d607e7591ae50ccf3c432a0aff08](https://github.com/torvalds/linux/commit/0c2669a9f4a1d607e7591ae50ccf3c432a0aff08) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0c2669a9f4a1d607e7591ae50ccf3c432a0aff08 (https://github.com/torvalds/linux/commit/0c2669a9f4a1d607e7591ae50ccf3c432a0aff08)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 3013；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0c2669a9f4a1d607e7591ae50ccf3c432a0aff08 Merge tag 'soc-fixes-7.3-2' of git://git.kernel.org/pub/scm/linux/kernel/git/soc/soc :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3010,8 +3010,6 @@ static const u32 gcc_eliza_critical_cbcrs[] = {
 	0x26034, /* GCC_CAMERA_XO_CLK */
 	0x27004, /* GCC_DISP_AHB_CLK */
 	0x71004, /* GCC_GPU_CFG_AHB_CLK */
-	0x52010, /* GCC_PCIE_RSCC_CFG_AHB_CLK */
-	0x52010, /* GCC_PCIE_RSCC_XO_CLK */
 	0x32004, /* GCC_VIDEO_AHB_CLK */
 	0x32038, /* GCC_VIDEO_XO_CLK */
 };
@@ -3045,6 +3043,10 @@ static const struct regmap_config gcc_eliza_regmap_config = {
 
 static void clk_eliza_regs_configure(struct device *dev, struct regmap *regmap)
 {
+	/* Keep clocks always enabled */
+	regmap_update_bits(regmap, 0x52010, BIT(20), BIT(20)); /* GCC_PCIE_RSCC_CFG_AHB_CLK */
+	regmap_update_bits(regmap, 0x52010, BIT(21), BIT(21)); /* GCC_PCIE_RSCC_XO_CLK */
+
 	/* FORCE_MEM_CORE_ON for ufs phy ice core and gcc ufs phy axi clocks  */
 	qcom_branch_set_force_mem_core(regmap, gcc_ufs_phy_ice_core_clk, true);
 	qcom_branch_set_force_mem_core(regmap, gcc_ufs_phy_axi_clk, true);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`qcom_branch_set_force_mem_core`
- 大写宏：`FORCE_MEM_CORE_ON`
- 大写宏：`GCC_CAMERA_XO_CLK`
- 大写宏：`GCC_DISP_AHB_CLK`
- 大写宏：`GCC_GPU_CFG_AHB_CLK`
- 大写宏：`GCC_PCIE_RSCC_CFG_AHB_CLK`
- 大写宏：`GCC_PCIE_RSCC_XO_CLK`
- 大写宏：`GCC_VIDEO_AHB_CLK`
- 大写宏：`GCC_VIDEO_XO_CLK`
- 外部类型：`device`
- 外部类型：`regmap`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0094cec626` → 本草稿移入 `cases/defect/auto-linux-0094cec626/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
