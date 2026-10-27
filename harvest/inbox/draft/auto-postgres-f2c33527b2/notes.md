# auto-postgres-f2c33527b2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#23bf901fa5306b245e23ace52e5a6777ea08bed6](https://github.com/postgres/postgres/commit/23bf901fa5306b245e23ace52e5a6777ea08bed6) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 163 |
| 编译错误数（gcc syntax-only） | 54（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #23bf901fa5306b245e23ace52e5a6777ea08bed6 (https://github.com/postgres/postgres/commit/23bf901fa5306b245e23ace52e5a6777ea08bed6)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 1063；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 23bf901fa5306b245e23ace52e5a6777ea08bed6 pg_stat_statements: Fix toplevel tracking across nesting levels :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -274,6 +274,19 @@ static const ShmemCallbacks pgss_shmem_callbacks = {
 /* Current nesting depth of planner/ExecutorRun/ProcessUtility calls */
 static int	nesting_level = 0;
 
+/*
+ * State saved in a QueryDesc at ExecutorStart, for use at ExecutorEnd.  A
+ * query may end at a different nesting level than the one it started at,
+ * for example a cursor declared in a transaction and closed at COMMIT.
+ */
+typedef struct pgssQueryState
+{
+	int			nesting_level;	/* nesting level at ExecutorStart */
+} pgssQueryState;
+
+/* ID of pg_stat_statements for QueryDesc extension state */
+static int	pgss_queryDesc_id = -1;
+
 /* Saved hook values */
 static post_parse_analyze_hook_type prev_post_parse_analyze_hook = NULL;
 static planner_hook_type prev_planner_hook = NULL;
