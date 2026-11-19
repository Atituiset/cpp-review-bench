# auto-postgres-1aaa3b08e6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#ba11c48390004ed49b889cbd718c9e5f691fe493](https://github.com/postgres/postgres/commit/ba11c48390004ed49b889cbd718c9e5f691fe493) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ba11c48390004ed49b889cbd718c9e5f691fe493 (https://github.com/postgres/postgres/commit/ba11c48390004ed49b889cbd718c9e5f691fe493)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 362；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ba11c48390004ed49b889cbd718c9e5f691fe493 Don't persist synced slots before a consistent snapshot is built. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -224,11 +224,22 @@ update_local_synced_slot(RemoteSlot *remote_slot, Oid remote_dbid)
 	ReplicationSlot *slot = MyReplicationSlot;
 	bool		updated_xmin_or_lsn = false;
 	bool		updated_config = false;
+	bool		consistent_snapshot_available = false;
 	SlotSyncSkipReason skip_reason = SS_SKIP_NONE;
 	XLogRecPtr	latestFlushPtr = GetStandbyFlushRecPtr(NULL);
 
 	Assert(slot->data.invalidated == RS_INVAL_NONE);
 
+	/*
+	 * A temporary slot reaching here is either newly created, so its
+	 * confirmed_flush is not set yet, or it was skipped by an earlier call
+	 * and still has the reason for that. If an earlier call had synced it
+	 * successfully, the caller would have made it persistent already.
+	 */
+	Assert(slot->data.persistency != RS_TEMPORARY ||
+		   !XLogRecPtrIsValid(slot->data.confirmed_flush) ||
+		   slot->slotsync_skip_reason != SS_SKIP_NONE);
+
 	/*
 	 * Make sure that concerned WAL is received and flushed before syncing
 	 * slot to target lsn received from the primary server.
@@ -335,16 +346,16 @@ update_local_synced_slot(RemoteSlot *remote_slot, Oid remote_dbid)
 			SpinLockRelease(&slot->mutex);
 
 			updated_xmin_or_lsn = true;
+			consistent_snapshot_available = true;
 		}
 		else
 		{
-			bool		found_consistent_snapshot;
 			XLogRecPtr	old_confirmed_lsn = slot->data.confirmed_flush;
 			XLogRecPtr	old_restart_lsn = slot->data.restart_lsn;
 			TransactionId old_catalog_xmin = slot->data.catalog_xmin;
 
 			LogicalSlotAdvanceAndCheckSnapState(remote_slot->confirmed_lsn,
-												&found_consistent_snapshot);
+												&consistent_snapshot_available);
 
 			/* Sanity check */
 			if (slot->data.confirmed_flush != remote_slot->confirmed_lsn)
@@ -355,22 +366,9 @@ update_local_synced_slot(RemoteSlot *remote_slot, Oid remote_dbid)
 										   LSN_FORMAT_ARGS(remote_slot->confirmed_lsn),
 										   LSN_FORMAT_ARGS(slot->data.confirmed_flush)));
 
-			/*
-			 * If we can't reach a consistent snapshot, the slot won't be
-			 * persisted. See update_and_persist_local_synced_slot().
-			 */
-			if (!found_consistent_snapshot)
-			{
-				Assert(MyReplicationSlot->data.persistency == RS_TEMPORARY);
-
-				ereport(LOG,
-						errmsg("could not synchronize replication slot \"%s\"",
-							   remote_slot->name),
-						errdetail("Synchronization could lead to data loss, because the standby could not build a consistent snapshot to decode WALs at LSN %X/%08X.",
-								  LSN_FORMAT_ARGS(slot->data.restart_lsn)));
-
-				skip_reason = SS_SKIP_NO_CONSISTENT_SNAPSHOT;
-			}
+			/* A persistent slot must always reach a consistent snapshot. */
+			Assert(consistent_snapshot_available ||
+				   slot->data.persistency == RS_TEMPORARY);
 
 			/*
 			 * It is possible that the slot's xmin or LSNs are not updated,
@@ -383,6 +381,25 @@ update_local_synced_slot(RemoteSlot *remote_slot, Oid remote_dbid)
 		}
 	}
 
+	/*
+	 * A temporary slot becomes sync-ready only when this call finds a
+	 * consistent snapshot for it. Otherwise report that no consistent
+	 * snapshot is available, which covers two cases. Either advancing the
+	 * slot above could not build one, or we did not try to advance because
+	 * the remote slot has not moved ahead of the local slot.
+	 */
+	if (slot->data.persistency == RS_TEMPORARY &&
+		!consistent_snapshot_available)
+	{
+		ereport(LOG,
+				errmsg("could not synchronize replication slot \"%s\"",
+					   remote_slot->name),
+				errdetail("Synchronization could lead to data loss, because the standby could not build a consistent snapshot to decode WALs at LSN %X/%08X.",
+						  LSN_FORMAT_ARGS(slot->data.restart_lsn)));
+
+		skip_reason = SS_SKIP_NO_CONSISTENT_SNAPSHOT;
+	}
+
 	/* Update slot sync skip stats */
 	update_slotsync_skip_stats(skip_reason);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errdetail`
- 外部函数：`errmsg`
- 外部函数：`pgstat_report_replslotsync`
- 大写宏：`LOG`
- 大写宏：`LSN`
- 大写宏：`LSN_FORMAT_ARGS`
- 大写宏：`NULL`
- 大写宏：`RS_INVAL_NONE`
- 大写宏：`RS_TEMPORARY`
- 大写宏：`SS_SKIP_NONE`
- 大写宏：`SS_SKIP_NO_CONSISTENT_SNAPSHOT`
- 大写宏：`WAL`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`LSNs`
- 外部类型：`Make`
- 外部类型：`MyReplicationSlot`
- 外部类型：`ReplicationSlot`
- 外部类型：`Sanity`
- 外部类型：`See`
- 外部类型：`SlotSyncSkipReason`
- 外部类型：`Synchronization`
- 外部类型：`TransactionId`
- 外部类型：`Update`
- 外部类型：`WALs`
- 外部类型：`XLogRecPtr`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-1aaa3b08e6` → 本草稿移入 `cases/defect/auto-postgres-1aaa3b08e6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
