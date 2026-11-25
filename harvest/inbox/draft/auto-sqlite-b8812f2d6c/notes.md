# auto-sqlite-b8812f2d6c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#f852f389434dd9e7a6376dccb5e0001dac81bcf3](https://github.com/sqlite/sqlite/commit/f852f389434dd9e7a6376dccb5e0001dac81bcf3) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 83 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f852f389434dd9e7a6376dccb5e0001dac81bcf3 (https://github.com/sqlite/sqlite/commit/f852f389434dd9e7a6376dccb5e0001dac81bcf3)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1105；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f852f389434dd9e7a6376dccb5e0001dac81bcf3 Reject ALTER TABLE RENAME COLUMN requests if the column participates in a :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -809,6 +809,7 @@ struct RenameCtx {
   int iCol;                       /* Index of column being renamed */
   Table *pTab;                    /* Table being ALTERed */
   const char *zOld;               /* Old column name */
+  char *zErr;                     /* Error message, or NULL */
 };
 
 #ifdef SQLITE_DEBUG
@@ -1082,14 +1083,14 @@ static RenameToken *renameTokenFind(
 }
 
 /*
-** Add to pCtx the names in USING clauses of pSrc that refer to the column
-** being renamed.  A USING(x) term names a column in the table on its right
-** and in the first table on its left that has such a column.  The name can
-** only be renamed when both of those are the table being altered, as in a
-** self-join.  Otherwise, the names in the two tables would no longer match,
-** and the rename fails when the schema is tested afterwards.
+** Check the joins in pSrc for a USING clause or a NATURAL join that
+** involves the column being renamed.  Renaming such a column could change
+** the meaning of the join, so record an error message in pCtx->zErr.
+** A USING(x) term, and a NATURAL join, names a column in the table on its
+** right and in the first table on its left that has such a column.  The
+** column is involved if either of those tables is the one being altered.
 */
-static void renameColumnUsingNames(
+static void renameColumnJoinCheck(
   Parse *pParse,
   RenameCtx *pCtx,
   const SrcList *pSrc
@@ -1100,23 +1101,37 @@ static void renameColumnUsingNames(
   zOld = pCtx->pTab->aCol[pCtx->iCol].zCnName;
   for(i=1; i<pSrc->nSrc; i++){
     const SrcItem *pItem = &pSrc->a[i];
-    int j;
+    const IdList *pUsing;
+    Table *pLeft = 0;
+    int j, k;
     if( pItem->fg.isUsing==0 ) continue;
-    if( pItem->pSTab!=pCtx->pTab ) continue;
+    pUsing = pItem->u3.pUsing;
+    for(k=0; k<pUsing->nId; k++){
+      if( 0==sqlite3_stricmp(pUsing->a[k].zName, zOld) ) break;
+    }
+    if( k>=pUsing->nId ) continue;
     for(j=0; ALWAYS(j<i); j++){
-      Table *pLeft = pSrc->a[j].pSTab;
-      if( ALWAYS(pLeft) && sqlite3ColumnIndex(pLeft, zOld)>=0 ){
-        if( pLeft==pCtx->pTab ){
-          const IdList *pUsing = pItem->u3.pUsing;
-          int k;
-          for(k=0; k<pUsing->nId; k++){
-            if( 0==sqlite3_stricmp(pUsing->a[k].zName, zOld) ){
-              renameTokenFind(pParse, pCtx, (const void*)pUsing->a[k].zName);
-            }
-          }
-        }
-        break;
+      pLeft = pSrc->a[j].pSTab;
+      if( ALWAYS(pLeft) && sqlite3ColumnIndex(pLeft, zOld)>=0 ) break;
+    }
+    if( pItem->pSTab==pCtx->pTab || pLeft==pCtx->pTab ){
+      int isNat = pItem->fg.isSynthUsing;
+      const char *zPrep;
+      const char *zName;
+      if( pParse->pNewTable ){
+        zPrep = isNat ? "in VIEW" : "of VIEW";
+        zName = pParse->pNewTable->zName;
+      }else{
+        zPrep = "within TRIGGER";
+        zName = pParse->pNewTrigger->zName;
       }
+      sqlite3DbFree(pParse->db, pCtx->zErr);
+      pCtx->zErr = sqlite3MPrintf(pParse->db,
+          "cannot rename %s because it %s %s %s", zOld,
+          isNat ? "participates in a NATURAL JOIN"
+                : "appears in a USING clause",
+          zPrep, zName);
+      return;
     }
   }
 }
@@ -1132,7 +1147,7 @@ static int renameColumnSelectCb(Walker *pWalker, Select *p){
     testcase( p->selFlags & SF_CopyCte );
     return WRC_Prune;
   }
-  renameColumnUsingNames(pWalker->pParse, pWalker->u.pRename, p->pSrc);
+  renameColumnJoinCheck(pWalker->pParse, pWalker->u.pRename, p->pSrc);
   renameWalkWith(pWalker, p);
   return WRC_Continue;
 }
@@ -1847,6 +1862,10 @@ static void renameColumnFunc(
   }
 
   assert( rc==SQLITE_OK );
+  if( sCtx.zErr ){
+    sqlite3_result_error(context, sCtx.zErr, -1);
+    goto renameColumnFunc_cleanup;
+  }
   rc = renameEditSql(context, &sCtx, zSql, zNew, bQuote);
 
 renameColumnFunc_done:
@@ -1860,8 +1879,10 @@ static void renameColumnFunc(
     }
   }
 
+renameColumnFunc_cleanup:
   renameParseCleanup(&sParse);
   renameTokenF
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memset`
- 外部函数：`resolved`
- 外部函数：`sqlite3ColumnIndex`
- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3RenameTokenRemap`
- 外部函数：`sqlite3SelectPrep`
- 外部函数：`sqlite3Strlen30`
- 外部函数：`sqlite3WalkExprList`
- 外部函数：`sqlite3WalkSelect`
- 外部函数：`sqlite3WithDup`
- 外部函数：`sqlite3WithPush`
- 外部函数：`sqlite3_context_db_handle`
- 外部函数：`sqlite3_stricmp`
- 外部函数：`testcase`
- 大写宏：`ALWAYS`
- 大写宏：`ENAME_NAME`
- 大写宏：`NEVER`
- 大写宏：`SQL`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`USING`
- 外部类型：`ALTERed`
- 外部类型：`Add`
- 外部类型：`And`
- 外部类型：`Expr`
- 外部类型：`ExprList`
- 外部类型：`IdList`
- 外部类型：`Index`
- 外部类型：`NameContext`
- 外部类型：`New`
- 外部类型：`Next`
- 外部类型：`Old`
- 外部类型：`Otherwise`
- 外部类型：`Parse`
- 外部类型：`Push`
- 外部类型：`Rename`
- 外部类型：`RenameCtx`
- 外部类型：`RenameToken`
- 外部类型：`Return`
- 外部类型：`SF_CopyCte`
- 外部类型：`SF_Expanded`
- 外部类型：`SF_Resolved`
- 外部类型：`Select`
- 外部类型：`SrcItem`
- 外部类型：`SrcList`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`Token`
- 外部类型：`True`
- 外部类型：`WRC_Continue`
- 外部类型：`WRC_Prune`
- 外部类型：`Walker`
- 外部类型：`We`
- 外部类型：`With`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-b8812f2d6c` → 本草稿移入 `cases/defect/auto-sqlite-b8812f2d6c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
