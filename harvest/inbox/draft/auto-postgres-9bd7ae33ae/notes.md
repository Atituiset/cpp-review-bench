# auto-postgres-9bd7ae33ae

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#50d6e533e4d9a0f70d798c534254007c83c0d428](https://github.com/postgres/postgres/commit/50d6e533e4d9a0f70d798c534254007c83c0d428) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 50 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #50d6e533e4d9a0f70d798c534254007c83c0d428 (https://github.com/postgres/postgres/commit/50d6e533e4d9a0f70d798c534254007c83c0d428)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 50d6e533e4d9a0f70d798c534254007c83c0d428 Fix crash when describing a FETCH statement whose cursor is gone :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -456,6 +456,52 @@ test_cancel(PGconn *conn)
 	fprintf(stderr, "ok\n");
 }
 
+/*
+ * Test Describe of a prepared FETCH statement after the cursor it
+ * references has been closed.
+ */
+static void
+test_describe_fetch(PGconn *conn)
+{
+	PGresult   *res;
+
+	fprintf(stderr, "test cursor describe and fetch... ");
+
+	res = PQexec(conn, "BEGIN");
+	if (PQresultStatus(res) != PGRES_COMMAND_OK)
+		pg_fatal("BEGIN failed: %s", PQerrorMessage(conn));
+	PQclear(res);
+
+	res = PQexec(conn, "DECLARE fetch_cursor CURSOR FOR SELECT 1");
+	if (PQresultStatus(res) != PGRES_COMMAND_OK)
+		pg_fatal("DECLARE CURSOR failed: %s", PQerrorMessage(conn));
+	PQclear(res);
+
+	/* prepare while the cursor exists, so that it caches a result desc */
+	res = PQprepare(conn, "fetch_one", "FETCH 1 FROM fetch_cursor", 0, NULL);
+	if (PQresultStatus(res) != PGRES_COMMAND_OK)
+		pg_fatal("PQprepare failed: %s", PQerrorMessage(conn));
+	PQclear(res);
+
+	res = PQexec(conn, "CLOSE fetch_cursor");
+	if (PQresultStatus(res) != PGRES_COMMAND_OK)
+		pg_fatal("CLOSE failed: %s", PQerrorMessage(conn));
+	PQclear(res);
+
+	/* describe fails after the cursor has been closed */
+	res = PQdescribePrepared(conn, "fetch_one");
+	if (PQresultStatus(res) != PGRES_FATAL_ERROR)
+		pg_fatal("expected FATAL_ERROR, got %s", PQresStatus(PQresultStatus(res)));
+	PQclear(res);
+
+	res = PQexec(conn, "ROLLBACK");
+	if (PQresultStatus(res) != PGRES_COMMAND_OK)
+		pg_fatal("ROLLBACK failed: %s", PQerrorMessage(conn));
+	PQclear(res);
+
+	fprintf(stderr, "ok\n");
+}
+
 static void
 test_disallowed_in_pipeline(PGconn *conn)
 {
@@ -2117,6 +2163,7 @@ static void
 print_test_list(void)
 {
 	printf("cancel\n");
+	printf("describe_fetch\n");
 	printf("disallowed_in_pipeline\n");
 	printf("multi_pipelines\n");
 	printf("nosync\n");
@@ -2223,6 +2270,8 @@ main(int argc, char **argv)
 
 	if (strcmp(testname, "cancel") == 0)
 		test_cancel(conn);
+	else if (strcmp(testname, "describe_fetch") == 0)
+		test_describe_fetch(conn);
 	else if (strcmp(testname, "disallowed_in_pipeline") == 0)
 		test_disallowed_in_pipeline(conn);
 	else if (strcmp(testname, "multi_pipelines") == 0)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`consume_query_cancel_impl`
- 外部函数：`count`
- 外部函数：`exit`
- 外部函数：`fflush`
- 外部函数：`fprintf`
- 外部函数：`getenv`
- 外部函数：`pfree`
- 外部函数：`pg_debug`
- 外部函数：`pg_fatal`
- 外部函数：`pg_fatal_impl`
- 外部函数：`pg_free`
- 外部函数：`pg_malloc_array`
- 外部函数：`pg_sleep`
- 外部函数：`pg_usleep`
- 外部函数：`psprintf`
- 外部函数：`send_cancellable_query_impl`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vfprintf`
- 大写宏：`AND`
- 大写宏：`CONNECTION_OK`
- 大写宏：`FROM`
- 大写宏：`INT4OID`
- 大写宏：`NULL`
- 大写宏：`PGRES_FATAL_ERROR`
- 大写宏：`PGRES_TUPLES_OK`
- 大写宏：`PG_DIAG_SQLSTATE`
- 大写宏：`PG_TEST_TIMEOUT_DEFAULT`
- 大写宏：`SELECT`
- 大写宏：`TEXTOID`
- 大写宏：`WHERE`
- 外部类型：`Connection`
- 外部类型：`ExecStatusType`
- 外部类型：`Make`
- 外部类型：`Oid`
- 外部类型：`PGcancel`
- 外部类型：`PGcancelConn`
- 外部类型：`PGconn`
- 外部类型：`PGresult`
- 外部类型：`PQcancel`
- 外部类型：`PQconninfoOption`
- 外部类型：`PQgetResult`
- 外部类型：`PQrequestCancel`
- 外部类型：`PgSleep`
- 外部类型：`Wait`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-9bd7ae33ae` → 本草稿移入 `cases/defect/auto-postgres-9bd7ae33ae/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
