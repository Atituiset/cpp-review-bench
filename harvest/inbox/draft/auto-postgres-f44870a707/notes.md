# auto-postgres-f44870a707

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#f2dd2a3bd44eb1d158e6e507b86dc3df787043b1](https://github.com/postgres/postgres/commit/f2dd2a3bd44eb1d158e6e507b86dc3df787043b1) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 188 |
| 编译错误数（gcc syntax-only） | 160（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f2dd2a3bd44eb1d158e6e507b86dc3df787043b1 (https://github.com/postgres/postgres/commit/f2dd2a3bd44eb1d158e6e507b86dc3df787043b1)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 2298；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f2dd2a3bd44eb1d158e6e507b86dc3df787043b1 Fix potential use-after-frees in WAL sender :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -384,6 +384,9 @@ WalSndErrorCleanup(void)
 	if (xlogreader != NULL && xlogreader->seg.ws_file >= 0)
 		wal_segment_close(xlogreader);
 
+	xlogreader = NULL;
+	logical_decoding_ctx = NULL;
+
 	if (MyReplicationSlot != NULL)
 		ReplicationSlotRelease();
 
@@ -1032,6 +1035,9 @@ StartReplication(StartReplicationCmd *cmd)
 		Assert(streamingDoneSending && streamingDoneReceiving);
 	}
 
+	XLogReaderFree(xlogreader);
+	xlogreader = NULL;
+
 	if (cmd->slotname)
 		ReplicationSlotRelease();
 
@@ -1601,6 +1607,8 @@ StartLogicalReplication(StartReplicationCmd *cmd)
 	WalSndLoop(XLogSendLogical);
 
 	FreeDecodingContext(logical_decoding_ctx);
+	logical_decoding_ctx = NULL;
+	xlogreader = NULL;
 	ReplicationSlotRelease();
 
 	replication_active = false;
@@ -2138,6 +2146,9 @@ exec_replication_command(const char *cmd_string)
 
 	CHECK_FOR_INTERRUPTS();
 
+	Assert(xlogreader == NULL);
+	Assert(logical_decoding_ctx == NULL);
+
 	/*
 	 * Prepare to parse and execute the command.
 	 *
@@ -2295,7 +2306,6 @@ exec_replication_command(const char *cmd_string)
 				/* dupe, but necessary per libpqrcv_endstreaming */
 				EndReplicationCommand(cmdtag);
 
-				Assert(xlogreader != NULL);
 				break;
 			}
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`pq_endmsgread`
- 外部函数：`pq_getbyte_if_available`
- 外部函数：`pq_getmessage`
- 外部函数：`pq_getmsgbyte`
- 外部函数：`pq_getmsgint64`
- 外部函数：`pq_putmessage_noblock`
- 外部函数：`pq_startmsgread`
- 外部函数：`proc_exit`
- 外部函数：`requirements`
- 外部函数：`resetStringInfo`
- 外部函数：`wal_segment_close`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`COMMERROR`
- 大写宏：`EOF`
- 大写宏：`ERRCODE_PROTOCOL_VIOLATION`
- 大写宏：`FATAL`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PGC_SIGHUP`
- 大写宏：`PQ_LARGE_MESSAGE_LIMIT`
- 大写宏：`PQ_SMALL_MESSAGE_LIMIT`
- 大写宏：`REACHED`
- 外部类型：`Check`
- 外部类型：`ConfigReloadPending`
- 外部类型：`CopyData`
- 外部类型：`CopyDone`
- 外部类型：`DestNone`
- 外部类型：`DestRemote`
- 外部类型：`If`
- 外部类型：`InvalidXLogRecPtr`
- 外部类型：`MyReplicationSlot`
- 外部类型：`One`
- 外部类型：`PqMsg_CopyData`
- 外部类型：`PqMsg_CopyDone`
- 外部类型：`PqMsg_Terminate`
- 外部类型：`PqReplMsg_HotStandbyFeedback`
- 外部类型：`PqReplMsg_PrimaryStatusRequest`
- 外部类型：`PqReplMsg_StandbyStatusUpdate`
- 外部类型：`Prepare`
- 外部类型：`Read`
- 外部类型：`Recheck`
- 外部类型：`ReplicationSlot`
- 外部类型：`Reply`
- 外部类型：`Reset`
- 外部类型：`Save`
- 外部类型：`TimeOffset`
- 外部类型：`TimestampTz`
- 外部类型：`Validate`
- 外部类型：`XLogRecPtr`
- 外部类型：`XLogSendLogical`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f44870a707` → 本草稿移入 `cases/defect/auto-postgres-f44870a707/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
