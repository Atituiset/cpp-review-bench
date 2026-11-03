# auto-postgres-9c692866f2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#51ae21aa313bb6b79ad914920279cd0d89e92220](https://github.com/postgres/postgres/commit/51ae21aa313bb6b79ad914920279cd0d89e92220) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 34 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #51ae21aa313bb6b79ad914920279cd0d89e92220 (https://github.com/postgres/postgres/commit/51ae21aa313bb6b79ad914920279cd0d89e92220)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 36；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 51ae21aa313bb6b79ad914920279cd0d89e92220 Add REPACK progress phases for logical decoding setup. :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -19,6 +19,7 @@
 #include "access/xlog_internal.h"
 #include "access/xlogutils.h"
 #include "access/xlogwait.h"
+#include "commands/progress.h"
 #include "commands/repack.h"
 #include "commands/repack_internal.h"
 #include "libpq/libpq.h"
@@ -33,7 +34,10 @@
 #define PGREPACK_PLUGIN   "pgrepack"
 
 static void RepackWorkerShutdown(int code, Datum arg);
-static LogicalDecodingContext *repack_setup_logical_decoding(Oid relid);
+static void set_repack_worker_setup_phase(DecodingWorkerShared *shared,
+										  int setup_phase);
+static LogicalDecodingContext *repack_setup_logical_decoding(Oid relid,
+															 DecodingWorkerShared *shared);
 static void repack_cleanup_logical_decoding(LogicalDecodingContext *ctx);
 static void export_initial_snapshot(Snapshot snapshot,
 									DecodingWorkerShared *shared);
@@ -139,7 +143,7 @@ RepackWorkerMain(Datum main_arg)
 	/*
 	 * Prepare to capture the concurrent data changes ourselves.
 	 */
-	decoding_ctx = repack_setup_logical_decoding(shared->relid);
+	decoding_ctx = repack_setup_logical_decoding(shared->relid, shared);
 
 	/* Announce that we're ready. */
 	SpinLockAcquire(&shared->mutex);
@@ -212,6 +216,19 @@ AmRepackWorker(void)
 	return am_repack_worker;
 }
 
+/*
+ * Tell the backend running REPACK which progress phase to report while we
+ * initialize the decoding.
+ */
+static void
+set_repack_worker_setup_phase(DecodingWorkerShared *shared, int setup_phase)
+{
+	SpinLockAcquire(&shared->mutex);
+	shared->setup_phase = setup_phase;
+	SpinLockRelease(&shared->mutex);
+	ConditionVariableSignal(&shared->cv);
+}
+
 /*
  * This function is much like pg_create_logical_replication_slot() except that
  * the new slot is neither released (if anyone else could read changes from
@@ -220,7 +237,7 @@ AmRepackWorker(void)
  * crash by restarting all the work from scratch).
  */
 static LogicalDecodingContext *
-repack_setup_logical_decoding(Oid relid)
+repack_setup_logical_decoding(Oid relid, DecodingWorkerShared *shared)
 {
 	Relation	rel;
 	Oid			toastrelid;
@@ -249,6 +266,8 @@ repack_setup_logical_decoding(Oid relid)
 	snprintf(slotname, NAMEDATALEN, "pg_repack_%d", MyProcPid);
 	ReplicationSlotCreate(slotname, true, RS_TEMPORARY, false, true,
 						  false, false);
+	set_repack_worker_setup_phase(shared,
+								  PROGRESS_REPACK_PHASE_ENABLE_LOGICAL_DECODING);
 	EnsureLogicalDecodingEnabled();
 
 	/*
@@ -272,6 +291,9 @@ repack_setup_logical_decoding(Oid relid)
 	}
 	table_close(rel, AccessShareLock);
 
+	set_repack_worker_setup_phase(shared,
+								  PROGRESS_REPACK_PHASE_INIT_LOGICAL_DECODING);
+
 	/*
 	 * Set up our logical decoding context.  We initially use the blocking
 	 * read_local_xlog_page until we find the start point, and switch to the
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dsm_detach`
- 外部函数：`dsm_segment_address`
- 外部函数：`palloc`
- 外部函数：`pfree`
- 外部函数：`released`
- 外部函数：`repack_setup_logical_decoding`
- 外部函数：`snprintf`
- 外部函数：`table_close`
- 大写宏：`MAXPGPATH`
- 大写宏：`NAMEDATALEN`
- 大写宏：`PROCSIG_REPACK_MESSAGE`
- 大写宏：`RS_TEMPORARY`
- 外部类型：`AccessShareLock`
- 外部类型：`Announce`
- 外部类型：`BufFile`
- 外部类型：`Datum`
- 外部类型：`DecodingWorkerShared`
- 外部类型：`Detach`
- 外部类型：`Detaching`
- 外部类型：`If`
- 外部类型：`Increase`
- 外部类型：`LogicalDecodingContext`
- 外部类型：`MyProcPid`
- 外部类型：`Oid`
- 外部类型：`Prepare`
- 外部类型：`ProcNumber`
- 外部类型：`Relation`
- 外部类型：`RepackDecodingState`
- 外部类型：`Set`
- 外部类型：`Size`
- 外部类型：`Snapshot`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`We`
- 外部类型：`pid_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-9c692866f2` → 本草稿移入 `cases/defect/auto-postgres-9c692866f2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
