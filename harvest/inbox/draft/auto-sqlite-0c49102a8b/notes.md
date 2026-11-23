# auto-sqlite-0c49102a8b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#248b8a33f4fbacaecba2990a23ecb0a170ec342e](https://github.com/sqlite/sqlite/commit/248b8a33f4fbacaecba2990a23ecb0a170ec342e) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #248b8a33f4fbacaecba2990a23ecb0a170ec342e (https://github.com/sqlite/sqlite/commit/248b8a33f4fbacaecba2990a23ecb0a170ec342e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 296；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 248b8a33f4fbacaecba2990a23ecb0a170ec342e Fix sort order for the fts5vocab virtual table in UTF16. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -51,6 +51,7 @@ struct Fts5VocabTable {
   Fts5Global *pGlobal;            /* FTS5 global object for this database */
   int eType;                      /* FTS5_VOCAB_COL, ROW or INSTANCE */
   unsigned bBusy;                 /* True if busy */
+  int bUtf8;                      /* True if the database encoding is UTF-8 */
 };
 
 struct Fts5VocabCursor {
@@ -99,6 +100,27 @@ struct Fts5VocabCursor {
 #define FTS5_VOCAB_COLUSED_MASK 0xFF
 
 
+/*
+** Return true if the main database encoding of db is known to be UTF-8.
+** Return false if it is some other encoding or if it cannot be determined.
+**
+** Terms are stored in the FTS5 index in UTF-8 and are visited in memcmp()
+** order of that representation.  That is the same as the order of the
+** BINARY collation only if the database encoding is UTF-8.
+*/
+static int fts5VocabIsUtf8(sqlite3 *db){
+  sqlite3_stmt *pStmt = 0;
+  int bRet = 0;
+  if( sqlite3_prepare_v2(db, "PRAGMA encoding", -1, &pStmt, 0)==SQLITE_OK ){
+    if( sqlite3_step(pStmt)==SQLITE_ROW ){
+      const char *z = (const char*)sqlite3_column_text(pStmt, 0);
+      bRet = (z && sqlite3_stricmp(z, "UTF-8")==0);
+    }
+  }
+  sqlite3_finalize(pStmt);
+  return bRet;
+}
+
 /*
 ** Translate a string containing an fts5vocab table type to an 
 ** FTS5_VOCAB_XXX constant. If successful, set *peType to the output
@@ -214,6 +236,7 @@ static int fts5VocabInitVtab(
       pRet->pGlobal = (Fts5Global*)pAux;
       pRet->eType = eType;
       pRet->db = db;
+      pRet->bUtf8 = fts5VocabIsUtf8(db);
       pRet->zFts5Tbl = (char*)&pRet[1];
       pRet->zFts5Db = &pRet->zFts5Tbl[nTab];
       memcpy(pRet->zFts5Tbl, zTab, nTab);
@@ -274,29 +297,30 @@ static int fts5VocabIsBinary(sqlite3_index_info *pInfo, int iCons){
 ** identically, as are greater-than and greater-than-or-equal.
 */
 static int fts5VocabBestIndexMethod(
-  sqlite3_vtab *pUnused,
+  sqlite3_vtab *pVTab,
   sqlite3_index_info *pInfo
 ){
+  Fts5VocabTable *pTab = (Fts5VocabTable*)pVTab;
   int i;
   int iTermEq = -1;
   int iTermGe = -1;
   int iTermLe = -1;
   int idxNum = (int)pInfo->colUsed;
   int nArg = 0;
 
-  UNUSED_PARAM(pUnused);
-
   assert( (pInfo->colUsed & FTS5_VOCAB_COLUSED_MASK)==pInfo->colUsed );
 
   for(i=0; i<pInfo->nConstraint; i++){
     struct sqlite3_index_constraint *p = &pInfo->aConstraint[i];
     if( p->usable==0 ) continue;
     if( p->iColumn==0 && fts5VocabIsBinary(pInfo, i) ){    /* term column */
       if( p->op==SQLITE_INDEX_CONSTRAINT_EQ ) iTermEq = i;
-      if( p->op==SQLITE_INDEX_CONSTRAINT_LE ) iTermLe = i;
-      if( p->op==SQLITE_INDEX_CONSTRAINT_LT ) iTermLe = i;
-      if( p->op==SQLITE_INDEX_CONSTRAINT_GE ) iTermGe = i;
-      if( p->op==SQLITE_INDEX_CONSTRAINT_GT ) iTermGe = i;
+      if( pTab->bUtf8 ){
+        if( p->op==SQLITE_INDEX_CONSTRAINT_LE ) iTermLe = i;
+        if( p->op==SQLITE_INDEX_CONSTRAINT_LT ) iTermLe = i;
+        if( p->op==SQLITE_INDEX_CONSTRAINT_GE ) iTermGe = i;
+        if( p->op==SQLITE_INDEX_CONSTRAINT_GT ) iTermGe = i;
+      }
     }
   }
 
@@ -322,8 +346,11 @@ static int fts5VocabBestIndexMethod(
   ** the "term" column (column 0). So if the user has requested this
   ** specifically - "ORDER BY term" or "ORDER BY term ASC" - set the
   ** sqlite3_index_info.orderByConsumed flag to tell the core the results
-  ** are already in sorted order.  */
-  if( pInfo->nOrderBy==1 
+  ** are already in sorted order.  This is only true for BINARY order
+  ** if the database encoding is UTF-8.  Otherwise, range constraints
+  ** and sorting are left to the SQLite core.  */
+  if( pTab->bUtf8
+   && pInfo->nOrderBy==1 
    && pInfo->aOrderBy[0].iColumn==0 
    && pInfo->aOrderBy[0].desc==0
   ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memcpy`
- 外部函数：`sqlite3_stricmp`
- 外部函数：`sqlite3_vtab_collation`
- 大写宏：`ASC`
- 大写宏：`FTS5`
- 大写宏：`FTS5_VOCAB_COL`
- 大写宏：`FTS5_VOCAB_XXX`
- 大写宏：`INSTANCE`
- 大写宏：`ORDER`
- 大写宏：`ROW`
- 大写宏：`SQLITE_INDEX_CONSTRAINT_EQ`
- 大写宏：`SQLITE_INDEX_CONSTRAINT_GE`
- 大写宏：`SQLITE_INDEX_CONSTRAINT_GT`
- 大写宏：`SQLITE_INDEX_CONSTRAINT_LE`
- 大写宏：`SQLITE_INDEX_CONSTRAINT_LT`
- 大写宏：`UNUSED_PARAM`
- 外部类型：`Fts5Global`
- 外部类型：`Fts5VocabCursor`
- 外部类型：`If`
- 外部类型：`So`
- 外部类型：`Translate`
- 外部类型：`True`
- 外部类型：`sqlite3_index_constraint`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-0c49102a8b` → 本草稿移入 `cases/defect/auto-sqlite-0c49102a8b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
