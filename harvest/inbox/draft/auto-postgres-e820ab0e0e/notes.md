# auto-postgres-e820ab0e0e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#3be29d9346db1d3225773e2a8e5a86b875696617](https://github.com/postgres/postgres/commit/3be29d9346db1d3225773e2a8e5a86b875696617) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3be29d9346db1d3225773e2a8e5a86b875696617 (https://github.com/postgres/postgres/commit/3be29d9346db1d3225773e2a8e5a86b875696617)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1793；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3be29d9346db1d3225773e2a8e5a86b875696617 Fix three blips with progress reporting data (heap and GIN) :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1788,10 +1788,6 @@ _gin_parallel_merge(GinBuildState *state)
 
 		/* discard the existing data */
 		GinBufferReset(buffer);
-
-		/* Report progress */
-		pgstat_progress_update_param(PROGRESS_CREATEIDX_TUPLES_DONE,
-									 ++numtuples);
 	}
 
 	/* release all the memory */
@@ -1944,6 +1940,7 @@ _gin_process_worker_data(GinBuildState *state, Tuplesortstate *worker_sort,
 									buffer->items, buffer->nfrozen, &ntuplen);
 
 			tuplesort_putgintuple(state->bs_sortstate, ntup, ntuplen);
+			state->bs_numtuples++;
 
 			pfree(ntup);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bytes`
- 外部函数：`key`
- 外部函数：`pfree`
- 外部函数：`pgstat_progress_update_param`
- 外部函数：`tuplesort_putgintuple`
- 大写宏：`GIN_CAT_NORM_KEY`
- 大写宏：`NULL`
- 大写宏：`PROGRESS_CREATEIDX_TUPLES_DONE`
- 大写宏：`TID`
- 外部类型：`Datum`
- 外部类型：`GinBuffer`
- 外部类型：`GinNullCategory`
- 外部类型：`ItemPointerData`
- 外部类型：`Not`
- 外部类型：`Number`
- 外部类型：`OffsetNumber`
- 外部类型：`Report`
- 外部类型：`Size`
- 外部类型：`SortSupport`
- 外部类型：`TIDs`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-e820ab0e0e` → 本草稿移入 `cases/defect/auto-postgres-e820ab0e0e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
