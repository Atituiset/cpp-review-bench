# auto-postgres-0b764791aa

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#76c0df7381e5e4d3fba06abddcb51216821c02bc](https://github.com/postgres/postgres/commit/76c0df7381e5e4d3fba06abddcb51216821c02bc) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 22（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #76c0df7381e5e4d3fba06abddcb51216821c02bc (https://github.com/postgres/postgres/commit/76c0df7381e5e4d3fba06abddcb51216821c02bc)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1073；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 76c0df7381e5e4d3fba06abddcb51216821c02bc Remove needless initialization work in ExecInitMemoize :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1037,40 +1037,60 @@ ExecInitMemoize(Memoize *node, EState *estate, int eflags)
 												 &TTSOpsMinimalTuple);
 	mstate->probeslot = MakeSingleTupleTableSlot(mstate->hashkeydesc,
 												 &TTSOpsVirtual);
-
 	mstate->param_exprs = palloc_array(ExprState *, nkeys);
+	mstate->cache_eq_expr = NULL;	/* May be adjusted below */
+	mstate->hashfunctions = NULL;	/* May be adjusted below */
 	mstate->collations = node->collations;	/* Just point directly to the plan
 											 * data */
-	mstate->hashfunctions = palloc_array(FmgrInfo, nkeys);
-
-	eqfuncoids = palloc_array(Oid, nkeys);
 
 	for (i = 0; i < nkeys; i++)
 	{
-		Oid			hashop = node->hashOperators[i];
-		Oid			left_hashfn;
-		Oid			right_hashfn;
 		Expr	   *param_expr = (Expr *) list_nth(node->param_exprs, i);
 
-		if (!get_op_hash_functions(hashop, &left_hashfn, &right_hashfn))
-			elog(ERROR, "could not find hash function for hash operator %u",
-				 hashop);
-
-		fmgr_info(left_hashfn, &mstate->hashfunctions[i]);
-
 		mstate->param_exprs[i] = ExecInitExpr(param_expr, (PlanState *) mstate);
-		eqfuncoids[i] = get_opcode(hashop);
 	}
 
-	mstate->cache_eq_expr = ExecBuildParamSetEqual(mstate->hashkeydesc,
-												   &TTSOpsMinimalTuple,
-												   &TTSOpsVirtual,
-												   eqfuncoids,
-												   node->collations,
-												   node->param_exprs,
-												   (PlanState *) mstate);
+	/*
+	 * Record if the cache keys should be compared bit by bit, or logically
+	 * using the hash equality operator
+	 */
+	mstate->binary_mode = node->binary_mode;
+
+	/*
+	 * In binary mode, the cache keys are hashed and compared using
+	 * datum_image_hash() and datum_image_eq(), so the hash functions and
+	 * equality expression are only needed when not in binary mode.
+	 */
+	if (!mstate->binary_mode)
+	{
+		mstate->hashfunctions = palloc_array(FmgrInfo, nkeys);
+		eqfuncoids = palloc_array(Oid, nkeys);
+
+		for (i = 0; i < nkeys; i++)
+		{
+			Oid			hashop = node->hashOperators[i];
+			Oid			left_hashfn;
+			Oid			right_hashfn;
+
+			if (!get_op_hash_functions(hashop, &left_hashfn, &right_hashfn))
+				elog(ERROR, "could not find hash function for hash operator %u",
+					 hashop);
+
+			fmgr_info(left_hashfn, &mstate->hashfunctions[i]);
+			eqfuncoids[i] = get_opcode(hashop);
+		}
+
+		mstate->cache_eq_expr = ExecBuildParamSetEqual(mstate->hashkeydesc,
+													   &TTSOpsMinimalTuple,
+													   &TTSOpsVirtual,
+													   eqfuncoids,
+													   node->collations,
+													   node->param_exprs,
+													   (PlanState *) mstate);
+
+		pfree(eqfuncoids);
+	}
 
-	pfree(eqfuncoids);
 	mstate->mem_used = 0;
 
 	/* Limit the total memory consumed by the cache to this */
@@ -1096,12 +1116,6 @@ ExecInitMemoize(Memoize *node, EState *estate, int eflags)
 	mstate->singlerow = node->singlerow;
 	mstate->keyparamids = node->keyparamids;
 
-	/*
-	 * Record if the cache keys should be compared bit by bit, or logically
-	 * using the type's hash equality operator
-	 */
-	mstate->binary_mode = node->binary_mode;
-
 	/* Zero the statistics counters */
 	memset(&mstate->stats, 0, sizeof(MemoizeInstrumentation));
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`fmgr_info`
- 外部函数：`get_op_hash_functions`
- 外部函数：`get_opcode`
- 外部函数：`list_nth`
- 外部函数：`memset`
- 外部函数：`palloc_array`
- 外部函数：`pfree`
- 大写宏：`ERROR`
- 外部类型：`Expr`
- 外部类型：`ExprState`
- 外部类型：`FmgrInfo`
- 外部类型：`Just`
- 外部类型：`Limit`
- 外部类型：`MemoizeInstrumentation`
- 外部类型：`Oid`
- 外部类型：`PlanState`
- 外部类型：`Record`
- 外部类型：`TTSOpsMinimalTuple`
- 外部类型：`TTSOpsVirtual`
- 外部类型：`Zero`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-0b764791aa` → 本草稿移入 `cases/defect/auto-postgres-0b764791aa/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
