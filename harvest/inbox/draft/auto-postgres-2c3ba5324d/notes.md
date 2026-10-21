# auto-postgres-2c3ba5324d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1999fe227df70249dce7c9037147b54c1090b4ad](https://github.com/postgres/postgres/commit/1999fe227df70249dce7c9037147b54c1090b4ad) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 60 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1999fe227df70249dce7c9037147b54c1090b4ad (https://github.com/postgres/postgres/commit/1999fe227df70249dce7c9037147b54c1090b4ad)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 3843；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1999fe227df70249dce7c9037147b54c1090b4ad Fix OLD/NEW aliases that EXPLAIN prints in RETURNING SubPlans. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3839,13 +3839,6 @@ set_deparse_context_plan(List *dpcontext, Plan *plan, List *ancestors)
 	dpns->ancestors = ancestors;
 	set_deparse_plan(dpns, plan);
 
-	/* For ModifyTable, set aliases for OLD and NEW in RETURNING */
-	if (IsA(plan, ModifyTable))
-	{
-		dpns->ret_old_alias = ((ModifyTable *) plan)->returningOldAlias;
-		dpns->ret_new_alias = ((ModifyTable *) plan)->returningNewAlias;
-	}
-
 	return dpcontext;
 }
 
@@ -5148,10 +5141,10 @@ get_rtable_name(int rtindex, deparse_context *context)
  * of a given Plan node
  *
  * This sets the plan, outer_plan, inner_plan, outer_tlist, inner_tlist,
- * and index_tlist fields.  Caller must already have adjusted the ancestors
- * list if necessary.  Note that the rtable, subplans, and ctes fields do
- * not need to change when shifting attention to different plan nodes in a
- * single plan tree.
+ * index_tlist, ret_old_alias, and ret_new_alias fields as appropriate.
+ * Caller must already have adjusted the ancestors list if necessary.
+ * Note that the rtable, subplans, and ctes fields do not need to change
+ * when shifting attention to different plan nodes in a single plan tree.
  */
 static void
 set_deparse_plan(deparse_namespace *dpns, Plan *plan)
@@ -5227,6 +5220,15 @@ set_deparse_plan(deparse_namespace *dpns, Plan *plan)
 		dpns->index_tlist = ((CustomScan *) plan)->custom_scan_tlist;
 	else
 		dpns->index_tlist = NIL;
+
+	/* Set up table aliases for OLD and NEW in RETURNING */
+	if (IsA(plan, ModifyTable))
+	{
+		dpns->ret_old_alias = ((ModifyTable *) plan)->returningOldAlias;
+		dpns->ret_new_alias = ((ModifyTable *) plan)->returningNewAlias;
+	}
+	else
+		dpns->ret_old_alias = dpns->ret_new_alias = NULL;
 }
 
 /*
@@ -7735,9 +7737,15 @@ get_variable(Var *var, int levelsup, bool istoplevel, deparse_context *context)
 
 		/* might be returning old/new column value */
 		if (var->varreturningtype == VAR_RETURNING_OLD)
+		{
 			refname = dpns->ret_old_alias;
+			Assert(refname);
+		}
 		else if (var->varreturningtype == VAR_RETURNING_NEW)
+		{
 			refname = dpns->ret_new_alias;
+			Assert(refname);
+		}
 		else
 			refname = (char *) list_nth(dpns->rtable_names, varno - 1);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`innerPlan`
- 外部函数：`lfirst`
- 外部函数：`linitial`
- 外部函数：`list_nth`
- 外部函数：`outerPlan`
- 外部函数：`plan`
- 大写宏：`CMD_INSERT`
- 大写宏：`CMD_MERGE`
- 大写宏：`CONFLICT`
- 大写宏：`ERROR`
- 大写宏：`INDEX_VAR`
- 大写宏：`INNER`
- 大写宏：`INNER_VAR`
- 大写宏：`INSERT`
- 大写宏：`MERGE`
- 大写宏：`NEW`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`OLD`
- 大写宏：`OUTER`
- 大写宏：`OUTER_VAR`
- 大写宏：`RETURNING`
- 大写宏：`SELECT`
- 大写宏：`UPDATE`
- 大写宏：`USING`
- 大写宏：`VAR_RETURNING_NEW`
- 大写宏：`VAR_RETURNING_OLD`
- 外部类型：`Append`
- 外部类型：`AppendRelInfo`
- 外部类型：`Are`
- 外部类型：`Array`
- 外部类型：`Caller`
- 外部类型：`CommonTableExpr`
- 外部类型：`CteScan`
- 外部类型：`CustomScan`
- 外部类型：`For`
- 外部类型：`ForeignScan`
- 外部类型：`IndexOnlyScan`
- 外部类型：`Likewise`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`MergeAppend`
- 外部类型：`ModifyTable`
- 外部类型：`Note`
- 外部类型：`Parallel`
- 外部类型：`Plan`
- 外部类型：`RTEs`
- 外部类型：`RangeTblEntry`
- 外部类型：`RecursiveUnion`
- 外部类型：`Remaining`
- 外部类型：`Set`
- 外部类型：`Similar`
- 外部类型：`Special`
- 外部类型：`SubPlans`
- 外部类型：`SubqueryScan`
- 外部类型：`This`
- 外部类型：`Vars`
- 外部类型：`We`
- 外部类型：`WorkTableScan`
- 外部类型：`Workspace`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-2c3ba5324d` → 本草稿移入 `cases/defect/auto-postgres-2c3ba5324d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
