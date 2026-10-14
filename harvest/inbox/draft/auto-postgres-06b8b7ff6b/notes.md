# auto-postgres-06b8b7ff6b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#eacee6b618718f9e5298df0629d9720fa4dd5e45](https://github.com/postgres/postgres/commit/eacee6b618718f9e5298df0629d9720fa4dd5e45) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #eacee6b618718f9e5298df0629d9720fa4dd5e45 (https://github.com/postgres/postgres/commit/eacee6b618718f9e5298df0629d9720fa4dd5e45)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT eacee6b618718f9e5298df0629d9720fa4dd5e45 Don't use SubPlan-containing expressions for startup-time pruning :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -46,6 +46,7 @@
 #include "nodes/makefuncs.h"
 #include "nodes/nodeFuncs.h"
 #include "optimizer/appendinfo.h"
+#include "optimizer/clauses.h"
 #include "optimizer/cost.h"
 #include "optimizer/optimizer.h"
 #include "optimizer/pathnode.h"
@@ -2073,7 +2074,11 @@ match_clause_to_partition_key(GeneratePruningStepsContext *context,
 
 			/*
 			 * See if there are any exec Params.  If so, we can only use this
-			 * expression during per-scan pruning.
+			 * expression during per-scan pruning.  The same is true if it
+			 * contains a SubPlan, as evaluating that requires the parent
+			 * node's PlanState, which startup-time pruning hasn't got.  (A
+			 * SubPlan's testexpr normally contains exec Params, so we'd have
+			 * caught it above, but the testexpr can be folded to a Const.)
 			 */
 			paramids = pull_exec_paramids(expr);
 			if (!bms_is_empty(paramids))
@@ -2082,6 +2087,11 @@ match_clause_to_partition_key(GeneratePruningStepsContext *context,
 				if (context->target != PARTTARGET_EXEC)
 					return PARTCLAUSE_UNSUPPORTED;
 			}
+			else if (contain_subplans((Node *) expr))
+			{
+				if (context->target != PARTTARGET_EXEC)
+					return PARTCLAUSE_UNSUPPORTED;
+			}
 			else
 			{
 				/* It's potentially usable, but mutable */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_member`
- 外部函数：`bms_is_empty`
- 外部函数：`expression_tree_walker`
- 大写宏：`NULL`
- 大写宏：`PARAM_EXEC`
- 大写宏：`PARTCLAUSE_UNSUPPORTED`
- 大写宏：`PARTTARGET_EXEC`
- 外部类型：`Bitmapset`
- 外部类型：`Expr`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`Node`
- 外部类型：`Param`
- 外部类型：`Params`
- 外部类型：`See`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-06b8b7ff6b` → 本草稿移入 `cases/defect/auto-postgres-06b8b7ff6b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
