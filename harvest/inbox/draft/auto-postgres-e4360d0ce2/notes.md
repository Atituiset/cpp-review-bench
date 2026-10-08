# auto-postgres-e4360d0ce2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1f2245371e31f0def347546f5ad250552a4eb8a4](https://github.com/postgres/postgres/commit/1f2245371e31f0def347546f5ad250552a4eb8a4) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 21 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1f2245371e31f0def347546f5ad250552a4eb8a4 (https://github.com/postgres/postgres/commit/1f2245371e31f0def347546f5ad250552a4eb8a4)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 743；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1f2245371e31f0def347546f5ad250552a4eb8a4 Fix slotsync when logical decoding is disabled and re-enabled. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -718,41 +718,6 @@ update_and_persist_local_synced_slot(RemoteSlot *remote_slot, Oid remote_dbid,
 		return false;
 	}
 
-	/*
-	 * Do not persist the slot if logical decoding got disabled concurrently.
-	 * This can happen if the last logical slot on the primary was dropped and
-	 * the corresponding XLOG_LOGICAL_DECODING_STATUS_CHANGE record was
-	 * replayed after we fetched the remote slot information: WAL records
-	 * following the slot's restart_lsn might lack the information required by
-	 * logical decoding, and the slot invalidation performed when replaying
-	 * the record could not find our slot as it was not created yet.
-	 *
-	 * It is important to perform this check after creating the slot and
-	 * before persisting it. This way, even if the status change record is
-	 * replayed after this check, the replay will invalidate our slot.
-	 *
-	 * If the check fails, we keep the temporary slot and let the caller
-	 * retry; the next cycle fetches the remote slot information again and
-	 * will drop this slot as the remote slot no longer exists.
-	 *
-	 * XXX: this check cannot detect the case where logical decoding is
-	 * already re-enabled by a slot creation on the primary at this point.
-	 * Detecting that would require comparing the slot's restart_lsn with the
-	 * LSN at which logical decoding was last enabled.
-	 */
-	if (!IsLogicalDecodingEnabled())
-	{
-		ereport(LOG,
-				errmsg("could not synchronize replication slot \"%s\"",
-					   remote_slot->name),
-				errdetail("Logical decoding was concurrently disabled."));
-
-		if (slot_persistence_pending)
-			*slot_persistence_pending = true;
-
-		return false;
-	}
-
 	ReplicationSlotPersist();
 
 	ereport(LOG,
@@ -883,6 +848,7 @@ synchronize_one_slot(RemoteSlot *remote_slot, Oid remote_dbid,
 	{
 		NameData	plugin_name;
 		TransactionId xmin_horizon = InvalidTransactionId;
+		XLogRecPtr	replay_lsn;
 
 		/* Skip creating the local slot if remote_slot is invalidated already */
 		if (remote_slot->invalidated != RS_INVAL_NONE)
@@ -901,6 +867,63 @@ synchronize_one_slot(RemoteSlot *remote_slot, Oid remote_dbid,
 							  remote_slot->failover,
 							  true);
 
+		/*
+		 * The remote slot information can predate a status change record that
+		 * this standby has already replayed. That happens when the last
+		 * logical slot on the primary is dropped, and possibly re-created
+		 * with the same name, after fetch_remote_slots() ran. The resulting
+		 * deactivation could not invalidate our slot because it did not exist
+		 * yet, and WAL following the (stale) remote restart_lsn may lack the
+		 * information logical decoding needs. Checking only whether logical
+		 * decoding is enabled is not enough, as it can have been disabled and
+		 * enabled again in the meantime.
+		 *
+		 * The check has to come after ReplicationSlotCreate(), which makes
+		 * the slot both visible and acquired. A deactivation replayed from
+		 * here on finds the slot in InvalidatePossiblyObsoleteSlot(), signals
+		 * a recovery conflict and waits for the slot to be released before
+		 * invalidating it (only in hot standby, which slot synchronization
+		 * requires anyway). Replay therefore cannot get past that record
+		 * behind our back, so the slot never needs to be rechecked before
+		 * being persisted.
+		 *
+		 * The check only runs once replay has reached the remote restart_lsn;
+		 * otherwise it is skipped and the slot is kept as-is. Without this, a
+		 * standby lagging behind the primary (replay paused, or a large
+		 * recovery_min_apply_delay) could fetch a live, valid restart_lsn
+		 * from the primary and have it rejected by
+		 * StandbyLogicalDecodingEnabledSince(), whose answer reflects only
+		 * WAL replayed so far and says nothing about an LSN replay hasn't
+		 * reached yet. That would drop a perfectly good slot every cycle.
+		 * Skipping the check is safe: only a deactivation at or after the
+		 * remote restart_lsn can leave the WAL that follows it without the
+	
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errdetail`
- 外部函数：`errmsg`
- 大写宏：`LOG`
- 大写宏：`LSN`
- 大写宏：`RS_INVAL_NONE`
- 大写宏：`WAL`
- 大写宏：`XLOG_LOGICAL_DECODING_STATUS_CHANGE`
- 大写宏：`XXX`
- 外部类型：`Detecting`
- 外部类型：`Do`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`InvalidTransactionId`
- 外部类型：`It`
- 外部类型：`Logical`
- 外部类型：`MyReplicationSlot`
- 外部类型：`NameData`
- 外部类型：`Skip`
- 外部类型：`This`
- 外部类型：`TransactionId`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-e4360d0ce2` → 本草稿移入 `cases/defect/auto-postgres-e4360d0ce2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
