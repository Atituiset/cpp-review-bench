# auto-postgres-6384212db3

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
| 外部依赖数（dep_count） | 60 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #11591f273e6a26fb55728b410538fa2627b19b28 (https://github.com/postgres/postgres/commit/11591f273e6a26fb55728b410538fa2627b19b28)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1044；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 11591f273e6a26fb55728b410538fa2627b19b28 Support asynchronous execution of MergeAppend subplans. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -64,7 +64,6 @@
 #include "executor/nodeAppend.h"
 #include "miscadmin.h"
 #include "pgstat.h"
-#include "storage/latch.h"
 #include "storage/lwlock.h"
 #include "utils/wait_event.h"
 
@@ -84,7 +83,6 @@ struct ParallelAppendState
 };
 
 #define INVALID_SUBPLAN_INDEX		-1
-#define EVENT_BUFFER_SIZE			16
 
 static TupleTableSlot *ExecAppend(PlanState *pstate);
 static bool choose_next_subplan_locally(AppendState *node);
@@ -1023,100 +1021,20 @@ ExecAppendAsyncEventWait(AppendState *node)
 {
 	int			nevents = node->as_nasyncplans + 2;
 	long		timeout = node->as_syncdone ? -1 : 0;
-	WaitEvent	occurred_event[EVENT_BUFFER_SIZE];
-	int			noccurred;
-	int			i;
 
 	/* We should never be called when there are no valid async subplans. */
 	Assert(node->as_nasyncremain > 0);
 
 	Assert(node->as_eventset == NULL);
 	node->as_eventset = CreateWaitEventSet(CurrentResourceOwner, nevents);
-	AddWaitEventToSet(node->as_eventset, WL_EXIT_ON_PM_DEATH, PGINVALID_SOCKET,
-					  NULL, NULL);
-
-	/* Give each waiting subplan a chance to add an event. */
-	i = -1;
-	while ((i = bms_next_member(node->as_asyncplans, i)) >= 0)
-	{
-		AsyncRequest *areq = node->as_asyncrequests[i];
-
-		if (areq->callback_pending)
-			ExecAsyncConfigureWait(areq);
-	}
-
-	/*
-	 * No need for further processing if none of the subplans configured any
-	 * events.
-	 */
-	if (GetNumRegisteredWaitEvents(node->as_eventset) == 1)
-	{
-		FreeWaitEventSet(node->as_eventset);
-		node->as_eventset = NULL;
-		return;
-	}
-
-	/*
-	 * Add the process latch to the set, so that we wake up to process the
-	 * standard interrupts with CHECK_FOR_INTERRUPTS().
-	 *
-	 * NOTE: For historical reasons, it's important that this is added to the
-	 * WaitEventSet after the ExecAsyncConfigureWait() calls.  Namely,
-	 * postgres_fdw calls "GetNumRegisteredWaitEvents(set) == 1" to check if
-	 * any other events are in the set.  That's a poor design, it's
-	 * questionable for postgres_fdw to be doing that in the first place, but
-	 * we cannot change it now.  The pattern has possibly been copied to other
-	 * extensions too.
-	 */
-	AddWaitEventToSet(node->as_eventset, WL_LATCH_SET, PGINVALID_SOCKET,
-					  MyLatch, NULL);
-
-	/* Return at most EVENT_BUFFER_SIZE events in one call. */
-	if (nevents > EVENT_BUFFER_SIZE)
-		nevents = EVENT_BUFFER_SIZE;
-
-	/* Wait or poll for file descriptor events. */
-	noccurred = WaitEventSetWait(node->as_eventset, timeout, occurred_event,
-								 nevents, WAIT_EVENT_APPEND_READY);
+	ExecAsyncEventWait(node->as_asyncplans,
+					   node->as_asyncrequests,
+					   node->as_eventset,
+					   nevents,
+					   timeout,
+					   WAIT_EVENT_APPEND_READY);
 	FreeWaitEventSet(node->as_eventset);
 	node->as_eventset = NULL;
-	if (noccurred == 0)
-		return;
-
-	/* Deliver notifications. */
-	for (i = 0; i < noccurred; i++)
-	{
-		WaitEvent  *w = &occurred_event[i];
-
-		/*
-		 * Each waiting subplan should have registered its wait event with
-		 * user_data pointing back to its AsyncRequest.
-		 */
-		if ((w->events & WL_SOCKET_READABLE) != 0)
-		{
-			AsyncRequest *areq = (AsyncRequest *) w->user_data;
-
-			if (areq->callback_pending)
-			{
-				/*
-				 * Mark it as no longer needing a callback.  We must do this
-				 * before dispatching the callback in case the callback resets
-				 * the flag.
-				 */
-				areq->callback_pending = false;
-
-				/* Do the actual work. */
-				ExecAsyncNotify(areq);
-			}
-		}
-
-		/* Handle standard interrupts */
-		if ((w->events & WL_LATCH_SET) != 0)
-		{
-			ResetLatch(MyLatch);
-			CHECK_FOR_INTERRUPTS();
-		}
-	}
 }
 
 /* ----------------------------------------------------------------
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_del_members`
- 外部函数：`bms_free`
- 外部函数：`bms_intersect`
- 外部函数：`bms_is_empty`
- 外部函数：`bms_next_member`
- 外部函数：`bms_num_members`
- 外部函数：`bms_overlap`
- 外部函数：`bms_prev_member`
- 外部函数：`castNode`
- 外部函数：`choose_next_subplan`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`NOT`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`PGINVALID_SOCKET`
- 大写宏：`WAIT_EVENT_APPEND_READY`
- 大写宏：`WL_EXIT_ON_PM_DEATH`
- 大写宏：`WL_LATCH_SET`
- 大写宏：`WL_SOCKET_READABLE`
- 外部类型：`Add`
- 外部类型：`Adjust`
- 外部类型：`And`
- 外部类型：`AppendState`
- 外部类型：`Appends`
- 外部类型：`AsyncRequest`
- 外部类型：`Backward`
- 外部类型：`Bitmapset`
- 外部类型：`Break`
- 外部类型：`CurrentResourceOwner`
- 外部类型：`Deliver`
- 外部类型：`Do`
- 外部类型：`Each`
- 外部类型：`Ensure`
- 外部类型：`ExecInitAppend`
- 外部类型：`For`
- 外部类型：`Get`
- 外部类型：`Give`
- 外部类型：`Handle`
- 外部类型：`If`
- 外部类型：`Init`
- 外部类型：`Initialize`
- 外部类型：`Make`
- 外部类型：`Mark`
- 外部类型：`MyLatch`
- 外部类型：`Namely`
- 外部类型：`No`
- 外部类型：`Nothing`
- 外部类型：`Otherwise`
- 外部类型：`PlanState`
- 外部类型：`ReScan`
- 外部类型：`Request`
- 外部类型：`Return`
- 外部类型：`Save`
- 外部类型：`Set`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`TupleTableSlot`
- 外部类型：`Wait`
- 外部类型：`WaitEvent`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-6384212db3` → 本草稿移入 `cases/defect/auto-postgres-6384212db3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
