# auto-linux-0ad84ad4e4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a74306e2e676f9775457366fc047a660fbf02f26](https://github.com/torvalds/linux/commit/a74306e2e676f9775457366fc047a660fbf02f26) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 27（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a74306e2e676f9775457366fc047a660fbf02f26 (https://github.com/torvalds/linux/commit/a74306e2e676f9775457366fc047a660fbf02f26)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 252；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a74306e2e676f9775457366fc047a660fbf02f26 Merge tag 'clk-fixes-for-linus-v7.3' of git://git.kernel.org/pub/scm/linux/kernel/git/clk/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -52,7 +52,7 @@ static const struct clk_ops ti_composite_gate_ops = {
 
 struct component_clk {
 	int num_parents;
-	struct clk_parent_data *parent_data;
+	const char **parent_names;
 	struct device_node *node;
 	int type;
 	struct clk_hw *hw;
@@ -116,7 +116,7 @@ static void __init _register_composite(void *user,
 	struct clk_hw_omap_comp *cclk = to_clk_hw_comp(hw);
 	struct component_clk *comp;
 	int num_parents = 0;
-	struct clk_parent_data *parent_data = NULL;
+	const char **parent_names = NULL;
 	const char *name;
 	int i;
 	int ret;
@@ -155,7 +155,7 @@ static void __init _register_composite(void *user,
 			continue;
 		if (comp->num_parents) {
 			num_parents = comp->num_parents;
-			parent_data = comp->parent_data;
+			parent_names = comp->parent_names;
 			break;
 		}
 	}
@@ -166,8 +166,8 @@ static void __init _register_composite(void *user,
 	}
 
 	name = ti_dt_clk_name(node);
-	clk = clk_register_composite_pdata(NULL, name,
-				     parent_data, num_parents,
+	clk = clk_register_composite(NULL, name,
+				     parent_names, num_parents,
 				     _get_hw(cclk, CLK_COMPONENT_TYPE_MUX),
 				     &ti_clk_mux_ops,
 				     _get_hw(cclk, CLK_COMPONENT_TYPE_DIVIDER),
@@ -190,7 +190,7 @@ static void __init _register_composite(void *user,
 		if (!cclk->comp_clks[i])
 			continue;
 		list_del(&cclk->comp_clks[i]->link);
-		kfree(cclk->comp_clks[i]->parent_data);
+		kfree(cclk->comp_clks[i]->parent_names);
 		kfree(cclk->comp_clks[i]);
 	}
 
@@ -237,9 +237,8 @@ int __init ti_clk_add_component(struct device_node *node, struct clk_hw *hw,
 				int type)
 {
 	unsigned int num_parents;
-	struct clk_parent_data *parent_data;
+	const char **parent_names;
 	struct component_clk *clk;
-	unsigned int i;
 
 	num_parents = of_clk_get_parent_count(node);
 
@@ -248,21 +247,20 @@ int __init ti_clk_add_component(struct device_node *node, struct clk_hw *hw,
 		return -EINVAL;
 	}
 
-	parent_data = kzalloc_objs(*parent_data, num_parents);
-	if (!parent_data)
+	parent_names = kcalloc(num_parents, sizeof(char *), GFP_KERNEL);
+	if (!parent_names)
 		return -ENOMEM;
 
-	for (i = 0; i < num_parents; i++)
-		parent_data[i].index = i;
+	of_clk_parent_fill(node, parent_names, num_parents);
 
 	clk = kzalloc_obj(*clk);
 	if (!clk) {
-		kfree(parent_data);
+		kfree(parent_names);
 		return -ENOMEM;
 	}
 
 	clk->num_parents = num_parents;
-	clk->parent_data = parent_data;
+	clk->parent_names = parent_names;
 	clk->hw = hw;
 	clk->node = node;
 	clk->type = type;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clk_register_composite_pdata`
- 外部函数：`container_of`
- 外部函数：`kfree`
- 外部函数：`kzalloc_obj`
- 外部函数：`kzalloc_objs`
- 外部函数：`list_del`
- 外部函数：`of_clk_get_parent_count`
- 外部函数：`ti_dt_clk_name`
- 外部函数：`to_clk_hw_comp`
- 大写宏：`CLK_COMPONENT_TYPE_DIVIDER`
- 大写宏：`CLK_COMPONENT_TYPE_MAX`
- 大写宏：`CLK_COMPONENT_TYPE_MUX`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`NULL`
- 外部类型：`clk_hw`
- 外部类型：`clk_parent_data`
- 外部类型：`device_node`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-0ad84ad4e4` → 本草稿移入 `cases/defect/auto-linux-0ad84ad4e4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
