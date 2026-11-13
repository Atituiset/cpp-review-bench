# auto-postgres-6e4aa63bce

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#87524d0dd8470819ce8aa3708a9bf3fa3f1d34fd](https://github.com/postgres/postgres/commit/87524d0dd8470819ce8aa3708a9bf3fa3f1d34fd) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 54 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #87524d0dd8470819ce8aa3708a9bf3fa3f1d34fd (https://github.com/postgres/postgres/commit/87524d0dd8470819ce8aa3708a9bf3fa3f1d34fd)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 169；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 87524d0dd8470819ce8aa3708a9bf3fa3f1d34fd Allow empty arrays of values in pg_restore_extended_stats() :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -166,7 +166,7 @@ static bool key_in_expr_argnames(JsonbValue *key);
 static bool check_all_expr_argnames_valid(JsonbContainer *cont, AttrNumber exprnum);
 static Datum array_in_safe(FmgrInfo *array_in, const char *s, Oid typid,
 						   int32 typmod, AttrNumber exprnum,
-						   const char *element_name, bool *ok);
+						   const char *element_name, bool allow_empty, bool *ok);
 static Datum import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 								 AttrNumber exprnum, FmgrInfo *array_in_fn,
 								 Oid typid, int32 typmod, Oid typcoll,
@@ -1059,13 +1059,18 @@ jbv_to_infunc_datum(JsonbValue *jval, PGFunction func, AttrNumber exprnum,
  *
  * If an error is encountered, capture it, and reduce the elevel to WARNING.
  *
+ * The array must be one-dimensional.  If "allow_empty" is true, an empty
+ * array is also accepted.
+ *
  * This is an adaptation of statatt_build_stavalues().
  */
 static Datum
 array_in_safe(FmgrInfo *array_in, const char *s, Oid typid, int32 typmod,
-			  AttrNumber exprnum, const char *element_name, bool *ok)
+			  AttrNumber exprnum, const char *element_name, bool allow_empty,
+			  bool *ok)
 {
 	Datum		result;
+	int			ndim;
 
 	ErrorSaveContext escontext = {
 		.type = T_ErrorSaveContext,
@@ -1096,12 +1101,19 @@ array_in_safe(FmgrInfo *array_in, const char *s, Oid typid, int32 typmod,
 		return (Datum) 0;
 	}
 
-	if (ARR_NDIM(DatumGetArrayTypeP(result)) != 1)
+	ndim = ARR_NDIM(DatumGetArrayTypeP(result));
+	if (ndim != 1 && !(allow_empty && ndim == 0))
 	{
-		ereport(WARNING,
-				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
-				 errmsg("could not import element \"%s\" in expression %d: must be a one-dimensional array",
-						element_name, exprnum)));
+		if (allow_empty)
+			ereport(WARNING,
+					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
+					 errmsg("could not import element \"%s\" in expression %d: must be an empty or one-dimensional array",
+							element_name, exprnum)));
+		else
+			ereport(WARNING,
+					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
+					 errmsg("could not import element \"%s\" in expression %d: must be a one-dimensional array",
+							element_name, exprnum)));
 		return (Datum) 0;
 	}
 
@@ -1354,14 +1366,14 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 		s = jbv_string_get_cstr(&val[MOST_COMMON_VALS_ELEM]);
 		stavalues = array_in_safe(array_in_fn, s, typid, typmod, exprnum,
 								  extexprargname[MOST_COMMON_VALS_ELEM],
-								  &val_ok);
+								  false, &val_ok);
 
 		pfree(s);
 
 		s = jbv_string_get_cstr(&val[MOST_COMMON_FREQS_ELEM]);
 		stanumbers = array_in_safe(array_in_fn, s, FLOAT4OID, -1, exprnum,
 								   extexprargname[MOST_COMMON_FREQS_ELEM],
-								   &num_ok);
+								   false, &num_ok);
 		pfree(s);
 
 		/* Only set the slot if both datums have been built */
