# auto-sqlite-cded1697ed

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#90ba7148c6d943ec5233002927c18e57cc8a5589](https://github.com/sqlite/sqlite/commit/90ba7148c6d943ec5233002927c18e57cc8a5589) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 31 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #90ba7148c6d943ec5233002927c18e57cc8a5589 (https://github.com/sqlite/sqlite/commit/90ba7148c6d943ec5233002927c18e57cc8a5589)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1326；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 90ba7148c6d943ec5233002927c18e57cc8a5589 Merge all recent trunk changes into this branch. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1301,6 +1301,37 @@ static int idxAuthCallback(
   return rc;
 }
 
+/*
+** Execute the CREATE statement in zSql, which was read from the
+** sqlite_schema table of the database being analyzed, using database
+** handle db. If no error occurs, set (*pzErr) to NULL and return
+** SQLITE_OK. Otherwise, return an SQLite error code and set (*pzErr) to
+** point to a buffer containing an English language error message.
+**
+** If the database was opened with "PRAGMA writable_schema=ON", the text
+** read from sqlite_schema is not guaranteed to be a single CREATE
+** statement. So the same rules are applied as when SQLite itself loads a
+** schema: text that does not begin with "CR" is ignored, as only CREATE
+** statements begin with those two letters, and only the first statement
+** in the text is executed. Anything that follows it is ignored.
+*/
+static int expertSchemaExec(sqlite3 *db, const char *zSql, char **pzErr){
+  int rc = SQLITE_OK;
+  if( pzErr ) *pzErr = 0;
+  if( (zSql[0]=='c' || zSql[0]=='C') && (zSql[1]=='r' || zSql[1]=='R') ){
+    sqlite3_stmt *pStmt = 0;
+    rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
+    if( rc==SQLITE_OK && pStmt ){
+      while( SQLITE_ROW==sqlite3_step(pStmt) ){}
+      rc = sqlite3_finalize(pStmt);
+    }
+    if( rc!=SQLITE_OK && pzErr ){
+      *pzErr = sqlite3_mprintf("%s", sqlite3_errmsg(db));
+    }
+  }
+  return rc;
+}
+
 static int idxProcessOneTrigger(
   sqlite3expert *p, 
   IdxWrite *pWrite, 
@@ -1323,7 +1354,7 @@ static int idxProcessOneTrigger(
   while( rc==SQLITE_OK && SQLITE_ROW==sqlite3_step(pSelect) ){
     const char *zCreate = (const char*)sqlite3_column_text(pSelect, 0);
     if( zCreate==0 ) continue;
-    rc = sqlite3_exec(p->dbv, zCreate, 0, 0, pzErr);
+    rc = expertSchemaExec(p->dbv, zCreate, pzErr);
   }
   idxFinalize(&rc, pSelect);
 
@@ -1448,7 +1479,7 @@ static int expertSchemaSql(sqlite3 *db, const char *zSql, char **pzErr){
   int rc = SQLITE_OK;
   char *zErr = 0;
 
-  rc = sqlite3_exec(db, zSql, 0, 0, &zErr);
+  rc = expertSchemaExec(db, zSql, &zErr);
   if( rc!=SQLITE_OK && zErr ){
     int nErr = STRLEN(zErr);
     if( nErr>=15 && memcmp(zErr, "no such module:", 15)==0 ){
@@ -1916,7 +1947,8 @@ static int idxPopulateStat1(sqlite3expert *p, char **pzErr){
 ** Define and possibly pretend to use a useless collation sequence.
 ** This pretense allows expert to accept SQL using custom collations.
 */
-int dummyCompare(void *up1, int up2, const void *up3, int up4, const void *up5){
+static int dummyCompare(void *up1, int up2, const void *up3, int up4,
+                        const void *up5){
   (void)up1;
   (void)up2;
   (void)up3;
@@ -1926,7 +1958,7 @@ int dummyCompare(void *up1, int up2, const void *up3, int up4, const void *up5){
   return 0;
 }
 /* And a callback to register above upon actual need */
-void useDummyCS(void *up1, sqlite3 *db, int etr, const char *zName){
+static void useDummyCS(void *up1, sqlite3 *db, int etr, const char *zName){
   (void)up1;
   sqlite3_create_collation_v2(db, zName, etr, 0, dummyCompare, 0);
 }
@@ -1936,21 +1968,21 @@ void useDummyCS(void *up1, sqlite3 *db, int etr, const char *zName){
 /*
 ** dummy functions for no-op implementation of UDFs during expert's work
 */
-void dummyUDF(sqlite3_context *up1, int up2, sqlite3_value **up3){
+static void dummyUDF(sqlite3_context *up1, int up2, sqlite3_value **up3){
   (void)up1;
   (void)up2;
   (void)up3;
   assert(0); /* VDBE should never be run. */
 }
-void dummyUDFvalue(sqlite3_context *up1){
+static void dummyUDFvalue(sqlite3_context *up1){
   (void)up1;
   assert(0); /* VDBE should never be run. */
 }
 
 /*
 ** Register UDFs from user database with another.
 */
-int registerUDFs(sqlite3 *dbSrc, sqlite3 *dbDst){
+static int registerUDFs(sqlite3 *dbSrc, sqlite3 *dbDst){
   sqlite3_stmt *pStmt;
   int rc = sqlite3_prepare_v2(dbSrc,
             "SELECT name,type,enc,narg,flags "
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memcmp`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_create_collation_v2`
- 外部函数：`sqlite3_exec`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_step`
- 大写宏：`EXPERT_REPORT_CANDIDATES`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`VDBE`
- 外部类型：`And`
- 外部类型：`Define`
- 外部类型：`Error`
- 外部类型：`For`
- 外部类型：`Hash`
- 外部类型：`IdxHash`
- 外部类型：`IdxScan`
- 外部类型：`IdxStatement`
- 外部类型：`IdxTable`
- 外部类型：`IdxWrite`
- 外部类型：`In`
- 外部类型：`List`
- 外部类型：`Percentage`
- 外部类型：`Register`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`UDFs`
- 外部类型：`User`
- 外部类型：`Vtab`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-cded1697ed` → 本草稿移入 `cases/defect/auto-sqlite-cded1697ed/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
