# auto-postgres-24bafccf74

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#fde4444b48c8dc069db2e29d096dbea45a158cc9](https://github.com/postgres/postgres/commit/fde4444b48c8dc069db2e29d096dbea45a158cc9) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 21 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fde4444b48c8dc069db2e29d096dbea45a158cc9 (https://github.com/postgres/postgres/commit/fde4444b48c8dc069db2e29d096dbea45a158cc9)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 1936；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fde4444b48c8dc069db2e29d096dbea45a158cc9 Remove unused PageIsPredicateLocked() :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -154,7 +154,6 @@
  *
  * predicate lock reporting
  *		GetPredicateLockStatusData(void)
- *		PageIsPredicateLocked(Relation relation, BlockNumber blkno)
  *
  * predicate lock maintenance
  *		GetSerializableTransactionSnapshot(Snapshot snapshot)
@@ -1920,45 +1919,6 @@ RegisterPredicateLockingXid(TransactionId xid)
 }
 
 
-/*
- * Check whether there are any predicate locks held by any transaction
- * for the page at the given block number.
- *
- * Note that the transaction may be completed but not yet subject to
- * cleanup due to overlapping serializable transactions.  This must
- * return valid information regardless of transaction isolation level.
- *
- * Also note that this doesn't check for a conflicting relation lock,
- * just a lock specifically on the given page.
- *
- * One use is to support proper behavior during GiST index vacuum.
- */
-bool
-PageIsPredicateLocked(Relation relation, BlockNumber blkno)
-{
-	PREDICATELOCKTARGETTAG targettag;
-	uint32		targettaghash;
-	LWLock	   *partitionLock;
-	PREDICATELOCKTARGET *target;
-
-	SET_PREDICATELOCKTARGETTAG_PAGE(targettag,
-									relation->rd_locator.dbOid,
-									relation->rd_id,
-									blkno);
-
-	targettaghash = PredicateLockTargetTagHashCode(&targettag);
-	partitionLock = PredicateLockHashPartitionLock(targettaghash);
-	LWLockAcquire(partitionLock, LW_SHARED);
-	target = (PREDICATELOCKTARGET *)
-		hash_search_with_hash_value(PredicateLockTargetHash,
-									&targettag, targettaghash,
-									HASH_FIND, NULL);
-	LWLockRelease(partitionLock);
-
-	return (target != NULL);
-}
-
-
 /*
  * Check whether a particular lock is held by this transaction.
  *
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`get_hash_value`
- 外部函数：`hash_search_with_hash_value`
- 大写宏：`HASH_FIND`
- 大写宏：`LW_SHARED`
- 大写宏：`NULL`
- 大写宏：`NUM_PREDICATELOCK_PARTITIONS`
- 大写宏：`PREDICATELOCKTARGET`
- 大写宏：`PREDICATELOCKTARGETTAG`
- 大写宏：`PREDICATELOCK_MANAGER_LWLOCK_OFFSET`
- 大写宏：`SET_PREDICATELOCKTARGETTAG_PAGE`
- 外部类型：`Also`
- 外部类型：`BlockNumber`
- 外部类型：`Check`
- 外部类型：`GiST`
- 外部类型：`LWLock`
- 外部类型：`MainLWLockArray`
- 外部类型：`Note`
- 外部类型：`One`
- 外部类型：`PredicateLockTargetHash`
- 外部类型：`Relation`
- 外部类型：`Snapshot`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-24bafccf74` → 本草稿移入 `cases/defect/auto-postgres-24bafccf74/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
