# auto-postgres-f98dd957f9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#9931482364a938af876307e84ae88ff4839838b0](https://github.com/postgres/postgres/commit/9931482364a938af876307e84ae88ff4839838b0) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9931482364a938af876307e84ae88ff4839838b0 (https://github.com/postgres/postgres/commit/9931482364a938af876307e84ae88ff4839838b0)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 206；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9931482364a938af876307e84ae88ff4839838b0 Avoid DELAY_CHKPT_START in smgr_bulk_finish() :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -40,7 +40,6 @@
 #include "access/xlogrecord.h"
 #include "storage/bufpage.h"
 #include "storage/bulk_write.h"
-#include "storage/proc.h"
 #include "storage/smgr.h"
 #include "utils/rel.h"
 
@@ -180,6 +179,8 @@ smgr_bulk_finish(BulkWriteState *bulkstate)
 	}
 	else
 	{
+		bool		need_sync;
+
 		/*
 		 * Permanent relation, WAL-logged normally.
 		 *
@@ -194,30 +195,24 @@ smgr_bulk_finish(BulkWriteState *bulkstate)
 		 * replay the WAL starting from the checkpoint, therefore it wouldn't
 		 * replay our earlier WAL records.  So if a checkpoint started after
 		 * the bulk write, fsync the files now.
+		 *
+		 * The sync request is registered before re-checking the redo pointer:
+		 * if it has still not moved, any checkpoint that moves it later
+		 * absorbs the request before syncing.  The first check skips the
+		 * request when a checkpoint already happened during the writes.
 		 */
+		need_sync = (bulkstate->start_RedoRecPtr != GetRedoRecPtr());
+		if (!need_sync)
+		{
+			smgrregistersync(bulkstate->smgr, bulkstate->forknum);
+			need_sync = (bulkstate->start_RedoRecPtr != GetRedoRecPtr());
+		}
 
-		/*
-		 * Prevent a checkpoint from starting between the GetRedoRecPtr() and
-		 * smgrregistersync() calls.
-		 */
-		Assert((MyProc->delayChkptFlags & DELAY_CHKPT_START) == 0);
-		MyProc->delayChkptFlags |= DELAY_CHKPT_START;
-
-		if (bulkstate->start_RedoRecPtr != GetRedoRecPtr())
+		if (need_sync)
 		{
-			/*
-			 * A checkpoint occurred and it didn't know about our writes, so
-			 * fsync() the relation ourselves.
-			 */
-			MyProc->delayChkptFlags &= ~DELAY_CHKPT_START;
 			smgrimmedsync(bulkstate->smgr, bulkstate->forknum);
 			elog(DEBUG1, "flushed relation because a checkpoint occurred concurrently");
 		}
-		else
-		{
-			smgrregistersync(bulkstate->smgr, bulkstate->forknum);
-			MyProc->delayChkptFlags &= ~DELAY_CHKPT_START;
-		}
 	}
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`fsync`
- 外部函数：`smgrimmedsync`
- 外部函数：`smgrregistersync`
- 大写宏：`DEBUG1`
- 大写宏：`DELAY_CHKPT_START`
- 大写宏：`WAL`
- 外部类型：`MyProc`
- 外部类型：`Permanent`
- 外部类型：`Prevent`
- 外部类型：`So`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f98dd957f9` → 本草稿移入 `cases/defect/auto-postgres-f98dd957f9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
