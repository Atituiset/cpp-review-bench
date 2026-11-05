# auto-postgres-4bcfdc0794

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b](https://github.com/postgres/postgres/commit/31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b (https://github.com/postgres/postgres/commit/31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1235；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 31f89191ff7b22b6c69ae5f3c6ab48c5d8bb0f2b Add stats kind filter to pgstat_drop_matching_entries() :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1229,25 +1229,53 @@ pgstat_drop_entry(PgStat_Kind kind, Oid dboid, uint64 objid,
 
 /*
  * Scan through the stats hashtables, dropping statistics if approved by the
- * optional do_drop() function.
+ * optional do_drop() function, or all entries if do_drop is NULL.
+ *
+ * If kind is PGSTAT_KIND_INVALID, all stats hashtables are scanned.  Otherwise,
+ * only the hashtable of the specified kind is scanned.
  */
 void
-pgstat_drop_matching_entries(bool (*do_drop) (PgStatShared_HashEntry *, Datum),
+pgstat_drop_matching_entries(PgStat_Kind kind,
+							 bool (*do_drop) (PgStatShared_HashEntry *, Datum),
 							 Datum match_data)
 {
+	bool		filter_by_kind = (kind != PGSTAT_KIND_INVALID);
+	dshash_table **hashes;
+	int			num_hashes;
 	dshash_seq_status hstat;
 	PgStatShared_HashEntry *ps;
 	uint64		not_freed_count = 0;
 
-	for (int h = 0; h < pgStatLocal.num_var_hashes; h++)
+	if (filter_by_kind)
+	{
+		Assert(kind >= PGSTAT_KIND_MIN && kind <= PGSTAT_KIND_MAX);
+		Assert(pgStatLocal.kind_hash[kind] != NULL);
+
+		hashes = &pgStatLocal.kind_hash[kind];
+		num_hashes = 1;
+	}
+	else
+	{
+		hashes = pgStatLocal.var_hashes;
+		num_hashes = pgStatLocal.num_var_hashes;
+	}
+
+	for (int h = 0; h < num_hashes; h++)
 	{
 		/* entries are removed, take an exclusive lock */
-		dshash_seq_init(&hstat, pgStatLocal.var_hashes[h], true);
+		dshash_seq_init(&hstat, hashes[h], true);
 		while ((ps = dshash_seq_next(&hstat)) != NULL)
 		{
 			if (ps->dropped)
 				continue;
 
+			/*
+			 * A kind-specific scan may still use the shared hash, which can
+			 * contain entries for other kinds.
+			 */
+			if (filter_by_kind && ps->key.kind != kind)
+				continue;
+
 			if (do_drop != NULL && !do_drop(ps, match_data))
 				continue;
 
@@ -1278,7 +1306,7 @@ pgstat_drop_matching_entries(bool (*do_drop) (PgStatShared_HashEntry *, Datum),
 void
 pgstat_drop_all_entries(void)
 {
-	pgstat_drop_matching_entries(NULL, 0);
+	pgstat_drop_matching_entries(PGSTAT_KIND_INVALID, NULL, 0);
 }
 
 static void
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bool`
- 外部函数：`dshash_seq_init`
- 外部函数：`dshash_seq_next`
- 外部函数：`pgstat_drop_matching_entries`
- 大写宏：`NULL`
- 外部类型：`Datum`
- 外部类型：`PgStatShared_HashEntry`
- 外部类型：`Scan`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-4bcfdc0794` → 本草稿移入 `cases/defect/auto-postgres-4bcfdc0794/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
