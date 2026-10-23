# auto-postgres-703fb6692d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#219334a2252729054db98288df64e61b2968f2b7](https://github.com/postgres/postgres/commit/219334a2252729054db98288df64e61b2968f2b7) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 72 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #219334a2252729054db98288df64e61b2968f2b7 (https://github.com/postgres/postgres/commit/219334a2252729054db98288df64e61b2968f2b7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 624；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 219334a2252729054db98288df64e61b2968f2b7 Add error context for sequence synchronization. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -163,6 +163,22 @@ get_sequences_string(List *seqindexes, StringInfo buf)
 	}
 }
 
+/*
+ * Error callback to give more context info about the sequence being
+ * synchronized.
+ */
+static void
+sequence_sync_error_callback(void *arg)
+{
+	LogicalRepSequenceInfo *seqinfo = *(LogicalRepSequenceInfo **) arg;
+
+	if (seqinfo == NULL)
+		return;
+
+	errcontext("synchronizing sequence \"%s.%s\" for subscription \"%s\"",
+			   seqinfo->nspname, seqinfo->seqname, MySubscription->name);
+}
+
 /*
  * report_sequence_errors
  *
@@ -621,9 +637,10 @@ copy_sequences(WalReceiverConn *conn)
 		while (tuplestore_gettupleslot(res->tuplestore, true, false, slot))
 		{
 			CopySeqResult sync_status;
-			LogicalRepSequenceInfo *seqinfo;
+			LogicalRepSequenceInfo *seqinfo = NULL;
 			Relation	sequence_rel = NULL;
 			int			seqidx;
+			ErrorContextCallback errcallback;
 
 			CHECK_FOR_INTERRUPTS();
 
@@ -633,12 +650,25 @@ copy_sequences(WalReceiverConn *conn)
 				ProcessConfigFile(PGC_SIGHUP);
 			}
 
+			/*
+			 * Set up callback to identify the sequence in error messages.
+			 * seqinfo will be filled once get_and_validate_seq_info()
+			 * resolves the sequence index.
+			 */
+			errcallback.callback = sequence_sync_error_callback;
+			errcallback.arg = &seqinfo;
+			errcallback.previous = error_context_stack;
+			error_context_stack = &errcallback;
+
 			sync_status = get_and_validate_seq_info(slot, &sequence_rel,
 													&seqinfo, &seqidx);
 			if (sync_status == COPYSEQ_SUCCESS)
 				sync_status = copy_sequence(seqinfo,
 											sequence_rel->rd_rel->relowner);
 
+			/* Pop the error context stack */
+			error_context_stack = errcallback.previous;
+
 			switch (sync_status)
 			{
 				case COPYSEQ_SUCCESS:
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`counter`
- 外部函数：`elog`
- 外部函数：`get_namespace_name`
- 外部函数：`has_sequence_privilege`
- 外部函数：`has_sequence_privilege_id`
- 外部函数：`list_length`
- 外部函数：`list_nth`
- 外部函数：`nextval`
- 外部函数：`pg_class_aclcheck`
- 外部函数：`pg_get_sequence_data`
- 外部函数：`skipped`
- 外部函数：`slot_getattr`
- 外部函数：`snapshot`
- 外部函数：`state`
- 外部函数：`strcmp`
- 外部函数：`table_close`
- 外部函数：`table_open`
- 外部函数：`try_table_open`
- 外部函数：`tuplestore_gettupleslot`
- 大写宏：`ACLCHECK_OK`
- 大写宏：`ACL_UPDATE`
- 大写宏：`ALTER`
- 大写宏：`CHECK_FOR_INTERRUPTS`
- 大写宏：`COPYSEQ_MISMATCH`
- 大写宏：`COPYSEQ_NOT_SUBSCRIBED`
- 大写宏：`COPYSEQ_PUBLISHER_INSUFFICIENT_PERM`
- 大写宏：`COPYSEQ_SKIPPED`
- 大写宏：`COPYSEQ_SUBSCRIBER_INSUFFICIENT_PERM`
- 大写宏：`COPYSEQ_SUCCESS`
- 大写宏：`ERROR`
- 大写宏：`GETSTRUCT`
- 大写宏：`LSN`
- 大写宏：`NULL`
- 大写宏：`PGC_SIGHUP`
- 大写宏：`PUBLICATION`
- 大写宏：`READY`
- 大写宏：`REFRESH`
- 大写宏：`SELECT`
- 大写宏：`SEQRELID`
- 大写宏：`SUBREL_STATE_READY`
- 大写宏：`SUBSCRIPTION`
- 大写宏：`SUBSCRIPTIONRELMAP`
- 大写宏：`VALUES`
- 外部类型：`AccessExclusiveLock`
- 外部类型：`AccessShareLock`
- 外部类型：`AclResult`
- 外部类型：`An`
- 外部类型：`Both`
- 外部类型：`Check`
- 外部类型：`Datum`
- 外部类型：`Form_pg_sequence`
- 外部类型：`HeapTuple`
- 外部类型：`Identify`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`LogicalRepSequenceInfo`
- 外部类型：`MySubscription`
- 外部类型：`NoLock`
- 外部类型：`Oid`
- 外部类型：`Record`
- 外部类型：`Relation`
- 外部类型：`RowExclusiveLock`
- 外部类型：`Sanity`
- 外部类型：`Sequence`
- 外部类型：`Since`
- 外部类型：`SubscriptionRelRelationId`
- 外部类型：`SubscriptionRelationId`
- 外部类型：`Take`
- 外部类型：`That`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Treat`
- 外部类型：`TupleTableSlot`
- 外部类型：`UserContext`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-703fb6692d` → 本草稿移入 `cases/defect/auto-postgres-703fb6692d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
