# auto-postgres-31aabee88b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#edbb1d2dcb18d0706e644d3048a21e4475debd95](https://github.com/postgres/postgres/commit/edbb1d2dcb18d0706e644d3048a21e4475debd95) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 49 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #edbb1d2dcb18d0706e644d3048a21e4475debd95 (https://github.com/postgres/postgres/commit/edbb1d2dcb18d0706e644d3048a21e4475debd95)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT edbb1d2dcb18d0706e644d3048a21e4475debd95 Add memory/disk usage for Function Scan nodes in EXPLAIN :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -130,6 +130,8 @@ static void show_hash_info(HashState *hashstate, ExplainState *es);
 static void show_material_info(MaterialState *mstate, ExplainState *es);
 static void show_windowagg_info(WindowAggState *winstate, ExplainState *es);
 static void show_ctescan_info(CteScanState *ctescanstate, ExplainState *es);
+static void show_functionscan_info(FunctionScanState *fsstate,
+								   ExplainState *es);
 static void show_table_func_scan_info(TableFuncScanState *tscanstate,
 									  ExplainState *es);
 static void show_recursive_union_info(RecursiveUnionState *rstate,
@@ -2097,6 +2099,7 @@ ExplainNode(PlanState *planstate, List *ancestors,
 			if (plan->qual)
 				show_instrumentation_count("Rows Removed by Filter", 1,
 										   planstate, es);
+			show_functionscan_info(castNode(FunctionScanState, planstate), es);
 			break;
 		case T_TableFuncScan:
 			if (es->verbose)
@@ -3570,6 +3573,75 @@ show_ctescan_info(CteScanState *ctescanstate, ExplainState *es)
 	show_storage_info(usedDisk, maxSpaceUsed, es);
 }
 
+/*
+ * Show information on Function Scan node, storage method and maximum
+ * memory/disk space used.
+ */
+static void
+show_functionscan_info(FunctionScanState *fsstate, ExplainState *es)
+{
+	bool		totalDiskUsed = false;
+	int64		totalSpaceUsed = 0;
+
+	if (!es->analyze)
+		return;
+
+	/*
+	 * Similar to nodeFunctionscan.c's update_storage_stats() function, we
+	 * need to fetch the stats from the final scan and use those values if
+	 * they're higher than what's already stored in FunctionScanState.stats.
+	 */
+	for (int i = 0; i < fsstate->nfuncs; i++)
+	{
+		FunctionScanPerFuncState *fs = &fsstate->funcstates[i];
+
+		if (fs->tstore != NULL)
+		{
+			bool		diskUsed;
+			int64		spaceUsed;
+
+			tuplestore_get_stats(fs->tstore, &diskUsed, &spaceUsed);
+			totalSpaceUsed += spaceUsed;
+			totalDiskUsed |= diskUsed;
+		}
+	}
+
+	fsstate->stats.maxSpaceUsed = Max(fsstate->stats.maxSpaceUsed,
+									  totalSpaceUsed);
+	fsstate->stats.usedDisk |= totalDiskUsed;
+
+	/*
+	 * Nothing to show for the leader if execution didn't get as far as
+	 * creating any tuplestore.
+	 */
+	if (fsstate->stats.maxSpaceUsed > 0)
+		show_storage_info(fsstate->stats.usedDisk,
+						  fsstate->stats.maxSpaceUsed, es);
+
+	if (fsstate->shared_info == NULL)
+		return;
+
+	/* Show details from parallel workers */
+	for (int n = 0; n < fsstate->shared_info->num_workers; n++)
+	{
+		FunctionScanInstrumentation *si;
+
+		si = &fsstate->shared_info->sinstrument[n];
+
+		/* Skip workers that didn't create a tuplestore */
+		if (si->maxSpaceUsed == 0)
+			continue;
+
+		if (es->workers_state)
+			ExplainOpenWorker(n, es);
+
+		show_storage_info(si->usedDisk, si->maxSpaceUsed, es);
+
+		if (es->workers_state)
+			ExplainCloseWorker(n, es);
+	}
+}
+
 /*
  * Show information on Table Function Scan node, storage method and maximum
  * memory/disk space used.
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`appendStringInfo`
- 外部函数：`initStringInfo`
- 外部函数：`show_storage_info`
- 外部函数：`tuplestore_get_stats`
- 大写宏：`ANALYZE`
- 大写宏：`EXPLAIN_FORMAT_TEXT`
- 大写宏：`INT64_FORMAT`
- 大写宏：`NULL`
- 大写宏：`TEXT`
- 外部类型：`CteScanState`
- 外部类型：`Disk`
- 外部类型：`ExplainState`
- 外部类型：`ExplainWorkersState`
- 外部类型：`Filter`
- 外部类型：`First`
- 外部类型：`Function`
- 外部类型：`In`
- 外部类型：`MaterialInstrumentation`
- 外部类型：`MaterialState`
- 外部类型：`Maximum`
- 外部类型：`Memory`
- 外部类型：`Nothing`
- 外部类型：`Number`
- 外部类型：`PlanState`
- 外部类型：`Push`
- 外部类型：`Recursive`
- 外部类型：`RecursiveUnionState`
- 外部类型：`Removed`
- 外部类型：`Resuming`
- 外部类型：`Rows`
- 外部类型：`Save`
- 外部类型：`Scan`
- 外部类型：`Show`
- 外部类型：`Skip`
- 外部类型：`Storage`
- 外部类型：`T_TableFuncScan`
- 外部类型：`Table`
- 外部类型：`TableFuncScanState`
- 外部类型：`The`
- 外部类型：`Tuplestorestate`
- 外部类型：`We`
- 外部类型：`WindowAggState`
- 外部类型：`Worker`
- 外部类型：`Workers`
- 外部类型：`node`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-31aabee88b` → 本草稿移入 `cases/defect/auto-postgres-31aabee88b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
