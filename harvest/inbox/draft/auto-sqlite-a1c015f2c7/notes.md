# auto-sqlite-a1c015f2c7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#6abdfa68f496262d4c11e98f056e35284b57b5c3](https://github.com/sqlite/sqlite/commit/6abdfa68f496262d4c11e98f056e35284b57b5c3) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 10（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6abdfa68f496262d4c11e98f056e35284b57b5c3 (https://github.com/sqlite/sqlite/commit/6abdfa68f496262d4c11e98f056e35284b57b5c3)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6abdfa68f496262d4c11e98f056e35284b57b5c3 Enhance PRAGMA integrity check to find surplus entries in a partial index. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1835,6 +1835,7 @@ void sqlite3Pragma(
 
       /* Make sure all the indices are constructed correctly.
       */
+      cnt = pObjTab ? 1 : 0;
       for(x=sqliteHashFirst(pTbls); x; x=sqliteHashNext(x)){
         Table *pTab = sqliteHashData(x);
         Index *pIdx, *pPk;
@@ -1845,8 +1846,19 @@ void sqlite3Pragma(
         int bStrict;            /* True for a STRICT table */
         int r2;                 /* Previous key for WITHOUT ROWID tables */
         int mxCol;              /* Maximum non-virtual column number */
+        int regIdxCnt;          /* reg[regIdxCnt+j] is the size of index j */
+        int nPartIdx = 0;       /* Number of partial indexes */
+        int regPartCnt = 0;     /* Row counts for the partial indexes */
+        int iPart;              /* Loop counter over partial indexes */
 
         if( tableSkipIntegrityCheck(pTab,pObjTab) ) continue;
+        /* Count the indexes the same way as for aRoot[] above */
+        if( HasRowid(pTab) ) cnt++;
+        regIdxCnt = 8+cnt;
+        for(pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext){
+          cnt++;
+          if( pIdx->pPartIdxWhere ) nPartIdx++;
+        }
         if( !IsOrdinaryTable(pTab) ) continue;
         if( isQuick || HasRowid(pTab) ){
           pPk = 0;
@@ -1862,6 +1874,15 @@ void sqlite3Pragma(
         */
         sqlite3VdbeAddOp2(v, OP_Integer, 0, 7);
         assert( sqlite3NoTempsInRange(pParse,1,7) );
+        if( !isQuick && nPartIdx>0 ){
+          /* Count the rows that satisfy the WHERE clause of each partial
+          ** index.  After the loop, compare each count with the number of
+          ** entries in that index. */
+          regPartCnt = sqlite3GetTempRange(pParse, nPartIdx);
+          for(iPart=0; iPart<nPartIdx; iPart++){
+            sqlite3VdbeAddOp2(v, OP_Integer, 0, regPartCnt+iPart);
+          }
+        }
         sqlite3VdbeAddOp2(v, OP_Rewind, iDataCur, 0); VdbeCoverage(v);
         loopTop = sqlite3VdbeAddOp2(v, OP_AddImm, 7, 1);
 
@@ -2061,6 +2082,7 @@ void sqlite3Pragma(
         }
         if( !isQuick ){ /* Omit the remaining tests for quick_check */
           /* Validate index entries for the current row */
+          iPart = 0;
           for(j=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, j++){
             int jmp2, jmp3, jmp4, jmp5, label6;
             int kk;
@@ -2069,6 +2091,11 @@ void sqlite3Pragma(
             r1 = sqlite3GenerateIndexKey(pParse, pIdx, iDataCur, 0, 0, &jmp3,
                                          pPrior, r1);
             pPrior = pIdx;
+            if( pIdx->pPartIdxWhere ){
+              /* Count this row for the partial index */
+              sqlite3VdbeAddOp2(v, OP_AddImm, regPartCnt+iPart, 1);
+              iPart++;
+            }
             /* Verify that an index entry exists for the current table row */
             sqlite3VdbeAddOp4Int(v, OP_Found, iIdxCur+j, ckUniq, r1,
                                         pIdx->nColumn); VdbeCoverage(v);
@@ -2157,6 +2184,26 @@ void sqlite3Pragma(
         }
         sqlite3VdbeAddOp2(v, OP_Next, iDataCur, loopTop); VdbeCoverage(v);
         sqlite3VdbeJumpHere(v, loopTop-1);
+        if( regPartCnt ){
+          /* Report any partial index whose size differs from its row count.
+          ** The per-row checks above find missing entries, so this finds
+          ** the surplus ones. */
+          assert( !isQuick );
+          sqlite3VdbeLoadString(v, 2, "wrong # of entries in index ");
+          for(j=0, iPart=0, pIdx=pTab->pIndex; pIdx; pIdx=pIdx->pNext, j++){
+            if( pIdx->pPartIdxWhere==0 ) continue;
+            addr = sqlite3VdbeAddOp3(v, OP_Eq, regPartCnt+iPart, 0,
+                                     regIdxCnt+j);
+            VdbeCoverageNeverNull(v);
+            sqlite3VdbeLoadString(v, 4, pIdx->zName);
+            sqlite3VdbeAddOp3(v, OP_Concat, 4, 2, 3);
+            integrityCheckResultRow(v, 0, -1);
+            sqlite3VdbeJumpHere(v, addr);
+            iPart++;
+          }
+        
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3GenerateIndexKey`
- 外部函数：`sqlite3NoTempsInRange`
- 外部函数：`sqlite3ReleaseTempRange`
- 外部函数：`sqlite3VdbeAddOp2`
- 外部函数：`sqlite3VdbeAddOp4Int`
- 外部函数：`sqlite3VdbeJumpHere`
- 外部函数：`sqliteHashData`
- 外部函数：`sqliteHashFirst`
- 大写宏：`ROWID`
- 大写宏：`STRICT`
- 大写宏：`WITHOUT`
- 外部类型：`Index`
- 外部类型：`Make`
- 外部类型：`Maximum`
- 外部类型：`OP_AddImm`
- 外部类型：`OP_Found`
- 外部类型：`OP_Integer`
- 外部类型：`OP_Next`
- 外部类型：`OP_Rewind`
- 外部类型：`Omit`
- 外部类型：`Previous`
- 外部类型：`TF_Imposter`
- 外部类型：`Table`
- 外部类型：`True`
- 外部类型：`Validate`
- 外部类型：`Verify`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-a1c015f2c7` → 本草稿移入 `cases/defect/auto-sqlite-a1c015f2c7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
