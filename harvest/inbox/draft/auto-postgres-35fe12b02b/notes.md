# auto-postgres-35fe12b02b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#11591f273e6a26fb55728b410538fa2627b19b28](https://github.com/postgres/postgres/commit/11591f273e6a26fb55728b410538fa2627b19b28) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 129 |
| 编译错误数（gcc syntax-only） | 95（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #11591f273e6a26fb55728b410538fa2627b19b28 (https://github.com/postgres/postgres/commit/11591f273e6a26fb55728b410538fa2627b19b28)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 8694；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 11591f273e6a26fb55728b410538fa2627b19b28 Support asynchronous execution of MergeAppend subplans. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1887,7 +1887,8 @@ postgresReScanForeignScan(ForeignScanState *node)
 
 	/*
 	 * If the node is async-capable, any asynchronous fetch made for it should
-	 * have been processed before we get here (see ExecAppendAsyncReset()).
+	 * have been processed before we get here (see ExecAppendAsyncReset() and
+	 * ExecMergeAppendAsyncReset()).
 	 */
 	Assert(!fsstate->async_capable || !fsstate->conn_state->pendingAreq ||
 		   fsstate->conn_state->pendingAreq->requestee != (PlanState *) node);
@@ -8663,11 +8664,13 @@ postgresForeignAsyncConfigureWait(AsyncRequest *areq)
 	ForeignScanState *node = (ForeignScanState *) areq->requestee;
 	PgFdwScanState *fsstate = (PgFdwScanState *) node->fdw_state;
 	AsyncRequest *pendingAreq = fsstate->conn_state->pendingAreq;
-	AppendState *requestor = (AppendState *) areq->requestor;
-	WaitEventSet *set = requestor->as_eventset;
+	PlanState  *requestor = areq->requestor;
+	WaitEventSet *set = ExecAsyncGetRequestorEventSet(requestor);
 
 	/* This should not be called unless callback_pending */
 	Assert(areq->callback_pending);
+	/* The requestor should be an Append or MergeAppend node */
+	Assert(IsA(requestor, AppendState) || IsA(requestor, MergeAppendState));
 
 	/*
 	 * If process_pending_request() has been invoked on the given request
@@ -8688,37 +8691,48 @@ postgresForeignAsyncConfigureWait(AsyncRequest *areq)
 	/* The core code would have registered postmaster death event */
 	Assert(GetNumRegisteredWaitEvents(set) >= 1);
 
-	/* Begin an asynchronous data fetch if not already done */
+	/* Begin an asynchronous data fetch if appropriate */
 	if (!pendingAreq)
 		fetch_more_data_begin(areq);
