# auto-postgres-ac0dcaf7a9

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
| 外部依赖数（dep_count） | 51 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #44539786b8296b6e73035142670e0d1ff2639078 (https://github.com/postgres/postgres/commit/44539786b8296b6e73035142670e0d1ff2639078)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 556；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 44539786b8296b6e73035142670e0d1ff2639078 Allow a stats kind to use its own dshash in pgstats :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -25,27 +25,27 @@
 /*
  * Types related to shared memory storage of statistics.
  *
- * Per-object statistics are stored in the "shared stats" hashtable. That
- * table's entries (PgStatShared_HashEntry) contain a pointer to the actual stats
- * data for the object (the size of the stats data varies depending on the
- * kind of stats). The table is keyed by PgStat_HashKey.
+ * Per-object statistics are stored in shared stats hashtables. Those tables'
+ * entries (PgStatShared_HashEntry) contain a pointer to the actual stats data
+ * for the object (the size of the stats data varies depending on the kind of
+ * stats). The tables are keyed by PgStat_HashKey.
  *
  * Once a backend has a reference to a shared stats entry, it increments the
  * entry's refcount. Even after stats data is dropped (e.g., due to a DROP
  * TABLE), the entry itself can only be deleted once all references have been
  * released.
  *
  * These refcounts, in combination with a backend local hashtable
- * (pgStatEntryRefHash, with entries pointing to PgStat_EntryRef) in front of
- * the shared hash table, mean that most stats work can happen without
- * touching the shared hash table, reducing contention.
+ * (pgStatEntryRefHash, with entries pointing to PgStat_EntryRef) that caches
+ * references to shared stats hashtable entries, mean that most stats work can
+ * happen without touching the shared stats hashtables, reducing contention.
  *
  * Once there are pending stats updates for a table PgStat_EntryRef->pending
  * is allocated to contain a working space for as-of-yet-unapplied stats
  * updates. Once the stats are flushed, PgStat_EntryRef->pending is freed.
  *
- * Each stat kind in the shared hash table has a fixed member
- * PgStatShared_Common as the first element.
+ * Each variable-numbered shared stats struct has PgStatShared_Common as the
+ * first element.
  */
 
 /*
@@ -186,7 +186,7 @@ typedef struct PgStat_EntryRef
 	 * stats eventually. Each stats kind utilizing pending data defines what
 	 * format its pending data has and needs to provide a
 	 * PgStat_KindInfo->flush_pending_cb callback to merge pending entries
-	 * into the shared stats hash table.
+	 * into shared stats hashtable entries.
 	 */
 	void	   *pending;
 	dlist_node	pending_node;	/* membership in pgStatPending list */
@@ -252,9 +252,19 @@ typedef struct PgStat_KindInfo
 	bool		track_entry_count:1;
 
 	/*
-	 * The size of an entry in the shared stats hash table (pointed to by
-	 * PgStatShared_HashEntry->body).  For fixed-numbered statistics, this is
-	 * the size of an entry in PgStat_ShmemControl->custom_data.
+	 * Should entries of this kind be stored in a dedicated dshash table
+	 * rather than the shared hash table? For variable-numbered stats only.
+	 *
+	 * A kind setting this option creates its own DSA area in the main shared
+	 * memory.  This is worth only for kinds expected to hold a large number
+	 * of entries.
+	 */
+	bool		own_hash:1;
+
+	/*
+	 * The size of an entry pointed to by PgStatShared_HashEntry->body.  For
+	 * fixed-numbered statistics, this is the size of an entry in
+	 * PgStat_ShmemControl->custom_data.
 	 */
 	uint32		shared_size;
 
@@ -541,19 +551,20 @@ typedef struct PgStatShared_Backend
 /*
  * Central shared memory entry for the cumulative stats system.
  *
- * Fixed amount stats, the dynamic shared memory hash table for
+ * Fixed amount stats, the dynamic shared memory hashtables for
  * non-fixed-amount stats, as well as remaining bits and pieces are all
  * reached from here.
  */
 typedef struct PgStat_ShmemControl
 {
 	void	   *raw_dsa_area;
 
-	/*
-	 * Stats for variable-numbered objects are kept in this shared hash table.
-	 * See comment above PgStat_Kind for details.
-	 */
-	dshash_table_handle hash_handle;	/* shared dbstat hash */
+	/* Shared hash for variable-numbered stats kinds without own_hash set. */
+	dshash_table_handle hash_handle;	/* shared stats hash */
+
+	/* Dedicated hash and DSA for variable-numbered kinds with own
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`created`
- 外部函数：`dropped`
- 外部函数：`entries`
- 外部函数：`lengthof`
- 外部函数：`object`
- 外部函数：`table`
- 大写宏：`DROP`
- 大写宏：`PGSTAT_KIND_BUILTIN_SIZE`
- 大写宏：`PGSTAT_KIND_CUSTOM_MIN`
- 大写宏：`PGSTAT_KIND_CUSTOM_SIZE`
- 大写宏：`PGSTAT_KIND_MAX`
- 大写宏：`TABLE`
- 外部类型：`Central`
- 外部类型：`Counters`
- 外部类型：`Data`
- 外部类型：`Each`
- 外部类型：`Even`
- 外部类型：`Fixed`
- 外部类型：`For`
- 外部类型：`Has`
- 外部类型：`Just`
- 外部类型：`MemoryContext`
- 外部类型：`Once`
- 外部类型：`Per`
- 外部类型：`PgStatShared_Common`
- 外部类型：`PgStatShared_HashEntry`
- 外部类型：`PgStat_ArchiverStats`
- 外部类型：`PgStat_BgWriterStats`
- 外部类型：`PgStat_CheckpointerStats`
- 外部类型：`PgStat_EntryRef`
- 外部类型：`PgStat_FetchConsistency`
- 外部类型：`PgStat_HashKey`
- 外部类型：`PgStat_IO`
- 外部类型：`PgStat_Kind`
- 外部类型：`PgStat_KindInfo`
- 外部类型：`PgStat_LocalState`
- 外部类型：`PgStat_Lock`
- 外部类型：`PgStat_SLRUStats`
- 外部类型：`PgStat_ShmemControl`
- 外部类型：`PgStat_Snapshot`
- 外部类型：`PgStat_WalStats`
- 外部类型：`See`
- 外部类型：`Stats`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`TimestampTz`
- 外部类型：`TopMemoryContext`
- 外部类型：`Types`
- 外部类型：`pgstat_snapshot_hash`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ac0dcaf7a9` → 本草稿移入 `cases/defect/auto-postgres-ac0dcaf7a9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
