# auto-postgres-0b1c2cd294

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#93ec486803b703fbc7fe7ea332da92e30cefc381](https://github.com/postgres/postgres/commit/93ec486803b703fbc7fe7ea332da92e30cefc381) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #93ec486803b703fbc7fe7ea332da92e30cefc381 (https://github.com/postgres/postgres/commit/93ec486803b703fbc7fe7ea332da92e30cefc381)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 18（原始 PR diff 行 1271；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 93ec486803b703fbc7fe7ea332da92e30cefc381 Fix issue with code manually adjusting a TupleTableSlot :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1229,7 +1229,7 @@ prepare_hash_slot(AggStatePerHash perhash,
  * read as null in the context of the current grouping set.  Also stash the
  * current group bitmap where GroupingExpr can get at it.
  *
- * This relies on three conditions:
+ * This relies on four conditions:
  *
  * 1) Nothing is ever going to try and extract the whole tuple from this slot,
  * only reference it in evaluations, which will only access individual
@@ -1242,6 +1242,12 @@ prepare_hash_slot(AggStatePerHash perhash,
  * 3) Within a given phase, we never need to recover the value of an attribute
  * once it has been set to null.
  *
+ * 4) We must extract all remaining required attributes from the tuple before
+ * manually adjusting the tts_isnull elements.  This is required to prevent
+ * further deforming that might be required for the HAVING clause or
+ * projection step from re-extracting the NULL bitmap into the tts_isnull
+ * array and overwriting our manual adjustments.
+ *
  * Poking into the slot this way is a bit ugly, but the consensus is that the
  * alternative was worse.
  */
@@ -1267,8 +1273,15 @@ prepare_projection_slot(AggState *aggstate, TupleTableSlot *slot, int currentSet
 		{
 			ListCell   *lc;
 
-			/* all_grouped_cols is arranged in desc order */
-			slot_getsomeattrs(slot, linitial_int(aggstate->all_grouped_cols));
+			/*
+			 * Sanity check on max_colno_needed value.  all_grouped_cols is
+			 * arranged in desc order.
+			 */
+			Assert(aggstate->max_colno_needed >=
+				   linitial_int(aggstate->all_grouped_cols));
+
+			/* deform all remaining columns, per condition #4 above */
+			slot_getsomeattrs(slot, aggstate->max_colno_needed);
 
 			foreach(lc, aggstate->all_grouped_cols)
 			{
@@ -3721,6 +3734,19 @@ ExecInitAgg(Agg *node, EState *estate, int eflags)
 		/* Initialize this to 1, meaning nothing spilled, yet */
 		aggstate->hash_batches_used = 1;
 	}
+	else if (aggstate->all_grouped_cols != NIL)
+	{
+		Bitmapset  *varattnos;
+		Bitmapset  *aggvarattnos;
+		int			max_varattno;
+
+		/* Figure out the highest outer-side varattno required. */
+		find_cols(aggstate, &aggvarattnos, &varattnos);
+		max_varattno = bms_prev_member(varattnos, -1);
+		max_varattno = Max(max_varattno, bms_prev_member(aggvarattnos, -1));
+
+		aggstate->max_colno_needed = max_varattno;
+	}
 
 	/*
 	 * Initialize current phase-dependent values to initial phase. The initial
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`linitial_int`
- 外部函数：`slot_getsomeattrs`
- 外部类型：`Also`
- 外部类型：`GroupingExpr`
- 外部类型：`Initialize`
- 外部类型：`ListCell`
- 外部类型：`Nothing`
- 外部类型：`Poking`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Within`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-0b1c2cd294` → 本草稿移入 `cases/defect/auto-postgres-0b1c2cd294/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
