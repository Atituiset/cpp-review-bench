# auto-postgres-e7077dda63

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#4a26e36f87f6f30a93bfe1b95264b4b12d6b9ac3](https://github.com/postgres/postgres/commit/4a26e36f87f6f30a93bfe1b95264b4b12d6b9ac3) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4a26e36f87f6f30a93bfe1b95264b4b12d6b9ac3 (https://github.com/postgres/postgres/commit/4a26e36f87f6f30a93bfe1b95264b4b12d6b9ac3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 18（原始 PR diff 行 767；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4a26e36f87f6f30a93bfe1b95264b4b12d6b9ac3 pg_rewind: Fix recovery delays at WAL page boundaries :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -743,8 +743,8 @@ perform_rewind(filemap_t *filemap, rewind_source *source,
 	{
 		/*
 		 * The source is a live server. Like in an online backup, it's
-		 * important that we recover all the WAL that was generated while we
-		 * were copying files.
+		 * important that we replay all the WAL needed to make the copied
+		 * files consistent.
 		 */
 		if (ControlFile_source_after.state == DB_IN_ARCHIVE_RECOVERY)
 		{
@@ -758,13 +758,19 @@ perform_rewind(filemap_t *filemap, rewind_source *source,
 		else
 		{
 			/*
-			 * Source is a production, non-standby, server. We must replay to
-			 * the last WAL insert location.
+			 * Source is a live primary server. Use its WAL flush location
+			 * after copying the files as the minimum recovery point. The
+			 * write-ahead rule ensures that all WAL needed for the copied
+			 * data has been flushed by then.
+			 *
+			 * Don't use the WAL insert location: at a page boundary, it can
+			 * point past the next page's header, requiring additional WAL to
+			 * be generated before recovery can reach consistency.
 			 */
 			if (ControlFile_source_after.state != DB_IN_PRODUCTION)
 				pg_fatal("source system was in unexpected state at end of rewind");
 
-			endrec = source->get_current_wal_insert_lsn(source);
+			endrec = source->get_current_wal_flush_lsn(source);
 			endtli = Max(ControlFile_source_after.checkPointCopy.ThisTimeLineID,
 						 ControlFile_source_after.minRecoveryPointTLI);
 		}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`get_current_wal_insert_lsn`
- 外部函数：`pg_fatal`
- 大写宏：`DB_IN_ARCHIVE_RECOVERY`
- 大写宏：`DB_IN_PRODUCTION`
- 大写宏：`WAL`
- 外部类型：`ControlFile_source_after`
- 外部类型：`Like`
- 外部类型：`Source`
- 外部类型：`The`
- 外部类型：`ThisTimeLineID`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-e7077dda63` → 本草稿移入 `cases/defect/auto-postgres-e7077dda63/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
