# auto-postgres-0d06a4c690

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
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #11591f273e6a26fb55728b410538fa2627b19b28 (https://github.com/postgres/postgres/commit/11591f273e6a26fb55728b410538fa2627b19b28)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 11591f273e6a26fb55728b410538fa2627b19b28 Support asynchronous execution of MergeAppend subplans. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -19,6 +19,10 @@
 #include "executor/instrument.h"
 #include "executor/nodeAppend.h"
 #include "executor/nodeForeignscan.h"
+#include "executor/nodeMergeAppend.h"
+#include "miscadmin.h"
+
+#define EVENT_BUFFER_SIZE			16
 
 /*
  * Asynchronously request a tuple from a designed async-capable node.
@@ -122,6 +126,9 @@ ExecAsyncResponse(AsyncRequest *areq)
 		case T_AppendState:
 			ExecAsyncAppendResponse(areq);
 			break;
+		case T_MergeAppendState:
+			ExecAsyncMergeAppendResponse(areq);
+			break;
 		default:
 			/* If the node doesn't support async, caller messed up. */
 			elog(ERROR, "unrecognized node type: %d",
@@ -153,3 +160,123 @@ ExecAsyncRequestPending(AsyncRequest *areq)
 	areq->request_complete = false;
 	areq->result = NULL;
 }
+
+/*
+ * Fetch the WaitEventSet associated with an async-aware plan.
+ */
+WaitEventSet *
+ExecAsyncGetRequestorEventSet(PlanState *requestor)
+{
+	switch (nodeTag(requestor))
+	{
+		case T_AppendState:
+			return ((AppendState *) requestor)->as_eventset;
+		case T_MergeAppendState:
+			return ((MergeAppendState *) requestor)->ms_eventset;
+		default:
+			/* If the node doesn't support async, caller messed up. */
+			elog(ERROR, "unrecognized node type: %d",
+				 (int) nodeTag(requestor));
+			return NULL;
+	}
+}
+
+/*
+ * Workhorse function for Exec{Merge}AppendAsyncEventWait.
+ */
+void
+ExecAsyncEventWait(Bitmapset *asyncplans,
+				   AsyncRequest **asyncrequests,
+				   WaitEventSet *eventset,
+				   int nevents,
+				   long timeout,
+				   uint32 wait_event_info)
+{
+	WaitEvent	occurred_event[EVENT_BUFFER_SIZE];
+	int			noccurred;
+	int			i;
+
+	/*
+	 * Add WL_EXIT_ON_PM_DEATH to the set, to exit immediately if the
+	 * postmaster dies.
+	 */
+	AddWaitEventToSet(eventset, WL_EXIT_ON_PM_DEATH, PGINVALID_SOCKET,
+					  NULL, NULL);
+
+	/* Give each waiting subplan a chance to add an event. */
+	i = -1;
+	while ((i = bms_next_member(asyncplans, i)) >= 0)
+	{
+		AsyncRequest *areq = asyncrequests[i];
+
+		if (areq->callback_pending)
+			ExecAsyncConfigureWait(areq);
+	}
+
+	/*
+	 * No need for further processing if none of the subplans configured any
+	 * events.
+	 */
+	if (GetNumRegisteredWaitEvents(eventset) == 1)
+		return;
+
+	/*
+	 * Add the process latch to the set, so that we wake up to process the
+	 * standard interrupts with CHECK_FOR_INTERRUPTS().
+	 *
+	 * NOTE: For historical reasons, it's important that this is added to the
+	 * WaitEventSet after the ExecAsyncConfigureWait() calls.  Namely,
+	 * postgres_fdw calls "GetNumRegisteredWaitEvents(set) == 1" to check if
+	 * any other events are in the set.  That's a poor design, it's
+	 * questionable for postgres_fdw to be doing that in the first place, but
+	 * we cannot change it now.  The pattern has possibly been copied to other
+	 * extensions too.
+	 */
+	AddWaitEventToSet(eventset, WL_LATCH_SET, PGINVALID_SOCKET,
+					  MyLatch, NULL);
+
+	/* Return at most EVENT_BUFFER_SIZE events in one call. */
+	if (nevents > EVENT_BUFFER_SIZE)
+		nevents = EVENT_BUFFER_SIZE;
+
+	/* Wait or poll for file descriptor events. */
+	noccurred = WaitEventSetWait(eventset, timeout, occurred_event, nevents,
+								 wait_event_info);
+	if (noccurred == 0)
+		return;
+
+	/* Deliver notifications. */
+	for (i = 0; i < noccurred; i++)
+	{
+		WaitEvent  *w = &occurred_event[i];
+
+		/*
+		 * Each waiting subplan should have registered its wait event with
+		 * user_data pointing back to its AsyncRequest.
+		 */
+		if ((w->events & WL_SOCKET_READABLE) != 0)
+		{
+			AsyncRequest *areq = (AsyncRequest *) w->user_data;
+
+			if (areq->callback_pending)
+			{
+				/*
+				 * Mark it as no longer needing a callback.  We must do this
+				 * before dispatching the callback in case the callback resets
+				 * the flag.
+				 */
+				areq->callback_pending = false;
+
+				/* Do the actual work. */
+				ExecAsyncNotify(areq);
+			}
+		}
+
+		/* Handle standard interrupts */
+		if ((w->events & WL_LATCH_SET) != 0)
+		{
+			ResetLatch(MyLatch);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 大写宏：`ERROR`
- 大写宏：`NULL`
- 外部类型：`Asynchronously`
- 外部类型：`If`
- 外部类型：`T_AppendState`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-0d06a4c690` → 本草稿移入 `cases/defect/auto-postgres-0d06a4c690/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
