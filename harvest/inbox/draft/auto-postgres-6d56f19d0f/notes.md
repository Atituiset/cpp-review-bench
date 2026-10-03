# auto-postgres-6d56f19d0f

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
| 外部依赖数（dep_count） | 203 |
| 编译错误数（gcc syntax-only） | 134（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #425daf545d9146e008220ae0b21415982220cd3f (https://github.com/postgres/postgres/commit/425daf545d9146e008220ae0b21415982220cd3f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 6929；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 425daf545d9146e008220ae0b21415982220cd3f Remove batching from RI fast-path checks :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3932,26 +3932,13 @@ typedef struct AfterTriggersData
 	/* per-subtransaction-level data: */
 	AfterTriggersTransData *trans_stack;	/* array of structs shown below */
 	int			maxtransdepth;	/* allocated len of above array */
-
-	List	   *batch_callbacks;	/* List of AfterTriggerCallbackItem; for
-									 * deferred constraints */
-	bool		firing_batch_callbacks; /* true when in
-										 * FireAfterTriggerBatchCallbacks() */
-
-	/*
-	 * Incremented around the trigger-firing loops in AfterTriggerEndQuery,
-	 * AfterTriggerFireDeferred, and AfterTriggerSetState.  Used by
-	 * AfterTriggerIsActive() to signal that after-trigger firing is active.
-	 */
-	int			firing_depth;
 } AfterTriggersData;
 
 struct AfterTriggersQueryData
 {
 	AfterTriggerEventList events;	/* events pending from this query */
 	Tuplestorestate *fdw_tuplestore;	/* foreign tuples for said events */
 	List	   *tables;			/* list of AfterTriggersTableData, see below */
-	List	   *batch_callbacks;	/* List of AfterTriggerCallbackItem */
 };
 
 struct AfterTriggersTransData
@@ -3960,8 +3947,6 @@ struct AfterTriggersTransData
 	SetConstraintState state;	/* saved S C state, or NULL if not yet saved */
 	AfterTriggerEventList events;	/* saved list pointer */
 	int			query_depth;	/* saved query_depth */
-	int			firing_depth;	/* saved firing_depth */
-	bool		firing_batch_callbacks; /* saved firing_batch_callbacks */
 	CommandId	firing_counter; /* saved firing_counter */
 };
 
@@ -3983,13 +3968,6 @@ struct AfterTriggersTableData
 	TupleTableSlot *storeslot;	/* for converting to tuplestore's format */
 };
 
-/* Entry in afterTriggers.batch_callbacks */
-typedef struct AfterTriggerCallbackItem
-{
-	AfterTriggerBatchCallback callback;
-	void	   *arg;
-} AfterTriggerCallbackItem;
-
 static AfterTriggersData afterTriggers;
 
 static void AfterTriggerExecute(EState *estate,
@@ -4025,7 +4003,6 @@ static SetConstraintState SetConstraintStateAddItem(SetConstraintState state,
 													Oid tgoid, bool tgisdeferred);
 static void cancel_prior_stmt_triggers(Oid relid, CmdType cmdType, int tgevent);
 
-static void FireAfterTriggerBatchCallbacks(List *callbacks);
 
 /*
  * Get the FDW tuplestore for the current trigger query level, creating it
@@ -5151,9 +5128,6 @@ AfterTriggerBeginXact(void)
 	 */
 	afterTriggers.firing_counter = (CommandId) 1;	/* mustn't be 0 */
 	afterTriggers.query_depth = -1;
-	afterTriggers.firing_depth = 0;
-	afterTriggers.batch_callbacks = NIL;
-	afterTriggers.firing_batch_callbacks = false;
 
 	/*
 	 * Verify that there is no leftover state remaining.  If these assertions
@@ -5238,7 +5212,6 @@ AfterTriggerEndQuery(EState *estate)
 	 */
 	qs = &afterTriggers.query_stack[afterTriggers.query_depth];
 
-	afterTriggers.firing_depth++;
 	for (;;)
 	{
 		if (afterTriggerMarkEvents(&qs->events, &afterTriggers.events, true))
@@ -5276,23 +5249,10 @@ AfterTriggerEndQuery(EState *estate)
 			break;
 	}
 
-	/*
-	 * Fire batch callbacks before releasing query-level storage and before
-	 * decrementing query_depth.  Callbacks may do real work (index probes,
-	 * error reporting).
-	 *
-	 * Recompute qs first: the loop above refreshes it after each
-	 * afterTriggerInvokeEvents() call (see comment there), but the "all
-	 * fired" break exits without doing so, leaving qs potentially stale here.
-	 */
-	qs = &afterTriggers.query_stack[afterTriggers.query_depth];
-	FireAfterTriggerBatchCallbacks(qs->batch_callbacks);
-
 	/* Release query-level-local storage, including tuplestores if any */
 	AfterTriggerFreeQuery(&afterTriggers.query_stack[afterTriggers.query_depth]);
 
 	afterTriggers.query_depth--;
-	afterTriggers.firing_depth--;
 }
 
 
