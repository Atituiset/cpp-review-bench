# auto-linux-2ea0b75326

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#21a7f44fd0765fcc5194130b28767786eba1d1e7](https://github.com/torvalds/linux/commit/21a7f44fd0765fcc5194130b28767786eba1d1e7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #21a7f44fd0765fcc5194130b28767786eba1d1e7 (https://github.com/torvalds/linux/commit/21a7f44fd0765fcc5194130b28767786eba1d1e7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 790；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 21a7f44fd0765fcc5194130b28767786eba1d1e7 Merge tag 'devicetree-for-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/robh/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -767,34 +767,27 @@ void __init of_irq_init(const struct of_device_id *matches)
 	}
 }
 
-static int of_check_msi_parent(struct device_node *dev_node, struct device_node **msi_node)
+static int of_check_msi_parent(struct device_node *dev_node, struct device_node **msi_node,
+			       struct of_phandle_args *msi_spec)
 {
-	struct of_phandle_args msi_spec;
 	int ret;
 
 	/*
 	 * An msi-parent phandle with a missing or == 0 #msi-cells
 	 * property identifies a 1:1 ID translation mapping.
 	 *
-	 * Set the msi controller node if the firmware matches this
-	 * condition.
+	 * @msi_spec keeps a reference to the target node whenever the
+	 * phandle parses, -EINVAL included, and the caller releases it.
 	 */
 	ret = of_parse_phandle_with_optional_args(dev_node, "msi-parent", "#msi-cells",
-						  0, &msi_spec);
+						  0, msi_spec);
 	if (ret)
 		return ret;
 
-	if ((*msi_node && *msi_node != msi_spec.np) || msi_spec.args_count != 0)
-		ret = -EINVAL;
-
-	if (!ret) {
-		/* Return with a node reference held */
-		*msi_node = msi_spec.np;
-		return 0;
-	}
-	of_node_put(msi_spec.np);
+	if ((*msi_node && *msi_node != msi_spec->np) || msi_spec->args_count != 0)
+		return -EINVAL;
 
-	return ret;
+	return 0;
 }
 
 /**
@@ -808,7 +801,9 @@ static int of_check_msi_parent(struct device_node *dev_node, struct device_node
  * @id_in: Device ID.
  *
  * Walk up the device hierarchy looking for devices with a "msi-map"
- * or "msi-parent" property. If found, apply the mapping to @id_in.
+ * or "msi-parent" property. If found, apply the mapping to @id_in. With
+ * @msi_np non-NULL, a device declaring an msi-parent ends the walk, usable
+ * or not.
  *
  * Returns: The mapped MSI id.
  */
@@ -823,6 +818,7 @@ u32 of_msi_xlate(struct device *dev, struct device_node **msi_np, u32 id_in)
 	 */
 	for (parent_dev = dev; parent_dev; parent_dev = parent_dev->parent) {
 		struct of_phandle_args msi_spec = {};
+		int ret;
 
 		if (!of_map_msi_id(parent_dev->of_node, id_in, msi_np, &msi_spec)) {
 			if (msi_spec.np) {
@@ -837,8 +833,17 @@ u32 of_msi_xlate(struct device *dev, struct device_node **msi_np, u32 id_in)
 			break;
 		}
 		/* -ENODEV: msi-map absent → check for msi-parent */
-		if (msi_np && !of_check_msi_parent(parent_dev->of_node, msi_np))
+		if (!msi_np)
+			continue;
+
+		ret = of_check_msi_parent(parent_dev->of_node, msi_np, &msi_spec);
+		if (msi_spec.np) {
+			/* A declared msi-parent names the controller, usable or not */
+			if (!ret && !*msi_np)
+				*msi_np = of_node_get(msi_spec.np);
+			of_node_put(msi_spec.np);
 			break;
+		}
 	}
 	return id_out;
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`of_map_msi_id`
- 外部函数：`of_node_put`
- 外部函数：`of_parse_phandle_with_optional_args`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`MSI`
- 外部类型：`An`
- 外部类型：`Device`
- 外部类型：`If`
- 外部类型：`Return`
- 外部类型：`Returns`
- 外部类型：`Set`
- 外部类型：`The`
- 外部类型：`Walk`
- 外部类型：`device_node`
- 外部类型：`of_phandle_args`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-2ea0b75326` → 本草稿移入 `cases/defect/auto-linux-2ea0b75326/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
