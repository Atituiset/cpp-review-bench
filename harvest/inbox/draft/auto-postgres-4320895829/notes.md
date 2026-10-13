# auto-postgres-4320895829

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
| 外部依赖数（dep_count） | 118 |
| 编译错误数（gcc syntax-only） | 27（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #44539786b8296b6e73035142670e0d1ff2639078 (https://github.com/postgres/postgres/commit/44539786b8296b6e73035142670e0d1ff2639078)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1135；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 44539786b8296b6e73035142670e0d1ff2639078 Allow a stats kind to use its own dshash in pgstats :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -66,7 +66,7 @@ const ShmemCallbacks StatsShmemCallbacks = {
 	.init_fn = StatsShmemInit,
 };
 
-/* parameter for the shared hash */
+/* Parameters for shared hashtables. */
 static const dshash_parameters dsh_params = {
 	sizeof(PgStat_HashKey),
 	sizeof(PgStatShared_HashEntry),
@@ -105,8 +105,8 @@ static MemoryContext pgStatEntryRefHashContext = NULL;
  */
 
 /*
- * The size of the shared memory allocation for stats stored in the shared
- * stats hash table. This allocation will be done as part of the main shared
+ * The size of the shared memory allocation for stats stored in a stats
+ * hashtable. This allocation will be done as part of the main shared
  * memory, rather than dynamic shared memory, allowing it to be initialized in
  * postmaster.
  */
@@ -139,6 +139,17 @@ StatsShmemSize(void)
 	sz = MAXALIGN(sizeof(PgStat_ShmemControl));
 	sz = add_size(sz, pgstat_dsa_init_size());
 
+	/* Add per-kind DSA space for variable-numbered own_hash kinds. */
+	for (PgStat_Kind kind = PGSTAT_KIND_MIN; kind <= PGSTAT_KIND_MAX; kind++)
+	{
+		const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);
+
+		if (!kind_info || kind_info->fixed_amount || !kind_info->own_hash)
+			continue;
+
+		sz = add_size(sz, pgstat_dsa_init_size());
+	}
+
 	/* Add shared memory for all the custom fixed-numbered statistics */
 	for (PgStat_Kind kind = PGSTAT_KIND_CUSTOM_MIN; kind <= PGSTAT_KIND_CUSTOM_MAX; kind++)
 	{
@@ -210,6 +221,41 @@ StatsShmemInit(void *arg)
 	/* lift limit set above */
 	dsa_set_size_limit(dsa, -1);
 
+	/*
+	 * Create dedicated DSA areas and hash tables for kinds that have
+	 * variable-numbered stats and set own_hash to true.
+	 */
+	memset(ctl->raw_kind_dsa_area, 0, sizeof(ctl->raw_kind_dsa_area));
+	for (PgStat_Kind kind = PGSTAT_KIND_MIN; kind <= PGSTAT_KIND_MAX; kind++)
+	{
+		const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);
+		dsa_area   *kind_dsa;
+		dshash_table *kind_dsh;
+
+		if (!kind_info || kind_info->fixed_amount || !kind_info->own_hash)
+			continue;
+
+		/* Create a per-kind DSA in pre-allocated shared memory */
+		ctl->raw_kind_dsa_area[kind] = p;
+		p += pgstat_dsa_init_size();
+		kind_dsa = dsa_create_in_place(ctl->raw_kind_dsa_area[kind],
+									   pgstat_dsa_init_size(),
+									   LWTRANCHE_PGSTATS_DSA, NULL);
+		dsa_pin(kind_dsa);
+
+		/* Temporarily limit to keep dshash in the in-place area */
+		dsa_set_size_limit(kind_dsa, pgstat_dsa_init_size());
+
+		kind_dsh = dshash_create(kind_dsa, &dsh_params, NULL);
+		ctl->kind_hash_handles[kind] = dshash_get_hash_table_handle(kind_dsh);
+
+		dsa_set_size_limit(kind_dsa, -1);
+
+		/* detach, as postmaster never accesses them again. */
+		dshash_detach(kind_dsh);
+		dsa_detach(kind_dsa);
+	}
+
 	/*
 	 * Postmaster will never access these again, thus free the local
 	 * dsa/dshash references.
@@ -255,36 +301,90 @@ StatsShmemInit(void *arg)
 void
 pgstat_attach_shmem(void)
 {
+	dsa_area   *shared_dsa;
+	dshash_table *shared_hash;
 	MemoryContext oldcontext;
 
-	Assert(pgStatLocal.dsa == NULL);
+	Assert(pgStatLocal.num_var_hashes == 0);
 
 	/* stats shared memory persists for the backend lifetime */
 	oldcontext = MemoryContextSwitchTo(TopMemoryContext);
 
-	pgStatLocal.dsa = dsa_attach_in_place(pgStatLocal.shmem->raw_dsa_area,
-										  NULL);
-	dsa_pin_mapping(pgStatLocal.dsa);
+	shared_dsa = dsa_attach_in_place(pgStatLocal.shmem->raw_dsa_area,
+									 NULL);
+	dsa_pin_mapping(shared_dsa);
+
+	shared_hash = dshash_attach(shared_dsa, &dsh_params,
+								pgStatLocal.shmem->hash_handle,
+								NULL);
+
+	/* Build per-kind mappings for the hash tables. */
+	pgStatLocal.var_hashes[pgStatLocal.num_var_hashes++] = shared_hash;
 
