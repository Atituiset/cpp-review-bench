# auto-postgres-2f196713b3

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
| 外部依赖数（dep_count） | 5 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #73e69062c120f669f71531dabeacfb8041c85daf (https://github.com/postgres/postgres/commit/73e69062c120f669f71531dabeacfb8041c85daf)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 3160；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 73e69062c120f669f71531dabeacfb8041c85daf Fix EPQ run-time pruning using the parent's PARAM_EXEC values :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3157,12 +3157,17 @@ EvalPlanQualStart(EPQState *epqstate, Plan *planTree)
 	 * and MergeAppend subplans as the parent did.
 	 */
 	rcestate->es_part_prune_infos = parentestate->es_part_prune_infos;
-	rcestate->es_part_prune_states = parentestate->es_part_prune_states;
 	rcestate->es_part_prune_results = parentestate->es_part_prune_results;
 
 	/* We'll also borrow the es_partition_directory from the parent state */
 	rcestate->es_partition_directory = parentestate->es_partition_directory;
 
+	/*
+	 * The PartitionPruneStates can't be borrowed, though, since exec pruning
+	 * must see our own es_param_exec_vals.
+	 */
+	ExecCreatePartitionPruneStates(rcestate);
+
 	/*
 	 * Initialize private state information for each SubPlan.  We must do this
 	 * before running ExecInitNode on the main query tree, since
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部类型：`ExecInitNode`
- 外部类型：`Initialize`
- 外部类型：`MergeAppend`
- 外部类型：`SubPlan`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-2f196713b3` → 本草稿移入 `cases/defect/auto-postgres-2f196713b3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
