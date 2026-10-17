# auto-postgres-1e933501d7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#df1be4ddc8345235ffc736adc7dc954f0f944daa](https://github.com/postgres/postgres/commit/df1be4ddc8345235ffc736adc7dc954f0f944daa) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #df1be4ddc8345235ffc736adc7dc954f0f944daa (https://github.com/postgres/postgres/commit/df1be4ddc8345235ffc736adc7dc954f0f944daa)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 756；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT df1be4ddc8345235ffc736adc7dc954f0f944daa Avoid unnecessary end-of-recovery barrier waits :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -664,9 +664,9 @@ DisableLogicalDecoding(void)
 }
 
 /*
- * Updates the logical decoding status at end of recovery, and ensures that
- * all running processes have the updated XLogLogicalInfo status. This
- * function must be called before accepting writes.
+ * Update the logical decoding status at end of recovery, synchronizing
+ * process-local WAL logging state when necessary. This function must
+ * be called before accepting writes.
  */
 void
 UpdateLogicalDecodingStatusEndOfRecovery(void)
@@ -746,16 +746,25 @@ UpdateLogicalDecodingStatusEndOfRecovery(void)
 		LWLockRelease(LogicalDecodingControlLock);
 
 	/*
-	 * Ensure all running processes have the updated status. We don't need to
-	 * wait for running transactions to finish as we don't accept any writes
-	 * yet. On the other hand, we need to wait for synchronizing
-	 * XLogLogicalInfo even if we've not updated the status above as the
-	 * status have been turned on and off during recovery, having running
-	 * processes have different status on their local caches.
+	 * With wal_level = logical, XLogLogicalInfoActive() is always true, so no
+	 * barrier is needed.
+	 *
+	 * Otherwise, notify all processes to refresh XLogLogicalInfo. Wait only
+	 * if new_status is true, even if the shared status is unchanged, since
+	 * recovery may have left stale caches. Writes are not yet allowed, so we
+	 * need not wait for running transactions to finish.
+	 *
+	 * If new_status is false, stale caches only cause extra logical
+	 * information in WAL, which is safe (see DisableLogicalDecoding()).
 	 */
-	if (IsUnderPostmaster)
-		WaitForProcSignalBarrier(
-								 EmitProcSignalBarrier(PROCSIGNAL_BARRIER_UPDATE_XLOG_LOGICAL_INFO));
+	if (IsUnderPostmaster && wal_level != WAL_LEVEL_LOGICAL)
+	{
+		uint64		generation;
+
+		generation = EmitProcSignalBarrier(PROCSIGNAL_BARRIER_UPDATE_XLOG_LOGICAL_INFO);
+		if (new_status)
+			WaitForProcSignalBarrier(generation);
+	}
 
 	INJECTION_POINT("startup-logical-decoding-status-change-end-of-recovery", NULL);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`exist`
- 大写宏：`DEBUG1`
- 大写宏：`INJECTION_POINT`
- 大写宏：`LW_EXCLUSIVE`
- 大写宏：`NULL`
- 大写宏：`PROCSIGNAL_BARRIER_UPDATE_XLOG_LOGICAL_INFO`
- 大写宏：`RM_XLOG_ID`
- 大写宏：`WAL`
- 大写宏：`WAL_LEVEL_LOGICAL`
- 大写宏：`XLOG_LOGICAL_DECODING_STATUS_CHANGE`
- 外部类型：`Ensure`
- 外部类型：`For`
- 外部类型：`Here`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`IsUnderPostmaster`
- 外部类型：`It`
- 外部类型：`LogicalDecodingControlLock`
- 外部类型：`LogicalDecodingCtl`
- 外部类型：`Now`
- 外部类型：`On`
- 外部类型：`Such`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Unlike`
- 外部类型：`Update`
- 外部类型：`Updates`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`With`
- 外部类型：`XLogLogicalInfo`
- 外部类型：`XLogRecPtr`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-1e933501d7` → 本草稿移入 `cases/defect/auto-postgres-1e933501d7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
