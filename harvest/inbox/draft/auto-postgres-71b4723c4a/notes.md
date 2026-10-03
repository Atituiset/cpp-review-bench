# auto-postgres-71b4723c4a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#425daf545d9146e008220ae0b21415982220cd3f](https://github.com/postgres/postgres/commit/425daf545d9146e008220ae0b21415982220cd3f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 352 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #425daf545d9146e008220ae0b21415982220cd3f (https://github.com/postgres/postgres/commit/425daf545d9146e008220ae0b21415982220cd3f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 3023；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 425daf545d9146e008220ae0b21415982220cd3f Remove batching from RI fast-path checks :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -23,7 +23,6 @@
 
 #include "postgres.h"
 
-#include "access/amapi.h"
 #include "access/genam.h"
 #include "access/htup_details.h"
 #include "access/skey.h"
@@ -230,84 +229,6 @@ typedef struct RI_CompareHashEntry
 	FmgrInfo	cast_func_finfo;	/* in case we must coerce input */
 } RI_CompareHashEntry;
 
-/*
- * Maximum number of FK rows buffered before flushing.
- *
- * Larger batches amortize per-flush overhead and let the SK_SEARCHARRAY
- * path walk more leaf pages in a single sorted traversal.  But each
- * buffered row is a materialized HeapTuple in flush_cxt, and the matched[]
- * scan in ri_FastPathFlushArray() is O(batch_size) per index match.
- * Benchmarking showed little difference between 16 and 64, with 256
- * consistently slower.  64 is a reasonable default.
- */
-#define RI_FASTPATH_BATCH_SIZE	64
-
-/*
- * RI_FastPathKey
- *		Hash key for an RI_FastPathEntry.
- *
- * A constraint can be checked in nested trigger-firing cycles.  Each cycle
- * must have a separate entry so that its rows are checked with that cycle's
- * snapshot and its resources are released by that cycle's callback.
- */
-typedef struct RI_FastPathKey
-{
-	Oid			conoid;			/* pg_constraint OID */
-	int			query_depth;	/* after-trigger query depth */
-} RI_FastPathKey;
-
-/*
- * RI_FastPathEntry
- *		Per-constraint, per-firing-cycle cache of resources needed by
- *		ri_FastPathBatchFlush().
- *
- * Created lazily by ri_FastPathGetEntry() on first use within a
- * trigger-firing batch and torn down by ri_FastPathTeardown() at batch end.
- *
- * FK tuples are buffered in batch[] across trigger invocations and
- * flushed when the buffer fills or the batch ends.
- *
- * RI_FastPathEntry is not subject to cache invalidation.  The cached
- * relations are held open with locks for the transaction duration, preventing
- * relcache invalidation.  The entry itself is torn down at batch end by
- * ri_FastPathEndBatch(); on abort, ResourceOwner releases the cached
- * relations and AtEOXact_RI() NULLs the static cache pointer to prevent
- * any subsequent access.
- */
-typedef struct RI_FastPathEntry
-{
-	RI_FastPathKey key;			/* hash key */
-	Oid			fk_relid;		/* for ri_FastPathEndBatch() */
-	Relation	pk_rel;
-	Relation	idx_rel;
-	TupleTableSlot *pk_slot;
-	TupleTableSlot *fk_slot;
-	MemoryContext flush_cxt;	/* short-lived context for per-flush work */
-
-	/*
-	 * TODO: batch[] is HeapTuple[] because the AFTER trigger machinery
-	 * currently passes tuples as HeapTuples.  Once trigger infrastructure is
-	 * slotified, this should use a slot array or whatever batched tuple
-	 * storage abstraction exists at that point to be TAM-agnostic.
-	 */
-	HeapTuple	batch[RI_FASTPATH_BATCH_SIZE];
-	int			batch_count;
-
-	/*
-	 * true while this entry's batch is being flushed; guards against
-	 * re-entrant ri_FastPathBatchAdd from user code run during the flush.
-	 */
-	bool		flushing;
-
-	/*
-	 * Subtransaction whose resource owner opened this entry's relations.
-	 * AtEOSubXact_RI() drops only entries matching an aborting subxact, so a
-	 * subxact abort during outer-level trigger firing leaves the outer batch
-	 * intact.
-	 */
-	SubTransactionId subid;
-} RI_FastPathEntry;
-
 /*
  * Local data
  */
@@ -316,9 +237,6 @@ static HTAB *ri_query_cache = NULL;
 static HTAB *ri_compare_cache = NULL;
 static dclist_head ri_constraint_cache_valid_list;
 
