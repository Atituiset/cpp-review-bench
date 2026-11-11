# auto-postgres-8d74d301bc

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#5bf919736b20ffdc8d646c33ed2d9054609b23ec](https://github.com/postgres/postgres/commit/5bf919736b20ffdc8d646c33ed2d9054609b23ec) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5bf919736b20ffdc8d646c33ed2d9054609b23ec (https://github.com/postgres/postgres/commit/5bf919736b20ffdc8d646c33ed2d9054609b23ec)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1426；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5bf919736b20ffdc8d646c33ed2d9054609b23ec Fix restart of interruptible bgworkers after database commands :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1418,17 +1418,21 @@ GetBackgroundWorkerTypeByPid(pid_t pid)
 
 /*
  * Terminate all background workers connected to the given database, if the
- * workers can be interrupted.
+ * workers can be interrupted.  The workers are signaled directly, so that
+ * they can be restarted based on their bgw_restart_time.
  */
 void
 TerminateBackgroundWorkersForDatabase(Oid databaseId)
 {
-	bool		signal_postmaster = false;
+	pid_t	   *pids;
+	int			npids = 0;
 
 	elog(DEBUG1, "attempting worker termination for database %u",
 		 databaseId);
 
-	LWLockAcquire(BackgroundWorkerLock, LW_EXCLUSIVE);
+	pids = palloc_array(pid_t, BackgroundWorkerData->total_slots);
+
+	LWLockAcquire(BackgroundWorkerLock, LW_SHARED);
 
 	/*
 	 * Iterate through slots, looking for workers connected to the given
@@ -1447,20 +1451,22 @@ TerminateBackgroundWorkersForDatabase(Oid databaseId)
 			LWLockAcquire(ProcArrayLock, LW_SHARED);
 			proc = BackendPidGetProcWithLock(pid);
 			if (proc && proc->databaseId == databaseId)
-			{
-				slot->terminate = true;
-				signal_postmaster = true;
-
-				elog(DEBUG1, "termination requested for worker (PID %d) on database %u",
-					 (int) pid, databaseId);
-			}
+				pids[npids++] = pid;
 			LWLockRelease(ProcArrayLock);
 		}
 	}
 
 	LWLockRelease(BackgroundWorkerLock);
 
-	/* Make sure the postmaster notices the change to shared memory. */
-	if (signal_postmaster)
-		SendPostmasterSignal(PMSIGNAL_BACKGROUND_WORKER_CHANGE);
+	/*
+	 * Signal the workers, avoiding kill() while holding the locks.
+	 */
+	for (int i = 0; i < npids; i++)
+	{
+		elog(DEBUG1, "termination requested for worker (PID %d) on database %u",
+			 (int) pids[i], databaseId);
+		(void) kill(pids[i], SIGTERM);	/* ignore any error */
+	}
+
+	pfree(pids);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`worker`
- 大写宏：`DEBUG1`
- 大写宏：`LW_EXCLUSIVE`
- 大写宏：`LW_SHARED`
- 大写宏：`PID`
- 大写宏：`PMSIGNAL_BACKGROUND_WORKER_CHANGE`
- 外部类型：`BackgroundWorkerLock`
- 外部类型：`Iterate`
- 外部类型：`Make`
- 外部类型：`Oid`
- 外部类型：`ProcArrayLock`
- 外部类型：`Terminate`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-8d74d301bc` → 本草稿移入 `cases/defect/auto-postgres-8d74d301bc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
