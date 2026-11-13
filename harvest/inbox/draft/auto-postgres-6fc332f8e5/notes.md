# auto-postgres-6fc332f8e5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#0dbe240dc0e14448fe82f6b356947d4b5c93f89f](https://github.com/postgres/postgres/commit/0dbe240dc0e14448fe82f6b356947d4b5c93f89f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 350 |
| 编译错误数（gcc syntax-only） | 227（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0dbe240dc0e14448fe82f6b356947d4b5c93f89f (https://github.com/postgres/postgres/commit/0dbe240dc0e14448fe82f6b356947d4b5c93f89f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0dbe240dc0e14448fe82f6b356947d4b5c93f89f Fix some edge cases with characteristics after COMMIT/ROLLBACK AND CHAIN :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3417,6 +3417,14 @@ CommitTransactionCommandInternal(void)
 			if (s->blockState == TBLOCK_END)
 			{
 				Assert(s->parent == NULL);
+
+				/*
+				 * Save the characteristics again, now that the
+				 * subtransactions are committed.  A subtransaction may have
+				 * switched to read-only, which should not be inherited by the
+				 * next AND CHAIN, if any.
+				 */
+				SaveTransactionCharacteristics(&savetc);
 				CommitTransaction();
 				s->blockState = TBLOCK_DEFAULT;
 				if (s->chain)
@@ -3547,6 +3555,10 @@ static bool
 AbortCurrentTransactionInternal(void)
 {
 	TransactionState s = CurrentTransactionState;
+	SavedTransactionCharacteristics savetc;
+
+	/* Must save in case we need to restore below */
+	SaveTransactionCharacteristics(&savetc);
 
 	switch (s->blockState)
 	{
@@ -3604,6 +3616,12 @@ AbortCurrentTransactionInternal(void)
 		case TBLOCK_INPROGRESS:
 		case TBLOCK_PARALLEL_INPROGRESS:
 			AbortTransaction();
+
+			/*
+			 * AbortTransaction() has reset the transaction characteristics;
+			 * restore them for AND CHAIN.
+			 */
+			RestoreTransactionCharacteristics(&savetc);
 			s->blockState = TBLOCK_ABORT;
 			/* CleanupTransaction happens when we exit TBLOCK_ABORT_END */
 			break;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pgstat_get_transactional_drops`
- 外部函数：`smgrGetPendingDeletes`
- 外部函数：`xactGetCommittedChildren`
- 外部函数：`xactGetCommittedInvalidationMessages`
- 大写宏：`BEGIN`
- 大写宏：`COMMIT`
- 大写宏：`GUC`
- 大写宏：`INJECTION_POINT_LOAD`
- 大写宏：`NULL`
- 大写宏：`PG_USED_FOR_ASSERTS_ONLY`
- 大写宏：`PREPARE`
- 大写宏：`RELEASE`
- 大写宏：`ROLLBACK`
- 大写宏：`TBLOCK_ABORT`
- 大写宏：`TBLOCK_ABORT_END`
- 大写宏：`TBLOCK_ABORT_PENDING`
- 大写宏：`TBLOCK_BEGIN`
- 大写宏：`TBLOCK_DEFAULT`
- 大写宏：`TBLOCK_END`
- 大写宏：`TBLOCK_IMPLICIT_INPROGRESS`
- 大写宏：`TBLOCK_INPROGRESS`
- 大写宏：`TBLOCK_PARALLEL_INPROGRESS`
- 大写宏：`TBLOCK_PREPARE`
- 大写宏：`TBLOCK_STARTED`
- 大写宏：`TBLOCK_SUBABORT`
- 大写宏：`TBLOCK_SUBABORT_END`
- 大写宏：`TBLOCK_SUBABORT_PENDING`
- 大写宏：`TBLOCK_SUBABORT_RESTART`
- 大写宏：`TBLOCK_SUBBEGIN`
- 大写宏：`TBLOCK_SUBCOMMIT`
- 大写宏：`TBLOCK_SUBINPROGRESS`
- 大写宏：`TBLOCK_SUBRELEASE`
- 大写宏：`TBLOCK_SUBRESTART`
- 大写宏：`TRANS_ABORT`
- 大写宏：`TRANS_COMMIT`
- 大写宏：`TRANS_DEFAULT`
- 大写宏：`TRANS_INPROGRESS`
- 大写宏：`TRANS_PREPARE`
- 大写宏：`TRANS_START`
- 大写宏：`WAL`
- 大写宏：`XID`
- 外部类型：`CurrentMemoryContext`
- 外部类型：`CurrentTransactionState`
- 外部类型：`CurrentUserId`
- 外部类型：`DDLs`
- 外部类型：`Enter`
- 外部类型：`ExitParallelMode`
- 外部类型：`FullTransactionId`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`InvalidTransactionId`
- 外部类型：`Load`
- 外部类型：`Log`
- 外部类型：`MemoryContext`
- 外部类型：`Normally`
- 外部类型：`Oid`
- 外部类型：`RelFileLocator`
- 外部类型：`RelcacheInitFileInval`
- 外部类型：`ResourceOwner`
- 外部类型：`SecurityRestrictionContext`
- 外部类型：`SharedInvalidationMessage`
- 外部类型：`SubTransactionId`
- 外部类型：`SubXactCallback`
- 外部类型：`SubXactCallbackItem`
- 外部类型：`TimestampTz`
- 外部类型：`TransactionId`
- 外部类型：`TransactionStateData`
- 外部类型：`XIDs`
- 外部类型：`XactCallback`
- 外部类型：`XactCallbackItem`
- 外部类型：`XactLastRecEnd`
- 外部类型：`XactTopFullTransactionId`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-6fc332f8e5` → 本草稿移入 `cases/defect/auto-postgres-6fc332f8e5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
