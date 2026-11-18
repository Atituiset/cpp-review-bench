# auto-postgres-55023d4db4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d9c6f22c3b2beaf34e47cf47fd21703886be8011](https://github.com/postgres/postgres/commit/d9c6f22c3b2beaf34e47cf47fd21703886be8011) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d9c6f22c3b2beaf34e47cf47fd21703886be8011 (https://github.com/postgres/postgres/commit/d9c6f22c3b2beaf34e47cf47fd21703886be8011)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 321；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d9c6f22c3b2beaf34e47cf47fd21703886be8011 Remove tuplesort_set_allowbounded() and adjust tuplesort_reset() :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -318,7 +318,7 @@ switchToPresortedPrefixMode(PlanState *pstate)
 	else
 	{
 		/* Next group of presorted data */
-		tuplesort_reset(node->prefixsort_state);
+		tuplesort_reset(node->prefixsort_state, node->bounded);
 	}
 
 	/*
@@ -609,7 +609,7 @@ ExecIncrementalSort(PlanState *pstate)
 		else
 		{
 			/* Reset sort for the next batch. */
-			tuplesort_reset(fullsort_state);
+			tuplesort_reset(fullsort_state, node->bounded);
 		}
 
 		/*
@@ -1103,19 +1103,13 @@ ExecReScanIncrementalSort(IncrementalSortState *node)
 	 * column functions by checking to see if the full sort state has been
 	 * initialized yet, setting the sort states to null here might actually
 	 * cause a leak.  The IncrementalSortState's bounded flag may change
-	 * between scans, per ExecSetTupleBound(), so we must toggle that flag in
-	 * the sort options before calling tuplesort_reset().
+	 * between scans, per ExecSetTupleBound(), so we must pass the current
+	 * bounded setting to tuplesort_reset().
 	 */
 	if (node->fullsort_state != NULL)
-	{
-		tuplesort_set_allowbounded(node->fullsort_state, node->bounded);
-		tuplesort_reset(node->fullsort_state);
-	}
+		tuplesort_reset(node->fullsort_state, node->bounded);
 	if (node->prefixsort_state != NULL)
-	{
-		tuplesort_set_allowbounded(node->prefixsort_state, node->bounded);
-		tuplesort_reset(node->prefixsort_state);
-	}
+		tuplesort_reset(node->prefixsort_state, node->bounded);
 
 	/*
 	 * If chgParam of subnode is not null, then the plan will be re-scanned by
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`tuplesort_set_allowbounded`
- 大写宏：`NULL`
- 外部类型：`If`
- 外部类型：`IncrementalSortState`
- 外部类型：`Next`
- 外部类型：`Reset`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-55023d4db4` → 本草稿移入 `cases/defect/auto-postgres-55023d4db4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
