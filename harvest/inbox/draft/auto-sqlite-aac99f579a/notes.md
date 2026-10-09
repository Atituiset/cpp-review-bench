# auto-sqlite-aac99f579a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#f19a90c56977df9e8d85e6b162c684f5bdd89467](https://github.com/sqlite/sqlite/commit/f19a90c56977df9e8d85e6b162c684f5bdd89467) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 34 |
| 编译错误数（gcc syntax-only） | 27（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f19a90c56977df9e8d85e6b162c684f5bdd89467 (https://github.com/sqlite/sqlite/commit/f19a90c56977df9e8d85e6b162c684f5bdd89467)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 422；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f19a90c56977df9e8d85e6b162c684f5bdd89467 Improvements to the display of DDL SQL statements in the CLI and in :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -419,7 +419,7 @@ static void dump_table(const char *zTab, FILE *out){
   const char *zSep;         /* Separator string */
   sqlite3_str *pIns;        /* Beginning of the INSERT statement */
 
-  pStmt = db_prepare("SELECT sql FROM aux.sqlite_schema"
+  pStmt = db_prepare("SELECT shell_complete_sql(sql) FROM aux.sqlite_schema"
                      " WHERE name=%Q COLLATE nocase", zTab);
   if( SQLITE_ROW==sqlite3_step(pStmt) ){
     sqlite3_fprintf(out, "%s;\n", sqlite3_column_text(pStmt,0));
@@ -469,7 +469,7 @@ static void dump_table(const char *zTab, FILE *out){
     sqlite3_finalize(pStmt);
     strFree(pIns);
   } /* endif !g.bSchemaOnly */
-  pStmt = db_prepare("SELECT sql FROM aux.sqlite_schema"
+  pStmt = db_prepare("SELECT shell_complete_sql(sql) FROM aux.sqlite_schema"
                      " WHERE type='index' AND tbl_name=%Q COLLATE nocase"
                      "   AND sql IS NOT NULL",
                      zTab);
@@ -716,7 +716,7 @@ static void diff_one_table(const char *zTab, FILE *out){
 
   /* Create indexes that are missing in the source */
   pStmt = db_prepare(
-    "SELECT sql FROM aux.sqlite_schema"
+    "SELECT shell_complete_sql(sql) FROM aux.sqlite_schema"
     " WHERE type='index' AND tbl_name=%Q COLLATE nocase"
     "   AND sql IS NOT NULL"
     "   AND sql NOT IN (SELECT sql FROM main.sqlite_schema"
@@ -1824,6 +1824,88 @@ static void module_name_func(
   sqlite3_result_text(pCtx, zToken, -1, SQLITE_TRANSIENT);
 }
 
+/*
+** Append text on pOut that is sufficient to complete the statement zBase
+** (according to sqlite3_complete()).  eFlags:
+**
+**    0x01     Include the final ";" if needed
+**    0x04     Assume zBase ends with '\n'
+*/
+static void appendCompletion(
+  sqlite3_str *pOut,      /* Append the completion here */
+  const char *zBase,      /* Existing SQL text */
+  int eFlags              /* 0x01:  Include ";",   0x04: Assume '\n' */
+){
+  sqlite3_int64 R = zBase ? sqlite3_incomplete(zBase) : 0;
+  int cc = (R>>16)&0xff;
+  int nParen = R>>32;
+  int eSemi = (R>>8)&0xff;
+  int eStat = R&0xff;
+  int bSemi = (eFlags & 0x01)!=0;
+  if( cc==0 ){
+    /* no-op */
+  }else if( cc=='-' ){
+    if( (eFlags & 0x04)==0 ) sqlite3_str_appendchar(pOut, 1, '\n');
+  }else if( cc=='/' ){
+    sqlite3_str_append(pOut,"*/",2);
+  }else{
+    sqlite3_str_appendchar(pOut, 1, cc);
+  }
+  if( nParen>0 ){
+    sqlite3_str_appendf(pOut, "%.*c",nParen,')');
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
+** SQL function:  shell_complete_sql(SQL)
+**                shell_complete_sql(SQL, FLAGS)
+**
+** NOTE:  Only the first one-argument form is registered in this application.
+**
+** If the SQL input is a string, return a new string which is the same
+** text with all pending quotes and comments finish, so that the
+** SQL is ready to receive its final semicolon.  The optional FLAGS
+** parameter can be an integer where bits mean:
+**
+**   0x01        Include the final semicolon
+**   0x02        Only return the completion text.  Omit the original SQL.
+**   0x04        Assume that SQL has a \n at the end.
+**
+** The /C substitution in the CLI prompt is computed using the
+** equivalent of shell_complete_sql(SQL,3);
+*/
+static void shell_complete_sql(
+  sqlite3_context *pCtx,
+  int nVal,
+  sqlite3_value **apVal
+){
+  const char *zSql;      /* Input SQL text */
+  sqlite3_str *pOut;     /* Completed SQL */
+  int flags;
+
+  if( nVal!=1 && nVal!=2 ) return;
+  if( nVal==2 ){
+    flags = 0x7 & sqlite3_value_int64(apVal[1]);
+  }else{
+    flags = 0;
+  }
+  zSql = (const char*)sqlite3_value_text(apVal[0]);
+  if( zSql==0 ) return;
+  pOut = sqlite3_str_new(sqlite3_context_db_handle(pCtx));
+  if( (flags & 0x02)==0 ) sq
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`exit`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_db_config`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_fprintf`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_open_v2`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_result_text`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_str_finish`
- 外部函数：`sqlite3_str_new`
- 外部函数：`sqlite3_str_value`
- 外部函数：`sqlite3_str_vappendf`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`AND`
- 大写宏：`COLLATE`
- 大写宏：`FROM`
- 大写宏：`INSERT`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_DBCONFIG_DEFENSIVE`
- 大写宏：`SQLITE_DBCONFIG_TRUSTED_SCHEMA`
- 大写宏：`SQLITE_OPEN_READONLY`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_TRANSIENT`
- 大写宏：`WHERE`
- 外部类型：`Beginning`
- 外部类型：`Create`
- 外部类型：`Return`
- 外部类型：`SQLite`
- 外部类型：`Separator`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-aac99f579a` → 本草稿移入 `cases/defect/auto-sqlite-aac99f579a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
