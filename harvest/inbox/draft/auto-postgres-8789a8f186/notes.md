# auto-postgres-8789a8f186

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
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #142fd8ff13dee8ff6665615027386908e9768d43 (https://github.com/postgres/postgres/commit/142fd8ff13dee8ff6665615027386908e9768d43)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 445；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 142fd8ff13dee8ff6665615027386908e9768d43 Fix import of statistics for domains over [multi]range types and tsvector :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -435,21 +435,23 @@ stats_fill_fcinfo_from_arg_pairs(FunctionCallInfo pairs_fcinfo,
  * This duplicates the logic in examine_attribute() but it will not skip the
  * attribute if the attstattarget is 0.
  *
+ * *atttypid and *atttypmod describe the type as declared.  *basetypcache is
+ * the cache entry of the base type behind any domain.
+ *
  * This information, retrieved from pg_attribute and pg_type with some
  * specific handling for index expressions, is a prerequisite to calling
  * any of the other statatt_*() functions.
  */
 void
 statatt_get_type(Oid reloid, AttrNumber attnum,
 				 Oid *atttypid, int32 *atttypmod,
-				 char *atttyptype, Oid *atttypcoll,
+				 TypeCacheEntry **basetypcache, Oid *atttypcoll,
 				 Oid *eq_opr, Oid *lt_opr)
 {
 	Relation	rel = relation_open(reloid, AccessShareLock);
 	Form_pg_attribute attr;
 	HeapTuple	atup;
 	Node	   *expr;
-	TypeCacheEntry *typcache;
 
 	atup = SearchSysCache2(ATTNUM, ObjectIdGetDatum(reloid),
 						   Int16GetDatum(attnum));
@@ -496,35 +498,42 @@ statatt_get_type(Oid reloid, AttrNumber attnum,
 	ReleaseSysCache(atup);
 
 	/* finds the right operators even if atttypid is a domain */
-	typcache = lookup_type_cache(*atttypid, TYPECACHE_LT_OPR | TYPECACHE_EQ_OPR);
-	*atttyptype = typcache->typtype;
-	*eq_opr = typcache->eq_opr;
-	*lt_opr = typcache->lt_opr;
+	*basetypcache = lookup_type_cache(*atttypid, TYPECACHE_LT_OPR |
+									  TYPECACHE_EQ_OPR |
+									  TYPECACHE_DOMAIN_BASE_INFO);
+	if (OidIsValid((*basetypcache)->domainBaseType))
+		*basetypcache = lookup_type_cache((*basetypcache)->domainBaseType,
+										  TYPECACHE_LT_OPR |
+										  TYPECACHE_EQ_OPR);
+
+	*eq_opr = (*basetypcache)->eq_opr;
+	*lt_opr = (*basetypcache)->lt_opr;
 
 	/*
 	 * Special case: collation for tsvector is DEFAULT_COLLATION_OID. See
 	 * compute_tsvector_stats().
 	 */
-	if (*atttypid == TSVECTOROID)
+	if ((*basetypcache)->type_id == TSVECTOROID)
 		*atttypcoll = DEFAULT_COLLATION_OID;
 
 	relation_close(rel, NoLock);
 }
 
 /*
- * Derive element type information from the attribute type.  This information
- * is needed when the given type is one that contains elements of other types.
+ * Derive element type information from the base type of an attribute.  This
+ * information is needed when the given type is one that contains elements of
+ * other types.
  *
- * The atttypid and atttyptype should be derived from a previous call to
+ * The type cache entry should be derived from a previous call to
  * statatt_get_type().
  */
 bool
-statatt_get_elem_type(Oid atttypid, char atttyptype,
+statatt_get_elem_type(TypeCacheEntry *basetypcache,
 					  Oid *elemtypid, Oid *elem_eq_opr)
 {
 	TypeCacheEntry *elemtypcache;
 
-	if (atttypid == TSVECTOROID)
+	if (basetypcache->type_id == TSVECTOROID)
 	{
 		/*
 		 * Special case: element type for tsvector is text. See
@@ -534,8 +543,8 @@ statatt_get_elem_type(Oid atttypid, char atttyptype,
 	}
 	else
 	{
-		/* find underlying element type through any domain */
-		*elemtypid = get_base_element_type(atttypid);
+		/* find the underlying element type */
+		*elemtypid = get_element_type(basetypcache->type_id);
 	}
 
 	if (!OidIsValid(*elemtypid))
@@ -551,6 +560,33 @@ statatt_get_elem_type(Oid atttypid, char atttyptype,
 	return true;
 }
 
+/*
+ * Derive the range type to use from the attribute type, returning false if
+ * the attribute cannot have range statistics at all.
+ *
+ * For a multirange type, we step down to its range type, because
+ * compute_range_stats() stores range bounds even when analyzing a multirange
+ * column (see also range_typanalyze() and multirange_typanalyze()).
+ *
+ * The type cache entry should be derived from a previous call to
+ * statatt_get_type(), so that any domain has already been looked through.
+ */
+bool
+statatt_get_range_type(TypeCacheEntry *basetypcache, Oid *rangetypid)
+{
+	if (basetypcache->typtype == TYPTYPE_MULTIRANGE)
+		*rangetypid = get_multirange_range(basetypcache->type_id);
+	else if (bas
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`compute_tsvector_stats`
- 外部函数：`get_base_element_type`
- 外部函数：`lookup_type_cache`
- 外部函数：`relation_close`
- 外部函数：`relation_open`
- 外部函数：`statatt_get_elem_type`
- 大写宏：`ATTNUM`
- 大写宏：`DEFAULT_COLLATION_OID`
- 大写宏：`TSVECTOROID`
- 大写宏：`TYPECACHE_EQ_OPR`
- 大写宏：`TYPECACHE_LT_OPR`
- 外部类型：`AccessShareLock`
- 外部类型：`AttrNumber`
- 外部类型：`Build`
- 外部类型：`Derive`
- 外部类型：`Form_pg_attribute`
- 外部类型：`HeapTuple`
- 外部类型：`NoLock`
- 外部类型：`Node`
- 外部类型：`Oid`
- 外部类型：`Relation`
- 外部类型：`See`
- 外部类型：`Special`
- 外部类型：`The`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-8789a8f186` → 本草稿移入 `cases/defect/auto-postgres-8789a8f186/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
