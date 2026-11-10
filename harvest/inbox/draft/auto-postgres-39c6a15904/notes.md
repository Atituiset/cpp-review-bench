# auto-postgres-39c6a15904

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#952bab8b74d40fcfd25e1949865c3907e676504e](https://github.com/postgres/postgres/commit/952bab8b74d40fcfd25e1949865c3907e676504e) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #952bab8b74d40fcfd25e1949865c3907e676504e (https://github.com/postgres/postgres/commit/952bab8b74d40fcfd25e1949865c3907e676504e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1031；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 952bab8b74d40fcfd25e1949865c3907e676504e Simplify leader error reporting for parallel apply worker failures. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1027,39 +1027,23 @@ ProcessParallelApplyMessage(StringInfo msg)
 	switch (msgtype)
 	{
 		case PqMsg_ErrorResponse:
-			{
-				ErrorData	edata;
-
-				/* Parse ErrorResponse. */
-				pq_parse_errornotice(msg, &edata);
-
-				/*
-				 * If desired, add a context line to show that this is a
-				 * message propagated from a parallel apply worker. Otherwise,
-				 * it can sometimes be confusing to understand what actually
-				 * happened.
-				 */
-				if (edata.context)
-					edata.context = psprintf("%s\n%s", edata.context,
-											 _("logical replication parallel apply worker"));
-				else
-					edata.context = pstrdup(_("logical replication parallel apply worker"));
 
-				/*
-				 * Context beyond that should use the error context callbacks
-				 * that were in effect in LogicalRepApplyLoop().
-				 */
-				error_context_stack = apply_error_context_stack;
+			/*
+			 * The parallel apply worker has already reported the error with
+			 * the context of the transaction that failed.  Report the error
+			 * of the leader apply worker with the error context callbacks
+			 * that were in effect.
+			 */
+			error_context_stack = apply_error_context_stack;
 
-				/*
-				 * The actual error must have been reported by the parallel
-				 * apply worker.
-				 */
-				ereport(ERROR,
-						(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
-						 errmsg("logical replication parallel apply worker exited due to error"),
-						 errcontext("%s", edata.context)));
-			}
+			/*
+			 * The actual error must have been reported by the parallel apply
+			 * worker.
+			 */
+			ereport(ERROR,
+					(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
+					 errmsg("logical replication parallel apply worker exited due to error"),
+					 errdetail("The leader apply worker will now exit and terminate any other parallel apply workers.")));
 
 			/*
 			 * Don't need to do anything about NoticeResponse and
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errcontext`
- 外部函数：`errmsg`
- 外部函数：`pq_parse_errornotice`
- 外部函数：`psprintf`
- 外部函数：`pstrdup`
- 大写宏：`ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE`
- 大写宏：`ERROR`
- 外部类型：`Context`
- 外部类型：`Don`
- 外部类型：`ErrorData`
- 外部类型：`ErrorResponse`
- 外部类型：`If`
- 外部类型：`NoticeResponse`
- 外部类型：`Otherwise`
- 外部类型：`Parse`
- 外部类型：`PqMsg_ErrorResponse`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-39c6a15904` → 本草稿移入 `cases/defect/auto-postgres-39c6a15904/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
