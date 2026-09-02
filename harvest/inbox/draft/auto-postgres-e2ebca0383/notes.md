# auto-postgres-e2ebca0383

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
| 外部依赖数（dep_count） | 82 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cc9a8eb112fa9cf5eb868306955198adb98e497e (https://github.com/postgres/postgres/commit/cc9a8eb112fa9cf5eb868306955198adb98e497e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 107；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cc9a8eb112fa9cf5eb868306955198adb98e497e Remove fake FunctionCallInfos from the stats restore and import code :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -104,12 +104,12 @@ static struct StatsArgInfo cleararginfo[] =
 	[C_NUM_ATTRIBUTE_STATS_ARGS] = {0}
 };
 
-static bool attribute_statistics_update(FunctionCallInfo fcinfo);
+static bool attribute_statistics_update(const NullableDatum *args);
 static bool attribute_statistics_update_internal(Oid reloid,
 												 const char *attname,
 												 AttrNumber attnum,
 												 bool inherited,
-												 FunctionCallInfo fcinfo);
+												 const AttributeStatsValues *statvalues);
 static void upsert_pg_statistic(Relation starel, HeapTuple oldtup,
 								const Datum *values, const bool *nulls, const bool *replaces);
 static bool delete_pg_statistic(Oid reloid, AttrNumber attnum, bool stainherit);
@@ -131,7 +131,7 @@ static bool delete_pg_statistic(Oid reloid, AttrNumber attnum, bool stainherit);
  * and other statistic kinds may still be updated.
  */
 static bool
-attribute_statistics_update(FunctionCallInfo fcinfo)
+attribute_statistics_update(const NullableDatum *args)
 {
 	char	   *nspname;
 	char	   *relname;
@@ -140,12 +140,13 @@ attribute_statistics_update(FunctionCallInfo fcinfo)
 	AttrNumber	attnum;
 	bool		inherited;
 	Oid			locked_table = InvalidOid;
+	AttributeStatsValues values;
 
-	stats_check_required_arg(fcinfo, attarginfo, ATTRELSCHEMA_ARG);
-	stats_check_required_arg(fcinfo, attarginfo, ATTRELNAME_ARG);
+	stats_check_required_arg(args, attarginfo, ATTRELSCHEMA_ARG);
+	stats_check_required_arg(args, attarginfo, ATTRELNAME_ARG);
 
-	nspname = TextDatumGetCString(PG_GETARG_DATUM(ATTRELSCHEMA_ARG));
-	relname = TextDatumGetCString(PG_GETARG_DATUM(ATTRELNAME_ARG));
+	nspname = TextDatumGetCString(args[ATTRELSCHEMA_ARG].value);
+	relname = TextDatumGetCString(args[ATTRELNAME_ARG].value);
 
 	if (RecoveryInProgress())
 		ereport(ERROR,
@@ -159,13 +160,13 @@ attribute_statistics_update(FunctionCallInfo fcinfo)
 									  RangeVarCallbackForStats, &locked_table);
 
 	/* user can specify either attname or attnum, but not both */
-	if (!PG_ARGISNULL(ATTNAME_ARG))
+	if (!args[ATTNAME_ARG].isnull)
 	{
-		if (!PG_ARGISNULL(ATTNUM_ARG))
+		if (!args[ATTNUM_ARG].isnull)
 			ereport(ERROR,
 					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
 					 errmsg("cannot specify both \"%s\" and \"%s\"", "attname", "attnum")));
-		attname = TextDatumGetCString(PG_GETARG_DATUM(ATTNAME_ARG));
+		attname = TextDatumGetCString(args[ATTNAME_ARG].value);
 		attnum = get_attnum(reloid, attname);
 		/* note that this test covers attisdropped cases too: */
 		if (attnum == InvalidAttrNumber)
@@ -174,9 +175,9 @@ attribute_statistics_update(FunctionCallInfo fcinfo)
 					 errmsg("column \"%s\" of relation \"%s\" does not exist",
 							attname, relname)));
 	}
-	else if (!PG_ARGISNULL(ATTNUM_ARG))
+	else if (!args[ATTNUM_ARG].isnull)
 	{
-		attnum = PG_GETARG_INT16(ATTNUM_ARG);
+		attnum = DatumGetInt16(args[ATTNUM_ARG].value);
 		attname = get_attname(reloid, attnum, true);
 		/* annoyingly, get_attname doesn't check attisdropped */
 		if (attname == NULL ||
@@ -201,11 +202,28 @@ attribute_statistics_update(FunctionCallInfo fcinfo)
 				 errmsg("cannot modify statistics on system column \"%s\"",
 						attname)));
 
