# auto-linux-08cf89e1af

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#2b19759b46aee85627b58f3722846424201f24a7](https://github.com/torvalds/linux/commit/2b19759b46aee85627b58f3722846424201f24a7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2b19759b46aee85627b58f3722846424201f24a7 (https://github.com/torvalds/linux/commit/2b19759b46aee85627b58f3722846424201f24a7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2772；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2b19759b46aee85627b58f3722846424201f24a7 Merge tag 'irq-urgent-2026-10-25' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2748,6 +2748,28 @@ int irq_get_irqchip_state(unsigned int irq, enum irqchip_irq_state which, bool *
 }
 EXPORT_SYMBOL_GPL(irq_get_irqchip_state);
 
+int __irq_set_irqchip_state(struct irq_desc *desc, enum irqchip_irq_state which, bool val)
+{
+	struct irq_data *data = irq_desc_get_irq_data(desc);
+	struct irq_chip *chip;
+
+	do {
+		chip = irq_data_get_irq_chip(data);
+
+		if (WARN_ON_ONCE(!chip))
+			return -ENODEV;
+
+		if (chip->irq_set_irqchip_state)
+			break;
+
+		data = irqd_get_parent_data(data);
+	} while (data);
+
+	if (data)
+		return chip->irq_set_irqchip_state(data, which, val);
+	return -EINVAL;
+}
+
 /**
  * irq_set_irqchip_state - set the state of a forwarded interrupt.
  * @irq:	Interrupt line that is forwarded to a VM
@@ -2762,25 +2784,8 @@ EXPORT_SYMBOL_GPL(irq_get_irqchip_state);
  */
 int irq_set_irqchip_state(unsigned int irq, enum irqchip_irq_state which, bool val)
 {
-	scoped_irqdesc_get_and_buslock(irq, 0) {
-		struct irq_data *data = irq_desc_get_irq_data(scoped_irqdesc);
-		struct irq_chip *chip;
-
-		do {
-			chip = irq_data_get_irq_chip(data);
-
-			if (WARN_ON_ONCE(!chip))
-				return -ENODEV;
-
-			if (chip->irq_set_irqchip_state)
-				break;
-
-			data = irqd_get_parent_data(data);
-		} while (data);
-
-		if (data)
-			return chip->irq_set_irqchip_state(data, which, val);
-	}
+	scoped_irqdesc_get_and_buslock(irq, 0)
+		return __irq_set_irqchip_state(scoped_irqdesc, which, val);
 	return -EINVAL;
 }
 EXPORT_SYMBOL_GPL(irq_set_irqchip_state);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`irq_data_get_irq_chip`
- 外部函数：`irq_desc_get_irq_data`
- 外部函数：`irqd_get_parent_data`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`EXPORT_SYMBOL_GPL`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Interrupt`
- 外部类型：`irq_chip`
- 外部类型：`irq_data`
- 外部类型：`irqchip_irq_state`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-08cf89e1af` → 本草稿移入 `cases/defect/auto-linux-08cf89e1af/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
