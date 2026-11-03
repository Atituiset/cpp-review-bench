# auto-sqlite-ceb020fcd8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#7b667bbf9fadc419b56b953dbe066afe534e2646](https://github.com/sqlite/sqlite/commit/7b667bbf9fadc419b56b953dbe066afe534e2646) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 46 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7b667bbf9fadc419b56b953dbe066afe534e2646 (https://github.com/sqlite/sqlite/commit/7b667bbf9fadc419b56b953dbe066afe534e2646)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2468；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7b667bbf9fadc419b56b953dbe066afe534e2646 Return SQLITE_LOCKED if an ALTER TABLE DROP COLUMN statement is attempted when there are other statements belonging to t :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2422,71 +2422,74 @@ void sqlite3AlterDropColumn(Parse *pParse, SrcList *pSrc, const Token *pName){
   renameTestSchema(pParse, zDb, iDb==1, "after drop column", 1);
 
   /* Edit rows of table on disk */
-  if( pParse->nErr==0 && (pTab->aCol[iCol].colFlags & COLFLAG_VIRTUAL)==0 ){
-    int i;
-    int addr;
-    int reg;
-    int regRec;
-    Index *pPk = 0;
-    int nField = 0;               /* Number of non-virtual columns after drop */
-    int iCur;
+  if( pParse->nErr==0 ){
     Vdbe *v = sqlite3GetVdbe(pParse);
-    iCur = pParse->nTab++;
-    sqlite3OpenTable(pParse, iCur, iDb, pTab, OP_OpenWrite);
-    addr = sqlite3VdbeAddOp1(v, OP_Rewind, iCur); VdbeCoverage(v);
-    reg = ++pParse->nMem;
-    if( HasRowid(pTab) ){
-      sqlite3VdbeAddOp2(v, OP_Rowid, iCur, reg);
-      pParse->nMem += pTab->nCol;
-    }else{
-      pPk = sqlite3PrimaryKeyIndex(pTab);
-      pParse->nMem += pPk->nColumn;
-      for(i=0; i<pPk->nKeyCol; i++){
-        sqlite3VdbeAddOp3(v, OP_Column, iCur, i, reg+i+1);
-      }
-      nField = pPk->nKeyCol;
-    }
-    regRec = ++pParse->nMem;
-    for(i=0; i<pTab->nCol; i++){
-      if( i!=iCol && (pTab->aCol[i].colFlags & COLFLAG_VIRTUAL)==0 ){
-        int regOut;
-        if( pPk ){
-          int iPos = sqlite3TableColumnToIndex(pPk, i);
-          int iColPos = sqlite3TableColumnToIndex(pPk, iCol);
-          if( iPos<pPk->nKeyCol ) continue;
-          regOut = reg+1+iPos-(iPos>iColPos);
-        }else{
-          regOut = reg+1+nField;
+    sqlite3VdbeAddOp0(v, OP_DbLock);
+    if( (pTab->aCol[iCol].colFlags & COLFLAG_VIRTUAL)==0 ){
+      int i;
+      int addr;
+      int reg;
+      int regRec;
+      Index *pPk = 0;
+      int nField = 0;             /* Number of non-virtual columns after drop */
+      int iCur;
+      iCur = pParse->nTab++;
+      sqlite3OpenTable(pParse, iCur, iDb, pTab, OP_OpenWrite);
+      addr = sqlite3VdbeAddOp1(v, OP_Rewind, iCur); VdbeCoverage(v);
+      reg = ++pParse->nMem;
+      if( HasRowid(pTab) ){
+        sqlite3VdbeAddOp2(v, OP_Rowid, iCur, reg);
+        pParse->nMem += pTab->nCol;
+      }else{
+        pPk = sqlite3PrimaryKeyIndex(pTab);
+        pParse->nMem += pPk->nColumn;
+        for(i=0; i<pPk->nKeyCol; i++){
+          sqlite3VdbeAddOp3(v, OP_Column, iCur, i, reg+i+1);
         }
-        if( i==pTab->iPKey ){
-          sqlite3VdbeAddOp2(v, OP_Null, 0, regOut);
-        }else{
-          sqlite3ExprCodeGetColumnOfTable(v, pTab, iCur, i, regOut);
-          assert( pParse->nErr
-               || sqlite3VdbeGetOp(v,sqlite3VdbeCurrentAddr(v)-1)->opcode
-                    != OP_RealAffinity
-               || pTab->aCol[i].affinity==SQLITE_AFF_REAL );
-          sqlite3VdbeDeletePriorOpcode(v, OP_RealAffinity);
+        nField = pPk->nKeyCol;
+      }
+      regRec = ++pParse->nMem;
+      for(i=0; i<pTab->nCol; i++){
+        if( i!=iCol && (pTab->aCol[i].colFlags & COLFLAG_VIRTUAL)==0 ){
+          int regOut;
+          if( pPk ){
+            int iPos = sqlite3TableColumnToIndex(pPk, i);
+            int iColPos = sqlite3TableColumnToIndex(pPk, iCol);
+            if( iPos<pPk->nKeyCol ) continue;
+            regOut = reg+1+iPos-(iPos>iColPos);
+          }else{
+            regOut = reg+1+nField;
+          }
+          if( i==pTab->iPKey ){
+            sqlite3VdbeAddOp2(v, OP_Null, 0, regOut);
+          }else{
+            sqlite3ExprCodeGetColumnOfTable(v, pTab, iCur, i, regOut);
+            assert( pParse->nErr
+                 || sqlite3VdbeGetOp(v,sqlite3VdbeCurrentAddr(v)-1)->opcode
+                      != OP_RealAffinity
+                 || pTab->aCol[i].affinity==SQLITE_AFF_REAL );
+            sqlite3VdbeDeletePriorOpcode(v, OP_RealAffinity);
+          }
+          nField++;
         }
-        nField++;
       }
+      if( nField==0 ){
+        /* dbsqlfuzz 5f09e7bcc78b4954d06bf9f2400d7715f48d1fef */
+        pParse->nMem++;
+        sqlite3VdbeAddOp2(v, OP_Null, 0, reg+1);
+        nField = 1;
+      }
+      sqlite3
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3ExprCodeGetColumnOfTable`
- 外部函数：`sqlite3GetVdbe`
- 外部函数：`sqlite3NestedParse`
- 外部函数：`sqlite3OpenTable`
- 外部函数：`sqlite3PrimaryKeyIndex`
- 外部函数：`sqlite3TableColumnToIndex`
- 外部函数：`sqlite3VdbeAddOp1`
- 外部函数：`sqlite3VdbeAddOp2`
- 外部函数：`sqlite3VdbeAddOp3`
- 外部函数：`sqlite3VdbeAddOp4Int`
- 外部函数：`sqlite3VdbeChangeP5`
- 外部函数：`sqlite3VdbeCurrentAddr`
- 外部函数：`sqlite3VdbeDeletePriorOpcode`
- 外部函数：`sqlite3VdbeGetOp`
- 外部函数：`sqlite3VdbeJumpHere`
- 外部函数：`sqlite_rename_test`
- 大写宏：`AND`
- 大写宏：`COLFLAG_VIRTUAL`
- 大写宏：`DQS`
- 大写宏：`ESCAPE`
- 大写宏：`FROM`
- 大写宏：`LEGACY_SCHEMA_TABLE`
- 大写宏：`LIKE`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`OPFLAG_SAVEPOSITION`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_AFF_REAL`
- 大写宏：`WHERE`
- 外部类型：`Do`
- 外部类型：`Edit`
- 外部类型：`Index`
- 外部类型：`Name`
- 外部类型：`Number`
- 外部类型：`OP_Column`
- 外部类型：`OP_IdxInsert`
- 外部类型：`OP_Insert`
- 外部类型：`OP_MakeRecord`
- 外部类型：`OP_Next`
- 外部类型：`OP_Null`
- 外部类型：`OP_OpenWrite`
- 外部类型：`OP_RealAffinity`
- 外部类型：`OP_Rewind`
- 外部类型：`OP_Rowid`
- 外部类型：`Parse`
- 外部类型：`True`
- 外部类型：`Vdbe`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-ceb020fcd8` → 本草稿移入 `cases/defect/auto-sqlite-ceb020fcd8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