-	stats_check_required_arg(fcinfo, attarginfo, INHERITED_ARG);
-	inherited = PG_GETARG_BOOL(INHERITED_ARG);
+	stats_check_required_arg(args, attarginfo, INHERITED_ARG);
+	inherited = DatumGetBool(args[INHERITED_ARG].value);
+
+	/* Collect the values to apply */
+	values.version.value = (Datum) 0;
+	values.version.isnull = true;
+	values.null_frac = args[NULL_FRAC_ARG];
+	values.avg_width = args[AVG_WIDTH_ARG];
+	values.n_distinct = args[N_DISTINCT_ARG];
+	values.most_common_vals = args[MOST_COMMON_VALS_ARG];
+	values.most_common_freqs = args[MOST_COMMON_FREQS_ARG];
+	values.histogram_bounds = args[HISTOGRAM_BOUNDS_ARG];
+	values.correlation = args[CORRELATION_ARG];
+	values.most_common_elems = args[MOST_COMMON_ELEMS_ARG];
+	values.most_common_elem_freqs = args[MOST_COMMON_ELEM_FREQS_ARG];
+	values.elem_count_histogram 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`construct_array_builtin`
- 外部函数：`delete_pg_statistic`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`get_attname`
- 外部函数：`get_attnum`
- 外部函数：`statatt_build_stavalues`
- 外部函数：`stats_check_arg_array`
- 外部函数：`stats_check_arg_pair`
- 外部函数：`stats_check_required_arg`
- 外部函数：`upsert_pg_statistic`
- 大写宏：`ATTNAME_ARG`
- 大写宏：`ATTNUM_ARG`
- 大写宏：`ATTRELNAME_ARG`
- 大写宏：`ATTRELSCHEMA_ARG`
- 大写宏：`AVG_WIDTH_ARG`
- 大写宏：`CORRELATION_ARG`
- 大写宏：`C_NUM_ATTRIBUTE_STATS_ARGS`
- 大写宏：`ELEM_COUNT_HISTOGRAM_ARG`
- 大写宏：`ERRCODE_INVALID_PARAMETER_VALUE`
- 大写宏：`ERROR`
- 大写宏：`FLOAT4OID`
- 大写宏：`HISTOGRAM_BOUNDS_ARG`
- 大写宏：`INHERITED_ARG`
- 大写宏：`MOST_COMMON_ELEMS_ARG`
- 大写宏：`MOST_COMMON_ELEM_FREQS_ARG`
- 大写宏：`MOST_COMMON_FREQS_ARG`
- 大写宏：`MOST_COMMON_VALS_ARG`
- 大写宏：`NULL`
- 大写宏：`NULL_FRAC_ARG`
- 大写宏：`N_DISTINCT_ARG`
- 大写宏：`PG_ARGISNULL`
- 大写宏：`PG_GETARG_BOOL`
- 大写宏：`PG_GETARG_DATUM`
- 大写宏：`PG_GETARG_INT16`
- 大写宏：`RANGE_BOUNDS_HISTOGRAM_ARG`
- 大写宏：`RANGE_EMPTY_FRAC_ARG`
- 大写宏：`RANGE_LENGTH_HISTOGRAM_ARG`
- 大写宏：`STATISTIC_KIND_CORRELATION`
- 大写宏：`STATISTIC_KIND_MCELEM`
- 大写宏：`STATISTIC_KIND_MCV`
- 外部类型：`Anum_pg_statistic_stadistinct`
- 外部类型：`Anum_pg_statistic_stanullfrac`
- 外部类型：`Anum_pg_statistic_stawidth`
- 外部类型：`ArrayType`
- 外部类型：`AttrNumber`
- 外部类型：`Datum`
- 外部类型：`FmgrInfo`
- 外部类型：`FunctionCallInfo`
- 外部类型：`HeapTuple`
- 外部类型：`InvalidAttrNumber`
- 外部类型：`InvalidOid`
- 外部类型：`Natts_pg_statistic`
- 外部类型：`Oid`
- 外部类型：`RangeVarCallbackForStats`
- 外部类型：`Relation`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-e2ebca0383` → 本草稿移入 `cases/defect/auto-postgres-e2ebca0383/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
