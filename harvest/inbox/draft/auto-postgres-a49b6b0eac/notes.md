# auto-postgres-a49b6b0eac

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#6a0a66b3e7ac394c499708b790effea9a564ef0d](https://github.com/postgres/postgres/commit/6a0a66b3e7ac394c499708b790effea9a564ef0d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 15 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6a0a66b3e7ac394c499708b790effea9a564ef0d (https://github.com/postgres/postgres/commit/6a0a66b3e7ac394c499708b790effea9a564ef0d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 540；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6a0a66b3e7ac394c499708b790effea9a564ef0d Record logical replication conflicts in the conflict log table. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -537,7 +537,7 @@ BuildConflictIndexInfo(ResultRelInfo *resultRelInfo, Oid conflictindex)
 static void
 update_most_recent_deletion_info(TupleTableSlot *scanslot,
 								 TransactionId oldestxmin,
-								 TransactionId *delete_xid,
+								 FullTransactionId *delete_xid,
 								 TimestampTz *delete_time,
 								 ReplOriginId *delete_origin)
 {
@@ -579,7 +579,18 @@ update_most_recent_deletion_info(TupleTableSlot *scanslot,
 	if (TransactionIdGetCommitTsData(xmax, &localts, &localorigin) &&
 		TimestampDifferenceExceeds(*delete_time, localts, 0))
 	{
-		*delete_xid = xmax;
+		/*
+		 * Convert xmax to a full transaction ID while the buffer is pinned.
+		 * This is safe because VACUUM cannot advance relfrozenxid past an
+		 * xmax on a pinned page. The result is absolute, so it stays valid
+		 * after the pin is released.
+		 *
+		 * XXX We can't rely on the conflict detection slot's xmin instead, as
+		 * a parallel apply worker uses the leader's oldest_nonremovable_xid,
+		 * which the leader can advance or clear at any time.
+		 */
+		*delete_xid = FullTransactionIdFromAllowableAt(ReadNextFullTransactionId(),
+													   xmax);
 		*delete_time = localts;
 		*delete_origin = localorigin;
 	}
@@ -625,7 +636,7 @@ bool
 RelationFindDeletedTupleInfoSeq(Relation rel, Oid identidxoid,
 								TupleTableSlot *searchslot,
 								TransactionId oldestxmin,
-								TransactionId *delete_xid,
+								FullTransactionId *delete_xid,
 								ReplOriginId *delete_origin,
 								TimestampTz *delete_time)
 {
@@ -637,7 +648,7 @@ RelationFindDeletedTupleInfoSeq(Relation rel, Oid identidxoid,
 
 	Assert(equalTupleDescs(desc, searchslot->tts_tupleDescriptor));
 
-	*delete_xid = InvalidTransactionId;
+	*delete_xid = InvalidFullTransactionId;
 	*delete_origin = InvalidReplOriginId;
 	*delete_time = 0;
 
@@ -713,7 +724,7 @@ RelationFindDeletedTupleInfoByIndex(Relation rel, Oid idxoid,
 									bool skipduplicates,
 									TupleTableSlot *searchslot,
 									TransactionId oldestxmin,
-									TransactionId *delete_xid,
+									FullTransactionId *delete_xid,
 									ReplOriginId *delete_origin,
 									TimestampTz *delete_time)
 {
@@ -728,7 +739,7 @@ RelationFindDeletedTupleInfoByIndex(Relation rel, Oid idxoid,
 	Assert(equalTupleDescs(desc, searchslot->tts_tupleDescriptor));
 	Assert(OidIsValid(idxoid));
 
-	*delete_xid = InvalidTransactionId;
+	*delete_xid = InvalidFullTransactionId;
 	*delete_time = 0;
 	*delete_origin = InvalidReplOriginId;
 
@@ -862,13 +873,22 @@ CheckAndReportConflict(ResultRelInfo *resultRelInfo, EState *estate,
 							  &conflictslot))
 		{
 			ConflictTupleInfo *conflicttuple = palloc0_object(ConflictTupleInfo);
+			TransactionId xmin;
 
 			conflicttuple->slot = conflictslot;
 			conflicttuple->indexoid = uniqueidx;
 
-			GetTupleTransactionInfo(conflictslot, &conflicttuple->xmin,
+			GetTupleTransactionInfo(conflictslot, &xmin,
 									&conflicttuple->origin, &conflicttuple->ts);
 
+			/*
+			 * Convert xmin to a full transaction ID. This is safe because the
+			 * tuple's buffer is pinned, preventing VACUUM from freezing it.
+			 */
+			conflicttuple->xmin =
+				FullTransactionIdFromAllowableAt(ReadNextFullTransactionId(),
+												 xmin);
+
 			conflicttuples = lappend(conflicttuples, conflicttuple);
 		}
 	}
@@ -877,7 +897,7 @@ CheckAndReportConflict(ResultRelInfo *resultRelInfo, EState *estate,
 	if (conflicttuples)
 		ReportApplyConflict(estate, resultRelInfo, ERROR,
 							list_length(conflicttuples) > 1 ? CT_MULTIPLE_UNIQUE_CONFLICTS : type,
-							searchslot, remoteslot, conflicttuples);
+							NULL, searchslot, remoteslot, conflicttuples);
 }
 
 /*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`equalTupleDescs`
- 外部函数：`lappend`
- 外部函数：`list_length`
- 外部函数：`palloc0_object`
- 大写宏：`CT_MULTIPLE_UNIQUE_CONFLICTS`
- 大写宏：`ERROR`
- 外部类型：`ConflictTupleInfo`
- 外部类型：`InvalidReplOriginId`
- 外部类型：`InvalidTransactionId`
- 外部类型：`Oid`
- 外部类型：`Relation`
- 外部类型：`ReplOriginId`
- 外部类型：`TimestampTz`
- 外部类型：`TransactionId`
- 外部类型：`TupleTableSlot`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-a49b6b0eac` → 本草稿移入 `cases/defect/auto-postgres-a49b6b0eac/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
