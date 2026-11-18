# auto-postgres-22430668aa

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1b4abc23f3e3fa8f2274b74b7912c90bed31b242](https://github.com/postgres/postgres/commit/1b4abc23f3e3fa8f2274b74b7912c90bed31b242) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 41 |
| 编译错误数（gcc syntax-only） | 42（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1b4abc23f3e3fa8f2274b74b7912c90bed31b242 (https://github.com/postgres/postgres/commit/1b4abc23f3e3fa8f2274b74b7912c90bed31b242)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 37；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1b4abc23f3e3fa8f2274b74b7912c90bed31b242 xid_wraparound: Fix XID consumption accounting :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -26,6 +26,8 @@ static FullTransactionId consume_xids_common(FullTransactionId untilxid, uint64
 
 /*
  * Consume the specified number of XIDs.
+ *
+ * Returns the last XID assigned by this function.
  */
 PG_FUNCTION_INFO_V1(consume_xids);
 Datum
@@ -34,19 +36,21 @@ consume_xids(PG_FUNCTION_ARGS)
 	int64		nxids = PG_GETARG_INT64(0);
 	FullTransactionId lastxid;
 
-	if (nxids < 0)
+	if (nxids <= 0)
 		elog(ERROR, "invalid nxids argument: %" PRId64, nxids);
 
-	if (nxids == 0)
-		lastxid = ReadNextFullTransactionId();
-	else
-		lastxid = consume_xids_common(InvalidFullTransactionId, (uint64) nxids);
+	lastxid = consume_xids_common(InvalidFullTransactionId, (uint64) nxids);
 
 	PG_RETURN_FULLTRANSACTIONID(lastxid);
 }
 
 /*
- * Consume XIDs, up to the given XID.
+ * Advance XID allocation so that the next XID is at least the target.
+ * This may consume XIDs beyond the target.
+ *
+ * Ensures that the current transaction has a top-level XID.
+ * Returns the last XID assigned by this call, or the next XID observed
+ * if no XID was assigned.
  */
 PG_FUNCTION_INFO_V1(consume_xids_until);
 Datum
@@ -64,6 +68,23 @@ consume_xids_until(PG_FUNCTION_ARGS)
 	PG_RETURN_FULLTRANSACTIONID(lastxid);
 }
 
+/*
+ * These constants copied from .c files, because they're private.
+ */
+#define COMMIT_TS_XACTS_PER_PAGE (BLCKSZ / 10)
+#define SUBTRANS_XACTS_PER_PAGE (BLCKSZ / sizeof(TransactionId))
+#define CLOG_XACTS_PER_BYTE 4
+#define CLOG_XACTS_PER_PAGE (BLCKSZ * CLOG_XACTS_PER_BYTE)
+
+/*
+ * Take the shortcut in consume_xids_common() only if we have more than this
+ * many XIDs left to consume.  XidSkip() never skips more than
+ * COMMIT_TS_XACTS_PER_PAGE XIDs, which is 3276 with the largest supported
+ * block size (32kB), so this is enough to make sure that the shortcut never
+ * consumes more XIDs than requested.
+ */
+#define XID_SHORTCUT_THRESHOLD 4000
+
 /*
  * Common functionality between the two public functions.
  */
@@ -89,7 +110,11 @@ consume_xids_common(FullTransactionId untilxid, uint64 nxids)
 	 * the cache overflows, but beyond that, we don't keep track of the
 	 * consumed XIDs.
 	 */
-	(void) GetTopTransactionId();
+	if (!FullTransactionIdIsValid(GetTopFullTransactionIdIfAny()))
+	{
+		lastxid = GetTopFullTransactionId();
+		consumed++;
+	}
 
 	for (;;)
 	{
@@ -115,7 +140,7 @@ consume_xids_common(FullTransactionId untilxid, uint64 nxids)
 		 * If we still have plenty of XIDs to consume, try to take a shortcut
 		 * and bump up the nextXid counter directly.
 		 */
-		if (xids_left > 2000 &&
+		if (xids_left > XID_SHORTCUT_THRESHOLD &&
 			consumed - last_reported_at < REPORT_INTERVAL &&
 			MyProc->subxidStatus.overflowed)
 		{
@@ -153,14 +178,6 @@ consume_xids_common(FullTransactionId untilxid, uint64 nxids)
 	return lastxid;
 }
 
-/*
- * These constants copied from .c files, because they're private.
- */
-#define COMMIT_TS_XACTS_PER_PAGE (BLCKSZ / 10)
-#define SUBTRANS_XACTS_PER_PAGE (BLCKSZ / sizeof(TransactionId))
-#define CLOG_XACTS_PER_BYTE 4
-#define CLOG_XACTS_PER_PAGE (BLCKSZ * CLOG_XACTS_PER_BYTE)
-
 /*
  * All the interesting action in GetNewTransactionId happens when we extend
  * the SLRUs, or at the uint32 wraparound. If the nextXid counter is not close
@@ -193,6 +210,9 @@ XidSkip(FullTransactionId fullxid)
 		return 0;
 	distance = Min(distance, CLOG_XACTS_PER_PAGE - rem);
 
+	/* The shortcut threshold in consume_xids_common() relies on this */
+	Assert(distance <= XID_SHORTCUT_THRESHOLD);
+
 	return distance;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 大写宏：`BLCKSZ`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`ERROR`
- 大写宏：`LW_EXCLUSIVE`
- 大写宏：`NOTICE`
- 大写宏：`PGPROC`
- 大写宏：`PG_FUNCTION_ARGS`
- 大写宏：`PG_FUNCTION_INFO_V1`
- 大写宏：`PG_GETARG_FULLTRANSACTIONID`
- 大写宏：`PG_GETARG_INT64`
- 大写宏：`PG_RETURN_FULLTRANSACTIONID`
- 大写宏：`SLRU`
- 大写宏：`UINT32_MAX`
- 大写宏：`XID`
- 外部类型：`All`
- 外部类型：`Call`
- 外部类型：`Common`
- 外部类型：`Consume`
- 外部类型：`Datum`
- 外部类型：`For`
- 外部类型：`FullTransactionId`
- 外部类型：`GetNewTransactionId`
- 外部类型：`Go`
- 外部类型：`How`
- 外部类型：`If`
- 外部类型：`InvalidFullTransactionId`
- 外部类型：`MyProc`
- 外部类型：`PRId64`
- 外部类型：`PRIu64`
- 外部类型：`Print`
- 外部类型：`Report`
- 外部类型：`SLRUs`
- 外部类型：`Slow`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`TransactionId`
- 外部类型：`TransamVariables`
- 外部类型：`We`
- 外部类型：`XIDs`
- 外部类型：`XidGenLock`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-22430668aa` → 本草稿移入 `cases/defect/auto-postgres-22430668aa/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
