# auto-postgres-fb99430c05

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#3b249c728fd126c684be77ae834cb7eaa38d0962](https://github.com/postgres/postgres/commit/3b249c728fd126c684be77ae834cb7eaa38d0962) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 6 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3b249c728fd126c684be77ae834cb7eaa38d0962 (https://github.com/postgres/postgres/commit/3b249c728fd126c684be77ae834cb7eaa38d0962)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3b249c728fd126c684be77ae834cb7eaa38d0962 Fix losing a sinval catchup signal that arrives during catchup :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -556,6 +556,8 @@ SIGetDataEntries(SharedInvalidationMessage *data, int datasize)
  * backends as "reset" in the array, and sending PROCSIG_CATCHUP_INTERRUPT
  * to some backend that seems to be getting too far behind.  We signal at
  * most one backend at a time, for reasons explained at the top of the file.
+ * We also clear catchupInterruptPending, unless this backend has been
+ * signaled again since we last caught up.
  *
  * Caution: because we transiently release write lock when we have to signal
  * some other backend, it is NOT guaranteed that there are still minFree
@@ -655,6 +657,20 @@ SICleanupQueue(bool callerHasWriteLock, int minFree)
 	else
 		segP->nextThreshold = (numMsgs / CLEANUP_QUANTUM + 1) * CLEANUP_QUANTUM;
 
+	/*
+	 * Clear our local catchupInterruptPending flag, now that we have
+	 * processed the catchup request.
+	 *
+	 * "signaled" is expected to be false if we were called in response to a
+	 * catchup signal, because the caller consumes all the messages for this
+	 * backend before performing cleanup.  But it's possible that more
+	 * messages arrived after that, causing us to be signaled again.  Don't
+	 * clear catchupInterruptPending in that case, or we'd miss the new
+	 * catchup request.
+	 */
+	if (!segP->procState[MyProcNumber].signaled)
+		catchupInterruptPending = false;
+
 	/*
 	 * Lastly, signal anyone who needs a catchup interrupt.  Since
 	 * SendProcSignal() might not be fast, we don't want to hold locks while
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`NOT`
- 大写宏：`PROCSIG_CATCHUP_INTERRUPT`
- 外部类型：`Caution`
- 外部类型：`Lastly`
- 外部类型：`Since`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-fb99430c05` → 本草稿移入 `cases/defect/auto-postgres-fb99430c05/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
