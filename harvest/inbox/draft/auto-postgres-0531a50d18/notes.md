# auto-postgres-0531a50d18

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#ad36e3608c8cb6f0848737ec81e548d4d3a0af3c](https://github.com/postgres/postgres/commit/ad36e3608c8cb6f0848737ec81e548d4d3a0af3c) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 101 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ad36e3608c8cb6f0848737ec81e548d4d3a0af3c (https://github.com/postgres/postgres/commit/ad36e3608c8cb6f0848737ec81e548d4d3a0af3c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 223；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ad36e3608c8cb6f0848737ec81e548d4d3a0af3c Fix tuple search during apply after concurrent index DDL. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -177,9 +177,14 @@ should_refetch_tuple(TM_Result res, TM_FailureData *tmfd)
  *
  * If a matching tuple is found, lock it with lockmode, fill the slot with its
  * contents, and return true.  Return false otherwise.
+ *
+ * 'skipduplicates' specifies whether the first matching tuple can be used
+ * without comparing it against 'searchslot'. If false, all matching tuples are
+ * compared against 'searchslot', which must contain a complete row.
  */
 bool
 RelationFindReplTupleByIndex(Relation rel, Oid idxoid,
+							 bool skipduplicates,
 							 LockTupleMode lockmode,
 							 TupleTableSlot *searchslot,
 							 TupleTableSlot *outslot)
@@ -192,13 +197,10 @@ RelationFindReplTupleByIndex(Relation rel, Oid idxoid,
 	Relation	idxrel;
 	bool		found;
 	TypeCacheEntry **eq = NULL;
-	bool		isIdxSafeToSkipDuplicates;
 
 	/* Open the index. */
 	idxrel = index_open(idxoid, RowExclusiveLock);
 
-	isIdxSafeToSkipDuplicates = (GetRelationIdentityOrPK(rel) == idxoid);
-
 	InitDirtySnapshot(snap);
 
 	/* Build scan key. */
@@ -220,7 +222,7 @@ RelationFindReplTupleByIndex(Relation rel, Oid idxoid,
 		 * Avoid expensive equality check if the index is primary key or
 		 * replica identity index.
 		 */
-		if (!isIdxSafeToSkipDuplicates)
+		if (!skipduplicates)
 		{
 			if (eq == NULL)
 				eq = palloc0_array(TypeCacheEntry *, outslot->tts_tupleDescriptor->natts);
@@ -629,9 +631,12 @@ RelationFindDeletedTupleInfoSeq(Relation rel, TupleTableSlot *searchslot,
 /*
  * Similar to RelationFindDeletedTupleInfoSeq() but using index scan to locate
  * the deleted tuple.
+ *
+ * 'skipduplicates' works as in RelationFindReplTupleByIndex().
  */
 bool
 RelationFindDeletedTupleInfoByIndex(Relation rel, Oid idxoid,
+									bool skipduplicates,
 									TupleTableSlot *searchslot,
 									TransactionId oldestxmin,
 									TransactionId *delete_xid,
@@ -644,7 +649,6 @@ RelationFindDeletedTupleInfoByIndex(Relation rel, Oid idxoid,
 	IndexScanDesc scan;
 	TupleTableSlot *scanslot;
 	TypeCacheEntry **eq = NULL;
-	bool		isIdxSafeToSkipDuplicates;
 	TupleDesc	desc PG_USED_FOR_ASSERTS_ONLY = RelationGetDescr(rel);
 
 	Assert(equalTupleDescs(desc, searchslot->tts_tupleDescriptor));
@@ -654,8 +658,6 @@ RelationFindDeletedTupleInfoByIndex(Relation rel, Oid idxoid,
 	*delete_time = 0;
 	*delete_origin = InvalidReplOriginId;
 
-	isIdxSafeToSkipDuplicates = (GetRelationIdentityOrPK(rel) == idxoid);
-
 	scanslot = table_slot_create(rel, NULL);
 
 	idxrel = index_open(idxoid, RowExclusiveLock);
@@ -681,7 +683,7 @@ RelationFindDeletedTupleInfoByIndex(Relation rel, Oid idxoid,
 		 * Avoid expensive equality check if the index is primary key or
 		 * replica identity index.
 		 */
-		if (!isIdxSafeToSkipDuplicates)
+		if (!skipduplicates)
 		{
 			if (eq == NULL)
 				eq = palloc0_array(TypeCacheEntry *, scanslot->tts_tupleDescriptor->natts);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`d`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`get_opclass_family`
- 外部函数：`get_opclass_input_type`
- 外部函数：`get_opcode`
- 外部函数：`get_opfamily_member`
- 外部函数：`index_beginscan`
- 外部函数：`index_close`
- 外部函数：`index_endscan`
- 外部函数：`index_open`
- 外部函数：`index_rescan`
- 外部函数：`palloc0_array`
- 外部函数：`slot_getallattrs`
- 外部函数：`table_index_getnext_slot`
- 外部函数：`table_tuple_lock`
- 大写宏：`COMPARE_EQ`
- 大写宏：`ERRCODE_T_R_SERIALIZATION_FAILURE`
- 大写宏：`ERROR`
- 大写宏：`INDEXRELID`
- 大写宏：`INDEX_MAX_KEYS`
- 大写宏：`LOG`
- 大写宏：`NULL`
- 大写宏：`SK_ISNULL`
- 大写宏：`SK_SEARCHNULL`
- 大写宏：`SO_NONE`
- 大写宏：`XXX`
- 外部类型：`Anum_pg_index_indclass`
- 外部类型：`Avoid`
- 外部类型：`Bitmapset`
- 外部类型：`Build`
- 外部类型：`Check`
- 外部类型：`Currently`
- 外部类型：`Datum`
- 外部类型：`Don`
- 外部类型：`Form_pg_attribute`
- 外部类型：`ForwardScanDirection`
- 外部类型：`Found`
- 外部类型：`If`
- 外部类型：`Improve`
- 外部类型：`IndexScanDesc`
- 外部类型：`Initialize`
- 外部类型：`Load`
- 外部类型：`LockTupleMode`
- 外部类型：`LockWaitBlock`
- 外部类型：`NoLock`
- 外部类型：`Oid`
- 外部类型：`Open`
- 外部类型：`RegProcedure`
- 外部类型：`Relation`
- 外部类型：`Return`
- 外部类型：`RowExclusiveLock`
- 外部类型：`ScanKey`
- 外部类型：`ScanKeyData`
- 外部类型：`SnapshotData`
- 外部类型：`Start`
- 外部类型：`StrategyNumber`
- 外部类型：`TM_Deleted`
- 外部类型：`TM_FailureData`
- 外部类型：`TM_Invisible`
- 外部类型：`TM_Ok`
- 外部类型：`TM_Result`
- 外部类型：`TM_Updated`
- 外部类型：`There`
- 外部类型：`TransactionId`
- 外部类型：`Try`
- 外部类型：`TupleTableSlot`
- 外部类型：`TypeCacheEntry`
- 外部类型：`We`
- 外部类型：`XLTW_None`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-0531a50d18` → 本草稿移入 `cases/defect/auto-postgres-0531a50d18/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
