# auto-postgres-3870172761

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#72b9498df9a369e747f8d96efb5a6cd33c2451e2](https://github.com/postgres/postgres/commit/72b9498df9a369e747f8d96efb5a6cd33c2451e2) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #72b9498df9a369e747f8d96efb5a6cd33c2451e2 (https://github.com/postgres/postgres/commit/72b9498df9a369e747f8d96efb5a6cd33c2451e2)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1203；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 72b9498df9a369e747f8d96efb5a6cd33c2451e2 file_fdw: Fix ANALYZE crashes with DEFAULT markers :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1200,7 +1200,8 @@ file_acquire_sample_rows(Relation onerel, int elevel,
 	CopyFromState cstate;
 	ErrorContextCallback errcallback;
 	MemoryContext oldcontext = CurrentMemoryContext;
-	MemoryContext tupcontext;
+	EState	   *estate;
+	ExprContext *econtext;
 
 	Assert(onerel);
 	Assert(targrows > 0);
@@ -1219,12 +1220,11 @@ file_acquire_sample_rows(Relation onerel, int elevel,
 						   options);
 
 	/*
-	 * Use per-tuple memory context to prevent leak of memory used to read
-	 * rows from the file with Copy routines.
+	 * Create an expression context to evaluate DEFAULT expressions.  Use its
+	 * per-tuple memory context for reading rows with the Copy routines, too.
 	 */
-	tupcontext = AllocSetContextCreate(CurrentMemoryContext,
-									   "file_fdw temporary context",
-									   ALLOCSET_DEFAULT_SIZES);
+	estate = CreateExecutorState();
+	econtext = GetPerTupleExprContext(estate);
 
 	/* Prepare for sampling rows */
 	reservoir_init_selection_state(&rstate, targrows);
@@ -1242,11 +1242,13 @@ file_acquire_sample_rows(Relation onerel, int elevel,
 		/* Check for user-requested abort or sleep */
 		vacuum_delay_point(true);
 
-		/* Fetch next row */
-		MemoryContextReset(tupcontext);
-		MemoryContextSwitchTo(tupcontext);
+		/* Clean up the previous row's expression evaluation state */
+		ResetPerTupleExprContext(estate);
+
+		/* Fetch next row in the per-tuple memory context */
+		MemoryContextSwitchTo(GetPerTupleMemoryContext(estate));
 
-		found = NextCopyFrom(cstate, NULL, values, nulls);
+		found = NextCopyFrom(cstate, econtext, values, nulls);
 
 		MemoryContextSwitchTo(oldcontext);
 
@@ -1311,7 +1313,7 @@ file_acquire_sample_rows(Relation onerel, int elevel,
 	error_context_stack = errcallback.previous;
 
 	/* Clean up. */
-	MemoryContextDelete(tupcontext);
+	FreeExecutorState(estate);
 
 	if (cstate->opts.on_error == COPY_ON_ERROR_IGNORE &&
 		cstate->num_errors > 0 &&
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`reservoir_init_selection_state`
- 外部函数：`vacuum_delay_point`
- 大写宏：`ALLOCSET_DEFAULT_SIZES`
- 大写宏：`COPY_ON_ERROR_IGNORE`
- 大写宏：`NULL`
- 外部类型：`Check`
- 外部类型：`Clean`
- 外部类型：`Copy`
- 外部类型：`CopyFromState`
- 外部类型：`CurrentMemoryContext`
- 外部类型：`ErrorContextCallback`
- 外部类型：`Fetch`
- 外部类型：`MemoryContext`
- 外部类型：`Prepare`
- 外部类型：`Use`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-3870172761` → 本草稿移入 `cases/defect/auto-postgres-3870172761/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
