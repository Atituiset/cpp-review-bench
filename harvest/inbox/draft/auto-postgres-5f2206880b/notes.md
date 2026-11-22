# auto-postgres-5f2206880b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b291954ea5a4855ded56c334859d7de9fea372eb](https://github.com/postgres/postgres/commit/b291954ea5a4855ded56c334859d7de9fea372eb) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b291954ea5a4855ded56c334859d7de9fea372eb (https://github.com/postgres/postgres/commit/b291954ea5a4855ded56c334859d7de9fea372eb)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 3336；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b291954ea5a4855ded56c334859d7de9fea372eb Fix stale source values in MERGE after a concurrent update. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3309,6 +3309,7 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 	List	   *actionStates;
 	TupleTableSlot *newslot = NULL;
 	TupleTableSlot *rslot = NULL;
+	TupleTableSlot *planSlot = context->planSlot;
 	EState	   *estate = context->estate;
 	ExprContext *econtext = mtstate->ps.ps_ExprContext;
 	bool		isNull;
@@ -3333,7 +3334,7 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 	 * or NOT MATCHED BY SOURCE tuple and UPDATE/DELETE actions.
 	 */
 	econtext->ecxt_scantuple = resultRelInfo->ri_oldTupleSlot;
-	econtext->ecxt_innertuple = context->planSlot;
+	econtext->ecxt_innertuple = planSlot;
 	econtext->ecxt_outertuple = NULL;
 
 	/*
@@ -3717,6 +3718,28 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 								{
 									actionStates = mergeActions[MERGE_WHEN_NOT_MATCHED_BY_SOURCE];
 
+									/*
+									 * Since we have transitioned from MATCHED
+									 * to NOT MATCHED BY SOURCE, we need
+									 * all-NULL source values for RETURNING.
+									 * We can't simply use epqslot, because in
+									 * the case of a full join, it might
+									 * contain the NOT MATCHED BY TARGET
+									 * tuple, with non-NULL source values.
+									 * Instead, build a new all-NULL plan slot
+									 * for use locally, leaving
+									 * context->planSlot unchanged for any
+									 * subsequent NOT MATCHED BY TARGET
+									 * actions.  We don't bother setting
+									 * "junk" attributes, since they are no
+									 * longer needed.
+									 */
+									planSlot =
+										MakeSingleTupleTableSlot(context->planSlot->tts_tupleDescriptor,
+																 &TTSOpsVirtual);
+									ExecStoreAllNullTuple(planSlot);
+									econtext->ecxt_innertuple = planSlot;
+
 									/*
 									 * If we have both NOT MATCHED BY SOURCE
 									 * and NOT MATCHED BY TARGET actions (a
@@ -3816,7 +3839,7 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 												 false,
 												 resultRelInfo->ri_oldTupleSlot,
 												 newslot,
-												 context->planSlot);
+												 planSlot);
 					break;
 
 				case CMD_DELETE:
@@ -3825,7 +3848,7 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 												 true,
 												 resultRelInfo->ri_oldTupleSlot,
 												 NULL,
-												 context->planSlot);
+												 planSlot);
 					break;
 
 				case CMD_NOTHING:
@@ -3848,6 +3871,14 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 	 * Successfully executed an action or no qualifying action was found.
 	 */
 out:
+	if (planSlot != context->planSlot)
+	{
+		econtext->ecxt_innertuple = NULL;
+		if (resultRelInfo->ri_projectReturning != NULL)
+			resultRelInfo->ri_projectReturning->pi_exprContext->ecxt_outertuple = NULL;
+		ExecDropSingleTupleTableSlot(planSlot);
+	}
+
 	if (ItemPointerIsValid(&lockedtid))
 		UnlockTuple(resultRelInfo->ri_RelationDesc, &lockedtid,
 					InplaceUpdateTupleLock);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`actions`
- 大写宏：`CMD_DELETE`
- 大写宏：`CMD_NOTHING`
- 大写宏：`DELETE`
- 大写宏：`MATCHED`
- 大写宏：`MERGE_WHEN_NOT_MATCHED_BY_SOURCE`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`SOURCE`
- 大写宏：`TARGET`
- 大写宏：`UPDATE`
- 外部类型：`EState`
- 外部类型：`ExprContext`
- 外部类型：`If`
- 外部类型：`InplaceUpdateTupleLock`
- 外部类型：`List`
- 外部类型：`Successfully`
- 外部类型：`TupleTableSlot`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-5f2206880b` → 本草稿移入 `cases/defect/auto-postgres-5f2206880b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
