# auto-postgres-3471dc25e9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d0e87c22d8351645cf77a42e60ee47f260c6c2e2](https://github.com/postgres/postgres/commit/d0e87c22d8351645cf77a42e60ee47f260c6c2e2) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 61 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d0e87c22d8351645cf77a42e60ee47f260c6c2e2 (https://github.com/postgres/postgres/commit/d0e87c22d8351645cf77a42e60ee47f260c6c2e2)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d0e87c22d8351645cf77a42e60ee47f260c6c2e2 Move index-only scan heap fetch comment. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -365,9 +365,9 @@ heapam_index_getnext_slot(IndexScanDesc scan, ScanDirection direction,
 										 ItemPointerGetBlockNumber(&scan->xs_heaptid),
 										 &hscan->xs_vmbuffer);
 
-			/* Page isn't all-visible, so verify visibility with a heap fetch */
 			if (unlikely(!all_visible))
 			{
+				/* Page isn't all-visible, so need a heap fetch */
 				if (!heapam_index_only_heap_fetch(scan))
 				{
 					/* No visible tuple */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`a`
- 外部函数：`heap_page_prune_opt`
- 外部函数：`heapam_index_only_heap_fetch`
- 外部函数：`incorrect`
- 外部函数：`pg_assume`
- 外部函数：`unlikely`
- 大写宏：`HEAP_ONLY`
- 大写宏：`HOT`
- 大写宏：`MVCC`
- 大写宏：`NULL`
- 大写宏：`SSI`
- 大写宏：`TID`
- 大写宏：`TTS_IS_BUFFERTUPLE`
- 大写宏：`XXX`
- 外部类型：`At`
- 外部类型：`BlockNumber`
- 外部类型：`Buffer`
- 外部类型：`BufferHeapTupleTableSlot`
- 外部类型：`But`
- 外部类型：`Check`
- 外部类型：`FirstOffsetNumber`
- 外部类型：`Follow`
- 外部类型：`GlobalVisState`
- 外部类型：`Having`
- 外部类型：`HeapTuple`
- 外部类型：`HeapTupleData`
- 外部类型：`HeapTupleHeader`
- 外部类型：`If`
- 外部类型：`Index`
- 外部类型：`IndexScanDesc`
- 外部类型：`IndexScanHeapData`
- 外部类型：`InvalidTransactionId`
- 外部类型：`ItemId`
- 外部类型：`ItemPointer`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`OffsetNumber`
- 外部类型：`Page`
- 外部类型：`Plain`
- 外部类型：`Prune`
- 外部类型：`RecentXmin`
- 外部类型：`Relation`
- 外部类型：`Remember`
- 外部类型：`Returning`
- 外部类型：`Satisfies`
- 外部类型：`Scan`
- 外部类型：`Shouldn`
- 外部类型：`Snapshot`
- 外部类型：`The`
- 外部类型：`TransactionId`
- 外部类型：`TupleTableSlot`
- 外部类型：`Update`
- 外部类型：`We`
- 外部类型：`When`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-3471dc25e9` → 本草稿移入 `cases/defect/auto-postgres-3471dc25e9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
