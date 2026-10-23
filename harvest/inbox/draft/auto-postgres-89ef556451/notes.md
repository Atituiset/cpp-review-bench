# auto-postgres-89ef556451

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#871dee0ccbe292cf4c2874aa7b460d242262d242](https://github.com/postgres/postgres/commit/871dee0ccbe292cf4c2874aa7b460d242262d242) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 44 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #871dee0ccbe292cf4c2874aa7b460d242262d242 (https://github.com/postgres/postgres/commit/871dee0ccbe292cf4c2874aa7b460d242262d242)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 871dee0ccbe292cf4c2874aa7b460d242262d242 Clear async.c's isAdvancing flag when a notify read is aborted. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2413,13 +2413,24 @@ SignalBackends(void)
  *	This is called at transaction abort.
  *
  *	Revert any staged listen/unlisten changes and clean up transaction state.
- *	This only does anything if we abort after PreCommit_Notify has staged
- *	some entries.
  */
 void
 AtAbort_Notify(void)
 {
-	/* Revert staged listen/unlisten changes */
+	/*
+	 * If we failed during asyncQueueReadAllNotifications(), clear our
+	 * isAdvancing flag, else SignalBackends() would never wake us again.
+	 * Since only our own process changes this flag, it is safe to check it
+	 * without the lock.
+	 */
+	if (amRegisteredListener && QUEUE_BACKEND_IS_ADVANCING(MyProcNumber))
+	{
+		LWLockAcquire(NotifyQueueLock, LW_SHARED);
+		QUEUE_BACKEND_IS_ADVANCING(MyProcNumber) = false;
+		LWLockRelease(NotifyQueueLock);
+	}
+
+	/* Revert any staged listen/unlisten changes */
 	ApplyPendingListenActions(false);
 
 	/* If we're no longer listening on anything, unregister */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dsa_free`
- 外部函数：`dsa_get_address`
- 外部函数：`dshash_delete_entry`
- 外部函数：`dshash_find`
- 外部函数：`dshash_release_lock`
- 外部函数：`elog`
- 外部函数：`exist`
- 外部函数：`hash_search`
- 外部函数：`hash_seq_init`
- 外部函数：`hash_seq_search`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`strlcpy`
- 大写宏：`DSA`
- 大写宏：`HASH_REMOVE`
- 大写宏：`HASH_SEQ_STATUS`
- 大写宏：`LISTEN`
- 大写宏：`NAMEDATALEN`
- 大写宏：`NULL`
- 大写宏：`PANIC`
- 大写宏：`PENDING_LISTEN`
- 大写宏：`PENDING_UNLISTEN`
- 大写宏：`UNLISTEN`
- 外部类型：`Allocated`
- 外部类型：`Entry`
- 外部类型：`Find`
- 外部类型：`For`
- 外部类型：`GlobalChannelEntry`
- 外部类型：`GlobalChannelKey`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`ListenerEntry`
- 外部类型：`MyDatabaseId`
- 外部类型：`MyProcNumber`
- 外部类型：`Note`
- 外部类型：`Number`
- 外部类型：`Oid`
- 外部类型：`PendingListenEntry`
- 外部类型：`PreCommit`
- 外部类型：`PreCommit_Notify`
- 外部类型：`ProcNumber`
- 外部类型：`Quick`
- 外部类型：`Revert`
- 外部类型：`Scan`
- 外部类型：`Staged`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-89ef556451` → 本草稿移入 `cases/defect/auto-postgres-89ef556451/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
