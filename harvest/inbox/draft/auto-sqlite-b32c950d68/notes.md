# auto-sqlite-b32c950d68

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#dd33a69869fa628f572cd6b04d4487f19a526f3a](https://github.com/sqlite/sqlite/commit/dd33a69869fa628f572cd6b04d4487f19a526f3a) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 61 |
| 编译错误数（gcc syntax-only） | 23（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #dd33a69869fa628f572cd6b04d4487f19a526f3a (https://github.com/sqlite/sqlite/commit/dd33a69869fa628f572cd6b04d4487f19a526f3a)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1123；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT dd33a69869fa628f572cd6b04d4487f19a526f3a Update RBU to correctly handle short-rows with non-NULL default values. Report [bugs:/info/2026-10-20T03:22:29Z | 2026-1 :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -267,6 +267,7 @@ struct RbuObjIter {
   int nTblCol;                    /* Size of azTblCol[] array */
   char **azTblCol;                /* Array of unquoted target column names */
   char **azTblType;               /* Array of target column types */
+  char **azTblDflt;               /* Array of target column DEFAULT exprs */
   int *aiSrcOrder;                /* src table col -> target table col */
   u8 *abTblPk;                    /* Array of flags, set on target PK columns */
   u8 *abNotNull;                  /* Array of flags, set on NOT NULL columns */
@@ -832,10 +833,12 @@ static void rbuObjIterFreeCols(RbuObjIter *pIter){
   for(i=0; i<pIter->nTblCol; i++){
     sqlite3_free(pIter->azTblCol[i]);
     sqlite3_free(pIter->azTblType[i]);
+    sqlite3_free(pIter->azTblDflt[i]);
   }
   sqlite3_free(pIter->azTblCol);
   pIter->azTblCol = 0;
   pIter->azTblType = 0;
+  pIter->azTblDflt = 0;
   pIter->aiSrcOrder = 0;
   pIter->abTblPk = 0;
   pIter->abNotNull = 0;
@@ -1120,14 +1123,15 @@ static void *rbuMalloc(sqlite3rbu *p, sqlite3_int64 nByte){
 ** error code in the RBU handle passed as the first argument.
 */
 static void rbuAllocateIterArrays(sqlite3rbu *p, RbuObjIter *pIter, int nCol){
-  sqlite3_int64 nByte = (2*sizeof(char*) + sizeof(int) + 3*sizeof(u8)) * nCol;
+  sqlite3_int64 nByte = (3*sizeof(char*) + sizeof(int) + 3*sizeof(u8)) * nCol;
   char **azNew;
 
   azNew = (char**)rbuMalloc(p, nByte);
   if( azNew ){
     pIter->azTblCol = azNew;
     pIter->azTblType = &azNew[nCol];
-    pIter->aiSrcOrder = (int*)&pIter->azTblType[nCol];
+    pIter->azTblDflt = &azNew[nCol*2];
+    pIter->aiSrcOrder = (int*)&pIter->azTblDflt[nCol];
     pIter->abTblPk = (u8*)&pIter->aiSrcOrder[nCol];
     pIter->abNotNull = (u8*)&pIter->abTblPk[nCol];
     pIter->abIndexed = (u8*)&pIter->abNotNull[nCol];
@@ -1439,8 +1443,8 @@ static int rbuObjIterCacheTableInfo(sqlite3rbu *p, RbuObjIter *pIter){
     bStrict = rbuTableIsStrict(p, pIter->zTbl);
 
     /* Check that all non-HIDDEN columns in the destination table are also
-    ** present in the input table. Populate the abTblPk[], azTblType[] and
-    ** aiTblOrder[] arrays at the same time.
+    ** present in the input table. Populate the abTblPk[], azTblType[],
+    ** azTblDflt[] and aiTblOrder[] arrays at the same time.
     **
     ** RBU does not support tables with GENERATED columns. If the target
     ** table has any, return an error.  */
@@ -1473,6 +1477,7 @@ static int rbuObjIterCacheTableInfo(sqlite3rbu *p, RbuObjIter *pIter){
         int iPk = sqlite3_column_int(pStmt, 5);
         int bNotNull = sqlite3_column_int(pStmt, 3);
         const char *zType = (const char*)sqlite3_column_text(pStmt, 2);
+        const char *zDflt = (const char*)sqlite3_column_text(pStmt, 4);
 
         if( i!=iOrder ){
           SWAP(int, pIter->aiSrcOrder[i], pIter->aiSrcOrder[iOrder]);
@@ -1483,6 +1488,7 @@ static int rbuObjIterCacheTableInfo(sqlite3rbu *p, RbuObjIter *pIter){
         ** imposter it would mean NUMERIC, so declare it without a type. */
         if( bStrict && sqlite3_stricmp(zType, "ANY")==0 ) zType = "";
         pIter->azTblType[iOrder] = rbuStrndup(zType, &p->rc);
+        pIter->azTblDflt[iOrder] = rbuStrndup(zDflt, &p->rc);
         assert( iPk>=0 );
         pIter->abTblPk[iOrder] = (u8)iPk;
         pIter->abNotNull[iOrder] = (u8)bNotNull || (iPk!=0);
@@ -2205,9 +2211,11 @@ static void rbuCreateImposterTable(sqlite3rbu *p, RbuObjIter *pIter){
         ** "PRIMARY KEY" to the imposter table column declaration. */
         zPk = "PRIMARY KEY ";
       }
-      zSql = rbuMPrintf(p, "%z%s\"%w\" %s %sCOLLATE %Q%s", 
+      zSql = rbuMPrintf(p, "%z%s\"%w\" %s %sCOLLATE %Q%s%s%s", 
           zSql, zComma, zCol, pIter->azTblType[iCol], zPk, zColl,
-          (pIter->abNotNull[iCol] ? " NOT NULL" : "")
+          (pIter->abNotNull[iCol] ? " NOT NULL" : ""),
+          (pIter->azTblDflt[iCol] ? " DEFAULT " : ""),
+          (pIter->azTblDflt[iCol] ? pIter->azTblDflt[iCol] : 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`db`
- 外部函数：`int`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`rbuSetupCheckpoint`
- 外部函数：`rbu_step`
- 外部函数：`sqlite3_db_handle`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`stage`
- 外部函数：`state`
- 外部函数：`strlen`
- 外部函数：`table_list`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PRAGMA`
- 大写宏：`RBU`
- 大写宏：`RBU_STAGE_CKPT`
- 大写宏：`RBU_STATE_STAGE`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`TYPE`
- 大写宏：`VFS`
- 外部类型：`Allocated`
- 外部类型：`Array`
- 外部类型：`Current`
- 外部类型：`Db`
- 外部类型：`Entries`
- 外部类型：`Error`
- 外部类型：`Fd`
- 外部类型：`File`
- 外部类型：`Iterator`
- 外部类型：`Largest`
- 外部类型：`Name`
- 外部类型：`Number`
- 外部类型：`Pages`
- 外部类型：`Path`
- 外部类型：`RbuFrame`
- 外部类型：`RbuObjIter`
- 外部类型：`Rows`
- 外部类型：`See`
- 外部类型：`Size`
- 外部类型：`The`
- 外部类型：`Total`
- 外部类型：`Used`
- 外部类型：`Value`
- 外部类型：`rbu_file`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-b32c950d68` → 本草稿移入 `cases/defect/auto-sqlite-b32c950d68/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
