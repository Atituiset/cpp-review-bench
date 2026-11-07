# auto-sqlite-9b2fa33350

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#ccfe1dfde637fcb4da93ad07dbd213f83606bde0](https://github.com/sqlite/sqlite/commit/ccfe1dfde637fcb4da93ad07dbd213f83606bde0) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 67 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ccfe1dfde637fcb4da93ad07dbd213f83606bde0 (https://github.com/sqlite/sqlite/commit/ccfe1dfde637fcb4da93ad07dbd213f83606bde0)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 556；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ccfe1dfde637fcb4da93ad07dbd213f83606bde0 The .recover command now omits extraneous text found at the end of the :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -553,11 +553,11 @@ static void recoverFinalize(sqlite3_recover *p, sqlite3_stmt *pStmt){
 ** Return the sqlite3_finalizer() or sqlite3_prepare() result code
 ** from running the zSql statement.
 */
-static int recoverOneStmt(sqlite3 *db, const char *zSql){
+static int recoverOneStmt(sqlite3 *db, const char *zSql, const char **pzTail){
   sqlite3_stmt *pStmt = 0;
   int rc;
   if( zSql==0 ) return SQLITE_OK;
-  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0);
+  rc = sqlite3_prepare_v2(db, zSql, -1, &pStmt, pzTail);
   if( rc ){
     sqlite3_finalize(pStmt);
     return rc;
@@ -578,7 +578,7 @@ static int recoverOneStmt(sqlite3 *db, const char *zSql){
 */
 static int recoverExec(sqlite3_recover *p, sqlite3 *db, const char *zSql){
   if( p->errCode==SQLITE_OK ){
-    int rc = recoverOneStmt(db, zSql);
+    int rc = recoverOneStmt(db, zSql, 0);
     if( rc ){
       recoverDbError(p, db);
     }
@@ -1260,15 +1260,24 @@ static int recoverWriteSchema1(sqlite3_recover *p){
       const char *zSql = (const char*)sqlite3_column_text(pSelect, 4);
       char *zFree = 0;
       int rc = SQLITE_OK;
+      const char *zTail = 0;
 
       if( bVirtual ){
         zSql = (const char*)(zFree = recoverMPrintf(p,
             "INSERT INTO sqlite_schema VALUES('table', %Q, %Q, 0, %Q)",
             zName, zName, zSql
         ));
       }
-      rc = recoverOneStmt(p->dbOut, zSql);
+      rc = recoverOneStmt(p->dbOut, zSql, &zTail);
       if( rc==SQLITE_OK ){
+        char *pToFree = 0;
+        if( zTail && zTail[0] ){
+          int n = (int)(zTail - zSql);
+          while( n>0 && (zSql[n-1]==';' || zSql[n-1]<=' ') ){ n--; }
+          pToFree = sqlite3_mprintf("%.*s", n, zSql);
+          zSql = pToFree;
+         
+        }
         recoverSqlCallback(p, zSql);
         if( bTable && !bVirtual ){
           if( SQLITE_ROW==sqlite3_step(pTblname) ){
@@ -1277,6 +1286,7 @@ static int recoverWriteSchema1(sqlite3_recover *p){
           }
           recoverReset(p, pTblname);
         }
+        sqlite3_free(pToFree);
       }else if( rc!=SQLITE_ERROR ){
         recoverDbError(p, p->dbOut);
       }
@@ -1319,7 +1329,7 @@ static int recoverWriteSchema2(sqlite3_recover *p){
   if( pSelect ){
     while( sqlite3_step(pSelect)==SQLITE_ROW ){
       const char *zSql = (const char*)sqlite3_column_text(pSelect, 1);
-      int rc = recoverOneStmt(p->dbOut, zSql);
+      int rc = recoverOneStmt(p->dbOut, zSql, 0);
       if( rc==SQLITE_OK ){
         recoverSqlCallback(p, zSql);
       }else if( rc!=SQLITE_ERROR ){
@@ -2704,7 +2714,7 @@ static void recoverStep(sqlite3_recover *p){
           if( bUseWrapper ) recoverUninstallWrapper(p);
         }while( p->errCode==SQLITE_NOTADB 
              && (bUseWrapper--) 
-             && SQLITE_OK==recoverOneStmt(p->dbIn, "ROLLBACK")
+             && SQLITE_OK==recoverOneStmt(p->dbIn, "ROLLBACK", 0)
         );
       }
 
@@ -2769,7 +2779,7 @@ static void recoverStep(sqlite3_recover *p){
       ** database. Regardless of whether or not an error has occurred, make
       ** an attempt to end the read transaction on the input database.  */
       recoverExec(p, p->dbOut, "COMMIT");
-      rc = recoverOneStmt(p->dbIn, "END");
+      rc = recoverOneStmt(p->dbIn, "END", 0);
       if( p->errCode==SQLITE_OK ) p->errCode = rc;
 
       recoverSqlCallback(p, "PRAGMA writable_schema = off");
@@ -2965,7 +2975,7 @@ int sqlite3_recover_finish(sqlite3_recover *p){
   }else{
     recoverFinalCleanup(p);
     if( p->bCloseTransaction && sqlite3_get_autocommit(p->dbIn)==0 ){
-      rc = recoverOneStmt(p->dbIn, "END");
+      rc = recoverOneStmt(p->dbIn, "END", 0);
       if( p->errCode==SQLITE_OK ) p->errCode = rc;
     }
     rc = p->errCode;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`db`
- 外部函数：`int`
- 外部函数：`memset`
- 外部函数：`recoverError`
- 外部函数：`recoverOneStmt`
- 外部函数：`sqlite3_column_text`
- 外部函数：`sqlite3_db_handle`
- 外部函数：`sqlite3_errcode`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_prepare`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_recover_config`
- 外部函数：`sqlite3_recover_errcode`
- 外部函数：`sqlite3_recover_errmsg`
- 外部函数：`sqlite3_recover_run`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_value_free`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`table`
- 外部函数：`use`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`xSql`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`NULL`
- 大写宏：`RECOVER_STATE_LOSTANDFOUND`
- 大写宏：`RECOVER_STATE_WRITING`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_CONSTRAINT`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_RECOVER_FREELIST_CORRUPT`
- 大写宏：`SQLITE_RECOVER_ROWIDS`
- 大写宏：`SQLITE_RECOVER_SLOWINDEXES`
- 大写宏：`SQLITE_ROW`
- 大写宏：`URI`
- 大写宏：`VALUES`
- 外部类型：`Copies`
- 外部类型：`Error`
- 外部类型：`Fields`
- 外部类型：`For`
- 外部类型：`Input`
- 外部类型：`List`
- 外部类型：`Name`
- 外部类型：`Output`
- 外部类型：`Pointer`
- 外部类型：`RecoverBitmap`
- 外部类型：`RecoverStateLAF`
- 外部类型：`RecoverStateW1`
- 外部类型：`RecoverTable`
- 外部类型：`Return`
- 外部类型：`State`
- 外部类型：`Values`
- 外部类型：`Variables`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-9b2fa33350` → 本草稿移入 `cases/defect/auto-sqlite-9b2fa33350/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
