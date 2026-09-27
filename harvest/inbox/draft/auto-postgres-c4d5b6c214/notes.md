# auto-postgres-c4d5b6c214

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
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #142fd8ff13dee8ff6665615027386908e9768d43 (https://github.com/postgres/postgres/commit/142fd8ff13dee8ff6665615027386908e9768d43)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 224；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 142fd8ff13dee8ff6665615027386908e9768d43 Fix import of statistics for domains over [multi]range types and tsvector :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -221,14 +221,16 @@ attribute_statistics_update_internal(Oid reloid,
 
 	Oid			atttypid = InvalidOid;
 	int32		atttypmod;
-	char		atttyptype;
+	TypeCacheEntry *basetypcache;
 	Oid			atttypcoll = InvalidOid;
 	Oid			eq_opr = InvalidOid;
 	Oid			lt_opr = InvalidOid;
 
 	Oid			elemtypid = InvalidOid;
 	Oid			elem_eq_opr = InvalidOid;
 
+	Oid			bounds_typid = InvalidOid;
+
 	FmgrInfo	array_in_fn;
 
 	bool		do_mcv = !PG_ARGISNULL(MOST_COMMON_FREQS_ARG) &&
@@ -296,14 +298,13 @@ attribute_statistics_update_internal(Oid reloid,
 	/* derive information from attribute */
 	statatt_get_type(reloid, attnum,
 					 &atttypid, &atttypmod,
-					 &atttyptype, &atttypcoll,
+					 &basetypcache, &atttypcoll,
 					 &eq_opr, &lt_opr);
 
 	/* if needed, derive element type */
 	if (do_mcelem || do_dechist)
 	{
-		if (!statatt_get_elem_type(atttypid, atttyptype,
-								   &elemtypid, &elem_eq_opr))
+		if (!statatt_get_elem_type(basetypcache, &elemtypid, &elem_eq_opr))
 		{
 			ereport(WARNING,
 					(errmsg("could not determine element type of column \"%s\"", attname),
@@ -334,7 +335,7 @@ attribute_statistics_update_internal(Oid reloid,
 
 	/* only range types can have range stats */
 	if ((do_range_length_histogram || do_bounds_histogram) &&
-		!(atttyptype == TYPTYPE_RANGE || atttyptype == TYPTYPE_MULTIRANGE))
+		!statatt_get_range_type(basetypcache, &bounds_typid))
 	{
 		ereport(WARNING,
 				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
@@ -498,14 +499,8 @@ attribute_statistics_update_internal(Oid reloid,
 	{
 		bool		converted = false;
 		Datum		stavalues;
-		Oid			bounds_typid = atttypid;
-
-		/*
-		 * If it's a multirange, step down to the range type, as is done by
-		 * multirange_typanalyze().
-		 */
-		if (type_is_multirange(atttypid))
-			bounds_typid = get_multirange_range(atttypid);
+
+		Assert(OidIsValid(bounds_typid));
 
 		stavalues = statatt_build_stavalues("range_bounds_histogram",
 											&array_in_fn,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`get_multirange_range`
- 外部函数：`multirange_typanalyze`
- 外部函数：`statatt_build_stavalues`
- 外部函数：`statatt_get_elem_type`
- 外部函数：`statatt_get_type`
- 外部函数：`type_is_multirange`
- 大写宏：`ERRCODE_INVALID_PARAMETER_VALUE`
- 大写宏：`MOST_COMMON_FREQS_ARG`
- 大写宏：`PG_ARGISNULL`
- 大写宏：`TYPTYPE_MULTIRANGE`
- 大写宏：`TYPTYPE_RANGE`
- 大写宏：`WARNING`
- 外部类型：`Datum`
- 外部类型：`FmgrInfo`
- 外部类型：`If`
- 外部类型：`InvalidOid`
- 外部类型：`Oid`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-c4d5b6c214` → 本草稿移入 `cases/defect/auto-postgres-c4d5b6c214/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
