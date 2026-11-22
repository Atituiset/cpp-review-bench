# auto-postgres-a1cda6e767

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#892cfb4cbfe0e37cc492f0e750e80cfa6394d26d](https://github.com/postgres/postgres/commit/892cfb4cbfe0e37cc492f0e750e80cfa6394d26d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #892cfb4cbfe0e37cc492f0e750e80cfa6394d26d (https://github.com/postgres/postgres/commit/892cfb4cbfe0e37cc492f0e750e80cfa6394d26d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 2002；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 892cfb4cbfe0e37cc492f0e750e80cfa6394d26d pg_basebackup: Fix crashes on malformed server responses :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1999,9 +1999,9 @@ BaseBackup(char *compression_algorithm, char *compression_detail,
 	if (PQresultStatus(res) != PGRES_TUPLES_OK)
 		pg_fatal("could not initiate base backup: %s",
 				 PQerrorMessage(conn));
-	if (PQntuples(res) != 1)
-		pg_fatal("server returned unexpected response to BASE_BACKUP command; got %d rows and %d fields, expected %d rows and %d fields",
-				 PQntuples(res), PQnfields(res), 1, 2);
+	if (PQntuples(res) != 1 || PQnfields(res) < 1)
+		pg_fatal("server returned unexpected response to BASE_BACKUP command; got %d rows and %d fields, expected %d rows and %d or more fields",
+				 PQntuples(res), PQnfields(res), 1, 1);
 
 	strlcpy(xlogstart, PQgetvalue(res, 0, 0), sizeof(xlogstart));
 
@@ -2030,8 +2030,9 @@ BaseBackup(char *compression_algorithm, char *compression_detail,
 	if (PQresultStatus(res) != PGRES_TUPLES_OK)
 		pg_fatal("could not get backup header: %s",
 				 PQerrorMessage(conn));
-	if (PQntuples(res) < 1)
-		pg_fatal("no data returned from server");
+	if (PQntuples(res) < 1 || PQnfields(res) < 3)
+		pg_fatal("could not get backup header: got %d rows and %d fields, expected %d or more rows and %d or more fields",
+				 PQntuples(res), PQnfields(res), 1, 3);
 
 	/*
 	 * Sum up the total size, for progress reporting
@@ -2172,8 +2173,9 @@ BaseBackup(char *compression_algorithm, char *compression_detail,
 	if (PQresultStatus(res) != PGRES_TUPLES_OK)
 		pg_fatal("backup failed: %s",
 				 PQerrorMessage(conn));
-	if (PQntuples(res) != 1)
-		pg_fatal("no write-ahead log end position returned from server");
+	if (PQntuples(res) != 1 || PQnfields(res) < 1)
+		pg_fatal("could not get write-ahead log end position: got %d rows and %d fields, expected %d rows and %d or more fields",
+				 PQntuples(res), PQnfields(res), 1, 1);
 	strlcpy(xlogend, PQgetvalue(res, 0, 0), sizeof(xlogend));
 	if (verbose && includewal != NO_WAL)
 		pg_log_info("write-ahead log end point: %s", xlogend);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pg_fatal`
- 外部函数：`pg_log_info`
- 外部函数：`strlcpy`
- 大写宏：`BASE_BACKUP`
- 大写宏：`NO_WAL`
- 大写宏：`PGRES_TUPLES_OK`
- 外部类型：`Sum`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-a1cda6e767` → 本草稿移入 `cases/defect/auto-postgres-a1cda6e767/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
