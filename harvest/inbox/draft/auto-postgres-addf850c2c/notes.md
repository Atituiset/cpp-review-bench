# auto-postgres-addf850c2c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#92a734bf5c7ace23131b711025c75aacd8144482](https://github.com/postgres/postgres/commit/92a734bf5c7ace23131b711025c75aacd8144482) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #92a734bf5c7ace23131b711025c75aacd8144482 (https://github.com/postgres/postgres/commit/92a734bf5c7ace23131b711025c75aacd8144482)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 686；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 92a734bf5c7ace23131b711025c75aacd8144482 Fix parallel autovacuum workers ignoring track_cost_delay_timing changes. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -86,6 +86,7 @@ typedef struct PVSharedCostParams
 	int			cost_page_dirty;
 	int			cost_page_hit;
 	int			cost_page_miss;
+	bool		track_cost_delay_timing;
 } PVSharedCostParams;
 
 /*
@@ -644,6 +645,7 @@ parallel_vacuum_set_cost_parameters(PVSharedCostParams *params)
 	params->cost_page_dirty = VacuumCostPageDirty;
 	params->cost_page_hit = VacuumCostPageHit;
 	params->cost_page_miss = VacuumCostPageMiss;
+	params->track_cost_delay_timing = track_cost_delay_timing;
 }
 
 /*
@@ -676,19 +678,21 @@ parallel_vacuum_update_shared_delay_params(void)
 	VacuumCostPageDirty = pv_shared_cost_params->cost_page_dirty;
 	VacuumCostPageHit = pv_shared_cost_params->cost_page_hit;
 	VacuumCostPageMiss = pv_shared_cost_params->cost_page_miss;
+	track_cost_delay_timing = pv_shared_cost_params->track_cost_delay_timing;
 	SpinLockRelease(&pv_shared_cost_params->mutex);
 
 	VacuumUpdateCosts();
 
 	shared_params_generation_local = params_generation;
 
 	elog(DEBUG2,
-		 "parallel autovacuum worker updated cost params: cost_limit=%d, cost_delay=%g, cost_page_miss=%d, cost_page_dirty=%d, cost_page_hit=%d",
+		 "parallel autovacuum worker updated cost params: cost_limit=%d, cost_delay=%g, cost_page_miss=%d, cost_page_dirty=%d, cost_page_hit=%d, track_cost_delay_timing=%s",
 		 vacuum_cost_limit,
 		 vacuum_cost_delay,
 		 VacuumCostPageMiss,
 		 VacuumCostPageDirty,
-		 VacuumCostPageHit);
+		 VacuumCostPageHit,
+		 track_cost_delay_timing ? "yes" : "no");
 }
 
 /*
@@ -715,7 +719,8 @@ parallel_vacuum_propagate_shared_delay_params(void)
 		vacuum_cost_limit == pv_shared_cost_params->cost_limit &&
 		VacuumCostPageDirty == pv_shared_cost_params->cost_page_dirty &&
 		VacuumCostPageHit == pv_shared_cost_params->cost_page_hit &&
-		VacuumCostPageMiss == pv_shared_cost_params->cost_page_miss)
+		VacuumCostPageMiss == pv_shared_cost_params->cost_page_miss &&
+		track_cost_delay_timing == pv_shared_cost_params->track_cost_delay_timing)
 		return;
 
 	/* Update the shared delay parameters */
@@ -1388,7 +1393,7 @@ parallel_vacuum_main(dsm_segment *seg, shm_toc *toc)
 						  &wal_usage[ParallelWorkerNumber]);
 
 	/* Report any remaining cost-based vacuum delay time */
-	if (track_cost_delay_timing)
+	if (parallel_vacuum_worker_delay_ns > 0)
 		pgstat_progress_parallel_incr_param(PROGRESS_VACUUM_DELAY_TIME,
 											parallel_vacuum_worker_delay_ns);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`pgstat_progress_parallel_incr_param`
- 外部函数：`worker`
- 大写宏：`DEBUG2`
- 大写宏：`PROGRESS_VACUUM_DELAY_TIME`
- 外部类型：`PVSharedCostParams`
- 外部类型：`Parallel`
- 外部类型：`ParallelWorkerNumber`
- 外部类型：`Parameters`
- 外部类型：`Report`
- 外部类型：`The`
- 外部类型：`Update`
- 外部类型：`VacuumCostPageDirty`
- 外部类型：`VacuumCostPageHit`
- 外部类型：`VacuumCostPageMiss`
- 外部类型：`slock_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-addf850c2c` → 本草稿移入 `cases/defect/auto-postgres-addf850c2c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
