# auto-sqlite-71da1267e2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#6f9d43469b47de16a198e114e0a037a069dba69c](https://github.com/sqlite/sqlite/commit/6f9d43469b47de16a198e114e0a037a069dba69c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 19 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6f9d43469b47de16a198e114e0a037a069dba69c (https://github.com/sqlite/sqlite/commit/6f9d43469b47de16a198e114e0a037a069dba69c)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 439；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6f9d43469b47de16a198e114e0a037a069dba69c Enhance both the CLI and the ext/misc/dbdump.c library so that it is more :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -301,6 +301,110 @@ static char *safeCommentText(const char *z){
   return zOut;
 }
 
+/*
+** Append to pOut whatever text is needed to complete the SQL statement
+** zBase: the end of an unterminated comment or string literal, any missing
+** close-parentheses, "END" for an unterminated trigger, and (if bSemi)
+** the final ";".  This is a copy of appendCompletion() in the CLI.
+**
+** The sql column of sqlite_schema comes from the (possibly hostile)
+** database file.  Without this, a statement ending in an open comment
+** could swallow the ";" and the text that follows it in the output.
+*/
+static void appendCompletion(sqlite3_str *pOut, const char *zBase, int bSemi){
+  sqlite3_int64 R = zBase ? sqlite3_incomplete(zBase) : 0;
+  int cc = (R>>16)&0xff;
+  int nParen = R>>32;
+  int eSemi = (R>>8)&0xff;
+  int eStat = R&0xff;
+  if( cc==0 ){
+    /* no-op */
+  }else if( cc=='-' ){
+    sqlite3_str_appendchar(pOut, 1, '\n');
+  }else if( cc=='/' ){
+    sqlite3_str_append(pOut, "*/", 2);
+  }else{
+    sqlite3_str_appendchar(pOut, 1, cc);
+  }
+  if( nParen>0 ){
+    sqlite3_str_appendf(pOut, "%.*c", nParen, ')');
+  }
+  if( eStat==SQLITE_OK || eStat==SQLITE_EMPTY ){
+    /* Nothing to add */
+  }else if( eSemi==1 ){
+    if( bSemi ) sqlite3_str_append(pOut, ";", 1);
+  }else if( eSemi==2 ){
+    sqlite3_str_append(pOut, "END;", 3+(bSemi!=0));
+  }else if( eSemi==3 ){
+    sqlite3_str_append(pOut, ";END;", 4+(bSemi!=0));
+  }
+}
+
+/*
+** The text in z[] is an SQL statement taken from the sql column of
+** sqlite_schema.  Normally it is exactly one statement.  But if the
+** database file is corrupt or hostile it might contain additional
+** statements after the first one, separated by ";".  Return the number
+** of bytes in the prefix of z[] that form the first complete SQL statement,
+** including its terminating ";".  Set *pbComplete to true if the returned
+** prefix is such a complete statement.  If z[] has no complete statement
+** prefix, return strlen(z) and set *pbComplete to false, in which case
+** the caller should use appendCompletion() to close off the statement.
+**
+** The usual case is that z[] contains no ";" at all, which strchr()
+** detects immediately.  Otherwise each ";" is tried in turn as the end of
+** the statement, using sqlite3_complete() to skip over semicolons that are
+** inside of strings, comments, or CREATE TRIGGER bodies.
+**
+** z[] is modified temporarily but is restored before this routine returns.
+*/
+static int firstStatementLen(char *z, int *pbComplete){
+  char *zSemi = strchr(z, ';');
+  *pbComplete = 0;
+  while( zSemi ){
+    char c = zSemi[1];
+    int rc;
+    zSemi[1] = 0;
+    rc = sqlite3_complete(z);
+    zSemi[1] = c;
+    if( rc ){
+      *pbComplete = 1;
+      return (int)(zSemi + 1 - z);
+    }
+    zSemi = strchr(zSemi+1, ';');
+  }
+  return (int)strlen(z);
+}
+
+/*
+** Send the SQL statement zPrefix+zSql to the output callback, followed
+** by ";\n".  Any text after the first complete statement in zSql is
+** omitted, and appendCompletion() closes off any unterminated comment,
+** string, parenthesis or trigger body.
+*/
+static void output_statement(DState *p, const char *zPrefix, const char *zSql){
+  sqlite3_str *pOut = sqlite3_str_new(p->db);
+  char *z;
+  if( zPrefix ) sqlite3_str_appendall(pOut, zPrefix);
+  if( zSql ) sqlite3_str_appendall(pOut, zSql);
+  z = sqlite3_str_value(pOut);
+  if( z ){
+    int bComplete;
+    int n = firstStatementLen(z, &bComplete);
+    if( n<sqlite3_str_length(pOut) ) sqlite3_str_truncate(pOut, n);
+  }
+  appendCompletion(pOut, sqlite3_str_value(pOut), 1);
+  sqlite3_str_append(pOut, "\n", 1);
+  z = sqlite3_str_finish(pOut);
+  if( z==0 ){
+    p->rc = SQLITE_NOMEM;
+    p->nErr++;
+    return;
+  }
+  p->xCallback(z, p->pArg);
+  sqlite3_free(z);
+}
+
 /*
 ** Send mprintf-formatted content to the output callback.
 */
@@ -436,12 +540,10 @@ static int dump_callback(void *pArg, int nArg, char **azArg, char **azCol){
     
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`sqlite3_column_count`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_strglob`
- 外部函数：`strcmp`
- 外部函数：`va_start`
- 外部函数：`xCallback`
- 大写宏：`CREATE`
- 大写宏：`EXISTS`
- 大写宏：`NOT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_ROW`
- 大写宏：`TABLE`
- 外部类型：`DState`
- 外部类型：`Execute`
- 外部类型：`If`
- 外部类型：`Print`
- 外部类型：`Query`
- 外部类型：`Send`
- 外部类型：`That`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-71da1267e2` → 本草稿移入 `cases/defect/auto-sqlite-71da1267e2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
