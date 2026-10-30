# auto-sqlite-dea5bb1f29

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#c1bf3367ea188a82153b830399b00e82652db439](https://github.com/sqlite/sqlite/commit/c1bf3367ea188a82153b830399b00e82652db439) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 49 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c1bf3367ea188a82153b830399b00e82652db439 (https://github.com/sqlite/sqlite/commit/c1bf3367ea188a82153b830399b00e82652db439)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 2756；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c1bf3367ea188a82153b830399b00e82652db439 Do not let a TEMP table or an uninitialized schema prevent unauthorized :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1108,12 +1108,16 @@ int sqlite3WritableSchema(sqlite3 *db){
 ** When parsing the sqlite_schema table, this routine also checks to
 ** make sure the "type", "name", and "tbl_name" columns are consistent
 ** with the SQL.
+**
+** If shadow tables are read-only, names that are shadow table names
+** of a virtual table in schema zDb are also reserved.
 */
 int sqlite3CheckObjectName(
   Parse *pParse,            /* Parsing context */
   const char *zName,        /* Name of the object to check */
   const char *zType,        /* Type of this object */
-  const char *zTblName      /* Parent table name for triggers and indexes */
+  const char *zTblName,     /* Parent table name for triggers and indexes */
+  const char *zDb           /* Schema in which the object will be created */
 ){
   sqlite3 *db = pParse->db;
   if( sqlite3WritableSchema(db)
@@ -1133,7 +1137,8 @@ int sqlite3CheckObjectName(
     }
   }else{
     if( (pParse->nested==0 && 0==sqlite3StrNICmp(zName, "sqlite_", 7))
-     || (sqlite3ReadOnlyShadowTables(db) && sqlite3ShadowTableName(db, zName))
+     || (sqlite3ReadOnlyShadowTables(db)
+         && sqlite3ShadowTableName(db, zName, zDb))
     ){
       sqlite3ErrorMsg(pParse, "object name reserved for internal use: %s",
                       zName);
@@ -1320,10 +1325,14 @@ void sqlite3StartTable(
     if( IN_RENAME_OBJECT ){
       sqlite3RenameTokenMap(pParse, (void*)zName, pName);
     }
+    if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
+      goto begin_table_error;
+    }
   }
   pParse->sNameToken = *pName;
   if( zName==0 ) return;
-  if( sqlite3CheckObjectName(pParse, zName, isView?"view":"table", zName) ){
+  if( sqlite3CheckObjectName(pParse, zName, isView?"view":"table", zName,
+                             db->aDb[iDb].zDbSName) ){
     goto begin_table_error;
   }
   if( db->init.iDb==1 ) isTemp = 1;
@@ -1357,9 +1366,6 @@ void sqlite3StartTable(
   */
   if( !IN_SPECIAL_PARSE ){
     char *zDb = db->aDb[iDb].zDbSName;
-    if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
-      goto begin_table_error;
-    }
     pTable = sqlite3FindTable(db, zName, zDb);
     if( pTable ){
       if( !noErr ){
@@ -2670,21 +2676,34 @@ void sqlite3MarkAllShadowTablesOf(sqlite3 *db, Table *pTab){
 
 #ifndef SQLITE_OMIT_VIRTUALTABLE
 /*
-** Return true if zName is a shadow table name in the current database
-** connection.
+** Return true if zName is the name of a shadow table of a virtual table
+** in schema zDb.  If zDb is NULL, return true if zName is a shadow table
+** name of a virtual table in any schema of the database connection.
+**
+** Each schema is checked separately.  An ordinary table in one schema
+** (for example, a TEMP table) must not hide a same-named virtual table
+** in another schema.
 */
-int sqlite3ShadowTableName(sqlite3 *db, const char *zName){
+int sqlite3ShadowTableName(sqlite3 *db, const char *zName, const char *zDb){
   const char *zTail;            /* Pointer to the last "_" in zName */
   Table *pTab;                  /* Table that zName is a shadow of */
   char *zCopy;                  /* Transient copy of zName after last "_" */
+  int i;                        /* Loop counter over schemas */
+  int bRet = 0;                 /* Return value */
   zTail = strrchr(zName, '_');
   if( zTail==0 ) return 0;
   zCopy = sqlite3DbStrNDup(db, zName, (int)(zTail-zName));
-  pTab = zCopy ? sqlite3FindTable(db, zCopy, 0) : 0;
+  if( zCopy==0 ) return 0;
+  for(i=0; i<db->nDb && bRet==0; i++){
+    const char *zSchema = db->aDb[i].zDbSName;
+    if( zDb && sqlite3DbIsNamed(db, i, zDb)==0 ) continue;
+    pTab = sqlite3FindTable(db, zCopy, zSchema);
+    if( pTab && IsVirtual(pTab) ){
+      bRet = sqlite3IsShadowTableOf(db, pTab, zName);
+    }
+  }
   sqlite3DbFree(db, zCopy);
-  if( pTab==0 ) return 0;
-  if( !IsVirtual(pTab) ) return 0;
-  return sqlite3IsShadowTableOf(db, pTab, zName);
+  return bRet;
 }
 #endif /* ifndef SQLITE_OMIT_VIRTUALTABLE */
 
@@ -2753,7 +2772,10 @@ void sqlite3EndTable(
   p
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3BtreeHoldsAllMutexes`
- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3DbStrNDup`
- 外部函数：`sqlite3Dequote`
- 外部函数：`sqlite3ErrorMsg`
- 外部函数：`sqlite3HashFind`
- 外部函数：`sqlite3ReadSchema`
- 外部函数：`sqlite3RenameTokenMap`
- 外部函数：`sqlite3SchemaMutexHeld`
- 外部函数：`sqlite3StrICmp`
- 外部函数：`sqlite3StrNICmp`
- 外部函数：`sqlite3Strlen30`
- 外部函数：`sqlite3VtabInSync`
- 外部函数：`sqlite3_strnicmp`
- 外部函数：`strrchr`
- 外部函数：`testcase`
- 外部函数：`xShadowName`
- 大写宏：`IN_RENAME_OBJECT`
- 大写宏：`IN_SPECIAL_PARSE`
- 大写宏：`LEGACY_SCHEMA_TABLE`
- 大写宏：`LEGACY_TEMP_SCHEMA_TABLE`
- 大写宏：`PREFERRED_SCHEMA_TABLE`
- 大写宏：`PREFERRED_TEMP_SCHEMA_TABLE`
- 大写宏：`SQL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_VIRTUALTABLE`
- 大写宏：`TEMP`
- 外部类型：`All`
- 外部类型：`Attached`
- 外部类型：`But`
- 外部类型：`Length`
- 外部类型：`Make`
- 外部类型：`Match`
- 外部类型：`Module`
- 外部类型：`Name`
- 外部类型：`No`
- 外部类型：`Parent`
- 外部类型：`Parse`
- 外部类型：`Parsing`
- 外部类型：`Pointer`
- 外部类型：`Return`
- 外部类型：`SQLITE_Defensive`
- 外部类型：`SQLITE_WriteSchema`
- 外部类型：`TF_Shadow`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`Token`
- 外部类型：`Transient`
- 外部类型：`Type`
- 外部类型：`When`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-dea5bb1f29` → 本草稿移入 `cases/defect/auto-sqlite-dea5bb1f29/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
