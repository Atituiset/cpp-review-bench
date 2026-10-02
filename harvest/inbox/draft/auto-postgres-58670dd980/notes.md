# auto-postgres-58670dd980

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1378aa13430e264a990a18597e9d8fdae740927e](https://github.com/postgres/postgres/commit/1378aa13430e264a990a18597e9d8fdae740927e) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1378aa13430e264a990a18597e9d8fdae740927e (https://github.com/postgres/postgres/commit/1378aa13430e264a990a18597e9d8fdae740927e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 3046；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1378aa13430e264a990a18597e9d8fdae740927e Report per-index vacuum progress in pg_stat_progress_vacuum :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3038,17 +3038,27 @@ lazy_vacuum_one_index(Relation indrel, IndexBulkDeleteResult *istat,
 {
 	IndexVacuumInfo ivinfo;
 	LVSavedErrInfo saved_err_info;
+	const int	reset_index[] = {
+		PROGRESS_VACUUM_CURRENT_INDEX_RELID,
+		PROGRESS_SCAN_BLOCKS_TOTAL,
+		PROGRESS_SCAN_BLOCKS_DONE
+	};
+	const int64 reset_val[] = {(int64) InvalidOid, 0, 0};
 
 	ivinfo.index = indrel;
 	ivinfo.heaprel = vacrel->rel;
 	ivinfo.analyze_only = false;
 	ivinfo.is_autovacuum = AmAutoVacuumWorkerProcess();
-	ivinfo.report_progress = false;
+	ivinfo.report_progress = true;
 	ivinfo.estimated_count = true;
 	ivinfo.message_level = DEBUG2;
 	ivinfo.num_heap_tuples = reltuples;
 	ivinfo.strategy = vacrel->bstrategy;
 
+	/* Report which index we're currently processing */
+	pgstat_progress_update_param(PROGRESS_VACUUM_CURRENT_INDEX_RELID,
+								 (int64) RelationGetRelid(indrel));
+
 	/*
 	 * Update error traceback information.
 	 *
@@ -3070,6 +3080,8 @@ lazy_vacuum_one_index(Relation indrel, IndexBulkDeleteResult *istat,
 	pfree(vacrel->indname);
 	vacrel->indname = NULL;
 
+	pgstat_progress_update_multi_param(3, reset_index, reset_val);
+
 	return istat;
 }
 
@@ -3089,18 +3101,28 @@ lazy_cleanup_one_index(Relation indrel, IndexBulkDeleteResult *istat,
 {
 	IndexVacuumInfo ivinfo;
 	LVSavedErrInfo saved_err_info;
+	const int	reset_index[] = {
+		PROGRESS_VACUUM_CURRENT_INDEX_RELID,
+		PROGRESS_SCAN_BLOCKS_TOTAL,
+		PROGRESS_SCAN_BLOCKS_DONE
+	};
+	const int64 reset_val[] = {(int64) InvalidOid, 0, 0};
 
 	ivinfo.index = indrel;
 	ivinfo.heaprel = vacrel->rel;
 	ivinfo.analyze_only = false;
 	ivinfo.is_autovacuum = AmAutoVacuumWorkerProcess();
-	ivinfo.report_progress = false;
+	ivinfo.report_progress = true;
 	ivinfo.estimated_count = estimated_count;
 	ivinfo.message_level = DEBUG2;
 
 	ivinfo.num_heap_tuples = reltuples;
 	ivinfo.strategy = vacrel->bstrategy;
 
+	/* Report which index we're currently processing */
+	pgstat_progress_update_param(PROGRESS_VACUUM_CURRENT_INDEX_RELID,
+								 (int64) RelationGetRelid(indrel));
+
 	/*
 	 * Update error traceback information.
 	 *
@@ -3120,6 +3142,8 @@ lazy_cleanup_one_index(Relation indrel, IndexBulkDeleteResult *istat,
 	pfree(vacrel->indname);
 	vacrel->indname = NULL;
 
+	pgstat_progress_update_multi_param(3, reset_index, reset_val);
+
 	return istat;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pfree`
- 大写宏：`DEBUG2`
- 大写宏：`NULL`
- 大写宏：`VACUUM_ERRCB_PHASE_INDEX_CLEANUP`
- 大写宏：`VACUUM_ERRCB_PHASE_SCAN_HEAP`
- 大写宏：`VACUUM_ERRCB_PHASE_TRUNCATE`
- 大写宏：`VACUUM_ERRCB_PHASE_UNKNOWN`
- 大写宏：`VACUUM_ERRCB_PHASE_VACUUM_HEAP`
- 大写宏：`VACUUM_ERRCB_PHASE_VACUUM_INDEX`
- 外部类型：`BlockNumber`
- 外部类型：`IndexVacuumInfo`
- 外部类型：`LVSavedErrInfo`
- 外部类型：`OffsetNumber`
- 外部类型：`Update`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-58670dd980` → 本草稿移入 `cases/defect/auto-postgres-58670dd980/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