-static HTAB *ri_fastpath_cache = NULL;
-static bool ri_fastpath_flushing = false;
-
 /*
  * FastPathMeta objects detached from their cache entry by invalidation, but
  * possibly still referenced by an RI check further up the stack.  Released
@@ -376,18 +294,6 @@ static bool ri_PerformCheck(const RI_ConstraintInfo *riinfo,
 							bool detectNewRows, int expect_OK);
 static bool ri_FastPathCheck(RI_ConstraintInfo *riinfo,
 							 Relation fk_rel, TupleTableSlot *newslot);
-static bool ri_FastPathBatchAdd(RI_ConstraintInfo *riinfo,
-								Relation fk_rel, TupleTableSlot *newslot);
-static void ri_FastPathBatchFlush(RI_Fas
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`aclcheck_error`
- 外部函数：`anyrange`
- 外部函数：`appendStringInfoString`
- 外部函数：`attnumCollationId`
- 外部函数：`attnumTypeId`
- 外部函数：`bms_add_member`
- 外部函数：`buffered`
- 外部函数：`cache`
- 外部函数：`check_enable_rls`
- 外部函数：`construct_array`
- 外部函数：`dclist_container`
- 外部函数：`dclist_count`
- 外部函数：`dclist_delete_from`
- 外部函数：`dclist_push_tail`
- 外部函数：`elog`
- 外部函数：`end`
- 外部函数：`entry`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errdetail`
- 外部函数：`errmsg`
- 外部函数：`errtableconstraint`
- 外部函数：`find_coercion_pathway`
- 外部函数：`fired`
- 外部函数：`fmgr_info_copy`
- 外部函数：`fmgr_info_cxt`
- 外部函数：`format_type_be`
- 外部函数：`getBaseType`
- 外部函数：`getTypeOutputInfo`
- 外部函数：`get_func_name`
- 外部函数：`get_index_column_opclass`
- 外部函数：`get_namespace_name`
- 外部函数：`get_op_opfamily_properties`
- 外部函数：`get_op_opfamily_strategy`
- 外部函数：`get_opcode`
- 外部函数：`get_rel_relkind`
- 外部函数：`get_typlenbyvalalign`
- 外部函数：`hash_create`
- 外部函数：`hash_destroy`
- 外部函数：`hash_get_num_entries`
- 外部函数：`hash_search`
- 外部函数：`hash_seq_init`
- 外部函数：`hash_seq_search`
- 外部函数：`hash_seq_term`
- 外部函数：`index_beginscan`
- 外部函数：`index_close`
- 外部函数：`index_endscan`
- 外部函数：`index_open`
- 外部函数：`index_rescan`
- 外部函数：`initStringInfo`
- 外部函数：`lengthof`
- 外部函数：`list_make1`
- 外部函数：`local`
- 外部函数：`makeNode`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`needed`
- 外部函数：`object_aclcheck`
- 外部函数：`offsetof`
- 外部函数：`op_input_types`
- 外部函数：`operators`
- 外部函数：`palloc_object`
- 外部函数：`path`
- 外部函数：`pf_eq_oprs`
- 外部函数：`pfree`
- 外部函数：`pg_attribute_aclcheck`
- 外部函数：`pg_class_aclcheck`
- 外部函数：`range_agg`
- 外部函数：`ri_CompareWithCast`
- 外部函数：`ri_FastPathBatchFlush`
- 外部函数：`ri_FastPathTeardown`
- 外部函数：`ri_LockPKTuple`
- 外部函数：`ri_PerformCheck`
- 外部函数：`run`
- 外部函数：`semantics`
- 外部函数：`slot_getattr`
- 外部函数：`state`
- 外部函数：`table_close`
- 外部函数：`table_index_getnext_slot`
- 外部函数：`table_open`
- 外部函数：`table_slot_create`
- 外部函数：`unlikely`
- 外部函数：`work`
- 大写宏：`ACLCHECK_OK`
- 大写宏：`ACL_EXECUTE`
- 大写宏：`ACL_SELECT`
- 大写宏：`ACL_SELECT_FOR_UPDATE`
- 大写宏：`ACL_USAGE`
- 大写宏：`AFTER`
- 大写宏：`ALLOCSET_SMALL_SIZES`
- 大写宏：`ALTER`
- 大写宏：`AMOPOPID`
- 大写宏：`ANYARRAY`
- 大写宏：`ANYENUM`
- 大写宏：`BTREE_AM_OID`
- 大写宏：`CCI`
- 大写宏：`COERCION_IMPLICIT`
- 大写宏：`COERCION_PATH_FUNC`
- 大写宏：`COERCION_PATH_RELABELTYPE`
- 大写宏：`COMMITTED`
- 大写宏：`CONCURRENTLY`
- 大写宏：`CONSTRAINT_FOREIGN`
- 大写宏：`CONSTROID`
- 大写宏：`DDL`
- 大写宏：`DELETE`
- 大写宏：`DML`
- 大写宏：`ERRCODE_FOREIGN_KEY_VIOLATION`
- 大写宏：`ERRCODE_RESTRICT_VIOLATION`
- 大写宏：`ERRCODE_T_R_SERIALIZATION_FAILURE`
- 大写宏：`ERROR`
- 大写宏：`EXCEPTION`
- 大写宏：`FIND_LAST_VERSION`
- 大写宏：`FOR`
- 大写宏：`GETSTRUCT`
- 大写宏：`HASHCTL`
- 大写宏：`HASH_BLOBS`
- 大写宏：`HASH_CONTEXT`
- 大写宏：`HASH_ELEM`
- 大写宏：`HASH_ENTER`
- 大写宏：`HASH_REMOVE`
- 大写宏：`HASH_SEQ_STATUS`
- 大写宏：`HTAB`
- 大写宏：`INCLUDE`
- 大写宏：`INDEX_MAX_KEYS`
- 大写宏：`INJECTION_POINT`
- 大写宏：`KEY`
- 大写宏：`NIL`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`OBJECT_FUNCTION`
- 大写宏：`OBJECT_SCHEMA`
- 大写宏：`OID`
- 大写宏：`PERIOD`
- 大写宏：`PG_END_TRY`
- 大写宏：`PG_FINALLY`
- 大写宏：`PG_TRY`
- 大写宏：`READ`
- 大写宏：`RECORD`
- 大写宏：`REINDEX`
- 大写宏：`RELKIND_PARTITIONED_TABLE`
- 大写宏：`RESTRICT`
- 大写宏：`RI_FASTPATH_UNKNOWN`
- 大写宏：`RI_FASTPATH_UNUSABLE`
- 大写宏：`RI_FASTPATH_USABLE`
- 大写宏：`RI_PLAN_XXX`
- 大写宏：`RI_TRIGGER_NONE`
- 大写宏：`RLS`
- 大写宏：`RLS_ENABLED`
- 大写宏：`RTE_RELATION`
- 大写宏：`SECURITY_LOCAL_USERID_CHANGE`
- 大写宏：`SECURITY_NOFORCE_RLS`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SHARE`
- 大写宏：`SK_SEARCHARRAY`
- 大写宏：`SO_NONE`
- 大写宏：`SPI`
- 大写宏：`STABLE`
- 大写宏：`TABLE`
- 大写宏：`TAM`
- 大写宏：`TODO`
- 大写宏：`TUPLE_LOCK_FLAG_FIND_LAST_VERSION`
- 大写宏：`TUPLE_LOCK_FLAG_LOCK_UPDATE_IN_PROGRESS`
- 大写宏：`UPDATE`
- 大写宏：`USAGE`
- 大写宏：`WARNING`
- 大写宏：`XXX`
- 外部类型：`AccessShareLock`
- 外部类型：`AclMode`
- 外部类型：`AclResult`
- 外部类型：`Acquiring`
- 外部类型：`Adds`
- 外部类型：`Advance`
- 外部类型：`AfterTriggerBatchCallback`
- 外部类型：`All`
- 外部类型：`Also`
- 外部类型：`An`
- 外部类型：`And`
- 外部类型：`Any`
- 外部类型：`Arrange`
- 外部类型：`Array`
- 外部类型：`ArrayType`
- 外部类型：`As`
- 外部类型：`Assert`
- 外部类型：`At`
- 外部类型：`AtEOSubXact_RI`
- 外部类型：`AtEOXact_`
- 外部类型：`AtEOXact_RI`
- 外部类型：`AttrNumber`
- 外部类型：`BTEqualStrategyNumber`
- 外部类型：`Batched`
- 外部类型：`Begin`
- 外部类型：`Being`
- 外部类型：`Benchmarking`
- 外部类型：`Buffer`
- 外部类型：`Build`
- 外部类型：`BuildIndexInfo`
- 外部类型：`Builds`
- 外部类型：`But`
- 外部类型：`By`
- 外部类型：`Called`
- 外部类型：`Callers`
- 外部类型：`Calls`
- 外部类型：`Cast`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`CoerceViaIO`
- 外部类型：`CoercionPathType`
- 外部类型：`Create`
- 外部类型：`Created`
- 外部类型：`Datum`
- 外部类型：`Destroy`
- 外部类型：`Detach`
- 外部类型：`Determine`
- 外部类型：`Dispatches`
- 外部类型：`Do`
- 外部类型：`Done`
- 外部类型：`Each`
- 外部类型：`Entries`
- 外部类型：`Extract`
- 外部类型：`FKs`
- 外部类型：`Fall`
- 外部类型：`Fast`
- 外部类型：`FastPathMeta`
- 外部类型：`Fetch`
- 外部类型：`Find`
- 外部类型：`FirstLowInvalidHeapAttributeNumber`
- 外部类型：`Flush`
- 外部类型：`FmgrInfo`
- 外部类型：`FmgrInfos`
- 外部类型：`For`
- 外部类型：`Form`
- 外部类型：`Form_pg_attribute`
- 外部类型：`Form_pg_constraint`
- 外部类型：`ForwardScanDirection`
- 外部类型：`Freeing`
- 外部类型：`Get`
- 外部类型：`Hash`
- 外部类型：`HeapTuple`
- 外部类型：`HeapTuples`
- 外部类型：`If`
- 外部类型：`Ignore`
- 外部类型：`In`
- 外部类型：`IndexInfo`
- 外部类型：`IndexScanDesc`
- 外部类型：`InvalidOid`
- 外部类型：`InvalidSnapshot`
- 外部类型：`InvalidateConstraintCacheCallBack`
- 外部类型：`Invalidation`
- 外部类型：`Its`
- 外部类型：`Key`
- 外部类型：`Larger`
- 外部类型：`Leave`
- 外部类型：`Linear`
- 外部类型：`Link`
- 外部类型：`Local`
- 外部类型：`LockTupleKeyShare`
- 外部类型：`LockWaitBlock`
- 外部类型：`Look`
- 外部类型：`Make`
- 外部类型：`Map`
- 外部类型：`Maximum`
- 外部类型：`May`
- 外部类型：`MemoryContext`
- 外部类型：`Metadata`
- 外部类型：`Multi`
- 外部类型：`Must`
- 外部类型：`NULLs`
- 外部类型：`NameData`
- 外部类型：`NamespaceRelationId`
- 外部类型：`No`
- 外部类型：`NoLock`
- 外部类型：`Note`
- 外部类型：`Oid`
- 外部类型：`On`
- 外部类型：`Once`
- 外部类型：`Only`
- 外部类型：`Open`
- 外部类型：`Opening`
- 外部类型：`PartitionDirectory`
- 外部类型：`Partitioned`
- 外部类型：`Per`
- 外部类型：`Place`
- 外部类型：`Probe`
- 外部类型：`ProcedureRelationId`
- 外部类型：`Process`
- 外部类型：`Queue`
- 外部类型：`RI_CompareHashEntry`
- 外部类型：`RI_CompareKey`
- 外部类型：`RI_ConstraintInfo`
- 外部类型：`RI_FastPathEntry`
- 外部类型：`RI_FastPathKey`
- 外部类型：`RI_QueryHashEntry`
- 外部类型：`RI_QueryKey`
- 外部类型：`RTEPermissionInfo`
- 外部类型：`RangeTblEntry`
- 外部类型：`Re`
- 外部类型：`Register`
- 外部类型：`Registered`
- 外部类型：`Relation`
- 外部类型：`Release`
- 外部类型：`Released`
- 外部类型：`Reload`
- 外部类型：`Remove`
- 外部类型：`Report`
- 外部类型：`Reset`
- 外部类型：`ResourceOwner`
- 外部类型：`Return`
- 外部类型：`Returns`
- 外部类型：`RowShareLock`
- 外部类型：`SPIPlanPtr`
- 外部类型：`ScanKey`
- 外部类型：`ScanKeyData`
- 外部类型：`ScanKeys`
- 外部类型：`Scratch`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`Single`
- 外部类型：`Skip`
- 外部类型：`Snapshot`
- 外部类型：`StringInfoData`
- 外部类型：`SubTransactionId`
- 外部类型：`Subtransaction`
- 外部类型：`SysCacheIdentifier`
- 外部类型：`TABLEs`
- 外部类型：`TM_Deleted`
- 外部类型：`TM_FailureData`
- 外部类型：`TM_Invisible`
- 外部类型：`TM_Ok`
- 外部类型：`TM_Result`
- 外部类型：`TM_SelfModified`
- 外部类型：`TM_Updated`
- 外部类型：`TTSOpsHeapTuple`
- 外部类型：`Take`
- 外部类型：`Temporal`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`They`
- 外部类型：`This`
- 外部类型：`Thus`
- 外部类型：`TopMemoryContext`
- 外部类型：`TopTransactionContext`
- 外部类型：`Try`
- 外部类型：`TupleDesc`
- 外部类型：`TupleTableSlot`
- 外部类型：`Unique`
- 外部类型：`Unlike`
- 外部类型：`Use`
- 外部类型：`Used`
- 外部类型：`Uses`
- 外部类型：`Violations`
- 外部类型：`Walk`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`Zero`
- 外部类型：`intptr_t`
- 外部类型：`padding`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-71b4723c4a` → 本草稿移入 `cases/defect/auto-postgres-71b4723c4a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