-	else if (pendingAreq->requestor != areq->requestor)
+	else if (pendingAreq->requestor != requestor)
 	{
 		/*
-		 * This is the case when the in-process request was made by another
-		 * Append.  Note that it might be useless to process the request made
-		 * by that Append, because the query might not need tuples from that
-		 * Append anymore; so we avoid processing it to begin a fetch for the
-		 * given request if possible.  If there are any child subplans of the
-		 * same parent that are ready for new requests, skip the given
-		 * request.  Likewise, if there are any configured events other than
-		 * the postmaster death event, skip it.  Otherwise, process the
-		 * in-process request, then begin a fetch to configure the event
-		 * below, because we might otherwise end up with no configured events
-		 * other than the postmaster death event.
+		 * This is the case when an asynchronous request made by another
+		 * requestor is currently in-progress.  If our requestor is an Append,
+		 * consider skipping configuration of the file descriptor event to
+		 * improve concurrency.  If it's a MergeAppend, it cannot make
+		 * progress unless the given request returns a result; in which case
+		 * configure the file descriptor event below.
+		 */
+		if (IsA(requestor, AppendState))
+		{
+			/*
+			 * If the Append node already has results that are ready for new
+			 * requests or it has already configured file descriptor events
+			 * for other subplans, skip the configuration.
+			 */
+			if (!bms_is_empty(((AppendState *) requestor)->as_needrequest))
+				return;
+			if (GetNumRegisteredWaitEvents(set) > 1)
+				return;
+		}
+
+		/*
+		 * Otherwise, process the in-progress request, begin an asynchronous
+		 * data fetch for the given request, and configure the file descriptor
+		 * event below.
 		 */
-		if (!bms_is_empty(requestor->as_needrequest))
-			return;
-		if (GetNumRegisteredWaitEvents(set) > 1)
-			return;
 		process_pending_request(pendingAreq);
 		fetch_more_data_begin(areq);
 	}
 	else if (pendingAreq->requestee != areq->requestee)
 	{
 		/*
-		 * This is the case when the in-process request was made by the same
-		 * parent but for a different child.  Since we configure only the
-		 * event for the request made for that child, skip the given request.
+		 * This is the case when an asynch
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`appendStringInfo`
- 外部函数：`bms_is_empty`
- 外部函数：`castNode`
- 外部函数：`elog`
- 外部函数：`errcontext`
- 外部函数：`evaluations`
- 外部函数：`exec_rt_fetch`
- 外部函数：`heap_form_tuple`
- 外部函数：`here`
- 外部函数：`initStringInfo`
- 外部函数：`lfirst`
- 外部函数：`lfirst_int`
- 外部函数：`list_length`
- 外部函数：`list_nth`
- 外部函数：`list_nth_node`
- 外部函数：`memset`
- 外部函数：`palloc0_array`
- 外部函数：`palloc_array`
- 外部函数：`parameter`
- 外部函数：`pfree`
- 外部函数：`pgfdw_exec_query`
- 外部函数：`pgfdw_get_result`
- 外部函数：`pgfdw_report_error`
- 外部函数：`postgresForeignAsyncNotify`
- 外部函数：`set_config_option`
- 外部函数：`snprintf`
- 外部函数：`strVal`
- 大写宏：`CTID`
- 大写宏：`CURSOR`
- 大写宏：`DECLARE`
- 大写宏：`EOF`
- 大写宏：`ERROR`
- 大写宏：`FETCH`
- 大写宏：`FOR`
- 大写宏：`FROM`
- 大写宏：`GUC_ACTION_SAVE`
- 大写宏：`INTSTYLE_POSTGRES`
- 大写宏：`ISO`
- 大写宏：`NULL`
- 大写宏：`PGC_S_SESSION`
- 大写宏：`PGC_USERSET`
- 大写宏：`PGRES_COMMAND_OK`
- 大写宏：`PGRES_TUPLES_OK`
- 大写宏：`ROW_MARK_COPY`
- 大写宏：`SELECT`
- 大写宏：`USE_ISO_DATES`
- 外部类型：`Append`
- 外部类型：`AppendState`
- 外部类型：`Apply`
- 外部类型：`AsyncNotify`
- 外部类型：`AsyncRequest`
- 外部类型：`AttInMetadata`
- 外部类型：`AttrNumber`
- 外部类型：`Begin`
- 外部类型：`Build`
- 外部类型：`Check`
- 外部类型：`Clean`
- 外部类型：`Construct`
- 外部类型：`ConversionLocation`
- 外部类型：`Convert`
- 外部类型：`Create`
- 外部类型：`DateStyle`
- 外部类型：`Datum`
- 外部类型：`DatumTupleFields`
- 外部类型：`Do`
- 外部类型：`EState`
- 外部类型：`ErrorContextCallback`
- 外部类型：`EvalPlanQual`
- 外部类型：`Evaluate`
- 外部类型：`ExecAsyncResponse`
- 外部类型：`ExprContext`
- 外部类型：`ExprState`
- 外部类型：`First`
- 外部类型：`FmgrInfo`
- 外部类型：`For`
- 外部类型：`ForeignScan`
- 外部类型：`ForeignScanState`
- 外部类型：`Form_pg_attribute`
- 外部类型：`Get`
- 外部类型：`HeapTuple`
- 外部类型：`HeapTupleFields`
- 外部类型：`HeapTuples`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Initialize`
- 外部类型：`IntervalStyle`
- 外部类型：`InvalidTransactionId`
- 外部类型：`ItemPointer`
- 外部类型：`Likewise`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Look`
- 外部类型：`Mark`
- 外部类型：`MemoryContext`
- 外部类型：`Must`
- 外部类型：`Non`
- 外部类型：`Note`
- 外部类型：`Notice`
- 外部类型：`OIDs`
- 外部类型：`On`
- 外部类型：`Otherwise`
- 外部类型：`PGconn`
- 外部类型：`PGresult`
- 外部类型：`PQnfields`
- 外部类型：`PgFdwConnState`
- 外部类型：`PgFdwScanState`
- 外部类型：`PlanState`
- 外部类型：`RangeTblEntry`
- 外部类型：`Relation`
- 外部类型：`Remember`
- 外部类型：`Reset`
- 外部类型：`ScanTupleSlot`
- 外部类型：`See`
- 外部类型：`SelfItemPointerAttributeNumber`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`Stomp`
- 外部类型：`StringInfoData`
- 外部类型：`Target`
- 外部类型：`TargetEntry`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Thus`
- 外部类型：`TupleDesc`
- 外部类型：`Uninstall`
- 外部类型：`Unlike`
- 外部类型：`Update`
- 外部类型：`Use`
- 外部类型：`Var`
- 外部类型：`Vars`
- 外部类型：`WaitEventSet`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-35fe12b02b` → 本草稿移入 `cases/defect/auto-postgres-35fe12b02b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
