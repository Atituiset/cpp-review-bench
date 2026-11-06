# auto-postgres-c675a11766

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f](https://github.com/postgres/postgres/commit/edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 34 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f (https://github.com/postgres/postgres/commit/edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 602；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT edbe7ffdc8afe7f3c12675df6ef614bcd5017b3f Fix crash with single-input aggregates with multiple sort keys :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -595,11 +595,11 @@ initialize_aggregate(AggState *aggstate, AggStatePerTrans pertrans,
 
 
 		/*
-		 * We use a plain Datum sorter when there's a single input column;
-		 * otherwise sort the full tuple.  (See comments for
-		 * process_ordered_aggregate_single.)
+		 * We use a plain Datum sorter when there's a single input column and
+		 * a single sort key; otherwise sort the full tuple.  (See comments
+		 * for process_ordered_aggregate_single.)
 		 */
-		if (pertrans->numInputs == 1)
+		if (pertrans->numInputs == 1 && pertrans->numSortCols == 1)
 		{
 			Form_pg_attribute attr = TupleDescAttr(pertrans->sortdesc, 0);
 
@@ -935,7 +935,7 @@ process_ordered_aggregate_single(AggState *aggstate,
 
 /*
  * Run the transition function for a DISTINCT or ORDER BY aggregate
- * with more than one input.  This is called after we have completed
+ * with multiple inputs or sort keys.  This is called after we have completed
  * entering all the input values into the sort object.  We complete the
  * sort, read out the values in sorted order, and run the transition
  * function on each value (applying DISTINCT if appropriate).
@@ -1329,7 +1329,7 @@ finalize_aggregates(AggState *aggstate,
 			Assert(aggstate->aggstrategy != AGG_HASHED &&
 				   aggstate->aggstrategy != AGG_MIXED);
 
-			if (pertrans->numInputs == 1)
+			if (pertrans->numInputs == 1 && pertrans->numSortCols == 1)
 				process_ordered_aggregate_single(aggstate,
 												 pertrans,
 												 pergroupstate);
@@ -4333,9 +4333,15 @@ build_pertrans_for_aggref(AggStatePerTrans pertrans,
 							&pertrans->inputtypeLen,
 							&pertrans->inputtypeByVal);
 		}
-		else if (numDistinctCols > 0)
+
+		if (numDistinctCols > 1)
 		{
-			/* we will need an extra slot to store prior values */
+			/*
+			 * With more than one DISTINCT column we'll need an extra slot to
+			 * store the prior values.  Note that this can happen even with a
+			 * single input, since the aggregate's ORDER BY may list the same
+			 * argument more than once with different sort operators.
+			 */
 			pertrans->uniqslot =
 				ExecInitExtraTupleSlot(estate, pertrans->sortdesc,
 									   &TTSOpsMinimalTuple);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`datumCopy`
- 外部函数：`its`
- 外部函数：`pfree`
- 外部函数：`tuplesort_getdatum`
- 外部函数：`tuplesort_performsort`
- 大写宏：`DISTINCT`
- 大写宏：`NULL`
- 外部类型：`AggState`
- 外部类型：`AggStatePerGroup`
- 外部类型：`AggStatePerTrans`
- 外部类型：`But`
- 外部类型：`Clear`
- 外部类型：`Datum`
- 外部类型：`Don`
- 外部类型：`ExecAggCopyTransValue`
- 外部类型：`For`
- 外部类型：`Forget`
- 外部类型：`Form_pg_attribute`
- 外部类型：`FunctionCallInfo`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`Load`
- 外部类型：`MemoryContext`
- 外部类型：`Note`
- 外部类型：`Otherwise`
- 外部类型：`See`
- 外部类型：`This`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-c675a11766` → 本草稿移入 `cases/defect/auto-postgres-c675a11766/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