@@ -1400,7 +1412,7 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 
 		stavalues = array_in_safe(array_in_fn, s, typid, typmod, exprnum,
 								  extexprargname[HISTOGRAM_BOUNDS_ELEM],
-								  &val_ok);
+								  false, &val_ok);
 		pfree(s);
 
 		if (val_ok)
@@ -1444,14 +1456,14 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 		s = jbv_string_get_cstr(&val[MOST_COMMON_ELEMS_ELEM]);
 		stavalues = array_in_safe(array_in_fn, s, elemtypid, typmod, exprnum,
 								  extexprargname[MOST_COMMON_ELEMS_ELEM],
-								  &val_ok);
+								  true, &val_ok);
 		pfree(s);
 
 
 		s = jbv_string_get_cstr(&val[MOST_COMMON_ELEM_FREQS_ELEM]);
 		stanumbers = array_in_safe(array_in_fn, s, FLOAT4OID, -1, exprnum,
 								   extexprargname[MOST_COMMON_ELEM_FREQS_ELEM],
-								   &num_ok);
+								   false, &num_ok);
 		pfree(s);
 
 		/* Only set the slot if both datums have been built */
@@ -1474,7 +1486,7 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 		s = jbv_string_get_cstr(&val[ELEM_COUNT_HISTOGRAM_ELEM]);
 		stanumbers = array_in_safe(array_in_fn, s, FLOAT4OID, -1, exprnum,
 								   extexprargname[ELEM_COUNT_HISTOGRAM_EL
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`appendStringInfo`
- 外部函数：`array_contains_nulls`
- 外部函数：`array_in_safe`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`initStringInfo`
- 外部函数：`key_in_expr_argnames`
- 外部函数：`memcpy`
- 外部函数：`palloc0`
- 外部函数：`pfree`
- 外部函数：`statatt_check_bounds_histogram`
- 外部函数：`statatt_set_slot`
- 外部函数：`strlen`
- 外部函数：`strncmp`
- 大写宏：`ARR_NDIM`
- 大写宏：`BEGIN`
- 大写宏：`ELEM_COUNT_HISTOGRAM_ELEM`
- 大写宏：`ERRCODE_INVALID_PARAMETER_VALUE`
- 大写宏：`ERROR`
- 大写宏：`FLOAT4OID`
- 大写宏：`HISTOGRAM_BOUNDS_ELEM`
- 大写宏：`MOST_COMMON_ELEMS_ELEM`
- 大写宏：`MOST_COMMON_ELEM_FREQS_ELEM`
- 大写宏：`MOST_COMMON_FREQS_ELEM`
- 大写宏：`MOST_COMMON_VALS_ELEM`
- 大写宏：`NUM_ATTRIBUTE_STATS_ELEMS`
- 大写宏：`OBJECT`
- 大写宏：`RANGE_BOUNDS_HISTOGRAM_ELEM`
- 大写宏：`WARNING`
- 大写宏：`WJB_BEGIN_OBJECT`
- 大写宏：`WJB_END_OBJECT`
- 大写宏：`WJB_KEY`
- 大写宏：`WJB_VALUE`
- 外部类型：`AttrNumber`
- 外部类型：`Datum`
- 外部类型：`Element`
- 外部类型：`ErrorSaveContext`
- 外部类型：`FmgrInfo`
- 外部类型：`If`
- 外部类型：`JsonbContainer`
- 外部类型：`JsonbIterator`
- 外部类型：`JsonbIteratorToken`
- 外部类型：`JsonbValue`
- 外部类型：`Node`
- 外部类型：`Oid`
- 外部类型：`Only`
- 外部类型：`Overwriting`
- 外部类型：`Relation`
- 外部类型：`Skip`
- 外部类型：`StringInfoData`
- 外部类型：`T_ErrorSaveContext`
- 外部类型：`This`
- 外部类型：`WARNINGs`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-6e4aa63bce` → 本草稿移入 `cases/defect/auto-postgres-6e4aa63bce/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
