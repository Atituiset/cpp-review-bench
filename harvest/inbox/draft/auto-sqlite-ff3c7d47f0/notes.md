# auto-sqlite-ff3c7d47f0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#d246ad4b9c712d49a3a6920f3f96e269ee82fa56](https://github.com/sqlite/sqlite/commit/d246ad4b9c712d49a3a6920f3f96e269ee82fa56) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 65 |
| 编译错误数（gcc syntax-only） | 38（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #d246ad4b9c712d49a3a6920f3f96e269ee82fa56 (https://github.com/sqlite/sqlite/commit/d246ad4b9c712d49a3a6920f3f96e269ee82fa56)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1358；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT d246ad4b9c712d49a3a6920f3f96e269ee82fa56 The recover extension no longer drops the last column of a table with a :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -62,6 +62,12 @@ typedef struct RecoverColumn RecoverColumn;
 **   aCol[] is an array of nCol columns. In the order in which they appear 
 **   in the table.
 **
+** nField:
+**   The number of fields in each record of the table's b-tree. This may
+**   be less than nCol (VIRTUAL generated columns have no field) or, for
+**   a WITHOUT ROWID table with a column that appears more than once in
+**   the PRIMARY KEY, greater than nCol.
+**
 ** bIntkey:
 **   Set to true for intkey tables, false for WITHOUT ROWID.
 **
@@ -86,6 +92,7 @@ struct RecoverTable {
   u32 iRoot;                      /* Root page in original database */
   char *zTab;                     /* Name of table */
   int nCol;                       /* Number of columns in table */
+  int nField;                     /* Number of fields in each record */
   RecoverColumn *aCol;            /* Array of columns */
   int bIntkey;                    /* True for intkey, false for without rowid */
   int iRowidBind;                 /* If >0, bind rowid to INSERT here */
@@ -169,7 +176,7 @@ struct RecoverStateW1 {
   int nInsert;
 
   RecoverTable *pTab;             /* Table currently being written */
-  int nMax;                       /* Max column count in any schema table */
+  int nMax;                       /* Max field count in any schema table */
   sqlite3_value **apVal;          /* Array of nMax values */
   int nVal;                       /* Number of valid entries in apVal[] */
   int bHaveRowid;
@@ -1129,22 +1136,32 @@ static void recoverAddTable(
         csr += (n+1);
       }
 
+      pNew->nField = iField;
       pNew->pNext = p->pTblList;
       p->pTblList = pNew;
       pNew->bIntkey = 1;
     }
 
     recoverFinalize(p, pStmt);
 
+    /* If this is a WITHOUT ROWID table, the fields of each record are
+    ** ordered as reported by PRAGMA index_xinfo on the table. A column may
+    ** appear more than once if it is listed more than once in the PRIMARY
+    ** KEY (with different collation sequences). In that case use the first
+    ** field - the others are redundant copies.  */
     pStmt = recoverPreparePrintf(p, p->dbOut, "PRAGMA index_xinfo(%Q)", zName);
     while( pStmt && sqlite3_step(pStmt)==SQLITE_ROW ){
       int iField = sqlite3_column_int(pStmt, 0);
       int iCol = sqlite3_column_int(pStmt, 1);
 
-      assert( iCol<pNew->nCol );
-      pNew->aCol[iCol].iField = iField;
-
-      pNew->bIntkey = 0;
+      assert( iCol>=0 && iCol<pNew->nCol );
+      if( pNew->bIntkey ){
+        int ii;
+        for(ii=0; ii<pNew->nCol; ii++) pNew->aCol[ii].iField = -1;
+        pNew->bIntkey = 0;
+      }
+      if( pNew->aCol[iCol].iField<0 ) pNew->aCol[iCol].iField = iField;
+      pNew->nField = iField+1;
       iPk = -2;
     }
     recoverFinalize(p, pStmt);
@@ -1334,7 +1351,7 @@ static sqlite3_stmt *recoverInsertStmt(
 
   if( nField<=0 ) return 0;
 
-  assert( nField<=pTab->nCol );
+  assert( nField<=pTab->nField );
 
   zSql = recoverMPrintf(p, "INSERT OR IGNORE INTO %Q(", pTab->zTab);
 
@@ -1350,12 +1367,18 @@ static sqlite3_stmt *recoverInsertStmt(
     zSep = ", ";
   }
 
-  for(ii=0; ii<nField; ii++){
-    int eHidden = pTab->aCol[ii].eHidden;
-    if( eHidden!=RECOVER_EHIDDEN_VIRTUAL
-     && eHidden!=RECOVER_EHIDDEN_STORED
-    ){
-      assert( pTab->aCol[ii].iField>=0 && pTab->aCol[ii].iBind>=1 );
+  /* Include each column that has a bind parameter and either has a field
+  ** in the nField-field record or is the IPK.  This must match the set of
+  ** parameters bound by recoverWriteDataStep().  Note that nField counts
+  ** record fields, not table columns - VIRTUAL generated columns have no
+  ** field (iField<0), and the order of fields need not match the order of
+  ** columns (WITHOUT ROWID tables).  */
+  for(ii=0; ii<pTab->nCol; ii++){
+    RecoverColumn *pCol = &pTab->aCol[ii];
+    if( pCol->iBind>0 && (pCol->iField<nField || pCol->bIPK) ){
+      assert( pCol->iField>=0 );
+      assert( pCol->eHidden!=RECOVER_EHIDDEN_VIRTUAL )
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`db`
- 外部函数：`index_xinfo`
- 外部函数：`int`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_db_handle`
- 外部函数：`sqlite3_errcode`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_finalize`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_recover_config`
- 外部函数：`sqlite3_recover_errcode`
- 外部函数：`sqlite3_recover_errmsg`
- 外部函数：`sqlite3_recover_run`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`table`
- 外部函数：`use`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 大写宏：`IGNORE`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`NULL`
- 大写宏：`PRAGMA`
- 大写宏：`RECOVER_STATE_LOSTANDFOUND`
- 大写宏：`RECOVER_STATE_WRITING`
- 大写宏：`ROWID`
- 大写宏：`SELECT`
- 大写宏：`SQL`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_RECOVER_FREELIST_CORRUPT`
- 大写宏：`SQLITE_RECOVER_ROWIDS`
- 大写宏：`SQLITE_RECOVER_SLOWINDEXES`
- 大写宏：`SQLITE_ROW`
- 大写宏：`URI`
- 大写宏：`WITHOUT`
- 外部类型：`Array`
- 外部类型：`Copies`
- 外部类型：`Error`
- 外部类型：`Fields`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Input`
- 外部类型：`List`
- 外部类型：`Max`
- 外部类型：`Name`
- 外部类型：`Number`
- 外部类型：`Output`
- 外部类型：`Pointer`
- 外部类型：`RecoverColumn`
- 外部类型：`RecoverStateLAF`
- 外部类型：`RecoverStateW1`
- 外部类型：`RecoverTable`
- 外部类型：`Root`
- 外部类型：`Set`
- 外部类型：`State`
- 外部类型：`Stored`
- 外部类型：`Table`
- 外部类型：`True`
- 外部类型：`Values`
- 外部类型：`Variables`
- 外部类型：`Virtual`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-ff3c7d47f0` → 本草稿移入 `cases/defect/auto-sqlite-ff3c7d47f0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
