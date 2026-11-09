# auto-linux-961dfda5e9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#2651f6deb3326129a052897df193052dc28ea94f](https://github.com/torvalds/linux/commit/2651f6deb3326129a052897df193052dc28ea94f) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 39 |
| 编译错误数（gcc syntax-only） | 65（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2651f6deb3326129a052897df193052dc28ea94f (https://github.com/torvalds/linux/commit/2651f6deb3326129a052897df193052dc28ea94f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 898；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2651f6deb3326129a052897df193052dc28ea94f Merge tag 'mips-fixes_7.4_1' of git://git.kernel.org/pub/scm/linux/kernel/git/mips/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -892,15 +892,21 @@ static void __init au1000_init_irq(struct alchemy_irqmap *map)
 	mips_cpu_irq_init();
 
 	/* register all 64 possible IC0+IC1 irq sources as type "none".
+	 * Register each chip with the IRQ core before setting its type so
+	 * the descriptors are included in interrupt enumeration.
 	 * Use set_irq_type() to set edge/level behaviour at runtime.
 	 */
 	for (irq_nr = AU1000_INTC0_INT_BASE;
-	     (irq_nr < AU1000_INTC0_INT_BASE + 32); irq_nr++)
+	     (irq_nr < AU1000_INTC0_INT_BASE + 32); irq_nr++) {
+		irq_set_chip(irq_nr, &au1x_ic0_chip);
 		au1x_ic_settype(irq_get_irq_data(irq_nr), IRQ_TYPE_NONE);
+	}
 
 	for (irq_nr = AU1000_INTC1_INT_BASE;
-	     (irq_nr < AU1000_INTC1_INT_BASE + 32); irq_nr++)
+	     (irq_nr < AU1000_INTC1_INT_BASE + 32); irq_nr++) {
+		irq_set_chip(irq_nr, &au1x_ic1_chip);
 		au1x_ic_settype(irq_get_irq_data(irq_nr), IRQ_TYPE_NONE);
+	}
 
 	/*
 	 * Initialize IC0, which is fixed per processor.
@@ -947,6 +953,7 @@ static void __init alchemy_gpic_init_irq(const struct alchemy_irqmap *dints)
 
 	/* register an irq_chip for them, with 2nd highest priority */
 	for (i = ALCHEMY_GPIC_INT_BASE; i <= ALCHEMY_GPIC_INT_LAST; i++) {
+		irq_set_chip(i, &au1300_gpic);
 		au1300_set_irq_priority(i, 1);
 		au1300_gpic_settype(irq_get_irq_data(i), IRQ_TYPE_NONE);
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__raw_readl`
- 外部函数：`__raw_writel`
- 外部函数：`irq_get_irq_data`
- 外部函数：`irq_set_chip_handler_name_locked`
- 外部函数：`mips_cpu_irq_init`
- 外部函数：`set_irq_type`
- 外部函数：`wmb`
- 大写宏：`ALCHEMY_GPIC_INT_BASE`
- 大写宏：`ALCHEMY_GPIC_INT_LAST`
- 大写宏：`AU1000_IC0_PHYS_ADDR`
- 大写宏：`AU1000_IC1_PHYS_ADDR`
- 大写宏：`AU1000_INTC0_INT_BASE`
- 大写宏：`AU1000_INTC1_INT_BASE`
- 大写宏：`AU1300_GPIC_ADDR`
- 大写宏：`AU1300_GPIC_PINCFG`
- 大写宏：`EINVAL`
- 大写宏：`GPIC_CFG_IC_EDGE_BOTH`
- 大写宏：`GPIC_CFG_IC_EDGE_FALL`
- 大写宏：`GPIC_CFG_IC_EDGE_RISE`
- 大写宏：`GPIC_CFG_IC_LEVEL_HIGH`
- 大写宏：`GPIC_CFG_IC_LEVEL_LOW`
- 大写宏：`GPIC_CFG_IC_MASK`
- 大写宏：`GPIC_CFG_IC_OFF`
- 大写宏：`GPIC_CFG_IL_MASK`
- 大写宏：`GPIC_CFG_IL_SET`
- 大写宏：`IC0`
- 大写宏：`IC1`
- 大写宏：`IRQ_TYPE_EDGE_BOTH`
- 大写宏：`IRQ_TYPE_EDGE_FALLING`
- 大写宏：`IRQ_TYPE_EDGE_RISING`
- 大写宏：`IRQ_TYPE_LEVEL_HIGH`
- 大写宏：`IRQ_TYPE_LEVEL_LOW`
- 大写宏：`IRQ_TYPE_NONE`
- 大写宏：`KSEG1ADDR`
- 大写宏：`NULL`
- 外部类型：`Initialize`
- 外部类型：`Use`
- 外部类型：`irq_chip`
- 外部类型：`irq_data`
- 外部类型：`irq_flow_handler_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-961dfda5e9` → 本草稿移入 `cases/defect/auto-linux-961dfda5e9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
