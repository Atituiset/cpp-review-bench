# auto-postgres-d9d643fc39

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#a241bc4fedefae3c0b0952c0a9d3a28d7747bfe3](https://github.com/postgres/postgres/commit/a241bc4fedefae3c0b0952c0a9d3a28d7747bfe3) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 3 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a241bc4fedefae3c0b0952c0a9d3a28d7747bfe3 (https://github.com/postgres/postgres/commit/a241bc4fedefae3c0b0952c0a9d3a28d7747bfe3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a241bc4fedefae3c0b0952c0a9d3a28d7747bfe3 Show Material node Maximum Storage for parallel workers :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -197,6 +197,7 @@ ExecInitMaterial(Material *node, EState *estate, int eflags)
 
 	matstate->eof_underlying = false;
 	matstate->tuplestorestate = NULL;
+	matstate->shared_info = NULL;
 
 	/*
 	 * Miscellaneous initialization
@@ -240,6 +241,29 @@ ExecInitMaterial(Material *node, EState *estate, int eflags)
 void
 ExecEndMaterial(MaterialState *node)
 {
+	/*
+	 * When ending a parallel worker, copy the tuplestore usage details into
+	 * shared memory so that they can be picked up by the main process to
+	 * report in EXPLAIN ANALYZE.  The Gather node above us may be rescanned,
+	 * in which case a new set of workers will write to the same slots, so
+	 * merge with any existing values rather than overwriting them.
+	 */
+	if (node->shared_info != NULL && IsParallelWorker() &&
+		node->tuplestorestate != NULL)
+	{
+		MaterialInstrumentation *si;
+		bool		usedDisk;
+		int64		maxSpaceUsed;
+
+		Assert(ParallelWorkerNumber < node->shared_info->num_workers);
+
+		tuplestore_get_stats(node->tuplestorestate, &usedDisk, &maxSpaceUsed);
+
+		si = &node->shared_info->sinstrument[ParallelWorkerNumber];
+		si->maxSpaceUsed = Max(si->maxSpaceUsed, maxSpaceUsed);
+		si->usedDisk |= usedDisk;
+	}
+
 	/*
 	 * Release tuplestore resources
 	 */
@@ -365,3 +389,89 @@ ExecReScanMaterial(MaterialState *node)
 		node->eof_underlying = false;
 	}
 }
+
+/* ----------------------------------------------------------------
+ *						Parallel Query Support
+ * ----------------------------------------------------------------
+ */
+
+/* ----------------------------------------------------------------
+ *		ExecMaterialEstimate
+ *
+ *		Estimate space required to propagate material statistics.
+ * ----------------------------------------------------------------
+ */
+void
+ExecMaterialEstimate(MaterialState *node, ParallelContext *pcxt)
+{
+	Size		size;
+
+	/* don't need this if not instrumenting or no workers */
+	if (!node->ss.ps.instrument || pcxt->nworkers == 0)
+		return;
+
+	size = mul_size(pcxt->nworkers, sizeof(MaterialInstrumentation));
+	size = add_size(size, offsetof(SharedMaterialInfo, sinstrument));
+	shm_toc_estimate_chunk(&pcxt->estimator, size);
+	shm_toc_estimate_keys(&pcxt->estimator, 1);
+}
+
+/* ----------------------------------------------------------------
+ *		ExecMaterialInitializeDSM
+ *
+ *		Initialize DSM space for material statistics.
+ * ----------------------------------------------------------------
+ */
+void
+ExecMaterialInitializeDSM(MaterialState *node, ParallelContext *pcxt)
+{
+	Size		size;
+
+	/* don't need this if not instrumenting or no workers */
+	if (!node->ss.ps.instrument || pcxt->nworkers == 0)
+		return;
+
+	size = offsetof(SharedMaterialInfo, sinstrument)
+		+ pcxt->nworkers * sizeof(MaterialInstrumentation);
+	node->shared_info = shm_toc_allocate(pcxt->toc, size);
+	/* ensure any unfilled slots will contain zeroes */
+	memset(node->shared_info, 0, size);
+	node->shared_info->num_workers = pcxt->nworkers;
+	shm_toc_insert(pcxt->toc, node->ss.ps.plan->plan_node_id,
+				   node->shared_info);
+}
+
+/* ----------------------------------------------------------------
+ *		ExecMaterialInitializeWorker
+ *
+ *		Attach worker to DSM space for material statistics.
+ * ----------------------------------------------------------------
+ */
+void
+ExecMaterialInitializeWorker(MaterialState *node, ParallelWorkerContext *pwcxt)
+{
+	node->shared_info =
+		shm_toc_lookup(pwcxt->toc, node->ss.ps.plan->plan_node_id, true);
+}
+
+/* ----------------------------------------------------------------
+ *		ExecMaterialRetrieveInstrumentation
+ *
+ *		Transfer material statistics from DSM to private memory.
+ * ----------------------------------------------------------------
+ */
+void
+ExecMaterialRetrieveInstrumentation(MaterialState *node)
+{
+	Size		size;
+	SharedMaterialInfo *si;
+
+	if (node->shared_info == NULL)
+		return;
+
+	size = offsetof(SharedMaterialInfo, sinstrument)
+		+ node->shared_info->num_workers * sizeof(Materia
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`NULL`
- 外部类型：`MaterialState`
- 外部类型：`Miscellaneous`
- 外部类型：`Release`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-d9d643fc39` → 本草稿移入 `cases/defect/auto-postgres-d9d643fc39/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
