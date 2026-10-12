# auto-postgres-296becfe5d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#c6b8af429b3592b00d3240a24c5a27077ff0dd82](https://github.com/postgres/postgres/commit/c6b8af429b3592b00d3240a24c5a27077ff0dd82) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 49 |
| 编译错误数（gcc syntax-only） | 33（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c6b8af429b3592b00d3240a24c5a27077ff0dd82 (https://github.com/postgres/postgres/commit/c6b8af429b3592b00d3240a24c5a27077ff0dd82)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 122；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c6b8af429b3592b00d3240a24c5a27077ff0dd82 Change tuplestore_get_stats() API :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -119,7 +119,7 @@ static void show_window_def(WindowAggState *planstate,
 static void show_window_keys(StringInfo buf, PlanState *planstate,
 							 int nkeys, AttrNumber *keycols,
 							 List *ancestors, ExplainState *es);
-static void show_storage_info(char *maxStorageType, int64 maxSpaceUsed,
+static void show_storage_info(bool diskUsed, int64 maxSpaceUsed,
 							  ExplainState *es);
 static void show_tablesample(TableSampleClause *tsc, PlanState *planstate,
 							 List *ancestors, ExplainState *es);
@@ -3006,21 +3006,22 @@ show_window_keys(StringInfo buf, PlanState *planstate,
  * Show information on storage method and maximum memory/disk space used.
  */
 static void
-show_storage_info(char *maxStorageType, int64 maxSpaceUsed, ExplainState *es)
+show_storage_info(bool diskUsed, int64 maxSpaceUsed, ExplainState *es)
 {
 	int64		maxSpaceUsedKB = BYTES_TO_KILOBYTES(maxSpaceUsed);
+	const char *storageType = diskUsed ? "Disk" : "Memory";
 
 	if (es->format != EXPLAIN_FORMAT_TEXT)
 	{
-		ExplainPropertyText("Storage", maxStorageType, es);
+		ExplainPropertyText("Storage", storageType, es);
 		ExplainPropertyInteger("Maximum Storage", "kB", maxSpaceUsedKB, es);
 	}
 	else
 	{
 		ExplainIndentText(es);
 		appendStringInfo(es->str,
 						 "Storage: %s  Maximum Storage: " INT64_FORMAT "kB\n",
-						 maxStorageType,
+						 storageType,
 						 maxSpaceUsedKB);
 	}
 }
@@ -3485,7 +3486,7 @@ show_hash_info(HashState *hashstate, ExplainState *es)
 static void
 show_material_info(MaterialState *mstate, ExplainState *es)
 {
-	char	   *maxStorageType;
+	bool		usedDisk;
 	int64		maxSpaceUsed;
 
 	Tuplestorestate *tupstore = mstate->tuplestorestate;
@@ -3497,8 +3498,8 @@ show_material_info(MaterialState *mstate, ExplainState *es)
 	if (!es->analyze || tupstore == NULL)
 		return;
 
-	tuplestore_get_stats(tupstore, &maxStorageType, &maxSpaceUsed);
-	show_storage_info(maxStorageType, maxSpaceUsed, es);
+	tuplestore_get_stats(tupstore, &usedDisk, &maxSpaceUsed);
+	show_storage_info(usedDisk, maxSpaceUsed, es);
 }
 
 /*
@@ -3508,7 +3509,7 @@ show_material_info(MaterialState *mstate, ExplainState *es)
 static void
 show_windowagg_info(WindowAggState *winstate, ExplainState *es)
 {
-	char	   *maxStorageType;
+	bool		usedDisk;
 	int64		maxSpaceUsed;
 
 	Tuplestorestate *tupstore = winstate->buffer;
@@ -3520,8 +3521,8 @@ show_windowagg_info(WindowAggState *winstate, ExplainState *es)
 	if (!es->analyze || tupstore == NULL)
 		return;
 
-	tuplestore_get_stats(tupstore, &maxStorageType, &maxSpaceUsed);
-	show_storage_info(maxStorageType, maxSpaceUsed, es);
+	tuplestore_get_stats(tupstore, &usedDisk, &maxSpaceUsed);
+	show_storage_info(usedDisk, maxSpaceUsed, es);
 }
 
 /*
@@ -3531,16 +3532,16 @@ show_windowagg_info(WindowAggState *winstate, ExplainState *es)
 static void
 show_ctescan_info(CteScanState *ctescanstate, ExplainState *es)
 {
-	char	   *maxStorageType;
+	bool		usedDisk;
 	int64		maxSpaceUsed;
 
 	Tuplestorestate *tupstore = ctescanstate->leader->cte_table;
 
 	if (!es->analyze || tupstore == NULL)
 		return;
 
-	tuplestore_get_stats(tupstore, &maxStorageType, &maxSpaceUsed);
-	show_storage_info(maxStorageType, maxSpaceUsed, es);
+	tuplestore_get_stats(tupstore, &usedDisk, &maxSpaceUsed);
+	show_storage_info(usedDisk, maxSpaceUsed, es);
 }
 
 /*
@@ -3550,16 +3551,16 @@ show_ctescan_info(CteScanState *ctescanstate, ExplainState *es)
 static void
 show_table_func_scan_info(TableFuncScanState *tscanstate, ExplainState *es)
 {
-	char	   *maxStorageType;
+	bool		usedDisk;
 	int64		maxSpaceUsed;
 
 	Tuplestorestate *tupstore = tscanstate->tupstore;
 
 	if (!es->analyze || tupstore == NULL)
 		return;
 
-	tuplestore_get_stats(tupstore, &maxStorageType, &maxSpaceUsed);
-	show_storage_info(maxStorageType, maxSpaceUsed, es);
+	tuplestore_get_stats(tupstore, &usedDisk, &maxSpaceUsed);
+	show_storage_info(usedDisk, maxSpaceUsed, es);
 }
 
 /*
@@ -3569,8 +3570,8 @@ show_table_func_scan_info(TableFuncScanState *tscanstate, ExplainState *es)

```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`appendStringInfo`
- 外部函数：`appendStringInfoChar`
- 外部函数：`appendStringInfoString`
- 外部函数：`deparse_expression`
- 外部函数：`elog`
- 外部函数：`get_func_name`
- 外部函数：`get_tle_by_resno`
- 外部函数：`lappend`
- 外部函数：`lfirst`
- 外部函数：`pfree`
- 外部函数：`s`
- 外部函数：`set_deparse_context_plan`
- 外部函数：`tuplestore_get_stats`
- 大写宏：`ERROR`
- 大写宏：`EXPLAIN_FORMAT_TEXT`
- 大写宏：`INT64_FORMAT`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`REPEATABLE`
- 外部类型：`AttrNumber`
- 外部类型：`CteScanState`
- 外部类型：`Deparse`
- 外部类型：`ExplainState`
- 外部类型：`Get`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`MaterialState`
- 外部类型：`Maximum`
- 外部类型：`Method`
- 外部类型：`Node`
- 外部类型：`Parameters`
- 外部类型：`Plan`
- 外部类型：`PlanState`
- 外部类型：`Print`
- 外部类型：`RecursiveUnionState`
- 外部类型：`Repeatable`
- 外部类型：`Sampling`
- 外部类型：`Seed`
- 外部类型：`Set`
- 外部类型：`Show`
- 外部类型：`Storage`
- 外部类型：`StringInfo`
- 外部类型：`TableFuncScanState`
- 外部类型：`TableSampleClause`
- 外部类型：`TargetEntry`
- 外部类型：`The`
- 外部类型：`Tuplestorestate`
- 外部类型：`We`
- 外部类型：`WindowAgg`
- 外部类型：`WindowAggState`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-296becfe5d` → 本草稿移入 `cases/defect/auto-postgres-296becfe5d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
