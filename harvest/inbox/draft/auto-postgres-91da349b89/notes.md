# auto-postgres-91da349b89

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#ba51d62be465c420b0fbe3b0efd0ccd488e79922](https://github.com/postgres/postgres/commit/ba51d62be465c420b0fbe3b0efd0ccd488e79922) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 2 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ba51d62be465c420b0fbe3b0efd0ccd488e79922 (https://github.com/postgres/postgres/commit/ba51d62be465c420b0fbe3b0efd0ccd488e79922)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ba51d62be465c420b0fbe3b0efd0ccd488e79922 Fix Parallel TID Range Scan rescans with a different TID range :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -395,6 +395,14 @@ table_block_parallelscan_reinitialize(Relation rel, ParallelTableScanDesc pscan)
 {
 	ParallelBlockTableScanDesc bpscan = (ParallelBlockTableScanDesc) pscan;
 
+	/*
+	 * Reset the start block and block limit too.  These are set by the first
+	 * participant to call table_block_parallelscan_startblock_init(), and a
+	 * rescan may want to scan a different range of blocks, e.g. a TID Range
+	 * Scan with different TID bounds.
+	 */
+	pg_atomic_write_u32(&bpscan->phs_startblock, InvalidBlockNumber);
+	pg_atomic_write_u32(&bpscan->phs_numblock, InvalidBlockNumber);
 	pg_atomic_write_u64(&bpscan->phs_nallocated, 0);
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pg_atomic_write_u64`
- 外部类型：`ParallelBlockTableScanDesc`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-91da349b89` → 本草稿移入 `cases/defect/auto-postgres-91da349b89/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
