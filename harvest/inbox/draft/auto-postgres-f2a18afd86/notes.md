# auto-postgres-f2a18afd86

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#a7f8cd811d65d1a51be87bdb890d7ebf7d49aa45](https://github.com/postgres/postgres/commit/a7f8cd811d65d1a51be87bdb890d7ebf7d49aa45) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 61 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a7f8cd811d65d1a51be87bdb890d7ebf7d49aa45 (https://github.com/postgres/postgres/commit/a7f8cd811d65d1a51be87bdb890d7ebf7d49aa45)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 65；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a7f8cd811d65d1a51be87bdb890d7ebf7d49aa45 Use tuplestore_clear instead of tuplestore_end in nodeTableFuncscan.c :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -59,11 +59,15 @@ TableFuncNext(TableFuncScanState *node)
 	scanslot = node->ss.ss_ScanTupleSlot;
 
 	/*
-	 * If first time through, read all tuples from function and put them in a
-	 * tuplestore. Subsequent calls just fetch tuples from tuplestore.
+	 * If first time through (or first since a parameter change), read all
+	 * tuples from function and put them in a tuplestore.  Subsequent calls
+	 * just fetch tuples from tuplestore.
 	 */
-	if (node->tupstore == NULL)
+	if (!node->tupstore_filled)
+	{
 		tfuncFetchRows(node, node->ss.ps.ps_ExprContext);
+		node->tupstore_filled = true;
+	}
 
 	/*
 	 * Get the next tuple from tuplestore.
@@ -172,6 +176,7 @@ ExecInitTableFuncScan(TableFuncScan *node, EState *estate, int eflags)
 							  "TableFunc per value context",
 							  ALLOCSET_DEFAULT_SIZES);
 	scanstate->opaque = NULL;	/* initialized at runtime */
+	scanstate->tupstore_filled = false;
 
 	scanstate->ns_names = tf->ns_names;
 
@@ -244,18 +249,17 @@ ExecReScanTableFuncScan(TableFuncScanState *node)
 	ExecScanReScan(&node->ss);
 
 	/*
-	 * Recompute when parameters are changed.
+	 * Recompute when parameters are changed.  It's important to use
+	 * tuplestore_clear() rather than tuplestore_end() here so that we keep
+	 * track of the maximum storage used by all rescans.
 	 */
 	if (chgparam)
 	{
 		if (node->tupstore != NULL)
-		{
-			tuplestore_end(node->tupstore);
-			node->tupstore = NULL;
-		}
+			tuplestore_clear(node->tupstore);
+		node->tupstore_filled = false;
 	}
-
-	if (node->tupstore != NULL)
+	else if (node->tupstore != NULL)
 		tuplestore_rescan(node->tupstore);
 }
 
@@ -275,9 +279,10 @@ tfuncFetchRows(TableFuncScanState *tstate, ExprContext *econtext)
 
 	Assert(tstate->opaque == NULL);
 
-	/* build tuplestore for the result */
+	/* build tuplestore for the result, unless we have one from a prior scan */
 	oldcxt = MemoryContextSwitchTo(econtext->ecxt_per_query_memory);
-	tstate->tupstore = tuplestore_begin_heap(false, false, work_mem);
+	if (tstate->tupstore == NULL)
+		tstate->tupstore = tuplestore_begin_heap(false, false, work_mem);
 
 	/*
 	 * Each call to fetch a new set of rows - of which there may be very many
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`call`
- 外部函数：`data`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errdetail`
- 外部函数：`errmsg`
- 外部函数：`lfirst`
- 外部函数：`lfirst_node`
- 外部函数：`strVal`
- 外部函数：`tuplestore_begin_heap`
- 外部函数：`tuplestore_end`
- 外部函数：`tuplestore_rescan`
- 大写宏：`ALLOCSET_DEFAULT_SIZES`
- 大写宏：`DEFAULT`
- 大写宏：`ERRCODE_NULL_VALUE_NOT_ALLOWED`
- 大写宏：`ERROR`
- 大写宏：`JSON_TABLE`
- 大写宏：`NULL`
- 大写宏：`PG_CATCH`
- 大写宏：`PG_END_TRY`
- 大写宏：`PG_RE_THROW`
- 大写宏：`PG_TRY`
- 大写宏：`URI`
- 大写宏：`XMLTABLE`
- 外部类型：`Datum`
- 外部类型：`Each`
- 外部类型：`Evaluate`
- 外部类型：`ExprContext`
- 外部类型：`ExprState`
- 外部类型：`Filter`
- 外部类型：`Form_pg_attribute`
- 外部类型：`FunctionScan`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`Install`
- 外部类型：`ListCell`
- 外部类型：`Load`
- 外部类型：`MemoryContext`
- 外部类型：`Our`
- 外部类型：`Recompute`
- 外部类型：`SetNamespace`
- 外部类型：`SetRowFilter`
- 外部类型：`String`
- 外部类型：`Subsequent`
- 外部类型：`TableFunc`
- 外部类型：`TableFuncRoutine`
- 外部类型：`TableFuncScan`
- 外部类型：`TableFuncScanState`
- 外部类型：`TupleDesc`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f2a18afd86` → 本草稿移入 `cases/defect/auto-postgres-f2a18afd86/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
