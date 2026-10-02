# auto-postgres-9eed081b05

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#80931d131a95ddcddc7b671d4fee7aceeb12a314](https://github.com/postgres/postgres/commit/80931d131a95ddcddc7b671d4fee7aceeb12a314) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 57 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #80931d131a95ddcddc7b671d4fee7aceeb12a314 (https://github.com/postgres/postgres/commit/80931d131a95ddcddc7b671d4fee7aceeb12a314)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 18310；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 80931d131a95ddcddc7b671d4fee7aceeb12a314 pg_dump: Include column statistics for index-backed constraints :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -18175,6 +18175,48 @@ getAttrName(int attrnum, const TableInfo *tblInfo)
 	return NULL;				/* keep compiler quiet */
 }
 
+/*
+ * appendIndexStatTargets
+ *	  append ALTER INDEX ... SET STATISTICS commands for the per-column
+ *	  statistics targets of an index (attstattarget), if any.
+ */
+static void
+appendIndexStatTargets(PQExpBuffer q, const IndxInfo *indxinfo)
+{
+	char	  **indstatcolsarray = NULL;
+	char	  **indstatvalsarray = NULL;
+	int			nstatcols = 0;
+	int			nstatvals = 0;
+
+	if (strlen(indxinfo->indstatcols) == 0 &&
+		strlen(indxinfo->indstatvals) == 0)
+		return;
+
+	if (!parsePGArray(indxinfo->indstatcols, &indstatcolsarray, &nstatcols))
+		pg_fatal("could not parse index statistic columns");
+	if (!parsePGArray(indxinfo->indstatvals, &indstatvalsarray, &nstatvals))
+		pg_fatal("could not parse index statistic values");
+	if (nstatcols != nstatvals)
+		pg_fatal("mismatched number of columns and values for index statistics");
+
+	for (int j = 0; j < nstatcols; j++)
+	{
+		appendPQExpBuffer(q, "ALTER INDEX %s ",
+						  fmtQualifiedDumpable(indxinfo));
+
+		/*
+		 * Note that this is a column number, so no quotes should be used.
+		 */
+		appendPQExpBuffer(q, "ALTER COLUMN %s ",
+						  indstatcolsarray[j]);
+		appendPQExpBuffer(q, "SET STATISTICS %s;\n",
+						  indstatvalsarray[j]);
+	}
+
+	free(indstatcolsarray);
+	free(indstatvalsarray);
+}
+
 /*
  * dumpIndex
  *	  write out to fout a user-defined index
@@ -18209,13 +18251,6 @@ dumpIndex(Archive *fout, const IndxInfo *indxinfo)
 	 */
 	if (!is_constraint)
 	{
-		char	   *indstatcols = indxinfo->indstatcols;
-		char	   *indstatvals = indxinfo->indstatvals;
-		char	  **indstatcolsarray = NULL;
-		char	  **indstatvalsarray = NULL;
-		int			nstatcols = 0;
-		int			nstatvals = 0;
-
 		if (dopt->binary_upgrade)
 			binary_upgrade_set_pg_class_oids(fout, q,
 											 indxinfo->dobj.catId.oid);
@@ -18239,35 +18274,8 @@ dumpIndex(Archive *fout, const IndxInfo *indxinfo)
 							  qindxname);
 		}
 
