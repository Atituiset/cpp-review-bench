# auto-postgres-00ba46e080

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#0fb258d1f2ca65b7299ba7c47045634df05efbff](https://github.com/postgres/postgres/commit/0fb258d1f2ca65b7299ba7c47045634df05efbff) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 49 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0fb258d1f2ca65b7299ba7c47045634df05efbff (https://github.com/postgres/postgres/commit/0fb258d1f2ca65b7299ba7c47045634df05efbff)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 7082；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0fb258d1f2ca65b7299ba7c47045634df05efbff postgres_fdw: Fix "may be used uninitialized" warning in foreign_join_ok() :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7045,8 +7045,6 @@ foreign_join_ok(PlannerInfo *root, RelOptInfo *joinrel, JoinType jointype,
 	PgFdwRelationInfo *fpinfo_i;
 	ListCell   *lc;
 	List	   *joinclauses;
-	bool		outer_is_function = false;
-	bool		inner_is_function = false;
 
 	/*
 	 * We support pushing down INNER, LEFT, RIGHT, FULL OUTER and SEMI joins.
@@ -7070,40 +7068,22 @@ foreign_join_ok(PlannerInfo *root, RelOptInfo *joinrel, JoinType jointype,
 	 * function RTE can be absorbed into joins on multiple foreign servers
 	 * (each call gets its own stub fpinfo and rechecks shippability for the
 	 * specific server).
+	 *
+	 * A function rel has no fdw_private of its own, so when one side is a
+	 * function RTE we replace its NULL fpinfo with a stub, and the rest of
+	 * this function and the cost estimator can then treat both sides
+	 * uniformly.  We hand the stub to the joinrel's deparser via the same
+	 * path the foreign side uses, but we never permanently attach it to the
+	 * function rel's fdw_private (different joinrels may pair the same
+	 * function RTE with different foreign servers).
 	 */
 	fpinfo = (PgFdwRelationInfo *) joinrel->fdw_private;
+	fpinfo_o = (PgFdwRelationInfo *) outerrel->fdw_private;
+	fpinfo_i = (PgFdwRelationInfo *) innerrel->fdw_private;
+
 	if (jointype == JOIN_INNER && innerrel->rtekind == RTE_FUNCTION &&
-		(fpinfo_o = (PgFdwRelationInfo *) outerrel->fdw_private) &&
-		fpinfo_o->pushdown_safe &&
+		fpinfo_o && fpinfo_o->pushdown_safe &&
 		function_rte_pushdown_ok(root, innerrel, outerrel))
-	{
-		inner_is_function = true;
-	}
-	else if (jointype == JOIN_INNER && outerrel->rtekind == RTE_FUNCTION &&
-			 (fpinfo_i = (PgFdwRelationInfo *) innerrel->fdw_private) &&
-			 fpinfo_i->pushdown_safe &&
-			 function_rte_pushdown_ok(root, outerrel, innerrel))
-	{
-		outer_is_function = true;
-	}
-	else
-	{
-		fpinfo_o = (PgFdwRelationInfo *) outerrel->fdw_private;
-		fpinfo_i = (PgFdwRelationInfo *) innerrel->fdw_private;
-		if (!fpinfo_o || !fpinfo_o->pushdown_safe ||
-			!fpinfo_i || !fpinfo_i->pushdown_safe)
-			return false;
-	}
-
-	/*
-	 * If one side is a function RTE, allocate a stub fpinfo so the rest of
-	 * this function and the cost estimator can treat it uniformly.  We hand
-	 * the stub to the joinrel's deparser via the same path the foreign side
-	 * uses, but we never permanently attach it to the function rel's
-	 * fdw_private (different joinrels may pair the same function RTE with
-	 * different foreign servers).
-	 */
-	if (inner_is_function)
 	{
 		fpinfo_i = init_func_stub_fpinfo(fpinfo_o, innerrel);
 
@@ -7118,15 +7098,20 @@ foreign_join_ok(PlannerInfo *root, RelOptInfo *joinrel, JoinType jointype,
 						   &fpinfo_i->remote_conds, &fpinfo_i->local_conds);
 		fpinfo->inner_func_fpinfo = fpinfo_i;
 	}
-	else if (outer_is_function)
+	else if (jointype == JOIN_INNER && outerrel->rtekind == RTE_FUNCTION &&
+			 fpinfo_i && fpinfo_i->pushdown_safe &&
+			 function_rte_pushdown_ok(root, outerrel, innerrel))
 	{
 		fpinfo_o = init_func_stub_fpinfo(fpinfo_i, outerrel);
 
-		/* See the comment in the inner_is_function branch above. */
+		/* See the comment in the branch above. */
 		classifyConditions(root, outerrel, fpinfo_o, outerrel->baserestrictinfo,
 						   &fpinfo_o->remote_conds, &fpinfo_o->local_conds);
 		fpinfo->outer_func_fpinfo = fpinfo_o;
 	}
+	else if (!fpinfo_o || !fpinfo_o->pushdown_safe ||
+			 !fpinfo_i || !fpinfo_i->pushdown_safe)
+		return false;
 
 	/*
 	 * If joining relations have local conditions, those conditions are
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_is_empty`
- 外部函数：`build_simple_rel`
- 外部函数：`classifyConditions`
- 外部函数：`contain_subplans`
- 外部函数：`f`
- 外部函数：`foreign_expr_walker`
- 外部函数：`get_expr_result_type`
- 外部函数：`is_foreign_expr`
- 外部函数：`lfirst`
- 外部函数：`list_length`
- 外部函数：`palloc0_object`
- 外部函数：`planner_rt_fetch`
- 外部函数：`postgresGetForeignJoinPaths`
- 外部函数：`psprintf`
- 大写宏：`FULL`
- 大写宏：`INNER`
- 大写宏：`JOIN_INNER`
- 大写宏：`LEFT`
- 大写宏：`OUTER`
- 大写宏：`PARAM_EXEC`
- 大写宏：`RECORDOID`
- 大写宏：`RIGHT`
- 大写宏：`RTE`
- 大写宏：`RTE_FUNCTION`
- 大写宏：`SEMI`
- 大写宏：`TYPEFUNC_SCALAR`
- 大写宏：`VOIDOID`
- 外部类型：`Expr`
- 外部类型：`FuncExpr`
- 外部类型：`Function`
- 外部类型：`If`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Note`
- 外部类型：`Oid`
- 外部类型：`Param`
- 外部类型：`PgFdwRelationInfo`
- 外部类型：`PlannerInfo`
- 外部类型：`RangeTblEntry`
- 外部类型：`RangeTblFunction`
- 外部类型：`Refuse`
- 外部类型：`Reject`
- 外部类型：`RelOptInfo`
- 外部类型：`See`
- 外部类型：`Server`
- 外部类型：`The`
- 外部类型：`TupleDesc`
- 外部类型：`TypeFuncClass`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-00ba46e080` → 本草稿移入 `cases/defect/auto-postgres-00ba46e080/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
