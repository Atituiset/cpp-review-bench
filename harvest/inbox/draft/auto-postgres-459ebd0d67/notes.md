# auto-postgres-459ebd0d67

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#6873a138f782cf816e6019bbe922857050df96c9](https://github.com/postgres/postgres/commit/6873a138f782cf816e6019bbe922857050df96c9) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 75 |
| 编译错误数（gcc syntax-only） | 38（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6873a138f782cf816e6019bbe922857050df96c9 (https://github.com/postgres/postgres/commit/6873a138f782cf816e6019bbe922857050df96c9)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 5580；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6873a138f782cf816e6019bbe922857050df96c9 Fix crash with wal_debug in EXEC_BACKEND child processes :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5570,22 +5570,6 @@ XLOGShmemInit(void *arg)
 	char	   *allocptr;
 	int			i;
 
-#ifdef WAL_DEBUG
-
-	/*
-	 * Create a memory context for WAL debugging that's exempt from the normal
-	 * "no pallocs in critical section" rule. Yes, that can lead to a PANIC if
-	 * an allocation fails, but wal_debug is not for production use anyway.
-	 */
-	if (walDebugCxt == NULL)
-	{
-		walDebugCxt = AllocSetContextCreate(TopMemoryContext,
-											"WAL Debug",
-											ALLOCSET_DEFAULT_SIZES);
-		MemoryContextAllowInCriticalSection(walDebugCxt, true);
-	}
-#endif
-
 	memset(XLogCtl, 0, sizeof(XLogCtlData));
 
 	/*
@@ -5669,6 +5653,30 @@ XLOGShmemAttach(void *arg)
 	WALInsertLocks = XLogCtl->Insert.WALInsertLocks;
 }
 
+/*
+ * Initialize process-local state needed by wal_debug.
+ *
+ * This must be called before XLogInsertRecord().
+ */
+#ifdef WAL_DEBUG
+void
+InitWalDebug(void)
+{
+	/*
+	 * Create a memory context for WAL debugging that's exempt from the normal
+	 * "no pallocs in critical section" rule. Yes, that can lead to a PANIC if
+	 * an allocation fails, but wal_debug is not for production use anyway.
+	 */
+	if (walDebugCxt == NULL)
+	{
+		walDebugCxt = AllocSetContextCreate(TopMemoryContext,
+											"WAL Debug",
+											ALLOCSET_DEFAULT_SIZES);
+		MemoryContextAllowInCriticalSection(walDebugCxt, true);
+	}
+}
+#endif
+
 /*
  * This func must be called ONCE on system install.  It creates pg_control
  * and the initial XLOG segment.
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`buffer`
- 外部函数：`cache`
- 外部函数：`inserted`
- 外部函数：`memset`
- 外部函数：`mode`
- 大写宏：`ALL`
- 大写宏：`ALLOCSET_DEFAULT_SIZES`
- 大写宏：`LSN`
- 大写宏：`NULL`
- 大写宏：`ONCE`
- 大写宏：`PANIC`
- 大写宏：`PG_CACHE_LINE_SIZE`
- 大写宏：`SIGHUP`
- 大写宏：`WAL`
- 大写宏：`WAL_DEBUG`
- 大写宏：`XLOG`
- 大写宏：`XLOG2_CHECKSUMS`
- 大写宏：`XLOG_BLCKSZ`
- 大写宏：`XLOG_FPW_CHANGE`
- 外部类型：`CheckPoint`
- 外部类型：`Checkpointer`
- 外部类型：`ControlFileLock`
- 外部类型：`Copies`
- 外部类型：`Create`
- 外部类型：`Curr`
- 外部类型：`CurrBytePos`
- 外部类型：`Debug`
- 外部类型：`During`
- 外部类型：`Fake`
- 外部类型：`Flush`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`InitializedUpTo`
- 外部类型：`InsertTimeLineID`
- 外部类型：`InstallXLogFileSegmentActive`
- 外部类型：`InvalidXLogRecPtr`
- 外部类型：`It`
- 外部类型：`LWLock`
- 外部类型：`Latest`
- 外部类型：`LogwrtRqst`
- 外部类型：`Make`
- 外部类型：`Most`
- 外部类型：`Must`
- 外部类型：`Only`
- 外部类型：`Otherwise`
- 外部类型：`PrevBytePos`
- 外部类型：`PrevTimeLineID`
- 外部类型：`Protected`
- 外部类型：`RecoveryState`
- 外部类型：`RedoRecPtr`
- 外部类型：`SharedRecoveryState`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`They`
- 外部类型：`This`
- 外部类型：`Time`
- 外部类型：`TimeLineID`
- 外部类型：`To`
- 外部类型：`TopMemoryContext`
- 外部类型：`Used`
- 外部类型：`WALBufMappingLock`
- 外部类型：`WALInsertLock`
- 外部类型：`WALInsertLockPadded`
- 外部类型：`WALInsertLocks`
- 外部类型：`WALWriteLock`
- 外部类型：`WalWriterSleeping`
- 外部类型：`We`
- 外部类型：`XLogCacheBlck`
- 外部类型：`XLogCtl`
- 外部类型：`XLogCtlData`
- 外部类型：`XLogCtlInsert`
- 外部类型：`XLogRecPtr`
- 外部类型：`XLogSegNo`
- 外部类型：`XLogwrtRqst`
- 外部类型：`Yes`
- 外部类型：`pg_time_t`
- 外部类型：`slock_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-459ebd0d67` → 本草稿移入 `cases/defect/auto-postgres-459ebd0d67/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