-		/*
-		 * If the index has any statistics on some of its columns, generate
-		 * the associated ALTER INDEX queries.
-		 */
-		if (strlen(indstatcols) != 0 || strlen(indstatvals) != 0)
-		{
-			int			j;
-
-			if (!parsePGArray(indstatcols, &indstatcolsarray, &nstatcols))
-				pg_fatal("could not parse index statistic columns");
-			if (!parsePGArray(indstatvals, &indstatvalsarray, &nstatvals))
-				pg_fatal("could not parse index statistic values");
-			if (nstatcols != nstatvals)
-				pg_fatal("mismatched number of columns and values for index statistics");
-
-			for (j = 0; j < nstatcols; j++)
-			{
-				appendPQExpBuffer(q, "ALTER INDEX %s ", qqindxname);
-
-				/*
-				 * Note that this is a column number, so no quotes should be
-				 * used.
-				 */
-				appendPQExpBuffer(q, "ALTER COLUMN %s ",
-								  indstatcolsarray[j]);
-				appendPQExpBuffer(q, "SET STATISTICS %s;\n",
-								  indstatvalsarray[j]);
-			}
-		}
+		/* Per-column statistics targets, if any */
+		appendIndexStatTargets(q, indxinfo);
 
 		/* Indexes can depend on extensions */
 		append_depends_on_extension(fout, q, &indxinfo->dobj,
@@ -18306,9 +18314,6 @@ dumpIndex(Archive *fout, const IndxInfo *indxinfo)
 									  .section = SECTION_POST_DATA,
 									  .createStmt = q->data,
 									  .dropStmt = delq->data));
-
-		free(indstatcolsarray);
-		free(indstatvalsarray);
 	}
 
 	/* Dump Index Comments */
@@ -18860,6 +18865,9 @@ dumpConstraint(Archive *fout, const ConstraintInfo *coninfo)
 							  fmtId(indxinfo->dobj.name));
 		}
 
+		/* Per-column statistics targets, if any */
+		appendIndexStatTargets(q, indxinfo);
+
 		/* If the index defines identity, we need to record that. */
 		if (indxinfo->indisreplident)
 		{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`appendPQExpBuffer`
- 外部函数：`appendPQExpBufferChar`
- 外部函数：`appendPQExpBufferStr`
- 外部函数：`binary_upgrade_set_next_heap_pg_class_oid`
- 外部函数：`binary_upgrade_set_next_heap_relfilenode`
- 外部函数：`binary_upgrade_set_next_index_pg_class_oid`
- 外部函数：`binary_upgrade_set_next_index_relfilenode`
- 外部函数：`binary_upgrade_set_next_toast_chunk_id_typoid`
- 外部函数：`binary_upgrade_set_next_toast_pg_class_oid`
- 外部函数：`binary_upgrade_set_next_toast_relfilenode`
- 外部函数：`bsearch`
- 外部函数：`createPQExpBuffer`
- 外部函数：`destroyPQExpBuffer`
- 外部函数：`fmtId`
- 外部函数：`free`
- 外部函数：`parsePGArray`
- 外部函数：`pg_cmp_u32`
- 外部函数：`pg_fatal`
- 外部函数：`pg_free`
- 外部函数：`pg_strdup`
- 外部函数：`strlen`
- 大写宏：`ALTER`
- 大写宏：`AND`
- 大写宏：`COLUMN`
- 大写宏：`DEPENDS`
- 大写宏：`EXTENSION`
- 大写宏：`FROM`
- 大写宏：`INDEX`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`PGRES_TUPLES_OK`
- 大写宏：`RELKIND_INDEX`
- 大写宏：`RELKIND_PARTITIONED_INDEX`
- 大写宏：`RELKIND_PARTITIONED_TABLE`
- 大写宏：`SECTION_POST_DATA`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`STATISTICS`
- 大写宏：`TOAST`
- 大写宏：`WHERE`
- 外部类型：`Also`
- 外部类型：`Archive`
- 外部类型：`BinaryUpgradeClassOidItem`
- 外部类型：`BinaryUpgradeClassOidItemCmp`
- 外部类型：`By`
- 外部类型：`Comments`
- 外部类型：`Dump`
- 外部类型：`DumpableObject`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Index`
- 外部类型：`Indexes`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`Oid`
- 外部类型：`One`
- 外部类型：`PGresult`
- 外部类型：`PQExpBuffer`
- 外部类型：`Preserve`
- 外部类型：`RelFileNumber`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-9eed081b05` → 本草稿移入 `cases/defect/auto-postgres-9eed081b05/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
