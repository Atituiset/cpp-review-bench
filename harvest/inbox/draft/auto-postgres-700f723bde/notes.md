# auto-postgres-700f723bde

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#852fd5b86e1a2ac1139c716eb4c96c09c03a632e](https://github.com/postgres/postgres/commit/852fd5b86e1a2ac1139c716eb4c96c09c03a632e) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 61 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #852fd5b86e1a2ac1139c716eb4c96c09c03a632e (https://github.com/postgres/postgres/commit/852fd5b86e1a2ac1139c716eb4c96c09c03a632e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1109；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 852fd5b86e1a2ac1139c716eb4c96c09c03a632e Fix join alias expansion in lateral TABLESAMPLE clauses :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -94,10 +94,11 @@ create_upper_paths_hook_type create_upper_paths_hook = NULL;
 #define EXPRKIND_APPINFO			7
 #define EXPRKIND_PHV				8
 #define EXPRKIND_TABLESAMPLE		9
-#define EXPRKIND_ARBITER_ELEM		10
-#define EXPRKIND_TABLEFUNC			11
-#define EXPRKIND_TABLEFUNC_LATERAL	12
-#define EXPRKIND_GROUPEXPR			13
+#define EXPRKIND_TABLESAMPLE_LATERAL	10
+#define EXPRKIND_ARBITER_ELEM		11
+#define EXPRKIND_TABLEFUNC			12
+#define EXPRKIND_TABLEFUNC_LATERAL	13
+#define EXPRKIND_GROUPEXPR			14
 
 /*
  * Data specific to grouping sets
@@ -1103,10 +1104,15 @@ subquery_planner(PlannerGlobal *glob, Query *parse, char *plan_name,
 		if (rte->rtekind == RTE_RELATION)
 		{
 			if (rte->tablesample)
+			{
+				/* Preprocess the tablesample expression(s) fully */
+				kind = rte->lateral ? EXPRKIND_TABLESAMPLE_LATERAL :
+					EXPRKIND_TABLESAMPLE;
 				rte->tablesample = (TableSampleClause *)
 					preprocess_expression(root,
 										  (Node *) rte->tablesample,
-										  EXPRKIND_TABLESAMPLE);
+										  kind);
+			}
 		}
 		else if (rte->rtekind == RTE_SUBQUERY)
 		{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`canonicalize_qual`
- 外部函数：`convert_saop_to_hashed_saop`
- 外部函数：`copyObject`
- 外部函数：`eval_const_expressions`
- 外部函数：`flatten_join_alias_vars`
- 外部函数：`make_ands_implicit`
- 外部函数：`nodes`
- 外部函数：`pprint`
- 外部函数：`printf`
- 外部函数：`query_tree_walker`
- 大写宏：`AND`
- 大写宏：`ANY`
- 大写宏：`EXPRKIND_RTFUNC_LATERAL`
- 大写宏：`NULL`
- 大写宏：`OPTIMIZER_DEBUG`
- 大写宏：`PHV`
- 大写宏：`RTE`
- 大写宏：`RTE_RELATION`
- 大写宏：`RTE_SUBQUERY`
- 大写宏：`TABLESAMPLE`
- 大写宏：`VALUES`
- 大写宏：`XXX`
- 外部类型：`Adjust`
- 外部类型：`After`
- 外部类型：`All`
- 外部类型：`Also`
- 外部类型：`But`
- 外部类型：`Check`
- 外部类型：`Const`
- 外部类型：`Data`
- 外部类型：`Expand`
- 外部类型：`Expr`
- 外部类型：`Fall`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Is`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`PHVs`
- 外部类型：`Param`
- 外部类型：`PlaceHolderVar`
- 外部类型：`PlaceHolderVars`
- 外部类型：`PlannerInfo`
- 外部类型：`Preprocess`
- 外部类型：`Previous`
- 外部类型：`Query`
- 外部类型：`RTEs`
- 外部类型：`Replace`
- 外部类型：`SS_process_sublinks`
- 外部类型：`SS_replace_correlation_vars`
- 外部类型：`ScalarArrayOpExpr`
- 外部类型：`Simplify`
- 外部类型：`SubLink`
- 外部类型：`SubLinks`
- 外部类型：`SubPlan`
- 外部类型：`SubPlans`
- 外部类型：`TableSampleClause`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Vars`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-700f723bde` → 本草稿移入 `cases/defect/auto-postgres-700f723bde/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
