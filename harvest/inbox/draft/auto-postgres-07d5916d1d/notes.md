# auto-postgres-07d5916d1d

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
| 外部依赖数（dep_count） | 18 |
| 编译错误数（gcc syntax-only） | 11（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #73e69062c120f669f71531dabeacfb8041c85daf (https://github.com/postgres/postgres/commit/73e69062c120f669f71531dabeacfb8041c85daf)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 131；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 73e69062c120f669f71531dabeacfb8041c85daf Fix EPQ run-time pruning using the parent's PARAM_EXEC values :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -113,8 +113,6 @@ typedef struct PartitionPruningData
  *						startup (at any hierarchy level).
  * do_exec_prune		true if pruning should be performed during
  *						executor run (at any hierarchy level).
- * initialized			true if InitExecPartitionPruneContexts has been called
- *						on this PartitionPruneState
  * num_partprunedata	Number of items in "partprunedata" array.
  * partprunedata		Array of PartitionPruningData pointers for the plan's
  *						partitioned relation(s), one for each partitioning
@@ -128,12 +126,12 @@ typedef struct PartitionPruneState
 	MemoryContext prune_context;
 	bool		do_initial_prune;
 	bool		do_exec_prune;
-	bool		initialized;
 	int			num_partprunedata;
 	PartitionPruningData *partprunedata[FLEXIBLE_ARRAY_MEMBER];
 } PartitionPruneState;
 
 extern void ExecDoInitialPruning(EState *estate);
+extern void ExecCreatePartitionPruneStates(EState *estate);
 extern PartitionPruneState *ExecInitPartitionExecPruning(PlanState *planstate,
 														 int n_total_subplans,
 														 int part_prune_index,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`relation`
- 外部函数：`run`
- 外部函数：`startup`
- 大写宏：`FLEXIBLE_ARRAY_MEMBER`
- 外部类型：`Array`
- 外部类型：`Bitmapset`
- 外部类型：`EState`
- 外部类型：`ExprContext`
- 外部类型：`InitExecPartitionPruneContexts`
- 外部类型：`List`
- 外部类型：`MemoryContext`
- 外部类型：`Number`
- 外部类型：`PartitionPruneContext`
- 外部类型：`PartitionPruneState`
- 外部类型：`PartitionPruningData`
- 外部类型：`PartitionedRelPruningData`
- 外部类型：`PlanState`
- 外部类型：`Relation`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-07d5916d1d` → 本草稿移入 `cases/defect/auto-postgres-07d5916d1d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
