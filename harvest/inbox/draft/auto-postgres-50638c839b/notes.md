# auto-postgres-50638c839b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#73e69062c120f669f71531dabeacfb8041c85daf](https://github.com/postgres/postgres/commit/73e69062c120f669f71531dabeacfb8041c85daf) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #73e69062c120f669f71531dabeacfb8041c85daf (https://github.com/postgres/postgres/commit/73e69062c120f669f71531dabeacfb8041c85daf)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2471；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 73e69062c120f669f71531dabeacfb8041c85daf Fix EPQ run-time pruning using the parent's PARAM_EXEC values :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1965,6 +1965,36 @@ ExecDoInitialPruning(EState *estate)
 	}
 }
 
+/*
+ * ExecCreatePartitionPruneStates
+ *		Create a PartitionPruneState for each PartitionPruneInfo entry in
+ *		estate->es_part_prune_infos, without performing initial pruning.
+ *
+ * This is for an EState that must reuse the initial pruning results of
+ * another EState, such as an EPQ recheck's EState, for which the caller must
+ * have set es_part_prune_results.  The PartitionPruneStates can't be shared
+ * with the other EState, since exec pruning must evaluate PARAM_EXEC Params
+ * using this EState's es_param_exec_vals.
+ */
+void
+ExecCreatePartitionPruneStates(EState *estate)
+{
+	ListCell   *lc;
+
+	Assert(estate->es_part_prune_states == NIL);
+
+	foreach(lc, estate->es_part_prune_infos)
+	{
+		PartitionPruneInfo *pruneinfo = lfirst_node(PartitionPruneInfo, lc);
+		Bitmapset  *all_leafpart_rtis = NULL;
+
+		estate->es_part_prune_states =
+			lappend(estate->es_part_prune_states,
+					CreatePartitionPruneState(estate, pruneinfo,
+											  &all_leafpart_rtis));
+	}
+}
+
 /*
  * ExecInitPartitionExecPruning
  *		Initialize the data structures needed for runtime "exec" partition
@@ -2116,8 +2146,6 @@ CreatePartitionPruneState(EState *estate, PartitionPruneInfo *pruneinfo,
 	prunestate->other_subplans = bms_copy(pruneinfo->other_subplans);
 	prunestate->do_initial_prune = false;	/* may be set below */
 	prunestate->do_exec_prune = false;	/* may be set below */
-	prunestate->initialized = false;	/* set in
-										 * InitExecPartitionPruneContexts */
 	prunestate->num_partprunedata = n_part_hierarchies;
 
 	/*
@@ -2463,16 +2491,6 @@ InitExecPartitionPruneContexts(PartitionPruneState *prunestate,
 	Assert(parent_plan != NULL);
 	estate = parent_plan->state;
 
-	/*
-	 * PartitionPruneStates are sometimes shared and this one may have been
-	 * initialized already.  Sharing of states occurs for EPQ, for example.
-	 * See EvalPlanQualStart().
-	 */
-	if (prunestate->initialized)
-		return;
-
-	prunestate->initialized = true;
-
 	/*
 	 * No need to fix subplans maps if initial pruning didn't eliminate any
 	 * subplans.
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_copy`
- 大写宏：`EPQ`
- 大写宏：`NULL`
- 外部类型：`ExecInitPartitionExecPruning`
- 外部类型：`InitExecPartitionPruneContexts`
- 外部类型：`Initialize`
- 外部类型：`No`
- 外部类型：`PartitionPruneStates`
- 外部类型：`See`
- 外部类型：`Sharing`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-50638c839b` → 本草稿移入 `cases/defect/auto-postgres-50638c839b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
