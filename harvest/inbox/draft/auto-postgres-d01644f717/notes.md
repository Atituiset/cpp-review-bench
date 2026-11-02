# auto-postgres-d01644f717

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#303c9b36205d11e41c7d5d61a1802ec50bccbf5a](https://github.com/postgres/postgres/commit/303c9b36205d11e41c7d5d61a1802ec50bccbf5a) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 33 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #303c9b36205d11e41c7d5d61a1802ec50bccbf5a (https://github.com/postgres/postgres/commit/303c9b36205d11e41c7d5d61a1802ec50bccbf5a)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 115；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 303c9b36205d11e41c7d5d61a1802ec50bccbf5a Fix reuse of stale hash tables by duplicated hashed SubPlans :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -101,7 +101,6 @@ ExecHashSubPlan(SubPlanState *node,
 {
 	bool		result = false;
 	SubPlan    *subplan = node->subplan;
-	PlanState  *planstate = node->planstate;
 	TupleTableSlot *slot;
 
 	/* Shouldn't have any direct correlation Vars */
@@ -112,7 +111,7 @@ ExecHashSubPlan(SubPlanState *node,
 	 * If first time through or we need to rescan the subplan, build the hash
 	 * table.
 	 */
-	if (node->hashtable == NULL || planstate->chgParam != NULL)
+	if (node->hashtable == NULL || node->hashtablestale)
 		buildSubPlanHash(node, econtext);
 
 	/*
@@ -505,6 +504,7 @@ buildSubPlanHash(SubPlanState *node, ExprContext *econtext)
 	 */
 	node->havehashrows = false;
 	node->havenullrows = false;
+	node->hashtablestale = false;
 
 	nentries = planstate->plan->plan_rows;
 
@@ -881,6 +881,7 @@ ExecInitSubPlan(SubPlan *subplan, PlanState *parent)
 	sstate->projRight = NULL;
 	sstate->hashtable = NULL;
 	sstate->hashnulls = NULL;
+	sstate->hashtablestale = false;
 	sstate->tuplesContext = NULL;
 	sstate->innerecontext = NULL;
 	sstate->keyColIdx = NULL;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`lfirst_int`
- 外部函数：`slot_attisnull`
- 外部函数：`slot_getattr`
- 外部函数：`table`
- 大写宏：`ANY_SUBLINK`
- 大写宏：`FALSE`
- 大写宏：`NULL`
- 大写宏：`UNKNOWN`
- 大写宏：`XXX`
- 外部类型：`Because`
- 外部类型：`Else`
- 外部类型：`ExecProject`
- 外部类型：`ExprContext`
- 外部类型：`ExprState`
- 外部类型：`If`
- 外部类型：`ListCell`
- 外部类型：`Load`
- 外部类型：`MemoryContext`
- 外部类型：`Note`
- 外部类型：`ParamExecData`
- 外部类型：`Params`
- 外部类型：`PlanState`
- 外部类型：`Reset`
- 外部类型：`Scan`
- 外部类型：`Shouldn`
- 外部类型：`Since`
- 外部类型：`SubPlan`
- 外部类型：`SubPlanState`
- 外部类型：`Switch`
- 外部类型：`TTSOpsVirtual`
- 外部类型：`This`
- 外部类型：`TupleTableSlot`
- 外部类型：`Vars`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-d01644f717` → 本草稿移入 `cases/defect/auto-postgres-d01644f717/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
