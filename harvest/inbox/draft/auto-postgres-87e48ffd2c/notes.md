# auto-postgres-87e48ffd2c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#4848fd6752ad6b88c5a066442df35ba208dfcec5](https://github.com/postgres/postgres/commit/4848fd6752ad6b88c5a066442df35ba208dfcec5) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4848fd6752ad6b88c5a066442df35ba208dfcec5 (https://github.com/postgres/postgres/commit/4848fd6752ad6b88c5a066442df35ba208dfcec5)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4848fd6752ad6b88c5a066442df35ba208dfcec5 Remove the redundant interface list and fix function headings in predicate.c :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -149,46 +149,6 @@
  *
  *-------------------------------------------------------------------------
  */
-/*
- * INTERFACE ROUTINES
- *
- * predicate lock reporting
- *		GetPredicateLockStatusData(void)
- *
- * predicate lock maintenance
- *		GetSerializableTransactionSnapshot(Snapshot snapshot)
- *		SetSerializableTransactionSnapshot(Snapshot snapshot,
- *										   VirtualTransactionId *sourcevxid)
- *		RegisterPredicateLockingXid(void)
- *		PredicateLockRelation(Relation relation, Snapshot snapshot)
- *		PredicateLockPage(Relation relation, BlockNumber blkno,
- *						Snapshot snapshot)
- *		PredicateLockTID(Relation relation, const ItemPointerData *tid, Snapshot snapshot,
- *						 TransactionId tuple_xid)
- *		PredicateLockPageSplit(Relation relation, BlockNumber oldblkno,
- *							   BlockNumber newblkno)
- *		PredicateLockPageCombine(Relation relation, BlockNumber oldblkno,
- *								 BlockNumber newblkno)
- *		TransferPredicateLocksToHeapRelation(Relation relation)
- *		ReleasePredicateLocks(bool isCommit, bool isReadOnlySafe)
- *
- * conflict detection (may also trigger rollback)
- *		CheckForSerializableConflictOut(Relation relation, TransactionId xid,
- *										Snapshot snapshot)
- *		CheckForSerializableConflictIn(Relation relation, const ItemPointerData *tid,
- *									   BlockNumber blkno)
- *		CheckTableForSerializableConflictIn(Relation relation)
- *
- * final rollback checking
- *		PreCommit_CheckForSerializationFailure(void)
- *
- * two-phase commit support
- *		AtPrepare_PredicateLocks(void);
- *		PostPrepare_PredicateLocks(TransactionId xid);
- *		PredicateLockTwoPhaseFinish(FullTransactionId fxid, bool isCommit);
- *		predicatelock_twophase_recover(FullTransactionId fxid, uint16 info,
- *									   void *recdata, uint32 len);
- */
 
 #include "postgres.h"
 
@@ -4671,7 +4631,7 @@ PreCommit_CheckForSerializationFailure(void)
  */
 
 /*
- * AtPrepare_Locks
+ * AtPrepare_PredicateLocks
  *		Do the preparatory work for a PREPARE: make 2PC state file
  *		records for all predicate locks currently held.
  */
@@ -4738,7 +4698,7 @@ AtPrepare_PredicateLocks(void)
 }
 
 /*
- * PostPrepare_Locks
+ * PostPrepare_PredicateLocks
  *		Clean up after successful PREPARE. Unlike the non-predicate
  *		lock manager, we do not need to transfer locks to a dummy
  *		PGPROC because our SERIALIZABLEXACT will stay around
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`detection`
- 外部函数：`predicatelock_twophase_recover`
- 大写宏：`INTERFACE`
- 大写宏：`PGPROC`
- 大写宏：`PREPARE`
- 大写宏：`ROUTINES`
- 大写宏：`SERIALIZABLEXACT`
- 外部类型：`AtPrepare_Locks`
- 外部类型：`BlockNumber`
- 外部类型：`Clean`
- 外部类型：`Do`
- 外部类型：`FullTransactionId`
- 外部类型：`ItemPointerData`
- 外部类型：`PostPrepare_Locks`
- 外部类型：`Relation`
- 外部类型：`Snapshot`
- 外部类型：`TransactionId`
- 外部类型：`Unlike`
- 外部类型：`VirtualTransactionId`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-87e48ffd2c` → 本草稿移入 `cases/defect/auto-postgres-87e48ffd2c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
