# auto-postgres-e3a8042d6c

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
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d9c6f22c3b2beaf34e47cf47fd21703886be8011 (https://github.com/postgres/postgres/commit/d9c6f22c3b2beaf34e47cf47fd21703886be8011)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 793；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d9c6f22c3b2beaf34e47cf47fd21703886be8011 Remove tuplesort_set_allowbounded() and adjust tuplesort_reset() :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -783,22 +783,6 @@ tuplesort_used_bound(Tuplesortstate *state)
 	return state->boundUsed;
 }
 
-/*
- * tuplesort_set_allowbounded
- *		Set or unset TUPLESORT_ALLOWBOUNDED sort flag on an existing state.
- *
- * NOTE: tuplesort_reset() must be called after this function for the changes
- * to take effect.
- */
-void
-tuplesort_set_allowbounded(Tuplesortstate *state, bool allowbounded)
-{
-	if (allowbounded)
-		state->base.sortopt |= TUPLESORT_ALLOWBOUNDED;
-	else
-		state->base.sortopt &= ~TUPLESORT_ALLOWBOUNDED;
-}
-
 /*
  * tuplesort_free
  *
@@ -924,14 +908,26 @@ tuplesort_updatemax(Tuplesortstate *state)
  *	Reset the tuplesort.  Reset all the data in the tuplesort, but leave the
  *	meta-information in.  After tuplesort_reset, tuplesort is ready to start
  *	a new sort.  This allows avoiding recreation of tuple sort states (and
- *	save resources) when sorting multiple small batches.
+ *	save resources) when sorting multiple small batches.  We also allow the
+ *	TUPLESORT_ALLOWBOUNDED flag to be set/unset here as the bounded
+ *	requirements may change between rescans in Incremental Sort, and we use
+ *	tuplesort_reset() to clear out the tuples ready for that new scan.
  */
 void
-tuplesort_reset(Tuplesortstate *state)
+tuplesort_reset(Tuplesortstate *state, bool allowbounded)
 {
 	tuplesort_updatemax(state);
 	tuplesort_free(state);
 
+	/*
+	 * Do this before calling tuplesort_begin_batch() as it's too late to
+	 * change it once the tuplecontext is recreated.
+	 */
+	if (allowbounded)
+		state->base.sortopt |= TUPLESORT_ALLOWBOUNDED;
+	else
+		state->base.sortopt &= ~TUPLESORT_ALLOWBOUNDED;
+
 	/*
 	 * After we've freed up per-batch memory, re-setup all of the state common
 	 * to both the first batch and any subsequent batch.
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`pg_rusage_show`
- 外部函数：`tuplesort_set_allowbounded`
- 大写宏：`API`
- 大写宏：`BLCKSZ`
- 大写宏：`LOG`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`TRACE_POSTGRESQL_SORT_DONE`
- 大写宏：`TSS_FINALMERGE`
- 大写宏：`TUPLESORT_ALLOWBOUNDED`
- 外部类型：`After`
- 外部类型：`Delete`
- 外部类型：`Free`
- 外部类型：`However`
- 外部类型：`In`
- 外部类型：`Is`
- 外部类型：`MemoryContext`
- 外部类型：`Note`
- 外部类型：`PRId64`
- 外部类型：`Reset`
- 外部类型：`Set`
- 外部类型：`Sort`
- 外部类型：`They`
- 外部类型：`This`
- 外部类型：`Tuplesortstate`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-e3a8042d6c` → 本草稿移入 `cases/defect/auto-postgres-e3a8042d6c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