@@ -5349,9 +5309,6 @@ AfterTriggerFreeQuery(AfterTriggersQueryData *qs)
 	 */
 	qs->tables = NIL;
 	list_free_deep(tables);
-
-	list_free_deep(qs->batch_callbacks);
-	qs->batch_callbacks = NIL;
 }
 
 
@@ -5391,34 +5348,17 @@ AfterTriggerFireDeferred(void)
 	 * Run all the remaining triggers.  Loop until they are all gone, in case
 	 * 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`a`
- 外部函数：`anyway`
- 外部函数：`bms_copy`
- 外部函数：`bms_equal`
- 外部函数：`call`
- 外部函数：`callback`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`execute_attr_map_slot`
- 外部函数：`fmgr_info`
- 外部函数：`heap_freetuple`
- 外部函数：`lappend`
- 外部函数：`lfirst`
- 外部函数：`list_free_deep`
- 外部函数：`memcpy`
- 外部函数：`old`
- 外部函数：`palloc0_object`
- 外部函数：`palloc_object`
- 外部函数：`pfree`
- 外部函数：`pgstat_end_function_usage`
- 外部函数：`pgstat_init_function_usage`
- 外部函数：`table_tuple_fetch_row_version`
- 外部函数：`triggers`
- 外部函数：`tuplestore_begin_heap`
- 外部函数：`tuplestore_end`
- 外部函数：`tuplestore_gettupleslot`
- 外部函数：`work`
- 大写宏：`AFTER`
- 大写宏：`AFTER_TRIGGER_DEFERRABLE`
- 大写宏：`AFTER_TRIGGER_INITDEFERRED`
- 大写宏：`ALL`
- 大写宏：`ALLOCSET_DEFAULT_SIZES`
- 大写宏：`ANALYZE`
- 大写宏：`BEGIN`
- 大写宏：`CMD_DELETE`
- 大写宏：`CMD_INSERT`
- 大写宏：`CMD_MERGE`
- 大写宏：`CMD_UPDATE`
- 大写宏：`CONSTRAINTS`
- 大写宏：`DEFERRABLE`
- 大写宏：`DML`
- 大写宏：`DONE`
- 大写宏：`ERRCODE_E_R_I_E_TRIGGER_PROTOCOL_VIOLATED`
- 大写宏：`ERRCODE_INSUFFICIENT_PRIVILEGE`
- 大写宏：`ERROR`
- 大写宏：`EXCEPTION`
- 大写宏：`EXPLAIN`
- 大写宏：`FDW`
- 大写宏：`FLEXIBLE_ARRAY_MEMBER`
- 大写宏：`IMMEDIATE`
- 大写宏：`LOCAL_FCINFO`
- 大写宏：`MAXALIGN`
- 大写宏：`NIL`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PG_END_TRY`
- 大写宏：`PG_FINALLY`
- 大写宏：`PG_TRY`
- 大写宏：`RELKIND_FOREIGN_TABLE`
- 大写宏：`ROW`
- 大写宏：`SECURITY_LOCAL_USERID_CHANGE`
- 大写宏：`SET`
- 大写宏：`TRIGGER_EVENT_OPMASK`
- 大写宏：`TRIGGER_EVENT_ROW`
- 大写宏：`TRIGGER_EVENT_UPDATE`
- 大写宏：`TRIGGER_FIRED_AFTER`
- 大写宏：`TRIGGER_FIRED_BY_DELETE`
- 大写宏：`TRIGGER_FIRED_BY_INSERT`
- 大写宏：`TRIGGER_FIRED_BY_UPDATE`
- 大写宏：`TRIGGER_FIRED_FOR_STATEMENT`
- 大写宏：`TRIGGER_FOR_UPDATE`
- 外部类型：`AfterTriggerBatchCallback`
- 外部类型：`AfterTriggerCallbackItem`
- 外部类型：`AfterTriggerCurrentQueryDepth`
- 外部类型：`AfterTriggerEndQuery`
- 外部类型：`AfterTriggerEndXact`
- 外部类型：`AfterTriggerEventChunk`
- 外部类型：`AfterTriggerEventData`
- 外部类型：`AfterTriggerEventDataNoOids`
- 外部类型：`AfterTriggerEventDataOneCtid`
- 外部类型：`AfterTriggerEventDataZeroCtids`
- 外部类型：`AfterTriggerEventList`
- 外部类型：`AfterTriggerEvents`
- 外部类型：`AfterTriggerFireDeferred`
- 外部类型：`AfterTriggerFreeQuery`
- 外部类型：`AfterTriggerIsActive`
- 外部类型：`AfterTriggerSetState`
- 外部类型：`AfterTriggerSharedData`
- 外部类型：`AfterTriggerTupleContext`
- 外部类型：`AfterTriggersData`
- 外部类型：`AfterTriggersQueryData`
- 外部类型：`AfterTriggersTableData`
- 外部类型：`AfterTriggersTransData`
- 外部类型：`All`
- 外部类型：`Allocate`
- 外部类型：`Another`
- 外部类型：`At`
- 外部类型：`Bitmapset`
- 外部类型：`But`
- 外部类型：`Call`
- 外部类型：`Callbacks`
- 外部类型：`Caller`
- 外部类型：`Catch`
- 外部类型：`Check`
- 外部类型：`Checking`
- 外部类型：`Chunk`
- 外部类型：`Clear`
- 外部类型：`CmdType`
- 外部类型：`CommandId`
- 外部类型：`Create`
- 外部类型：`CurTransactionContext`
- 外部类型：`CurTransactionResourceOwner`
- 外部类型：`CurrentMemoryContext`
- 外部类型：`CurrentResourceOwner`
- 外部类型：`Datum`
- 外部类型：`Do`
- 外部类型：`Don`
- 外部类型：`Drop`
- 外部类型：`During`
- 外部类型：`EState`
- 外部类型：`Entry`
- 外部类型：`ExecCallTriggerFunc`
- 外部类型：`Exit`
- 外部类型：`Fetch`
- 外部类型：`Fire`
- 外部类型：`FireAfterTriggerBatchCallbacks`
- 外部类型：`Flush`
- 外部类型：`FmgrInfo`
- 外部类型：`For`
- 外部类型：`Get`
- 外部类型：`HeapTuple`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Incremented`
- 外部类型：`Insert`
- 外部类型：`InvalidOid`
- 外部类型：`Invoke`
- 外部类型：`Is`
- 外部类型：`It`
- 外部类型：`ItemPointerData`
- 外部类型：`Lets`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`LocTriggerData`
- 外部类型：`Locate`
- 外部类型：`Look`
- 外部类型：`Loop`
- 外部类型：`Make`
- 外部类型：`Mark`
- 外部类型：`Memory`
- 外部类型：`MemoryContext`
- 外部类型：`Must`
- 外部类型：`MyTriggerDepth`
- 外部类型：`No`
- 外部类型：`Node`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`OIDs`
- 外部类型：`Oid`
- 外部类型：`Otherwise`
- 外部类型：`PgStat_FunctionCallUsage`
- 外部类型：`Protect`
- 外部类型：`Recompute`
- 外部类型：`Register`
- 外部类型：`RegisterAfterTriggerBatchCallback`
- 外部类型：`Relation`
- 外部类型：`Release`
- 外部类型：`Reset`
- 外部类型：`ResourceOwner`
- 外部类型：`Restore`
- 外部类型：`ResultRelInfo`
- 外部类型：`Return`
- 外部类型：`Returns`
- 外部类型：`Run`
- 外部类型：`Set`
- 外部类型：`SetConstraintStateData`
- 外部类型：`SetConstraintTriggerData`
- 外部类型：`Setup`
- 外部类型：`Size`
- 外部类型：`SnapshotAny`
- 外部类型：`So`
- 外部类型：`Store`
- 外部类型：`TTSOpsMinimalTuple`
- 外部类型：`T_TriggerData`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`Therefore`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`TopTransactionContext`
- 外部类型：`Trigger`
- 外部类型：`TriggerData`
- 外部类型：`TriggerDesc`
- 外部类型：`TriggerEvent`
- 外部类型：`TriggerInstrumentation`
- 外部类型：`Try`
- 外部类型：`TupleConversionMap`
- 外部类型：`TupleTableSlot`
- 外部类型：`Tuplestorestate`
- 外部类型：`Used`
- 外部类型：`Verify`
- 外部类型：`We`
- 外部类型：`When`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-6d56f19d0f` → 本草稿移入 `cases/defect/auto-postgres-6d56f19d0f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
