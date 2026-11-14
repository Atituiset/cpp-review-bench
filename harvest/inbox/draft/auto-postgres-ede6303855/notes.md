# auto-postgres-ede6303855

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
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #206e71935e1ed86e6116b118d7b77e78efcff8b8 (https://github.com/postgres/postgres/commit/206e71935e1ed86e6116b118d7b77e78efcff8b8)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 1（原始 PR diff 行 1597；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 206e71935e1ed86e6116b118d7b77e78efcff8b8 Exclude new pages from VACUUM reltuples estimates :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -305,6 +305,7 @@ typedef struct LVRelState
 
 	BlockNumber rel_pages;		/* total number of pages */
 	BlockNumber scanned_pages;	/* # pages examined (not skipped via VM) */
+	BlockNumber scanned_new_pages;	/* # scanned pages that were PageIsNew */
 
 	/*
 	 * Count of all-visible blocks eagerly scanned (for logging only). This
@@ -740,6 +741,7 @@ heap_vacuum_rel(Relation rel, const VacuumParams *params,
 
 	/* Initialize page counters explicitly (be tidy) */
 	vacrel->scanned_pages = 0;
+	vacrel->scanned_new_pages = 0;
 	vacrel->eager_scanned_pages = 0;
 	vacrel->removed_pages = 0;
 	vacrel->new_frozen_tuple_pages = 0;
@@ -955,8 +957,8 @@ heap_vacuum_rel(Relation rel, const VacuumParams *params,
 	/*
 	 * Now actually update rel's pg_class entry.
 	 *
-	 * new_live_tuples can be -1 when VACUUM skipped nearly all pages and had
-	 * no prior tuple-density estimate.
+	 * new_live_tuples can be -1 when VACUUM skipped nearly all initialized
+	 * pages and had no prior tuple-density estimate.
 	 */
 	vac_update_relstats(rel, new_rel_pages, vacrel->new_live_tuples,
 						new_rel_allvisible, new_rel_allfrozen,
@@ -1594,9 +1596,11 @@ lazy_scan_heap(LVRelState *vacrel)
 								 rel_pages);
 
 	/* now we can compute the new value for pg_class.reltuples */
-	vacrel->new_live_tuples = vac_estimate_reltuples(vacrel->rel, rel_pages,
-													 vacrel->scanned_pages,
-													 vacrel->live_tuples);
+	vacrel->new_live_tuples =
+		vac_estimate_reltuples(vacrel->rel, rel_pages,
+							   vacrel->scanned_pages,
+							   vacrel->scanned_new_pages,
+							   vacrel->live_tuples);
 
 	/*
 	 * Also compute the total number of surviving heap entries.  In the
@@ -1893,6 +1897,8 @@ lazy_scan_new_or_empty(LVRelState *vacrel, Buffer buf, BlockNumber blkno,
 
 	if (PageIsNew(page))
 	{
+		vacrel->scanned_new_pages++;
+
 		/*
 		 * All-zeroes pages can be left over if either a backend extends the
 		 * relation by a single page, but crashes before the newly initialized
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`examined`
- 外部函数：`explicitly`
- 外部函数：`scanned`
- 外部函数：`vac_estimate_reltuples`
- 外部函数：`vac_update_relstats`
- 大写宏：`VACUUM`
- 外部类型：`All`
- 外部类型：`Also`
- 外部类型：`BlockNumber`
- 外部类型：`Count`
- 外部类型：`In`
- 外部类型：`Initialize`
- 外部类型：`Now`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ede6303855` → 本草稿移入 `cases/defect/auto-postgres-ede6303855/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
