# auto-postgres-6d3cdb6112

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#cc9a8eb112fa9cf5eb868306955198adb98e497e](https://github.com/postgres/postgres/commit/cc9a8eb112fa9cf5eb868306955198adb98e497e) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 38 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cc9a8eb112fa9cf5eb868306955198adb98e497e (https://github.com/postgres/postgres/commit/cc9a8eb112fa9cf5eb868306955198adb98e497e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 6256；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cc9a8eb112fa9cf5eb868306955198adb98e497e Remove fake FunctionCallInfos from the stats restore and import code :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6253,11 +6253,12 @@ import_fetched_statistics(Relation relation,
 						  int attrcnt)
 {
 	PGresult   *res;
-	NullableDatum args[ATTSTATS_NUM_FIELDS];
+	NullableDatum version;
+	RelationStatsValues relvalues;
 
-	/* Set the 'version' parameter, which is common to both statistics. */
-	args[0].value = Int32GetDatum(remstats->version);
-	args[0].isnull = false;
+	/* Set the 'version' value, which is common to both statistics. */
+	version.value = Int32GetDatum(remstats->version);
+	version.isnull = false;
 
 	/*
 	 * We import attribute statistics first, if any, because those are more
@@ -6274,6 +6275,7 @@ import_fetched_statistics(Relation relation,
 		{
 			int			row = remattrmap[mapidx].res_index;
 			AttrNumber	attnum = remattrmap[mapidx].local_attnum;
+			AttributeStatsValues attvalues;
 
 			/* All mappings should have been assigned a result set row. */
 			Assert(row >= 0);
@@ -6284,41 +6286,38 @@ import_fetched_statistics(Relation relation,
 			/* Clear existing attribute statistics. */
 			delete_attribute_statistics(relation, attnum, false);
 
-			/* Set the remaining parameters. */
-			set_float_arg(&args[1],
+			/* Set the remaining values. */
+			attvalues.version = version;
+			set_float_arg(&attvalues.null_frac,
 						  get_opt_value(res, row, ATTSTATS_NULL_FRAC));
-			set_int32_arg(&args[2],
+			set_int32_arg(&attvalues.avg_width,
 						  get_opt_value(res, row, ATTSTATS_AVG_WIDTH));
-			set_float_arg(&args[3],
+			set_float_arg(&attvalues.n_distinct,
 						  get_opt_value(res, row, ATTSTATS_N_DISTINCT));
-			set_text_arg(&args[4],
+			set_text_arg(&attvalues.most_common_vals,
 						 get_opt_value(res, row, ATTSTATS_MOST_COMMON_VALS));
-			set_floatarr_arg(&args[5],
+			set_floatarr_arg(&attvalues.most_common_freqs,
 							 get_opt_value(res, row, ATTSTATS_MOST_COMMON_FREQS));
-			set_text_arg(&args[6],
+			set_text_arg(&attvalues.histogram_bounds,
 						 get_opt_value(res, row, ATTSTATS_HISTOGRAM_BOUNDS));
-			set_float_arg(&args[7],
+			set_float_arg(&attvalues.correlation,
 						  get_opt_value(res, row, ATTSTATS_CORRELATION));
-			set_text_arg(&args[8],
+			set_text_arg(&attvalues.most_common_elems,
 						 get_opt_value(res, row, ATTSTATS_MOST_COMMON_ELEMS));
-			set_floatarr_arg(&args[9],
+			set_floatarr_arg(&attvalues.most_common_elem_freqs,
 							 get_opt_value(res, row, ATTSTATS_MOST_COMMON_ELEM_FREQS));
-			set_floatarr_arg(&args[10],
+			set_floatarr_arg(&attvalues.elem_count_histogram,
 							 get_opt_value(res, row, ATTSTATS_ELEM_COUNT_HISTOGRAM));
-			set_text_arg(&args[11],
+			set_text_arg(&attvalues.range_length_histogram,
 						 get_opt_value(res, row, ATTSTATS_RANGE_LENGTH_HISTOGRAM));
-			set_float_arg(&args[12],
+			set_float_arg(&attvalues.range_empty_frac,
 						  get_opt_value(res, row, ATTSTATS_RANGE_EMPTY_FRAC));
-			set_text_arg(&args[13],
+			set_text_arg(&attvalues.range_bounds_histogram,
 						 get_opt_value(res, row, ATTSTATS_RANGE_BOUNDS_HISTOGRAM));
 
 			/* Try to import the statistics. */
 			if (!import_attribute_statistics(relation, attnum, false,
-											 &args[0], &args[1], &args[2],
-											 &args[3], &args[4], &args[5],
-											 &args[6], &args[7], &args[8],
-											 &args[9], &args[10], &args[11],
-											 &args[12], &args[13]))
+											 &attvalues))
 			{
 				ereport(WARNING,
 						errmsg("could not import statistics for foreign table \"%s.%s\" --- attribute statistics import failed for column \"%s\" of this foreign table",
@@ -6337,20 +6336,22 @@ import_fetched_statistics(Relation relation,
 	Assert(PQnfields(res) == RELSTATS_NUM_FIELDS);
 	Assert(PQntuples(res) == 1);
 
-	/* Set the remaining parameters. */
-	set_int32_arg(&args[1], get_opt_value(res, 0, RELSTATS_RELPAGES));
-	Assert(!args[1].isnull);
-	set_float_arg(&args[2], get_opt_value(res, 0, RELSTATS_RELTUPLES));
-	Assert(!args[2].isnull);
+	/* Set the remaining values. */
+	relvalues.version = version;
+	set_int32_arg(&relvalues.relpages,
+				  get_opt_value(res, 0,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`delete_attribute_statistics`
- 外部函数：`errmsg`
- 外部函数：`float4in_internal`
- 外部函数：`fmgr_info`
- 外部函数：`get_opt_value`
- 外部函数：`import_attribute_statistics`
- 外部函数：`import_relation_statistics`
- 外部函数：`pg_strtoint32`
- 大写宏：`ATTSTATS_AVG_WIDTH`
- 大写宏：`ATTSTATS_CORRELATION`
- 大写宏：`ATTSTATS_ELEM_COUNT_HISTOGRAM`
- 大写宏：`ATTSTATS_HISTOGRAM_BOUNDS`
- 大写宏：`ATTSTATS_MOST_COMMON_ELEMS`
- 大写宏：`ATTSTATS_MOST_COMMON_ELEM_FREQS`
- 大写宏：`ATTSTATS_MOST_COMMON_FREQS`
- 大写宏：`ATTSTATS_MOST_COMMON_VALS`
- 大写宏：`ATTSTATS_NULL_FRAC`
- 大写宏：`ATTSTATS_NUM_FIELDS`
- 大写宏：`ATTSTATS_N_DISTINCT`
- 大写宏：`ATTSTATS_RANGE_BOUNDS_HISTOGRAM`
- 大写宏：`ATTSTATS_RANGE_EMPTY_FRAC`
- 大写宏：`ATTSTATS_RANGE_LENGTH_HISTOGRAM`
- 大写宏：`FLOAT4OID`
- 大写宏：`F_ARRAY_IN`
- 大写宏：`NULL`
- 大写宏：`RELSTATS_NUM_FIELDS`
- 大写宏：`RELSTATS_RELPAGES`
- 大写宏：`RELSTATS_RELTUPLES`
- 大写宏：`WARNING`
- 外部类型：`All`
- 外部类型：`AttrNumber`
- 外部类型：`Clear`
- 外部类型：`Datum`
- 外部类型：`FmgrInfo`
- 外部类型：`NullableDatum`
- 外部类型：`PGresult`
- 外部类型：`Set`
- 外部类型：`Try`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-6d3cdb6112` → 本草稿移入 `cases/defect/auto-postgres-6d3cdb6112/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
