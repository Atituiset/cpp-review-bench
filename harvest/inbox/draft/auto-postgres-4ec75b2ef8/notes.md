# auto-postgres-4ec75b2ef8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#4f89416c9dbc78da5679d32072315af56fbeb8ed](https://github.com/postgres/postgres/commit/4f89416c9dbc78da5679d32072315af56fbeb8ed) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 153 |
| 编译错误数（gcc syntax-only） | 101（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4f89416c9dbc78da5679d32072315af56fbeb8ed (https://github.com/postgres/postgres/commit/4f89416c9dbc78da5679d32072315af56fbeb8ed)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 173；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4f89416c9dbc78da5679d32072315af56fbeb8ed Fix more issues with extended statistics on expressions :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -100,6 +100,18 @@ static StatsBuildData *make_build_data(Relation rel, StatExtEntry *stat,
 									   int numrows, HeapTuple *rows,
 									   VacAttrStats **stats, int stattarget);
 
+static inline void
+report_ext_stats_not_computed(Relation onerel, StatExtEntry *stat)
+{
+	if (!AmAutoVacuumWorkerProcess())
+		ereport(WARNING,
+				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
+				 errmsg("statistics object \"%s.%s\" could not be computed for relation \"%s.%s\"",
+						stat->schema, stat->name,
+						get_namespace_name(onerel->rd_rel->relnamespace),
+						RelationGetRelationName(onerel)),
+				 errtable(onerel)));
+}
 
 /*
  * Compute requested extended stats, using the rows sampled for the plain
@@ -170,14 +182,7 @@ BuildRelationExtStatistics(Relation onerel, bool inh, double totalrows,
 									  natts, vacattrstats);
 		if (!stats)
 		{
-			if (!AmAutoVacuumWorkerProcess())
-				ereport(WARNING,
-						(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
-						 errmsg("statistics object \"%s.%s\" could not be computed for relation \"%s.%s\"",
-								stat->schema, stat->name,
-								get_namespace_name(onerel->rd_rel->relnamespace),
-								RelationGetRelationName(onerel)),
-						 errtable(onerel)));
+			report_ext_stats_not_computed(onerel, stat);
 			continue;
 		}
 
@@ -196,6 +201,11 @@ BuildRelationExtStatistics(Relation onerel, bool inh, double totalrows,
 
 		/* evaluate expressions (if the statistics object has any) */
 		data = make_build_data(onerel, stat, numrows, rows, stats, stattarget);
+		if (!data)
+		{
+			report_ext_stats_not_computed(onerel, stat);
+			continue;
+		}
 
 		/* compute statistic of each requested type */
 		foreach(lc2, stat->types)
@@ -218,6 +228,12 @@ BuildRelationExtStatistics(Relation onerel, bool inh, double totalrows,
 					elog(ERROR, "requested expression stats, but there are no expressions");
 
 				exprdata = build_expr_data(stat->exprs, stattarget);
+				if (!exprdata)
+				{
+					report_ext_stats_not_computed(onerel, stat);
+					data = NULL;
+					break;
+				}
 				nexprs = list_length(stat->exprs);
 
 				compute_expr_stats(onerel, exprdata, nexprs, rows, numrows);
@@ -226,6 +242,9 @@ BuildRelationExtStatistics(Relation onerel, bool inh, double totalrows,
 			}
 		}
 
+		if (!data)
+			continue;
+
 		/* store the statistics in the catalog */
 		statext_store(stat->statOid, inh,
 					  ndistinct, dependencies, mcv, exprstats, stats);
@@ -2303,6 +2322,11 @@ build_expr_data(List *exprs, int stattarget)
 
 		thisdata->expr = expr;
 		thisdata->vacattrstat = examine_expression(expr, stattarget);
+		if (thisdata->vacattrstat == NULL)
+		{
+			pfree(exprdata);
+			return NULL;
+		}
 		idx++;
 	}
 
@@ -2496,6 +2520,8 @@ statext_expressions_load(Oid stxoid, bool inh, int idx)
  * Evaluate the expressions, so that we can use the results to build
  * all the requested statistics types. This matters especially for
  * expensive expressions, of course.
+ *
+ * Returns NULL if an expression cannot be analyzed.
  */
 static StatsBuildData *
 make_build_data(Relation rel, StatExtEntry *stat, int numrows, HeapTuple *rows,
@@ -2585,6 +2611,12 @@ make_build_data(Relation rel, StatExtEntry *stat, int numrows, HeapTuple *rows,
 		result->attnums[idx] = k;
 		result->stats[idx] = examine_expression(expr, stattarget);
 
+		if (result->stats[idx] == NULL)
+		{
+			pfree(result);
+			return NULL;
+		}
+
 		idx++;
 		k--;
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_num_members`
- 外部函数：`build_expr_data`
- 外部函数：`compute_expr_stats`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`errtable`
- 外部函数：`exprCollation`
- 外部函数：`exprType`
- 外部函数：`exprTypmod`
- 外部函数：`expressions`
- 外部函数：`fact`
- 外部函数：`get_namespace_name`
- 外部函数：`lfirst`
- 外部函数：`lfirst_int`
- 外部函数：`list_length`
- 外部函数：`lookup_var_attr_stats`
- 外部函数：`make_build_data`
- 外部函数：`negative`
- 外部函数：`palloc0_object`
- 外部函数：`pgstat_progress_update_param`
- 外部函数：`serialize_expr_stats`
- 外部函数：`statext_dependencies_build`
- 外部函数：`statext_mcv_build`
- 外部函数：`statext_ndistinct_build`
- 外部函数：`statext_store`
- 外部函数：`target`
- 大写宏：`ERRCODE_INVALID_OBJECT_DEFINITION`
- 大写宏：`ERROR`
- 大写宏：`GETSTRUCT`
- 大写宏：`MAX_STATISTICS_TARGET`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`PROGRESS_ANALYZE_EXT_STATS_COMPUTED`
- 大写宏：`STATS_EXT_DEPENDENCIES`
- 大写宏：`STATS_EXT_EXPRESSIONS`
- 大写宏：`STATS_EXT_MCV`
- 大写宏：`STATS_EXT_NDISTINCT`
- 大写宏：`TYPEOID`
- 大写宏：`WARNING`
- 外部类型：`AnlExprData`
- 外部类型：`As`
- 外部类型：`Bitmapset`
- 外部类型：`Check`
- 外部类型：`Compute`
- 外部类型：`Create`
- 外部类型：`Datum`
- 外部类型：`Don`
- 外部类型：`Form_pg_type`
- 外部类型：`HeapTuple`
- 外部类型：`If`
- 外部类型：`InvalidAttrNumber`
- 外部类型：`It`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`MCVList`
- 外部类型：`MVDependencies`
- 外部类型：`MVNDistinct`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`Oid`
- 外部类型：`StatExtEntry`
- 外部类型：`StatsBuildData`
- 外部类型：`The`
- 外部类型：`VacAttrStats`
- 外部类型：`We`
- 外部类型：`When`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-4ec75b2ef8` → 本草稿移入 `cases/defect/auto-postgres-4ec75b2ef8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
