# auto-sqlite-486805aaea

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#cb547ab3e931c7766e24834af5ef6c4578863e3d](https://github.com/sqlite/sqlite/commit/cb547ab3e931c7766e24834af5ef6c4578863e3d) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 31 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cb547ab3e931c7766e24834af5ef6c4578863e3d (https://github.com/sqlite/sqlite/commit/cb547ab3e931c7766e24834af5ef6c4578863e3d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 421；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cb547ab3e931c7766e24834af5ef6c4578863e3d Update the CLI and the sqldiff utility program to use "COLLATE nocase" when :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -40,6 +40,7 @@ struct GlobalVars {
   int bSchemaPK;            /* Use the schema-defined PK, not the true PK */
   int bHandleVtab;          /* Handle fts3, fts4, fts5 and rtree vtabs */
   unsigned fDebug;          /* Debug flags */
+  int bUnsafe;              /* Try to work through corrupt databases */
   int bSchemaCompare;       /* Doing single-table sqlite_schema compare */
   sqlite3 *db;              /* The database connection */
 } g;
@@ -418,7 +419,8 @@ static void dump_table(const char *zTab, FILE *out){
   const char *zSep;         /* Separator string */
   sqlite3_str *pIns;        /* Beginning of the INSERT statement */
 
-  pStmt = db_prepare("SELECT sql FROM aux.sqlite_schema WHERE name=%Q", zTab);
+  pStmt = db_prepare("SELECT sql FROM aux.sqlite_schema"
+                     " WHERE name=%Q COLLATE nocase", zTab);
   if( SQLITE_ROW==sqlite3_step(pStmt) ){
     sqlite3_fprintf(out, "%s;\n", sqlite3_column_text(pStmt,0));
   }
@@ -468,7 +470,8 @@ static void dump_table(const char *zTab, FILE *out){
     strFree(pIns);
   } /* endif !g.bSchemaOnly */
   pStmt = db_prepare("SELECT sql FROM aux.sqlite_schema"
-                     " WHERE type='index' AND tbl_name=%Q AND sql IS NOT NULL",
+                     " WHERE type='index' AND tbl_name=%Q COLLATE nocase"
+                     "   AND sql IS NOT NULL",
                      zTab);
   while( SQLITE_ROW==sqlite3_step(pStmt) ){
     sqlite3_fprintf(out, "%s;\n", sqlite3_column_text(pStmt,0));
@@ -653,10 +656,10 @@ static void diff_one_table(const char *zTab, FILE *out){
   /* Drop indexes that are missing in the destination */
   pStmt = db_prepare(
     "SELECT name FROM main.sqlite_schema"
-    " WHERE type='index' AND tbl_name=%Q"
+    " WHERE type='index' AND tbl_name=%Q COLLATE nocase"
     "   AND sql IS NOT NULL"
     "   AND sql NOT IN (SELECT sql FROM aux.sqlite_schema"
-    "                    WHERE type='index' AND tbl_name=%Q"
+    "                    WHERE type='index' AND tbl_name=%Q COLLATE nocase"
     "                      AND sql IS NOT NULL)",
     zTab, zTab);
   while( SQLITE_ROW==sqlite3_step(pStmt) ){
@@ -714,10 +717,10 @@ static void diff_one_table(const char *zTab, FILE *out){
   /* Create indexes that are missing in the source */
   pStmt = db_prepare(
     "SELECT sql FROM aux.sqlite_schema"
-    " WHERE type='index' AND tbl_name=%Q"
+    " WHERE type='index' AND tbl_name=%Q COLLATE nocase"
     "   AND sql IS NOT NULL"
     "   AND sql NOT IN (SELECT sql FROM main.sqlite_schema"
-    "                    WHERE type='index' AND tbl_name=%Q"
+    "                    WHERE type='index' AND tbl_name=%Q COLLATE nocase"
     "                      AND sql IS NOT NULL)",
     zTab, zTab);
   while( SQLITE_ROW==sqlite3_step(pStmt) ){
@@ -742,7 +745,8 @@ static void diff_one_table(const char *zTab, FILE *out){
 static void checkSchemasMatch(const char *zTab){
   sqlite3_stmt *pStmt = db_prepare(
       "SELECT A.sql=B.sql FROM main.sqlite_schema A, aux.sqlite_schema B"
-      " WHERE A.name=%Q AND B.name=%Q", zTab, zTab
+      " WHERE A.name=%Q COLLATE nocase"
+      "   AND B.name=%Q COLLATE nocase", zTab, zTab
   );
   if( SQLITE_ROW==sqlite3_step(pStmt) ){
     if( sqlite3_column_int(pStmt,0)==0 ){
@@ -1977,6 +1981,9 @@ int main(int argc, char **argv){
       if( strcmp(z,"transaction")==0 ){
         useTransaction = 1;
       }else
+      if( strcmp(z,"unsafe")==0 ){
+        g.bUnsafe = 1;
+      }else
       if( strcmp(z,"vtab")==0 ){
         g.bHandleVtab = 1;
       }else
@@ -2001,6 +2008,12 @@ int main(int argc, char **argv){
   if( rc ){
     cmdlineError("cannot open database file \"%s\"", zDb1);
   }
+  if( g.bUnsafe ){
+    sqlite3_db_config(g.db, SQLITE_DBCONFIG_WRITABLE_SCHEMA, 1, 0);
+  }else{
+    sqlite3_db_config(g.db, SQLITE_DBCONFIG_DEFENSIVE, 1, 0);
+    sqlite3_db_config(g.db, SQLITE_DBCONFIG_TRUSTED_SCHEMA, 0, 0);
+  }
   rc = sqlite3_exec(g.db, "SELECT * FROM sqlite_schema", 0, 0, &zErrMsg);
   if( rc || zE
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`exit`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_fprintf`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_str_finish`
- 外部函数：`sqlite3_str_new`
- 外部函数：`sqlite3_str_value`
- 外部函数：`sqlite3_str_vappendf`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`strcmp`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`AND`
- 大写宏：`FROM`
- 大写宏：`INSERT`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_ROW`
- 大写宏：`WHERE`
- 外部类型：`Beginning`
- 外部类型：`Create`
- 外部类型：`Debug`
- 外部类型：`Doing`
- 外部类型：`Drop`
- 外部类型：`Handle`
- 外部类型：`SQLite`
- 外部类型：`Separator`
- 外部类型：`The`
- 外部类型：`Use`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-486805aaea` → 本草稿移入 `cases/defect/auto-sqlite-486805aaea/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
