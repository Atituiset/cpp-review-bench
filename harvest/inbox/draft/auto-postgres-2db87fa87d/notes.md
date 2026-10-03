# auto-postgres-2db87fa87d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#5a5e3b88dead9cbea2352283addd927a32dc4d4b](https://github.com/postgres/postgres/commit/5a5e3b88dead9cbea2352283addd927a32dc4d4b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 60 |
| 编译错误数（gcc syntax-only） | 11（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5a5e3b88dead9cbea2352283addd927a32dc4d4b (https://github.com/postgres/postgres/commit/5a5e3b88dead9cbea2352283addd927a32dc4d4b)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 597；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5a5e3b88dead9cbea2352283addd927a32dc4d4b Use the relation map's index when searching deleted tuples sequentially. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -536,6 +536,10 @@ update_most_recent_deletion_info(TupleTableSlot *scanslot,
  * returns the transaction ID, origin, and commit timestamp of the transaction
  * that deleted this tuple.
  *
+ * If 'identidxoid' is valid, it is the replica identity or primary key
+ * index, and only its key columns are compared. Otherwise, all columns are
+ * compared.
+ *
  * 'oldestxmin' acts as a cutoff transaction ID. Tuples deleted by transactions
  * with IDs >= 'oldestxmin' are considered recently dead and are eligible for
  * conflict detection.
@@ -563,7 +567,8 @@ update_most_recent_deletion_info(TupleTableSlot *scanslot,
  * tuple was deleted most recently.
  */
 bool
-RelationFindDeletedTupleInfoSeq(Relation rel, TupleTableSlot *searchslot,
+RelationFindDeletedTupleInfoSeq(Relation rel, Oid identidxoid,
+								TupleTableSlot *searchslot,
 								TransactionId oldestxmin,
 								TransactionId *delete_xid,
 								ReplOriginId *delete_origin,
@@ -572,7 +577,7 @@ RelationFindDeletedTupleInfoSeq(Relation rel, TupleTableSlot *searchslot,
 	TupleTableSlot *scanslot;
 	TableScanDesc scan;
 	TypeCacheEntry **eq;
-	Bitmapset  *indexbitmap;
+	Bitmapset  *indexbitmap = NULL;
 	TupleDesc	desc PG_USED_FOR_ASSERTS_ONLY = RelationGetDescr(rel);
 
 	Assert(equalTupleDescs(desc, searchslot->tts_tupleDescriptor));
@@ -582,21 +587,35 @@ RelationFindDeletedTupleInfoSeq(Relation rel, TupleTableSlot *searchslot,
 	*delete_time = 0;
 
 	/*
-	 * If the relation has a replica identity key or a primary key that is
-	 * unusable for locating deleted tuples (see
-	 * IsIndexUsableForFindingDeletedTuple), a full table scan becomes
-	 * necessary. In such cases, comparing the entire tuple is not required,
-	 * since the remote tuple might not include all column values. Instead,
-	 * the indexed columns alone are sufficient to identify the target tuple
-	 * (see logicalrep_rel_mark_updatable).
+	 * If the caller's replica identity key or primary key is unusable for
+	 * locating deleted tuples (see IsIndexUsableForFindingDeletedTuple), a
+	 * full table scan becomes necessary. In such cases, comparing the entire
+	 * tuple is not required, since the remote tuple might not include all
+	 * column values. Instead, the indexed columns alone are sufficient to
+	 * identify the target tuple (see logicalrep_rel_mark_updatable).
 	 */
-	indexbitmap = RelationGetIndexAttrBitmap(rel,
-											 INDEX_ATTR_BITMAP_IDENTITY_KEY);
+	if (OidIsValid(identidxoid))
+	{
+		/* The index must have been locked already */
+		Relation	idxrel = index_open(identidxoid, NoLock);
 
-	/* fallback to PK if no replica identity */
-	if (!indexbitmap)
-		indexbitmap = RelationGetIndexAttrBitmap(rel,
-												 INDEX_ATTR_BITMAP_PRIMARY_KEY);
+		/*
+		 * The index may no longer be the replica identity if DROP INDEX
+		 * CONCURRENTLY or REINDEX CONCURRENTLY ran meanwhile, but it stays
+		 * unique and non-partial, which is all we rely on. See
+		 * FindReplTupleInLocalRel().
+		 */
+		Assert(idxrel->rd_index->indisunique);
+		Assert(heap_attisnull(idxrel->rd_indextuple, Anum_pg_index_indpred,
+							  NULL));
+
+		for (int i = 0; i < idxrel->rd_index->indnkeyatts; i++)
+			indexbitmap = bms_add_member(indexbitmap,
+										 idxrel->rd_index->indkey.values[i] -
+										 FirstLowInvalidHeapAttributeNumber);
+
+		index_close(idxrel, NoLock);
+	}
 
 	eq = palloc0_array(TypeCacheEntry *, searchslot->tts_tupleDescriptor->natts);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`aborted`
- 外部函数：`bms_is_member`
- 外部函数：`detection`
- 外部函数：`equalTupleDescs`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`format_type_be`
- 外部函数：`lookup_type_cache`
- 外部函数：`palloc0_array`
- 外部函数：`slot_getallattrs`
- 外部函数：`table_beginscan`
- 外部函数：`table_endscan`
- 外部函数：`table_rescan`
- 外部函数：`table_scan_getnextslot`
- 外部函数：`table_slot_create`
- 外部函数：`tuples`
- 大写宏：`BUFFER_LOCK_SHARE`
- 大写宏：`BUFFER_LOCK_UNLOCK`
- 大写宏：`ERRCODE_UNDEFINED_FUNCTION`
- 大写宏：`ERROR`
- 大写宏：`HEAPTUPLE_DEAD`
- 大写宏：`HEAPTUPLE_RECENTLY_DEAD`
- 大写宏：`INDEX_ATTR_BITMAP_IDENTITY_KEY`
- 大写宏：`INDEX_ATTR_BITMAP_PRIMARY_KEY`
- 大写宏：`MVCC`
- 大写宏：`NULL`
- 大写宏：`PG_USED_FOR_ASSERTS_ONLY`
- 大写宏：`SO_NONE`
- 大写宏：`TYPECACHE_EQ_OPR_FINFO`
- 外部类型：`Bitmapset`
- 外部类型：`Buffer`
- 外部类型：`BufferHeapTupleTableSlot`
- 外部类型：`Check`
- 外部类型：`FirstLowInvalidHeapAttributeNumber`
- 外部类型：`Form_pg_attribute`
- 外部类型：`ForwardScanDirection`
- 外部类型：`HeapTuple`
- 外部类型：`IDs`
- 外部类型：`If`
- 外部类型：`Ignore`
- 外部类型：`In`
- 外部类型：`Instead`
- 外部类型：`InvalidReplOriginId`
- 外部类型：`InvalidTransactionId`
- 外部类型：`IsIndexUsableForFindingDeletedTuple`
- 外部类型：`Relation`
- 外部类型：`ReplOriginId`
- 外部类型：`See`
- 外部类型：`Select`
- 外部类型：`SnapshotAny`
- 外部类型：`Start`
- 外部类型：`TableScanDesc`
- 外部类型：`TimestampTz`
- 外部类型：`TransactionId`
- 外部类型：`Try`
- 外部类型：`TupleDesc`
- 外部类型：`TupleTableSlot`
- 外部类型：`Tuples`
- 外部类型：`TypeCacheEntry`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-2db87fa87d` → 本草稿移入 `cases/defect/auto-postgres-2db87fa87d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
