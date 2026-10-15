# auto-postgres-e3e4c31248

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#964d88fac464a9b5f55c3d0c9c2ec72bcde5847d](https://github.com/postgres/postgres/commit/964d88fac464a9b5f55c3d0c9c2ec72bcde5847d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 180 |
| 编译错误数（gcc syntax-only） | 79（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #964d88fac464a9b5f55c3d0c9c2ec72bcde5847d (https://github.com/postgres/postgres/commit/964d88fac464a9b5f55c3d0c9c2ec72bcde5847d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 164；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 964d88fac464a9b5f55c3d0c9c2ec72bcde5847d Remove RI fast-path code for calling cast functions :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -161,30 +161,14 @@ typedef struct RI_ConstraintInfo
 	FastPathMeta *fpmeta;
 } RI_ConstraintInfo;
 
-typedef struct RI_CompareHashEntry RI_CompareHashEntry;
-
 /* Fast-path metadata for RI checks on foreign key referencing tables */
 typedef struct FastPathMeta
 {
-	FmgrInfo	eq_opr_finfo[RI_MAX_NUMKEYS];
-	FmgrInfo	cast_func_finfo[RI_MAX_NUMKEYS];
 	RegProcedure regops[RI_MAX_NUMKEYS];
 	Oid			subtypes[RI_MAX_NUMKEYS];
 	int			strats[RI_MAX_NUMKEYS];
 	AttrNumber	index_attnos[RI_MAX_NUMKEYS];	/* index column positions */
 
-	/*
-	 * fn_mcxt for the cached FmgrInfos above.  Cast and equality functions
-	 * (e.g. record_eq()) use fn_mcxt as scratch space, caching state there
-	 * and keeping a pointer to it in FmgrInfo.fn_extra.  Give them a context
-	 * of their own, created with this struct and destroyed with it in
-	 * AtEOXact_RI().
-	 *
-	 * Note this context must not be reset while the FmgrInfos remain in use,
-	 * since that would free the state fn_extra still points at.
-	 */
-	MemoryContext scratch_cxt;
-
 	/* Link in ri_fpmeta_dead_list while awaiting deferred release */
 	struct FastPathMeta *next_dead;
 } FastPathMeta;
@@ -313,11 +297,11 @@ static bool recheck_matched_pk_tuple(Relation idxrel, ScanKeyData *skeys,
 static void ri_CheckFunctionPermissions(const RI_ConstraintInfo *riinfo,
 										const FastPathMeta *fpmeta);
 static void build_index_scankeys(const RI_ConstraintInfo *riinfo,
-								 FastPathMeta *fpmeta,
-								 Relation idx_rel, Datum *pk_vals,
-								 char *pk_nulls, ScanKey skeys);
+								 const FastPathMeta *fpmeta,
+								 Relation idx_rel, const Datum *pk_vals,
+								 ScanKey skeys);
 static void ri_populate_fastpath_metadata(RI_ConstraintInfo *riinfo,
-										  Relation fk_rel, Relation idx_rel);
+										  Relation idx_rel);
 static void ri_ExtractValues(Relation rel, TupleTableSlot *slot,
 							 const RI_ConstraintInfo *riinfo, bool rel_is_pk,
 							 Datum *vals, char *nulls);
