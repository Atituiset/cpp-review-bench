# auto-postgres-884f281c0a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b4cc5e924b7a85f76637227002eb1a4be443d956](https://github.com/postgres/postgres/commit/b4cc5e924b7a85f76637227002eb1a4be443d956) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 147 |
| 编译错误数（gcc syntax-only） | 70（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b4cc5e924b7a85f76637227002eb1a4be443d956 (https://github.com/postgres/postgres/commit/b4cc5e924b7a85f76637227002eb1a4be443d956)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b4cc5e924b7a85f76637227002eb1a4be443d956 Correct the parallel GIN merge comment :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1643,17 +1643,14 @@ GinBufferCanAddKey(GinBuffer *buffer, GinTuple *tup)
 }
 
 /*
- * Within leader, wait for end of heap scan and merge per-worker results.
+ * Wait for all participants to finish the heap scan, then merge their
+ * sorted GIN tuples in the leader.  Combine TID lists for the same index
+ * attribute, null category, and key, writing the TIDs to the index in
+ * batches.
  *
- * After waiting for all workers to finish, merge the per-worker results into
- * the complete index. The results from each worker are sorted by block number
- * (start of the page range). While combining the per-worker results we merge
- * summaries for the same page range, and also fill-in empty summaries for
- * ranges without any tuples.
- *
- * Returns the total number of heap tuples scanned.  Also sets
- * indexInfo->ii_BrokenHotChain if any participant detected a broken HOT chain,
- * so that index.c can mark the index as unsafe for older snapshots.
+ * Return the total number of live heap tuples reported by the participants.
+ * Set indexInfo->ii_BrokenHotChain if any participant detected a potentially
+ * broken HOT chain.
  */
 static double
 _gin_parallel_merge(GinBuildState *state, IndexInfo *indexInfo)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`createPostingTree`
- 外部函数：`ginCompressPostingList`
- 外部函数：`ginInsertItemPointers`
- 外部函数：`ginMergeItemPointers`
- 外部函数：`ginReadTuple`
- 外部函数：`gintuple_get_attrnum`
- 外部函数：`gintuple_get_key`
- 外部函数：`pfree`
- 外部函数：`state`
- 外部函数：`worker`
- 外部函数：`workers`
- 大写宏：`DISABLE_LEADER_PARTICIPATION`
- 大写宏：`GIN`
- 大写宏：`HOT`
- 大写宏：`MVCC`
- 大写宏：`NULL`
- 大写宏：`TOC`
- 外部类型：`All`
- 外部类型：`And`
- 外部类型：`BlockNumber`
- 外部类型：`Buffer`
- 外部类型：`BufferUsage`
- 外部类型：`BuildAccumulator`
- 外部类型：`Can`
- 外部类型：`Compress`
- 外部类型：`ConditionVariable`
- 外部类型：`Datum`
- 外部类型：`GinBuildShared`
- 外部类型：`GinBuildState`
- 外部类型：`GinLeader`
- 外部类型：`GinMaxItemSize`
- 外部类型：`GinNullCategory`
- 外部类型：`GinPostingList`
- 外部类型：`GinState`
- 外部类型：`GinStatsData`
- 外部类型：`In`
- 外部类型：`IndexTuple`
- 外部类型：`Initialize`
- 外部类型：`It`
- 外部类型：`ItemPointerData`
- 外部类型：`Leader`
- 外部类型：`MemoryContext`
- 外部类型：`Mutable`
- 外部类型：`Now`
- 外部类型：`OffsetNumber`
- 外部类型：`Oid`
- 外部类型：`ParallelContext`
- 外部类型：`ParallelTableScanDescData`
- 外部类型：`Sharedsort`
- 外部类型：`Snapshot`
- 外部类型：`TIDs`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`They`
- 外部类型：`Tuplesortstate`
- 外部类型：`WalUsage`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-884f281c0a` → 本草稿移入 `cases/defect/auto-postgres-884f281c0a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
