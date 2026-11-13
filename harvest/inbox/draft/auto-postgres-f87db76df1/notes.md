# auto-postgres-f87db76df1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#9a849767eb281c9502aab4a7384a654004ec6ffe](https://github.com/postgres/postgres/commit/9a849767eb281c9502aab4a7384a654004ec6ffe) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 10 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9a849767eb281c9502aab4a7384a654004ec6ffe (https://github.com/postgres/postgres/commit/9a849767eb281c9502aab4a7384a654004ec6ffe)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2106；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9a849767eb281c9502aab4a7384a654004ec6ffe Fix crash decoding a change to a partition pending detach. :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2103,9 +2103,19 @@ get_rel_sync_entry(PGOutputData *data, Relation relation)
 		ListCell   *lc;
 		Oid			publish_as_relid = relid;
 		int			publish_ancestor_level = 0;
-		bool		am_partition = get_rel_relispartition(relid);
 		char		relkind = get_rel_relkind(relid);
 		List	   *rel_publications = NIL;
+		List	   *ancestors = NIL;
+
+		/*
+		 * A partition whose concurrent detach has been committed but not
+		 * finalized reports no ancestors, even though relispartition is still
+		 * set. Treat it as a standalone table. Its changes are no longer
+		 * published via its former ancestors, but can still be published via
+		 * its own publications.
+		 */
+		if (get_rel_relispartition(relid))
+			ancestors = get_partition_ancestors(relid);
 
 		/* Reload publications if needed before use. */
 		if (!publications_valid)
@@ -2211,9 +2221,8 @@ get_rel_sync_entry(PGOutputData *data, Relation relation)
 			{
 				List	   *exceptpubids = NIL;
 
-				if (am_partition)
+				if (ancestors)
 				{
-					List	   *ancestors = get_partition_ancestors(relid);
 					Oid			last_ancestor_relid = llast_oid(ancestors);
 
 					/*
@@ -2262,11 +2271,10 @@ get_rel_sync_entry(PGOutputData *data, Relation relation)
 				 * published via this publication, which will be used as the
 				 * relation via which to publish the partition's changes.
 				 */
-				if (am_partition)
+				if (ancestors)
 				{
 					Oid			ancestor;
 					int			level;
-					List	   *ancestors = get_partition_ancestors(relid);
 
 					ancestor = GetTopMostAncestorInPublication(pub->oid,
 															   ancestors,
@@ -2364,6 +2372,7 @@ get_rel_sync_entry(PGOutputData *data, Relation relation)
 		list_free(pubids);
 		list_free(schemaPubids);
 		list_free(rel_publications);
+		list_free(ancestors);
 
 		entry->replicate_valid = true;
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`get_partition_ancestors`
- 外部函数：`get_rel_relispartition`
- 外部函数：`get_rel_relkind`
- 外部函数：`list_free`
- 外部函数：`llast_oid`
- 大写宏：`NIL`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`Oid`
- 外部类型：`Reload`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f87db76df1` → 本草稿移入 `cases/defect/auto-postgres-f87db76df1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