@@ -356,6 +369,7 @@ static void pgss_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
 								DestReceiver *dest, QueryCompletion *qc);
 static void pgss_store(const char *query, int64 queryId,
 					   int query_location, int query_len,
+					   bool toplevel,
 					   pgssStoreKind kind,
 					   double total_time, uint64 rows,
 					   const BufferUsage *bufusage,
@@ -474,6 +488,11 @@ _PG_init(void)
 	 */
 	RegisterShmemCallbacks(&pgss_shmem_callbacks);
 
+	/*
+	 * Get an ID to store state in QueryDesc.
+	 */
+	pgss_queryDesc_id = GetQueryDescExtensionId("pg_stat_statements");
+
 	/*
 	 * Install hooks.
 	 */
@@ -866,6 +885,7 @@ pgss_post_parse_analyze(ParseState *pstate, Query *query, const JumbleState *jst
 				   query->queryId,
 				   query->stmt_location,
 				   query->stmt_len,
+				   nesting_level == 0,
 				   PGSS_INVALID,
 				   0,
 				   0,
@@ -948,6 +968,7 @@ pgss_planner(Query *parse,
 				   parse->queryId,
 				   parse->stmt_location,
 				   parse->stmt_len,
+				   nesting_level == 0,
 				   PGSS_PLAN,
 				   INSTR_TIME_GET_MILLISEC(duration),
 				   0,
@@ -992,6 +1013,8 @@ pgss_planner(Query *parse,
 static void
 pgss_ExecutorStart(QueryDesc *queryDesc, int eflags)
 {
+	pgssQueryState *qstate;
+
 	/*
 	 * If query has queryId zero, don't track it.  This prevents double
 	 * counting of optimizable statements that are directly contained in
@@ -1001,6 +1024,12 @@ pgss_ExecutorStart(QueryDesc *queryDesc, int eflags)
 	{
 		/* Request all summary instrumentation, i.e. timing, buffers and WAL */
 		queryDesc->query_instr_options |= INSTRUMENT_ALL;
+
+		/* Remember the nesting level for ExecutorEnd */
+		qstate = MemoryContextAlloc(GetMemoryChunkContext(queryDesc),
+									sizeof(pgssQueryState));
+		qstate->nesting_level = nesting_level;
+		SetQueryDescExtensionState(queryDesc, pgss_queryDesc_id, qstate);
 	}
 
 	if (prev_ExecutorStart)
@@ -1058,14 +1087,22 @@ static void
 pgss_ExecutorEnd(QueryDesc *queryDesc)
 {
 	int64		queryId = queryDesc->plannedstmt->queryId;
+	pgssQueryState *qstate = GetQueryDescExtensionState(queryDesc,
+														pgss_queryDesc_id);
 
+	/*
+	 * Use the nesting level saved at ExecutorStart, if available, since the
+	 * query may end at a different level (e.g. a cursor closed at COMMIT).
+	 */
 	if (queryId != INT64CONST(0) && queryDesc->query_instr &&
-		pgss_enabled(nesting_level))
+		qstate != NULL &&
+		pgss_enabled(qstate->nesting_level))
 	{
 		pgss_store(queryDesc->sourceText,
 				   queryId,
 				   queryDesc->plannedstmt->stmt_location,
 				   queryDesc->plannedstmt->stmt_len,
+				   qstate->nesting_level == 0,
 				   PGSS_EXEC,
 				   INSTR_TIME_GET_MILLISEC(queryDesc->query_instr->total),
 				   queryDesc->estate->es_total_processed,
@@ -1202,6 +1239,7 @@ pgss_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
 				   saved_queryId,
 				   saved_stmt_location,
 				   saved_stmt_len,
+				   nesting_level == 0,
 				   PGSS_EXEC,
 				   INSTR_TIME_GET_MILLISEC(duration),
 				   rows,
@@ -1267,6 +1305,7 @@ pgss_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
 static void
 pgss_store(const char *query, int64 queryId,
 		   int query_location, int query_len,

```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`appendBinaryStringInfo`
- 外部函数：`appendStringInfo`
- 外部函数：`chunk`
- 外部函数：`counters`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errcode_for_file_access`
- 外部函数：`errdetail`
- 外部函数：`errmsg`
- 外部函数：`fail`
- 外部函数：`fileno`
- 外部函数：`fstat`
- 外部函数：`ftruncate`
- 外部函数：`fwrite`
- 外部函数：`hash_get_num_entries`
- 外部函数：`hash_search`
- 外部函数：`hash_seq_init`
- 外部函数：`hash_seq_search`
- 外部函数：`hash_seq_term`
- 外部函数：`initStringInfoExt`
- 外部函数：`iteration`
- 外部函数：`lengths`
- 外部函数：`memset`
- 外部函数：`palloc_array`
- 外部函数：`palloc_extended`
- 外部函数：`pfree`
- 外部函数：`pg_pwrite`
- 外部函数：`pg_stat_statements_internal`
- 外部函数：`pgss_enabled`
- 外部函数：`prev_ExecutorEnd`
- 外部函数：`prev_ExecutorStart`
- 外部函数：`qsort`
- 外部函数：`read`
- 外部函数：`record_gc_qtexts`
- 外部函数：`standard_ExecutorEnd`
- 外部函数：`standard_ExecutorStart`
- 外部函数：`the`
- 外部函数：`unlikely`
- 外部函数：`unlink`
- 大写宏：`DEBUG1`
- 大写宏：`EFBIG`
- 大写宏：`ENOENT`
- 大写宏：`ERRCODE_OUT_OF_MEMORY`
- 大写宏：`FILE`
- 大写宏：`FIRST`
- 大写宏：`HASH_ENTER`
- 大写宏：`HASH_FIND`
- 大写宏：`HASH_REMOVE`
- 大写宏：`HASH_SEQ_STATUS`
- 大写宏：`INSTRUMENT_ALL`
- 大写宏：`INSTR_TIME_GET_MILLISEC`
- 大写宏：`INT64CONST`
- 大写宏：`INT_MAX`
- 大写宏：`JIT`
- 大写宏：`LOG`
- 大写宏：`LW_EXCLUSIVE`
- 大写宏：`LW_SHARED`
- 大写宏：`MCXT_ALLOC_HUGE`
- 大写宏：`MCXT_ALLOC_NO_OOM`
- 大写宏：`MUST`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`OOM`
- 大写宏：`O_CREAT`
- 大写宏：`O_RDONLY`
- 大写宏：`O_RDWR`
- 大写宏：`PGSS_EXEC`
- 大写宏：`PGSS_INVALID`
- 大写宏：`PGSS_PLAN`
- 大写宏：`PGSS_TRACK_ALL`
- 大写宏：`PGSS_TRACK_TOP`
- 大写宏：`PG_BINARY`
- 大写宏：`PG_BINARY_W`
- 大写宏：`PG_STAT_TMP_DIR`
- 大写宏：`PLAN_STMT_CACHE_CUSTOM`
- 大写宏：`PLAN_STMT_CACHE_GENERIC`
- 大写宏：`WAL`
- 外部类型：`Adjust`
- 外部类型：`Allocate`
- 外部类型：`Also`
- 外部类型：`And`
- 外部类型：`Append`
- 外部类型：`As`
- 外部类型：`Bogus`
- 外部类型：`BufferUsage`
- 外部类型：`Bump`
- 外部类型：`Calculate`
- 外部类型：`Check`
- 外部类型：`Clean`
- 外部类型：`Confine`
- 外部类型：`Copy`
- 外部类型：`Could`
- 外部类型：`Counters`
- 外部类型：`Create`
- 外部类型：`Current`
- 外部类型：`DestReceiver`
- 外部类型：`Destroy`
- 外部类型：`Determine`
- 外部类型：`Don`
- 外部类型：`ExecutorRun`
- 外部类型：`File`
- 外部类型：`Find`
- 外部类型：`Force`
- 外部类型：`Get`
- 外部类型：`Grab`
- 外部类型：`Here`
- 外部类型：`Hopefully`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Increment`
- 外部类型：`Install`
- 外部类型：`It`
- 外部类型：`JitInstrumentation`
- 外部类型：`JumbleState`
- 外部类型：`Likewise`
- 外部类型：`Load`
- 外部类型：`LocationLen`
- 外部类型：`Looks`
- 外部类型：`Lookup`
- 外部类型：`Make`
- 外部类型：`Mark`
- 外部类型：`MaxAllocHugeSize`
- 外部类型：`MyDatabaseId`
- 外部类型：`Need`
- 外部类型：`New`
- 外部类型：`Note`
- 外部类型：`Nothing`
- 外部类型：`Now`
- 外部类型：`Offset`
- 外部类型：`Oid`
- 外部类型：`Once`
- 外部类型：`Our`
- 外部类型：`PlannedStmtOrigin`
- 外部类型：`ProcessUtility`
- 外部类型：`QueryCompletion`
- 外部类型：`QueryDesc`
- 外部类型：`Read`
- 外部类型：`Record`
- 外部类型：`Request`
- 外部类型：`Reset`
- 外部类型：`Safety`
- 外部类型：`Saved`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`Size`
- 外部类型：`Sort`
- 外部类型：`Source`
- 外部类型：`Sticky`
- 外部类型：`StringInfo`
- 外部类型：`StringInfoData`
- 外部类型：`That`
- 外部类型：`This`
- 外部类型：`TimestampTz`
- 外部类型：`Trouble`
- 外部类型：`Truncate`
- 外部类型：`Unstick`
- 外部类型：`WalUsage`
- 外部类型：`We`
- 外部类型：`Welford`
- 外部类型：`When`
- 外部类型：`While`
- 外部类型：`Windows`
- 外部类型：`Writers`
- 外部类型：`off_t`
- 外部类型：`size_t`
- 外部类型：`slock_t`
- 外部类型：`stat`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f2c33527b2` → 本草稿移入 `cases/defect/auto-postgres-f2c33527b2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
