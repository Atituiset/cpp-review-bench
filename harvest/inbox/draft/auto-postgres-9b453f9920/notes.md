# auto-postgres-9b453f9920

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#708eb515812bf23ebc36a2c436eabe701b3c2f5d](https://github.com/postgres/postgres/commit/708eb515812bf23ebc36a2c436eabe701b3c2f5d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #708eb515812bf23ebc36a2c436eabe701b3c2f5d (https://github.com/postgres/postgres/commit/708eb515812bf23ebc36a2c436eabe701b3c2f5d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 708eb515812bf23ebc36a2c436eabe701b3c2f5d Fix wrong results from Memoize with a Param inside a cache key :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6448,7 +6448,8 @@ substitute_actual_parameters_in_from_mutator(Node *node,
 
 /*
  * pull_paramids
- *		Returns a Bitmapset containing the paramids of all Params in 'expr'.
+ *		Returns a Bitmapset containing the paramids of all PARAM_EXEC Params
+ *		in 'expr'.
  */
 Bitmapset *
 pull_paramids(Expr *expr)
@@ -6469,7 +6470,8 @@ pull_paramids_walker(Node *node, Bitmapset **context)
 	{
 		Param	   *param = (Param *) node;
 
-		*context = bms_add_member(*context, param->paramid);
+		if (param->paramkind == PARAM_EXEC)
+			*context = bms_add_member(*context, param->paramid);
 		return false;
 	}
 	return expression_tree_walker(node, pull_paramids_walker, context);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_member`
- 外部函数：`expression_tree_walker`
- 外部函数：`pull_paramids_walker`
- 大写宏：`NULL`
- 外部类型：`Bitmapset`
- 外部类型：`Expr`
- 外部类型：`Node`
- 外部类型：`Param`
- 外部类型：`Params`
- 外部类型：`Returns`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-9b453f9920` → 本草稿移入 `cases/defect/auto-postgres-9b453f9920/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
