# auto-postgres-0a5ab8d93c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#d6393fc40a59830a88a79bdf08e962122fa9c631](https://github.com/postgres/postgres/commit/d6393fc40a59830a88a79bdf08e962122fa9c631) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 60 |
| 编译错误数（gcc syntax-only） | 32（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d6393fc40a59830a88a79bdf08e962122fa9c631 (https://github.com/postgres/postgres/commit/d6393fc40a59830a88a79bdf08e962122fa9c631)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 944；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d6393fc40a59830a88a79bdf08e962122fa9c631 postgres_fdw: Fix corner cases in transaction mode propagation. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -112,6 +112,20 @@ static uint32 pgfdw_we_get_result = 0;
  */
 #define RETRY_CANCEL_TIMEOUT	1000
 
+/*
+ * Macro for constructing commit command to be sent
+ *
+ * We synchronize the read/write mode before committing remote transactions
+ * so deferred triggers on remote servers can run in the right mode.
+ */
+#define CONSTRUCT_COMMIT_COMMAND(sql, entry) \
+	do { \
+		if ((read_only_level > 0) && !(entry)->xact_read_only) \
+			strcpy((sql), "SET TRANSACTION READ ONLY; COMMIT TRANSACTION"); \
+		else \
+			strcpy((sql), "COMMIT TRANSACTION"); \
+	} while(0)
+
 /* Macro for constructing abort command to be sent */
 #define CONSTRUCT_ABORT_COMMAND(sql, entry, toplevel) \
 	do { \
@@ -941,7 +955,7 @@ begin_remote_xact(ConnCacheEntry *entry)
 			appendStringInfoString(&sql, "REPEATABLE READ");
 		if (ro)
 			appendStringInfoString(&sql, " READ ONLY");
-		if (XactDeferrable)
+		if (XactDeferrable && PQserverVersion(entry->conn) >= 90100)
 			appendStringInfoString(&sql, " DEFERRABLE");
 		entry->changing_xact_state = true;
 		do_sql_command(entry->conn, sql.data);
@@ -971,7 +985,7 @@ begin_remote_xact(ConnCacheEntry *entry)
 		if (entry->xact_depth == read_only_level)
 		{
 			entry->changing_xact_state = true;
-			do_sql_command(entry->conn, "SET transaction_read_only = on");
+			do_sql_command(entry->conn, "SET TRANSACTION READ ONLY");
 			entry->xact_read_only = true;
 			entry->changing_xact_state = false;
 		}
@@ -1004,7 +1018,7 @@ begin_remote_xact(ConnCacheEntry *entry)
 		initStringInfo(&sql);
 		appendStringInfo(&sql, "SAVEPOINT s%d", entry->xact_depth + 1);
 		if (ro)
-			appendStringInfoString(&sql, "; SET transaction_read_only = on");
+			appendStringInfoString(&sql, "; SET TRANSACTION READ ONLY");
 		entry->changing_xact_state = true;
 		do_sql_command(entry->conn, sql.data);
 		entry->xact_depth++;
@@ -1184,6 +1198,24 @@ pgfdw_xact_callback(XactEvent event, void *arg)
 	if (!xact_got_connection)
 		return;
 
+	/*
+	 * If we are called for pre-commit cleanup, ensure read_only_level is set
+	 * for later processing.  Note that we need to do this because the local
+	 * transaction may have become read-only since the last remote operation.
+	 */
+	if (event == XACT_EVENT_PARALLEL_PRE_COMMIT ||
+		event == XACT_EVENT_PRE_COMMIT)
+	{
+		if (XactReadOnly)
+		{
+			if (read_only_level == 0)
+				read_only_level = 1;
+			Assert(read_only_level == 1);
+		}
+		else
+			Assert(read_only_level == 0);
+	}
+
 	/*
 	 * Scan all connection cache entries to find open remote transactions, and
 	 * close them.
@@ -1200,6 +1232,8 @@ pgfdw_xact_callback(XactEvent event, void *arg)
 		/* If it has an open remote transaction, try to close it */
 		if (entry->xact_depth > 0)
 		{
+			char		sql[100];
+
 			elog(DEBUG3, "closing remote transaction on connection %p",
 				 entry->conn);
 
@@ -1215,14 +1249,17 @@ pgfdw_xact_callback(XactEvent event, void *arg)
 					pgfdw_reject_incomplete_xact_state_change(entry);
 
 					/* Commit all remote transactions during pre-commit */
+					CONSTRUCT_COMMIT_COMMAND(sql, entry);
 					entry->changing_xact_state = true;
 					if (entry->parallel_commit)
 					{
-						do_sql_command_begin(entry->conn, "COMMIT TRANSACTION");
+						do_sql_command_begin(entry->conn, sql);
 						pending_entries = lappend(pending_entries, entry);
 						continue;
 					}
-					do_sql_command(entry->conn, "COMMIT TRANSACTION");
+					do_sql_command(entry->conn, sql);
+					if ((read_only_level > 0) && !entry->xact_read_only)
+						entry->xact_read_only = true;
 					entry->changing_xact_state = false;
 
 					/*
@@ -2019,6 +2056,8 @@ pgfdw_finish_pre_commit_cleanup(List *pending_entries)
 	 */
 	foreach(lc, pending_entries)
 	{
+		char		sql[100];
+
 		entry = (ConnCacheEntry *) lfirst(lc);
 
 		Assert(entry->changing_xact_state);
@@ -2027,7 +2066,10 @@ pgfdw_finish_pre_commit_cleanup(List *pending_entries)
 		 * We might already have received the result on the socket, so pass
 		 * consume_input=true to try to consume it 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`abort`
- 外部函数：`appendStringInfo`
- 外部函数：`appendStringInfoString`
- 外部函数：`commit`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errcontext`
- 外部函数：`errdetail_internal`
- 外部函数：`errhint`
- 外部函数：`errmsg`
- 外部函数：`errmsg_internal`
- 外部函数：`initStringInfo`
- 外部函数：`key`
- 外部函数：`lappend`
- 外部函数：`libpqsrv_get_result_last`
- 外部函数：`pchomp`
- 外部函数：`pg_unreachable`
- 大写宏：`COMMIT`
- 大写宏：`DEBUG3`
- 大写宏：`DEFERRABLE`
- 大写宏：`ERRCODE_CONNECTION_FAILURE`
- 大写宏：`ERROR`
- 大写宏：`MAKE_SQLSTATE`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`ONLY`
- 大写宏：`PGRES_COMMAND_OK`
- 大写宏：`PG_DIAG_CONTEXT`
- 大写宏：`PG_DIAG_MESSAGE_DETAIL`
- 大写宏：`PG_DIAG_MESSAGE_HINT`
- 大写宏：`PG_DIAG_MESSAGE_PRIMARY`
- 大写宏：`PG_DIAG_SQLSTATE`
- 大写宏：`READ`
- 大写宏：`REPEATABLE`
- 大写宏：`SAVEPOINT`
- 大写宏：`SET`
- 大写宏：`SQL`
- 大写宏：`TRANSACTION`
- 外部类型：`Commit`
- 外部类型：`ConnCacheEntry`
- 外部类型：`ForeignServer`
- 外部类型：`If`
- 外部类型：`Macro`
- 外部类型：`Note`
- 外部类型：`Oid`
- 外部类型：`PGconn`
- 外部类型：`PGresult`
- 外部类型：`PQconsumeInput`
- 外部类型：`PQgetResult`
- 外部类型：`PgFdwConnState`
- 外部类型：`Remaining`
- 外部类型：`Scan`
- 外部类型：`This`
- 外部类型：`WaitLatchOrSocket`
- 外部类型：`XactDeferrable`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-0a5ab8d93c` → 本草稿移入 `cases/defect/auto-postgres-0a5ab8d93c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
