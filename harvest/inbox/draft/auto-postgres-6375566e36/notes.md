# auto-postgres-6375566e36

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#af090508018be5ace443ba6e2814500c10842a4d](https://github.com/postgres/postgres/commit/af090508018be5ace443ba6e2814500c10842a4d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #af090508018be5ace443ba6e2814500c10842a4d (https://github.com/postgres/postgres/commit/af090508018be5ace443ba6e2814500c10842a4d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 2435；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT af090508018be5ace443ba6e2814500c10842a4d Warn when REPACK or CLUSTER skips foreign partitions :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2432,7 +2432,46 @@ get_tables_to_repack_partitioned(RepackStmt *stmt, Relation rel,
 	 * Do not lock the children until they're processed.  Note that we do hold
 	 * a lock on the parent partitioned table.
 	 */
-	inhoids = find_all_inheritors(relid, NoLock, NULL);
+	inhoids = find_all_inheritors(RelationGetRelid(rel), NoLock, NULL);
+
+	/*
+	 * Warn about foreign-table partitions, which have no local storage and
+	 * would otherwise be skipped silently.  This has to look at the table's
+	 * partition tree: when an index is specified, the walk below covers the
+	 * index's partition tree, which cannot contain foreign tables.  Check
+	 * privileges first, like VACUUM does, so that a user who may not process
+	 * the partition gets the permission warning instead.
+	 */
+	foreach_oid(tableoid, inhoids)
+	{
+		char	   *relname;
+
+		if (get_rel_relkind(tableoid) != RELKIND_FOREIGN_TABLE ||
+			!repack_is_permitted_for_relation(stmt->command, tableoid,
+											  GetUserId(), false))
+			continue;
+
+		/*
+		 * We don't hold a lock on the partition's immediate parent if it's
+		 * below the first level, so it may have been dropped concurrently.
+		 */
+		relname = get_rel_name(tableoid);
+		if (relname == NULL)
+			continue;
+
+		ereport(WARNING,
+		/*- translator: second %s is the name of a SQL command, eg. REPACK */
+				errmsg("skipping \"%s\" --- cannot execute %s on foreign tables",
+					   relname, RepackCommandAsString(stmt->command)));
+		pfree(relname);
+	}
+
+	if (rel_is_index)
+	{
+		list_free(inhoids);
+		inhoids = find_all_inheritors(relid, NoLock, NULL);
+	}
+
 	foreach_oid(child_oid, inhoids)
 	{
 		Oid			table_oid,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`find_all_inheritors`
- 大写宏：`NULL`
- 外部类型：`Do`
- 外部类型：`NoLock`
- 外部类型：`Note`
- 外部类型：`Oid`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-6375566e36` → 本草稿移入 `cases/defect/auto-postgres-6375566e36/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