-	pgStatLocal.shared_hash = dshash_attach(pgStatLocal.dsa, &dsh_params,
-											pgStatLocal.shmem->hash_handle,
-											NULL);
+	for (PgStat_Kind kind = PGSTAT_KIND_MIN; kind <= PGSTAT_KIND_MAX; kind++)
+	{
+		const PgStat_KindInfo *kind_info = pgstat_get_kind_info(kind);
+
+		if (pgStatLocal.shmem->raw
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`accessed`
- 外部函数：`add_size`
- 外部函数：`bool`
- 外部函数：`dsa_allocate_extended`
- 外部函数：`dsa_attach_in_place`
- 外部函数：`dsa_create_in_place`
- 外部函数：`dsa_detach`
- 外部函数：`dsa_free`
- 外部函数：`dsa_get_address`
- 外部函数：`dsa_minimum_size`
- 外部函数：`dsa_pin`
- 外部函数：`dsa_pin_mapping`
- 外部函数：`dsa_release_in_place`
- 外部函数：`dsa_set_size_limit`
- 外部函数：`dshash_attach`
- 外部函数：`dshash_create`
- 外部函数：`dshash_create_in_place`
- 外部函数：`dshash_delete_current`
- 外部函数：`dshash_delete_entry`
- 外部函数：`dshash_detach`
- 外部函数：`dshash_find`
- 外部函数：`dshash_find_or_insert_extended`
- 外部函数：`dshash_get_hash_table_handle`
- 外部函数：`dshash_release_lock`
- 外部函数：`dshash_seq_init`
- 外部函数：`dshash_seq_next`
- 外部函数：`dshash_seq_term`
- 外部函数：`elog`
- 外部函数：`init_shmem_cb`
- 外部函数：`match`
- 外部函数：`memset`
- 外部函数：`pfree`
- 外部函数：`pg_atomic_fetch_add_u32`
- 外部函数：`pg_atomic_fetch_add_u64`
- 外部函数：`pg_atomic_fetch_sub_u32`
- 外部函数：`pg_atomic_init_u32`
- 外部函数：`pg_atomic_init_u64`
- 外部函数：`pg_atomic_read_u32`
- 外部函数：`pg_atomic_sub_fetch_u32`
- 外部函数：`pg_atomic_sub_fetch_u64`
- 外部函数：`pgstat_assert_is_up`
- 外部函数：`pgstat_delete_pending_entry`
- 外部函数：`pgstat_drop_matching_entries`
- 外部函数：`pgstat_dsa_init_size`
- 外部函数：`pgstat_entry_ref_hash_delete`
- 外部函数：`pgstat_entry_ref_hash_destroy`
- 外部函数：`pgstat_entry_ref_hash_iterate`
- 外部函数：`pgstat_entry_ref_hash_lookup`
- 外部函数：`pgstat_entry_ref_hash_start_iterate`
- 外部函数：`pgstat_get_entry_data`
- 外部函数：`pgstat_get_entry_len`
- 外部函数：`pgstat_get_kind_info`
- 外部函数：`pgstat_is_kind_builtin`
- 外部函数：`pgstat_reset_matching_entries`
- 外部函数：`reset_timestamp_cb`
- 外部函数：`unlikely`
- 大写宏：`ALLOCSET_SMALL_SIZES`
- 大写宏：`DSA`
- 大写宏：`DSA_ALLOC_NO_OOM`
- 大写宏：`DSA_ALLOC_ZERO`
- 大写宏：`DSHASH_INSERT_NO_OOM`
- 大写宏：`ERROR`
- 大写宏：`LWTRANCHE_PGSTATS_DATA`
- 大写宏：`LWTRANCHE_PGSTATS_DSA`
- 大写宏：`LW_EXCLUSIVE`
- 大写宏：`MAXALIGN`
- 大写宏：`NULL`
- 大写宏：`PGSTAT_KIND_CUSTOM_MAX`
- 大写宏：`PGSTAT_KIND_CUSTOM_MIN`
- 大写宏：`PGSTAT_KIND_MAX`
- 大写宏：`PGSTAT_KIND_MIN`
- 大写宏：`XXX`
- 外部类型：`Add`
- 外部类型：`Caller`
- 外部类型：`Clean`
- 外部类型：`Concurrent`
- 外部类型：`Create`
- 外部类型：`DSMs`
- 外部类型：`Datum`
- 外部类型：`Decrement`
- 外部类型：`Do`
- 外部类型：`Doing`
- 外部类型：`Ensure`
- 外部类型：`Even`
- 外部类型：`Fetch`
- 外部类型：`Hash`
- 外部类型：`If`
- 外部类型：`Increment`
- 外部类型：`Initialize`
- 外部类型：`InvalidDsaPointer`
- 外部类型：`It`
- 外部类型：`LWLock`
- 外部类型：`Link`
- 外部类型：`MemoryContext`
- 外部类型：`Oid`
- 外部类型：`PgStat`
- 外部类型：`PgStatShared_Common`
- 外部类型：`PgStatShared_HashEntry`
- 外部类型：`PgStat_EntryRef`
- 外部类型：`PgStat_EntryRefHashEntry`
- 外部类型：`PgStat_HashKey`
- 外部类型：`PgStat_Kind`
- 外部类型：`PgStat_KindInfo`
- 外部类型：`PgStat_ShmemControl`
- 外部类型：`Postmaster`
- 外部类型：`Ref`
- 外部类型：`ReleaseMatchCB`
- 外部类型：`Reset`
- 外部类型：`Same`
- 外部类型：`Scan`
- 外部类型：`Shared`
- 外部类型：`Signal`
- 外部类型：`Size`
- 外部类型：`StatsShmemInit`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`TimestampTz`
- 外部类型：`To`
- 外部类型：`TopMemoryContext`
- 外部类型：`We`
- 外部类型：`With`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-4320895829` → 本草稿移入 `cases/defect/auto-postgres-4320895829/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
