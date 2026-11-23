# auto-postgres-23d7b9a797

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#19db7e5b36e33c80a032e0e811176ad3b115b0ca](https://github.com/postgres/postgres/commit/19db7e5b36e33c80a032e0e811176ad3b115b0ca) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #19db7e5b36e33c80a032e0e811176ad3b115b0ca (https://github.com/postgres/postgres/commit/19db7e5b36e33c80a032e0e811176ad3b115b0ca)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 3874；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 19db7e5b36e33c80a032e0e811176ad3b115b0ca Fix cross-partition source values in MERGE after a concurrent update. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3309,7 +3309,7 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 	List	   *actionStates;
 	TupleTableSlot *newslot = NULL;
 	TupleTableSlot *rslot = NULL;
-	TupleTableSlot *planSlot = context->planSlot;
+	TupleTableSlot *origPlanSlot = context->planSlot;
 	EState	   *estate = context->estate;
 	ExprContext *econtext = mtstate->ps.ps_ExprContext;
 	bool		isNull;
@@ -3334,7 +3334,7 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 	 * or NOT MATCHED BY SOURCE tuple and UPDATE/DELETE actions.
 	 */
 	econtext->ecxt_scantuple = resultRelInfo->ri_oldTupleSlot;
-	econtext->ecxt_innertuple = planSlot;
+	econtext->ecxt_innertuple = origPlanSlot;
 	econtext->ecxt_outertuple = NULL;
 
 	/*
@@ -3727,18 +3727,16 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 									 * contain the NOT MATCHED BY TARGET
 									 * tuple, with non-NULL source values.
 									 * Instead, build a new all-NULL plan slot
-									 * for use locally, leaving
-									 * context->planSlot unchanged for any
-									 * subsequent NOT MATCHED BY TARGET
-									 * actions.  We don't bother setting
-									 * "junk" attributes, since they are no
-									 * longer needed.
+									 * to replace context->planSlot while we
+									 * execute the NOT MATCHED BY SOURCE
+									 * action.  We don't bother setting "junk"
+									 * attributes, since they aren't needed.
 									 */
-									planSlot =
+									context->planSlot =
 										MakeSingleTupleTableSlot(context->planSlot->tts_tupleDescriptor,
 																 &TTSOpsVirtual);
-									ExecStoreAllNullTuple(planSlot);
-									econtext->ecxt_innertuple = planSlot;
+									ExecStoreAllNullTuple(context->planSlot);
+									econtext->ecxt_innertuple = context->planSlot;
 
 									/*
 									 * If we have both NOT MATCHED BY SOURCE
@@ -3839,7 +3837,7 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 												 false,
 												 resultRelInfo->ri_oldTupleSlot,
 												 newslot,
-												 planSlot);
+												 context->planSlot);
 					break;
 
 				case CMD_DELETE:
@@ -3848,7 +3846,7 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 												 true,
 												 resultRelInfo->ri_oldTupleSlot,
 												 NULL,
-												 planSlot);
+												 context->planSlot);
 					break;
 
 				case CMD_NOTHING:
@@ -3871,12 +3869,13 @@ ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
 	 * Successfully executed an action or no qualifying action was found.
 	 */
 out:
-	if (planSlot != context->planSlot)
+	if (context->planSlot != origPlanSlot)
 	{
 		econtext->ecxt_innertuple = NULL;
 		if (resultRelInfo->ri_projectReturning != NULL)
 			resultRelInfo->ri_projectReturning->pi_exprContext->ecxt_outertuple = NULL;
-		ExecDropSingleTupleTableSlot(planSlot);
+		ExecDropSingleTupleTableSlot(context->planSlot);
+		context->planSlot = origPlanSlot;
 	}
 
 	if (ItemPointerIsValid(&lockedtid))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`CMD_DELETE`
- 大写宏：`CMD_NOTHING`
- 大写宏：`DELETE`
- 大写宏：`MATCHED`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`SOURCE`
- 大写宏：`TARGET`
- 大写宏：`UPDATE`
- 外部类型：`EState`
- 外部类型：`ExprContext`
- 外部类型：`If`
- 外部类型：`Instead`
- 外部类型：`List`
- 外部类型：`Successfully`
- 外部类型：`TTSOpsVirtual`
- 外部类型：`TupleTableSlot`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-23d7b9a797` → 本草稿移入 `cases/defect/auto-postgres-23d7b9a797/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
