# auto-postgres-ef5e501dab

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#bc85373ff1344c03abf0d74e698b3631936ca3c7](https://github.com/postgres/postgres/commit/bc85373ff1344c03abf0d74e698b3631936ca3c7) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 155 |
| 编译错误数（gcc syntax-only） | 199（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bc85373ff1344c03abf0d74e698b3631936ca3c7 (https://github.com/postgres/postgres/commit/bc85373ff1344c03abf0d74e698b3631936ca3c7)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 314；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bc85373ff1344c03abf0d74e698b3631936ca3c7 Fix lost wakeups in logical WAL senders :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -311,7 +311,8 @@ static void WalSndKeepaliveIfNecessary(void);
 static void WalSndCheckTimeOut(void);
 static void WalSndCheckShutdownTimeout(void);
 static long WalSndComputeSleeptime(TimestampTz now);
-static void WalSndWait(uint32 socket_events, long timeout, uint32 wait_event);
+static void WalSndWait(uint32 socket_events, long timeout, uint32 wait_event,
+					   XLogRecPtr target_lsn);
 static void WalSndPrepareWrite(LogicalDecodingContext *ctx, XLogRecPtr lsn, TransactionId xid, bool last_write);
 static void WalSndWriteData(LogicalDecodingContext *ctx, XLogRecPtr lsn, TransactionId xid, bool last_write);
 static void WalSndUpdateProgress(LogicalDecodingContext *ctx, XLogRecPtr lsn, TransactionId xid,
@@ -1754,7 +1755,7 @@ ProcessPendingWrites(void)
 
 		/* Sleep until something happens or we time out */
 		WalSndWait(WL_SOCKET_WRITEABLE | WL_SOCKET_READABLE, sleeptime,
-				   WAIT_EVENT_WAL_SENDER_WRITE_DATA);
+				   WAIT_EVENT_WAL_SENDER_WRITE_DATA, InvalidXLogRecPtr);
 
 		/* Clear any already-pending wakeups */
 		ResetLatch(MyLatch);
@@ -2095,7 +2096,7 @@ WalSndWaitForWal(XLogRecPtr loc)
 			last_flush = now;
 		}
 
-		WalSndWait(wakeEvents, sleeptime, wait_event);
+		WalSndWait(wakeEvents, sleeptime, wait_event, loc);
 	}
 
 	/* reactivate latch so WalSndLoop knows to continue */
@@ -3215,7 +3216,8 @@ WalSndLoop(WalSndSendDataCallback send_data)
 			}
 
 			/* Sleep until something happens or we time out */
-			WalSndWait(wakeEvents, sleeptime, WAIT_EVENT_WAL_SENDER_MAIN);
+			WalSndWait(wakeEvents, sleeptime, WAIT_EVENT_WAL_SENDER_MAIN,
+					   InvalidXLogRecPtr);
 		}
 	}
 }
@@ -3901,7 +3903,7 @@ WalSndDone(WalSndSendDataCallback send_data)
 
 			/* Sleep until something happens or we time out */
 			WalSndWait(WL_SOCKET_WRITEABLE, sleeptime,
-					   WAIT_EVENT_WAL_SENDER_WRITE_DATA);
+					   WAIT_EVENT_WAL_SENDER_WRITE_DATA, InvalidXLogRecPtr);
 
 			/* Clear any already-pending wakeups */
 			ResetLatch(MyLatch);
@@ -4101,9 +4103,13 @@ WalSndWakeup(bool physical, bool logical)
  * Wait for readiness on the FeBe socket, or a timeout.  The mask should be
  * composed of optional WL_SOCKET_WRITEABLE and WL_SOCKET_READABLE flags.  Exit
  * on postmaster death.
+ *
+ * target_lsn is the required WAL position for
+ * WAIT_EVENT_WAL_SENDER_WAIT_FOR_WAL and is unused for other wait events.
  */
 static void
-WalSndWait(uint32 socket_events, long timeout, uint32 wait_event)
+WalSndWait(uint32 socket_events, long timeout, uint32 wait_event,
+		   XLogRecPtr target_lsn)
 {
 	WaitEvent	event;
 
@@ -4182,8 +4188,33 @@ WalSndWait(uint32 socket_events, long timeout, uint32 wait_event)
 		}
 	}
 	else if (MyWalSnd->kind == REPLICATION_KIND_LOGICAL)
+	{
 		ConditionVariablePrepareToSleep(&WalSndCtl->wal_replay_cv);
 
+		/*
+		 * Recheck the WAL position tested by WalSndWaitForWal() now that
+		 * we're on the waitlist. Otherwise, we could miss a wakeup for a WAL
+		 * flush or replay that occurred before we joined it.
+		 */
+		if (wait_event == WAIT_EVENT_WAL_SENDER_WAIT_FOR_WAL)
+		{
+			XLogRecPtr	flushPtr;
+
+			Assert(XLogRecPtrIsValid(target_lsn));
+
+			if (!RecoveryInProgress())
+				flushPtr = GetFlushRecPtr(NULL);
+			else
+				flushPtr = GetXLogReplayRecPtr(NULL);
+
+			if (flushPtr >= target_lsn)
+			{
+				ConditionVariableCancelSleep();
+				return;
+			}
+		}
+	}
+
 	if (WaitEventSetWait(FeBeWaitSet, timeout, &event, 1, wait_event) == 1 &&
 		(event.events & WL_POSTMASTER_DEATH))
 	{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`memcpy`
- 外部函数：`pq_flush_if_writable`
- 外部函数：`pq_is_send_pending`
- 外部函数：`pq_putmessage_noblock`
- 外部函数：`pq_sendbyte`
- 外部函数：`pq_sendint64`
- 外部函数：`proc_exit`
- 外部函数：`requirements`
- 外部函数：`resetStringInfo`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`LSN`
- 大写宏：`PGC_SIGHUP`
- 大写宏：`SYNC_STANDBY_DEFINED`
- 大写宏：`WAIT_EVENT_WAL_SENDER_WRITE_DATA`
- 大写宏：`WL_SOCKET_READABLE`
- 大写宏：`WL_SOCKET_WRITEABLE`
- 外部类型：`Call`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`ConfigReloadPending`
- 外部类型：`CopyData`
- 外部类型：`DestNone`
- 外部类型：`DestRemote`
- 外部类型：`During`
- 外部类型：`Fill`
- 外部类型：`If`
- 外部类型：`InvalidXLogRecPtr`
- 外部类型：`It`
- 外部类型：`LogicalDecodingContext`
- 外部类型：`MyLatch`
- 外部类型：`PqMsg_CopyData`
- 外部类型：`PqReplMsg_WALData`
- 外部类型：`Process`
- 外部类型：`Recheck`
- 外部类型：`Reset`
- 外部类型：`Send`
- 外部类型：`Sleep`
- 外部类型：`So`
- 外部类型：`This`
- 外部类型：`TimestampTz`
- 外部类型：`Track`
- 外部类型：`TransactionId`
- 外部类型：`Try`
- 外部类型：`WalSndCtl`
- 外部类型：`WalSndCtlData`
- 外部类型：`WalSndLoop`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`XLogRecPtr`
- 外部类型：`XLogSendPhysical`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ef5e501dab` → 本草稿移入 `cases/defect/auto-postgres-ef5e501dab/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
