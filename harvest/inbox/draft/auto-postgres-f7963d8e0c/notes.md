# auto-postgres-f7963d8e0c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1813f951cad59ea7f86063831d0222c429989c91](https://github.com/postgres/postgres/commit/1813f951cad59ea7f86063831d0222c429989c91) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-29 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 114 |
| 编译错误数（gcc syntax-only） | 23（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1813f951cad59ea7f86063831d0222c429989c91 (https://github.com/postgres/postgres/commit/1813f951cad59ea7f86063831d0222c429989c91)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1436；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1813f951cad59ea7f86063831d0222c429989c91 Lock the TOAST table early in REPACK (CONCURRENTLY) :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -536,6 +536,16 @@ cluster_rel(RepackCommand cmd, Relation OldHeap, Oid indexOid,
 	if (concurrent)
 		check_concurrent_repack_requirements(OldHeap, &ident_idx);
 
+	/*
+	 * In concurrent mode, also lock the toast table.  Otherwise it would be
+	 * possible for the toast relfilenode to change (e.g. because VACUUM FULL
+	 * or REPACK is run on it).  This would break concurrent repack's system
+	 * for skipping decoding changes in other tables -- see
+	 * change_useless_for_repack().
+	 */
+	if (concurrent && OidIsValid(OldHeap->rd_rel->reltoastrelid))
+		LockRelationOid(OldHeap->rd_rel->reltoastrelid, lmode);
+
 	/*
 	 * Also check the state of indexes; this can abort the command for REPACK.
 	 * Historically this hasn't affected CLUSTER or VACUUM FULL, so don't do
@@ -1136,17 +1146,23 @@ rebuild_relation(Relation OldHeap, Relation index, bool verbose,
 		 */
 		BecomeLockGroupLeader();
 
+		/*
+		 * If there is a toast table, it must have been locked already.
+		 * Otherwise we risk it changing underneath us (catastrophic).
+		 */
+		Assert(!OidIsValid(OldHeap->rd_rel->reltoastrelid) ||
+			   CheckRelationOidLockedByMe(OldHeap->rd_rel->reltoastrelid,
+										  lmode, false));
+
 		/*
 		 * Start the worker that decodes data changes applied while we're
 		 * copying the table contents.
 		 *
 		 * Note that the worker has to wait for all transactions with XID
 		 * already assigned to finish. If some of those transactions is
 		 * waiting for a lock conflicting with ShareUpdateExclusiveLock on our
-		 * table (e.g.  it runs CREATE INDEX), we can end up in a deadlock.
-		 * Not sure this risk is worth unlocking/locking the table (and its
-		 * clustering index) and checking again if it's still eligible for
-		 * REPACK CONCURRENTLY.
+		 * table or its TOAST relation (e.g. it runs CREATE INDEX), we can end
+		 * up in a deadlock.
 		 */
 		start_repack_decoding_worker(tableOid);
 
@@ -1431,9 +1447,17 @@ copy_table_data(Relation NewHeap, Relation OldHeap, Relation OldIndex,
 	 *
 	 * We don't need to open the toast relation here, just lock it.  The lock
 	 * will be held till end of transaction.
+	 *
+	 * Concurrent repack must hold this lock already; see cluster_rel().
 	 */
 	if (OldHeap->rd_rel->reltoastrelid)
-		LockRelationOid(OldHeap->rd_rel->reltoastrelid, lmode);
+	{
+		if (!concurrent)
+			LockRelationOid(OldHeap->rd_rel->reltoastrelid, lmode);
+		else
+			CheckRelationOidLockedByMe(OldHeap->rd_rel->reltoastrelid,
+									   lmode, false);
+	}
 
 	/*
 	 * If both tables have TOAST tables, perform toast swap by content.  It is
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cluster_rel`
- 外部函数：`dsm_create`
- 外部函数：`dsm_segment_address`
- 外部函数：`dsm_segment_handle`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errdetail`
- 外部函数：`errdetail_relkind_not_supported`
- 外部函数：`errhint`
- 外部函数：`errmsg`
- 外部函数：`get_rel_name`
- 外部函数：`memset`
- 外部函数：`offsetof`
- 外部函数：`palloc0_object`
- 外部函数：`shm_mq_attach`
- 外部函数：`shm_mq_create`
- 外部函数：`shm_mq_get_queue`
- 外部函数：`shm_mq_get_sender`
- 外部函数：`shm_mq_set_handle`
- 外部函数：`shm_mq_set_receiver`
- 外部函数：`snprintf`
- 外部函数：`table`
- 大写宏：`ALTER`
- 大写宏：`BGWH_STARTED`
- 大写宏：`BGWH_STOPPED`
- 大写宏：`BGWORKER_BACKEND_DATABASE_CONNECTION`
- 大写宏：`BGWORKER_SHMEM_ACCESS`
- 大写宏：`BGW_MAXLEN`
- 大写宏：`BGW_NEVER_RESTART`
- 大写宏：`BUFFERALIGN`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`CLUSTER`
- 大写宏：`CONCURRENTLY`
- 大写宏：`CREATE`
- 大写宏：`ERRCODE_CONFIGURATION_LIMIT_EXCEEDED`
- 大写宏：`ERRCODE_FEATURE_NOT_SUPPORTED`
- 大写宏：`ERRCODE_INVALID_PARAMETER_VALUE`
- 大写宏：`ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE`
- 大写宏：`ERROR`
- 大写宏：`FULL`
- 大写宏：`IDENTITY`
- 大写宏：`INDEX`
- 大写宏：`MAXPGPATH`
- 大写宏：`MVCC`
- 大写宏：`NOTHING`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`RELKIND_MATVIEW`
- 大写宏：`RELPERSISTENCE_PERMANENT`
- 大写宏：`REPACK`
- 大写宏：`REPACK_ERROR_QUEUE_SIZE`
- 大写宏：`REPLICA`
- 大写宏：`REPLICA_IDENTITY_FULL`
- 大写宏：`REPLICA_IDENTITY_NOTHING`
- 大写宏：`TABLE`
- 大写宏：`TOAST`
- 大写宏：`USING`
- 大写宏：`VACUUM`
- 大写宏：`WAIT_EVENT_BGWORKER_STARTUP`
- 大写宏：`WAIT_EVENT_REPACK_WORKER_EXPORT`
- 大写宏：`WAL`
- 大写宏：`WAL_LEVEL_REPLICA`
- 大写宏：`WL_EXIT_ON_PM_DEATH`
- 大写宏：`WL_LATCH_SET`
- 大写宏：`XID`
- 外部类型：`Also`
- 外部类型：`BackgroundWorker`
- 外部类型：`BackgroundWorkerHandle`
- 外部类型：`BgWorkerStart_RecoveryFinished`
- 外部类型：`BgwHandleStatus`
- 外部类型：`Check`
- 外部类型：`Data`
- 外部类型：`DecodingWorker`
- 外部类型：`DecodingWorkerShared`
- 外部类型：`Handle`
- 外部类型：`Has`
- 外部类型：`Historically`
- 外部类型：`If`
- 外部类型：`InvalidXLogRecPtr`
- 外部类型：`It`
- 外部类型：`LockTimeout`
- 外部类型：`Make`
- 外部类型：`More`
- 外部类型：`MyDatabaseId`
- 外部类型：`MyLatch`
- 外部类型：`MyProc`
- 外部类型：`MyProcNumber`
- 外部类型：`MyProcPid`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`Nothing`
- 外部类型：`Now`
- 外部类型：`Obtain`
- 外部类型：`Oid`
- 外部类型：`OldHeap`
- 外部类型：`Otherwise`
- 外部类型：`Relation`
- 外部类型：`Removing`
- 外部类型：`RepackWorkerMain`
- 外部类型：`Replica`
- 外部类型：`Security`
- 外部类型：`Setup`
- 外部类型：`ShareUpdateExclusiveLock`
- 外部类型：`Size`
- 外部类型：`Start`
- 外部类型：`TableAmRoutine`
- 外部类型：`The`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`TransactionTimeout`
- 外部类型：`Transmit`
- 外部类型：`Use`
- 外部类型：`UserId`
- 外部类型：`We`
- 外部类型：`With`
- 外部类型：`Worker`
- 外部类型：`You`
- 外部类型：`pid_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f7963d8e0c` → 本草稿移入 `cases/defect/auto-postgres-f7963d8e0c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
