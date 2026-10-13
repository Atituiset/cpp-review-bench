# auto-postgres-d82812644c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#44539786b8296b6e73035142670e0d1ff2639078](https://github.com/postgres/postgres/commit/44539786b8296b6e73035142670e0d1ff2639078) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 71 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #44539786b8296b6e73035142670e0d1ff2639078 (https://github.com/postgres/postgres/commit/44539786b8296b6e73035142670e0d1ff2639078)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1218；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 44539786b8296b6e73035142670e0d1ff2639078 Allow a stats kind to use its own dshash in pgstats :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -18,25 +18,24 @@
  * Fixed-numbered stats are stored in plain (non-dynamic) shared memory.
  *
  * Statistics for variable-numbered objects are stored in dynamic shared
- * memory and can be found via a dshash hashtable. The statistics counters are
- * not part of the dshash entry (PgStatShared_HashEntry) directly, but are
- * separately allocated (PgStatShared_HashEntry->body). The separate
- * allocation allows different kinds of statistics to be stored in the same
+ * memory and can be found via shared stats dshash hashtables. The statistics
+ * counters are not part of the dshash entry (PgStatShared_HashEntry)
+ * directly, but are separately allocated (PgStatShared_HashEntry->body). The
+ * separate allocation allows different kinds of statistics to share a
  * hashtable without wasting space in PgStatShared_HashEntry.
  *
  * Variable-numbered stats are addressed by PgStat_HashKey while running.  It
  * is not possible to have statistics for an object that cannot be addressed
  * that way at runtime. A wider identifier can be used when serializing to
  * disk (used for replication slot stats).
  *
- * To avoid contention on the shared hashtable, each backend has a
- * backend-local hashtable (pgStatEntryRefHash) in front of the shared
- * hashtable, containing references (PgStat_EntryRef) to shared hashtable
- * entries. The shared hashtable only needs to be accessed when no prior
- * reference is found in the local hashtable. Besides pointing to the
- * shared hashtable entry (PgStatShared_HashEntry) PgStat_EntryRef also
- * contains a pointer to the shared statistics data, as a process-local
- * address, to reduce access costs.
+ * To avoid contention on the shared stats hashtables, each backend has a
+ * backend-local hashtable (pgStatEntryRefHash) containing references
+ * (PgStat_EntryRef) to shared stats hashtable entries. The shared stats
+ * hashtables only need to be accessed when no prior reference is found in the
+ * local hashtable. Besides pointing to the shared stats hashtable entry
+ * (PgStatShared_HashEntry), PgStat_EntryRef also contains a process-local
+ * pointer to the shared statistics data, to reduce access costs.
  *
  * The names for structs stored in shared memory are prefixed with
  * PgStatShared instead of PgStat. Each stats entry in shared memory is
@@ -1199,52 +1198,57 @@ pgstat_build_snapshot(void)
 	/*
 	 * Snapshot all variable stats.
 	 */
-	dshash_seq_init(&hstat, pgStatLocal.shared_hash, false);
-	while ((p = dshash_seq_next(&hstat)) != NULL)
+	for (int h = 0; h < pgStatLocal.num_var_hashes; h++)
 	{
-		PgStat_Kind kind = p->key.kind;
-		const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);
-		bool		found;
-		PgStat_SnapshotEntry *entry;
-		PgStatShared_Common *stats_data;
-
-		/*
-		 * Check if the stats object should be included in the snapshot.
-		 * Unless the stats kind can be accessed from all databases (e.g.,
-		 * database stats themselves), we only include stats for the current
-		 * database or objects not associated with a database (e.g. shared
-		 * relations).
-		 */
-		if (p->key.dboid != MyDatabaseId &&
-			p->key.dboid != InvalidOid &&
-			!kind_info->accessed_across_databases)
-			continue;
+		dshash_table *hash = pgStatLocal.var_hashes[h];
 
-		if (p->dropped)
-			continue;
+		dshash_seq_init(&hstat, hash, false);
+		while ((p = dshash_seq_next(&hstat)) != NULL)
+		{
+			PgStat_Kind kind = p->key.kind;
+			const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);
+			bool		found;
+			PgStat_SnapshotEntry *entry;
+			PgStatShared_Common *stats_data;
+
+			/*
+			 * Check if the stats object should be included in the snapshot.
+			 * Unless the stats kind can be accessed from all databases (e.g.,
+			 * database stats themselves), we only include stats for the
+			 * current database or objects not associated with a database
+			 * (e.g. shared relations).
+			 */
+			if (p->key.dboid != MyDatabaseId &&
+				p->key.dboid != InvalidOid &&
+				!kind_inf
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`allocated`
- 外部函数：`database`
- 外部函数：`databases`
- 外部函数：`disk`
- 外部函数：`dsa_get_address`
- 外部函数：`dshash_seq_init`
- 外部函数：`dshash_seq_next`
- 外部函数：`dshash_seq_term`
- 外部函数：`elog`
- 外部函数：`entry`
- 外部函数：`ereport`
- 外部函数：`errhint`
- 外部函数：`errmsg`
- 外部函数：`fwrite`
- 外部函数：`hashtable`
- 外部函数：`memcpy`
- 外部函数：`pg_atomic_read_u32`
- 外部函数：`pg_stat_lock_entry_shared`
- 外部函数：`pgstat_get_entry_data`
- 外部函数：`pgstat_get_entry_len`
- 外部函数：`pgstat_is_kind_builtin`
- 外部函数：`pgstat_is_kind_custom`
- 外部函数：`pgstat_snapshot_insert`
- 外部函数：`plain`
- 外部函数：`references`
- 外部函数：`write_chunk`
- 大写宏：`ERROR`
- 大写宏：`FILE`
- 大写宏：`LW_SHARED`
- 大写宏：`NULL`
- 大写宏：`PGSTAT_KIND_CUSTOM_MIN`
- 大写宏：`WARNING`
- 外部类型：`Acquire`
- 外部类型：`Besides`
- 外部类型：`Build`
- 外部类型：`Check`
- 外部类型：`Custom`
- 外部类型：`Each`
- 外部类型：`Fixed`
- 外部类型：`However`
- 外部类型：`InvalidOid`
- 外部类型：`It`
- 外部类型：`LWLock`
- 外部类型：`MyDatabaseId`
- 外部类型：`PRIu64`
- 外部类型：`PgStat`
- 外部类型：`PgStatShared`
- 外部类型：`PgStatShared_Common`
- 外部类型：`PgStatShared_HashEntry`
- 外部类型：`PgStat_EntryRef`
- 外部类型：`PgStat_H`
- 外部类型：`PgStat_HashKey`
- 外部类型：`PgStat_Kind`
- 外部类型：`PgStat_KindInfo`
- 外部类型：`PgStat_SnapshotEntry`
- 外部类型：`Snapshot`
- 外部类型：`Statistics`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Unless`
- 外部类型：`Variable`
- 外部类型：`Walk`
- 外部类型：`We`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-d82812644c` → 本草稿移入 `cases/defect/auto-postgres-d82812644c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
