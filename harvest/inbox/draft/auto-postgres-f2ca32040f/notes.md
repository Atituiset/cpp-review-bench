# auto-postgres-f2ca32040f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#206e71935e1ed86e6116b118d7b77e78efcff8b8](https://github.com/postgres/postgres/commit/206e71935e1ed86e6116b118d7b77e78efcff8b8) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #206e71935e1ed86e6116b118d7b77e78efcff8b8 (https://github.com/postgres/postgres/commit/206e71935e1ed86e6116b118d7b77e78efcff8b8)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 24（原始 PR diff 行 171；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 206e71935e1ed86e6116b118d7b77e78efcff8b8 Exclude new pages from VACUUM reltuples estimates :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -57,6 +57,7 @@ typedef struct StatApproxReadStreamPrivate
 	BlockNumber current_blocknum;
 	BlockNumber nblocks;
 	BlockNumber scanned;		/* count of pages actually read */
+	BlockNumber scanned_new;	/* count of scanned PageIsNew pages */
 	Buffer		vmbuffer;		/* for VM lookups */
 } StatApproxReadStreamPrivate;
 
@@ -133,6 +134,7 @@ statapprox_heap(Relation rel, output_type *stat)
 	p.current_blocknum = 0;
 	p.nblocks = nblocks;
 	p.scanned = 0;
+	p.scanned_new = 0;
 	p.vmbuffer = InvalidBuffer;
 
 	/*
@@ -168,7 +170,14 @@ statapprox_heap(Relation rel, output_type *stat)
 
 		stat->free_space += PageGetExactFreeSpace(page);
 
-		if (PageIsNew(page) || PageIsEmpty(page))
+		if (PageIsNew(page))
+		{
+			p.scanned_new++;
+			UnlockReleaseBuffer(buf);
+			continue;
+		}
+
+		if (PageIsEmpty(page))
 		{
 			UnlockReleaseBuffer(buf);
 			continue;
@@ -246,6 +255,7 @@ statapprox_heap(Relation rel, output_type *stat)
 	 * we already accounted for the space in those pages, too.
 	 */
 	stat->tuple_count = vac_estimate_reltuples(rel, nblocks, p.scanned,
+											   p.scanned_new,
 											   stat->tuple_count);
 
 	/* It's not clear if we could get -1 here, but be safe. */
@@ -256,6 +266,10 @@ statapprox_heap(Relation rel, output_type *stat)
 	 */
 	if (nblocks != 0)
 	{
+		/*
+		 * Includes PageIsNew blocks that were read but excluded from the
+		 * live-tuple estimate.
+		 */
 		stat->scanned_percent = 100.0 * p.scanned / nblocks;
 		stat->tuple_percent = 100.0 * stat->tuple_len / stat->table_len;
 		stat->dead_tuple_percent = 100.0 * stat->dead_tuple_len / stat->table_len;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`vac_estimate_reltuples`
- 外部类型：`BlockNumber`
- 外部类型：`Buffer`
- 外部类型：`InvalidBuffer`
- 外部类型：`It`
- 外部类型：`Relation`
- 外部类型：`StatApproxReadStreamPrivate`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f2ca32040f` → 本草稿移入 `cases/defect/auto-postgres-f2ca32040f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
