# auto-postgres-35f43cc044

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#126976a8ffcfa47f40d3df09243ebf4606001912](https://github.com/postgres/postgres/commit/126976a8ffcfa47f40d3df09243ebf4606001912) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 17 |
| 编译错误数（gcc syntax-only） | 20（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #126976a8ffcfa47f40d3df09243ebf4606001912 (https://github.com/postgres/postgres/commit/126976a8ffcfa47f40d3df09243ebf4606001912)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1147；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 126976a8ffcfa47f40d3df09243ebf4606001912 Emit null-keyed hash join outer tuples immediately when feasible. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -195,10 +195,12 @@
 
 static TupleTableSlot *ExecHashJoinOuterGetTuple(PlanState *outerNode,
 												 HashJoinState *hjstate,
-												 uint32 *hashvalue);
+												 uint32 *hashvalue,
+												 bool *is_null);
 static TupleTableSlot *ExecParallelHashJoinOuterGetTuple(PlanState *outerNode,
 														 HashJoinState *hjstate,
-														 uint32 *hashvalue);
+														 uint32 *hashvalue,
+														 bool *is_null);
 static TupleTableSlot *ExecHashJoinGetSavedTuple(HashJoinState *hjstate,
 												 BufFile *file,
 												 uint32 *hashvalue,
@@ -233,6 +235,7 @@ ExecHashJoinImpl(PlanState *pstate, bool parallel)
 	HashJoinTable hashtable;
 	TupleTableSlot *outerTupleSlot;
 	uint32		hashvalue;
+	bool		is_null;
 	int			batchno;
 	ParallelHashJoinState *parallel_state;
 
@@ -440,10 +443,13 @@ ExecHashJoinImpl(PlanState *pstate, bool parallel)
 				if (parallel)
 					outerTupleSlot =
 						ExecParallelHashJoinOuterGetTuple(outerNode, node,
-														  &hashvalue);
+														  &hashvalue,
+														  &is_null);
 				else
 					outerTupleSlot =
-						ExecHashJoinOuterGetTuple(outerNode, node, &hashvalue);
+						ExecHashJoinOuterGetTuple(outerNode, node,
+												  &hashvalue,
+												  &is_null);
 
 				if (TupIsNull(outerTupleSlot))
 				{
@@ -489,6 +495,16 @@ ExecHashJoinImpl(PlanState *pstate, bool parallel)
 				econtext->ecxt_outertuple = outerTupleSlot;
 				node->hj_MatchedOuter = false;
 
+				/*
+				 * If the tuple can't match anything, skip all the tedium and
+				 * go directly to emitting it in null-extended form.
+				 */
+				if (is_null)
+				{
+					node->hj_JoinState = HJ_FILL_OUTER_TUPLE;
+					continue;
+				}
+
 				/*
 				 * Find the corresponding bucket for this tuple in the main
 				 * hash table or skew hash table.
@@ -1104,12 +1120,15 @@ ExecEndHashJoin(HashJoinState *node)
  * Returns a null slot if no more outer tuples (within the current batch).
  *
  * On success, the tuple's hash value is stored at *hashvalue --- this is
- * either originally computed, or re-read from the temp file.
+ * either originally computed, or re-read from the temp file.  Also, we
+ * set *is_null to true if the tuple cannot have any matches due to
+ * containing a null join key (in which case the hashvalue is meaningless).
  */
 static TupleTableSlot *
 ExecHashJoinOuterGetTuple(PlanState *outerNode,
 						  HashJoinState *hjstate,
-						  uint32 *hashvalue)
+						  uint32 *hashvalue,
+						  bool *is_null)
 {
 	HashJoinTable hashtable = hjstate->hj_HashTable;
 	int			curbatch = hashtable->curbatch;
@@ -1129,8 +1148,6 @@ ExecHashJoinOuterGetTuple(PlanState *outerNode,
 
 		while (!TupIsNull(slot))
 		{
-			bool		isnull;
-
 			/*
 			 * We have to compute the tuple's hash value.
 			 */
@@ -1142,9 +1159,9 @@ ExecHashJoinOuterGetTuple(PlanState *outerNode,
 
 			*hashvalue = DatumGetUInt32(ExecEvalExprSwitchContext(hjstate->hj_OuterHash,
 																  econtext,
-																  &isnull));
+																  is_null));
 
-			if (!isnull)
+			if (!*is_null)
 			{
 				/* normal case with a non-null join key */
 				/* remember outer relation is not empty for possible rescan */
@@ -1154,10 +1171,8 @@ ExecHashJoinOuterGetTuple(PlanState *outerNode,
 			}
 			else if (hjstate->hj_KeepNullTuples)
 			{
-				/* null join key, but we must save tuple to be emitted later */
-				if (hjstate->hj_NullOuterTupleStore == NULL)
-					hjstate->hj_NullOuterTupleStore = ExecHashBuildNullTupleStore(hashtable);
-				tuplestore_puttupleslot(hjstate->hj_NullOuterTupleStore, slot);
+				/* null join key, but we must return tuple to be emitted */
+				return slot;
 			}
 
 			/*
@@ -1183,7 +1198,10 @@ ExecHashJoinOuterGetTuple(PlanState *outerNode,
 										 hashvalue,
 										 hjstate->hj_OuterTupleSlot);
 		if (!TupIsNull(slot))
+		{
+			*is_null = false;	/* else it'd not be in the batch file */
 			return slot;
+		}
 	}
 
 	/* End of this batch */
@@ -1196,7 +1214,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`palloc`
- 外部函数：`tuplestore_puttupleslot`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`NULL`
- 外部类型：`BufFile`
- 外部类型：`End`
- 外部类型：`ExprContext`
- 外部类型：`Find`
- 外部类型：`HashJoinState`
- 外部类型：`HashJoinTable`
- 外部类型：`MinimalTuple`
- 外部类型：`On`
- 外部类型：`ParallelHashJoinState`
- 外部类型：`PlanState`
- 外部类型：`Returns`
- 外部类型：`Since`
- 外部类型：`TupleTableSlot`
- 外部类型：`We`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-35f43cc044` → 本草稿移入 `cases/defect/auto-postgres-35f43cc044/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