@@ -2562,13 +2546,12 @@ InvalidateConstraintCacheCallBack(Datum arg, SysCacheIdentifier cacheid,
 			/*
 			 * Detach any fast-path metadata so that the next check
 			 * repopulates it, but do not free it here.  ri_FastPathCheck()
-			 * copies riinfo->fpmeta into a local (and takes FmgrInfo pointers
-			 * into it) and then runs index scans, tuple locking, and
-			 * user-supplied cast and equality functions, all of which can
-			 * accept invalidation messages and reach this callback.  Freeing
-			 * now would leave those callers reading freed memory.  Queue it
-			 * instead; AtEOXact_RI() releases it once no RI check can be
-			 * running.
+			 * copies riinfo->fpmeta into a local and then does catalog
+			 * lookups, index scans, tuple locking, and calls equality
+			 * functions, all of which can accept invalidation messages and
+			 * reach this callback.  Freeing now would leave it reading freed
+			 * memory.  Queue it instead; AtEOXact_RI() releases it once no RI
+			 * check can be running.
 			 */
 			if (riinfo->fpmeta)
 			{
@@ -2841,10 +2824,10 @@ ri_FastPathCheck(RI_ConstraintInfo *riinfo,
 	 * InvalidSnapshot, so SPI takes the snapshot after the
 	 * referenced-relation lock has been acquired.
 	 *
-	 * Make this snapshot active too, as SPI does.  STABLE cast and equality
-	 * functions use the active snapshot, so leaving the outer query's
-	 * snapshot active could hide changes made by earlier triggers even though
-	 * the index scan can see them.
+	 * Make this snapshot active too, as SPI does.  STABLE equality functions
+	 * use the active snapshot, so leaving the outer query's snapshot active
+	 * could hide changes made by earlier triggers even though the index scan
+	 * can see them.
 	 */
 	snapshot = RegisterSnapshot(GetTransactionSnapshot());
 	PushActiveSnapshot(snapshot);
@@ -2873,7 +2856,7 @@ ri_FastPathCheck(RI_ConstraintInfo *riinfo,
 	{
 		/* Reload to ensure it's valid. */
 		riinfo = ri_LoadConstraintInfo(riinfo->constraint_id);
-		ri_populate_fastpath_metadata
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`anyrange`
- 外部函数：`attnumTypeId`
- 外部函数：`build_index_scankeys`
- 外部函数：`elog`
- 外部函数：`entry`
- 外部函数：`get_ri_constraint_root`
- 外部函数：`hash_search`
- 外部函数：`memcpy`
- 外部函数：`operators`
- 外部函数：`range_agg`
- 外部函数：`record_eq`
- 外部函数：`ri_CheckFunctionPermissions`
- 外部函数：`ri_ExtractValues`
- 外部函数：`ri_InitHashTables`
- 外部函数：`ri_populate_fastpath_metadata`
- 大写宏：`CONSTRAINT_FOREIGN`
- 大写宏：`CONSTROID`
- 大写宏：`DELETE`
- 大写宏：`ERROR`
- 大写宏：`GETSTRUCT`
- 大写宏：`HASH_ENTER`
- 大写宏：`INDEX_MAX_KEYS`
- 大写宏：`OID`
- 大写宏：`PERIOD`
- 大写宏：`RI_FASTPATH_UNKNOWN`
- 大写宏：`RI_FASTPATH_UNUSABLE`
- 大写宏：`RI_FASTPATH_USABLE`
- 大写宏：`RI_PLAN_XXX`
- 大写宏：`SET`
- 大写宏：`UPDATE`
- 外部类型：`And`
- 外部类型：`AttrNumber`
- 外部类型：`Cast`
- 外部类型：`Datum`
- 外部类型：`Fast`
- 外部类型：`FastPathMeta`
- 外部类型：`Fetch`
- 外部类型：`Find`
- 外部类型：`FmgrInfo`
- 外部类型：`FmgrInfos`
- 外部类型：`Form_pg_constraint`
- 外部类型：`Give`
- 外部类型：`HeapTuple`
- 外部类型：`If`
- 外部类型：`Link`
- 外部类型：`MemoryContext`
- 外部类型：`NameData`
- 外部类型：`Note`
- 外部类型：`Oid`
- 外部类型：`On`
- 外部类型：`RI_CompareHashEntry`
- 外部类型：`RI_CompareKey`
- 外部类型：`RI_ConstraintInfo`
- 外部类型：`RI_QueryHashEntry`
- 外部类型：`RI_QueryKey`
- 外部类型：`RegProcedure`
- 外部类型：`Relation`
- 外部类型：`SPIPlanPtr`
- 外部类型：`ScanKey`
- 外部类型：`TupleTableSlot`
- 外部类型：`and`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-e3e4c31248` → 本草稿移入 `cases/defect/auto-postgres-e3e4c31248/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
