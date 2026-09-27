# auto-postgres-2f0e64e54f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#142fd8ff13dee8ff6665615027386908e9768d43](https://github.com/postgres/postgres/commit/142fd8ff13dee8ff6665615027386908e9768d43) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 39 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #142fd8ff13dee8ff6665615027386908e9768d43 (https://github.com/postgres/postgres/commit/142fd8ff13dee8ff6665615027386908e9768d43)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1243；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 142fd8ff13dee8ff6665615027386908e9768d43 Fix import of statistics for domains over [multi]range types and tsvector :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1115,14 +1115,15 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 					bool *pg_statistic_ok)
 {
 	const char *argname = extarginfo[EXPRESSIONS_ARG].argname;
-	TypeCacheEntry *typcache;
+	TypeCacheEntry *basetypcache;
 	Datum		values[Natts_pg_statistic];
 	bool		nulls[Natts_pg_statistic];
 	bool		replaces[Natts_pg_statistic];
 	HeapTuple	pgstup = NULL;
 	Datum		pgstdat = (Datum) 0;
 	Oid			elemtypid = InvalidOid;
 	Oid			elemeqopr = InvalidOid;
+	Oid			rtypid = InvalidOid;
 	bool		found[NUM_ATTRIBUTE_STATS_ELEMS] = {0};
 	JsonbValue	val[NUM_ATTRIBUTE_STATS_ELEMS] = {0};
 
@@ -1221,7 +1222,13 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 	}
 
 	/* This finds the right operators even if atttypid is a domain */
-	typcache = lookup_type_cache(typid, TYPECACHE_LT_OPR | TYPECACHE_EQ_OPR);
+	basetypcache = lookup_type_cache(typid, TYPECACHE_LT_OPR |
+									 TYPECACHE_EQ_OPR |
+									 TYPECACHE_DOMAIN_BASE_INFO);
+	if (OidIsValid(basetypcache->domainBaseType))
+		basetypcache = lookup_type_cache(basetypcache->domainBaseType,
+										 TYPECACHE_LT_OPR |
+										 TYPECACHE_EQ_OPR);
 
 	statatt_init_empty_tuple(InvalidOid, InvalidAttrNumber, false,
 							 values, nulls, replaces);
@@ -1230,7 +1237,7 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 	 * Special case: collation for tsvector is DEFAULT_COLLATION_OID. See
 	 * compute_tsvector_stats().
 	 */
-	if (typid == TSVECTOROID)
+	if (basetypcache->type_id == TSVECTOROID)
 		typcoll = DEFAULT_COLLATION_OID;
 
 	/*
@@ -1240,8 +1247,7 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 	 */
 	if (found[MOST_COMMON_ELEMS_ELEM] || found[ELEM_COUNT_HISTOGRAM_ELEM])
 	{
-		if (!statatt_get_elem_type(typid, typcache->typtype,
-								   &elemtypid, &elemeqopr))
+		if (!statatt_get_elem_type(basetypcache, &elemtypid, &elemeqopr))
 		{
 			ereport(WARNING,
 					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
@@ -1259,8 +1265,7 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 		found[RANGE_EMPTY_FRAC_ELEM] ||
 		found[RANGE_BOUNDS_HISTOGRAM_ELEM])
 	{
-		if (typcache->typtype != TYPTYPE_RANGE &&
-			typcache->typtype != TYPTYPE_MULTIRANGE)
+		if (!statatt_get_range_type(basetypcache, &rtypid))
 		{
 			ereport(WARNING,
 					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
@@ -1364,7 +1369,7 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 
 			statatt_set_slot(values, nulls, replaces,
 							 STATISTIC_KIND_MCV,
-							 typcache->eq_opr, typcoll,
+							 basetypcache->eq_opr, typcoll,
 							 stanumbers, false, stavalues, false);
 		}
 		else
@@ -1386,7 +1391,7 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 		if (val_ok)
 			statatt_set_slot(values, nulls, replaces,
 							 STATISTIC_KIND_HISTOGRAM,
-							 typcache->lt_opr, typcoll,
+							 basetypcache->lt_opr, typcoll,
 							 0, true, stavalues, false);
 		else
 			goto pg_statistic_error;
@@ -1405,7 +1410,7 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 
 			statatt_set_slot(values, nulls, replaces,
 							 STATISTIC_KIND_CORRELATION,
-							 typcache->lt_opr, typcoll,
+							 basetypcache->lt_opr, typcoll,
 							 stanumbers, false, 0, true);
 		}
 		else
@@ -1476,14 +1481,8 @@ import_pg_statistic(Relation pgsd, JsonbContainer *cont,
 		Datum		stavalues;
 		bool		val_ok = false;
 		char	   *s;
-		Oid			rtypid = typid;
 
-		/*
-		 * If it's a multirange, step down to the range type, as is done by
-		 * multirange_typanalyze().
-		 */
-		if (type_is_multirange(typid))
-			rtypid = get_multirange_range(typid);
+		Assert(OidIsValid(rtypid));
 
 		s = jbv_string_get_cstr(&val[RANGE_BOUNDS_HISTOGRAM_ELEM]);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`compute_tsvector_stats`
- 外部函数：`errcode`
- 外部函数：`get_multirange_range`
- 外部函数：`lookup_type_cache`
- 外部函数：`memcpy`
- 外部函数：`multirange_typanalyze`
- 外部函数：`palloc0`
- 外部函数：`statatt_get_elem_type`
- 外部函数：`statatt_init_empty_tuple`
- 外部函数：`statatt_set_slot`
- 外部函数：`type_is_multirange`
- 大写宏：`DEFAULT_COLLATION_OID`
- 大写宏：`ELEM_COUNT_HISTOGRAM_ELEM`
- 大写宏：`ERRCODE_INVALID_PARAMETER_VALUE`
- 大写宏：`EXPRESSIONS_ARG`
- 大写宏：`MOST_COMMON_ELEMS_ELEM`
- 大写宏：`NULL`
- 大写宏：`NUM_ATTRIBUTE_STATS_ELEMS`
- 大写宏：`RANGE_BOUNDS_HISTOGRAM_ELEM`
- 大写宏：`RANGE_EMPTY_FRAC_ELEM`
- 大写宏：`STATISTIC_KIND_CORRELATION`
- 大写宏：`STATISTIC_KIND_HISTOGRAM`
- 大写宏：`STATISTIC_KIND_MCV`
- 大写宏：`TSVECTOROID`
- 大写宏：`TYPECACHE_EQ_OPR`
- 大写宏：`TYPECACHE_LT_OPR`
- 大写宏：`TYPTYPE_MULTIRANGE`
- 大写宏：`TYPTYPE_RANGE`
- 大写宏：`WARNING`
- 外部类型：`Datum`
- 外部类型：`HeapTuple`
- 外部类型：`If`
- 外部类型：`InvalidAttrNumber`
- 外部类型：`InvalidOid`
- 外部类型：`JsonbValue`
- 外部类型：`Natts_pg_statistic`
- 外部类型：`Oid`
- 外部类型：`See`
- 外部类型：`Special`
- 外部类型：`This`
- 外部类型：`TypeCacheEntry`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-2f0e64e54f` → 本草稿移入 `cases/defect/auto-postgres-2f0e64e54f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
