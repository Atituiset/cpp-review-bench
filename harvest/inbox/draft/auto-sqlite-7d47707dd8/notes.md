# auto-sqlite-7d47707dd8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#13f31dd10a22dadf69f99c0dc8e6a94858214a52](https://github.com/sqlite/sqlite/commit/13f31dd10a22dadf69f99c0dc8e6a94858214a52) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 35 |
| 编译错误数（gcc syntax-only） | 63（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #13f31dd10a22dadf69f99c0dc8e6a94858214a52 (https://github.com/sqlite/sqlite/commit/13f31dd10a22dadf69f99c0dc8e6a94858214a52)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1868；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 13f31dd10a22dadf69f99c0dc8e6a94858214a52 Fix integrity-check problem causing an invalid corruption report on a table with (a) a VIRTUAL column, (b) an IPK to the :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -381,13 +381,22 @@ static void pragmaFunclistLine(
 ** Generate code to output a single-column result row with a value of the
 ** string held in register 3.  Decrement the result count in register 1
 ** and halt if the maximum number of result rows have been issued.
+**
+** If iCol is not negative, then after the result row has been output,
+** also load column iCol of cursor iCur into register 3. This is to ensure
+** the cursor's column cache is valid after it is invalidated by the
+** OP_ResultRow.
 */
-static int integrityCheckResultRow(Vdbe *v){
+static int integrityCheckResultRow(Vdbe *v, int iCur, int iCol){
   int addr;
   sqlite3VdbeAddOp2(v, OP_ResultRow, 3, 1);
   addr = sqlite3VdbeAddOp3(v, OP_IfPos, 1, sqlite3VdbeCurrentAddr(v)+2, 1);
   VdbeCoverage(v);
   sqlite3VdbeAddOp0(v, OP_Halt);
+  if( iCol>=0 ){
+    sqlite3VdbeAddOp3(v, OP_Column, iCur, iCol, 3);
+    sqlite3VdbeTypeofColumn(v, 3);
+  }
   return addr;
 }
 
@@ -1791,7 +1800,7 @@ void sqlite3Pragma(
          sqlite3MPrintf(db, "*** in database %s ***\n", db->aDb[i].zDbSName),
          P4_DYNAMIC);
       sqlite3VdbeAddOp3(v, OP_Concat, 2, 3, 3);
-      integrityCheckResultRow(v);
+      integrityCheckResultRow(v, 0, -1);
       sqlite3VdbeJumpHere(v, addr);
 
       /* Check that the indexes all have the right number of rows */
@@ -1817,7 +1826,7 @@ void sqlite3Pragma(
             VdbeCoverageNeverNull(v);
             sqlite3VdbeLoadString(v, 4, pIdx->zName);
             sqlite3VdbeAddOp3(v, OP_Concat, 4, 2, 3);
-            integrityCheckResultRow(v);
+            integrityCheckResultRow(v, 0, -1);
             sqlite3VdbeJumpHere(v, addr);
           }
           cnt++;
@@ -1863,11 +1872,7 @@ void sqlite3Pragma(
         */
         assert( !IsVirtual(pTab) );
         if( HasRowid(pTab) ){
-          mxCol = -1;
-          for(j=0; j<pTab->nCol; j++){
-            if( (pTab->aCol[j].colFlags & COLFLAG_VIRTUAL)==0 ) mxCol++;
-          }
-          if( mxCol==pTab->iPKey ) mxCol--;
+          mxCol = pTab->nNVCol-1;
         }else{
           /* COLFLAG_VIRTUAL columns are not included in the WITHOUT ROWID
           ** PK index column-count, so there is no need to account for them
@@ -1891,7 +1896,7 @@ void sqlite3Pragma(
                    "row not in PRIMARY KEY order for %s",
                     pTab->zName);
             sqlite3VdbeAddOp4(v, OP_String8, 0, 3, 0, zErr, P4_DYNAMIC);
-            integrityCheckResultRow(v);
+            integrityCheckResultRow(v, iDataCur, mxCol);
             sqlite3VdbeJumpHere(v, a1);
             sqlite3VdbeJumpHere(v, a1+1);
             for(j=0; j<pPk->nKeyCol; j++){
@@ -2028,7 +2033,7 @@ void sqlite3Pragma(
             sqlite3VdbeAddOp4(v, OP_String8, 0, 3, 0, zErr, P4_DYNAMIC);
           }
           sqlite3VdbeResolveLabel(v, labelError);
-          integrityCheckResultRow(v);
+          integrityCheckResultRow(v, iDataCur, mxCol);
           sqlite3VdbeResolveLabel(v, labelOk);
         }
         /* Verify CHECK constraints */
@@ -2050,7 +2055,7 @@ void sqlite3Pragma(
             zErr = sqlite3MPrintf(db, "CHECK constraint failed in %s",
                 pTab->zName);
             sqlite3VdbeAddOp4(v, OP_String8, 0, 3, 0, zErr, P4_DYNAMIC);
-            integrityCheckResultRow(v);
+            integrityCheckResultRow(v, 0, -1);
             sqlite3VdbeResolveLabel(v, addrCkOk);
           }
           sqlite3ExprListDelete(db, pCheck);
@@ -2076,7 +2081,7 @@ void sqlite3Pragma(
                                  "value for row ", pIdx->zName),
               P4_DYNAMIC);
             sqlite3VdbeAddOp3(v, OP_Concat, 7, 3, 3);
-            integrityCheckResultRow(v);
+            integrityCheckResultRow(v, 0, -1);
             sqlite3VdbeAddOp2(v, OP_Goto, 0, ckUniq);
 
             sqlite3VdbeJumpHere(v, jmp2);
@@ -2086,7 +2091,7 @@ void sqlite3Pragma(
             sqlite3VdbeAddOp3(v, OP_Concat, 4, 3, 3);
             jmp5 = sqlite3VdbeLoadString(v, 4, pIdx->zName);
             sqlite3VdbeAddOp
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3ExprListDelete`
- 外部函数：`sqlite3MPrintf`
- 外部函数：`sqlite3VdbeAddOp0`
- 外部函数：`sqlite3VdbeAddOp1`
- 外部函数：`sqlite3VdbeAddOp2`
- 外部函数：`sqlite3VdbeAddOp3`
- 外部函数：`sqlite3VdbeAddOp4`
- 外部函数：`sqlite3VdbeAppendP4`
- 外部函数：`sqlite3VdbeCurrentAddr`
- 外部函数：`sqlite3VdbeJumpHere`
- 外部函数：`sqlite3VdbeLoadString`
- 外部函数：`sqlite3VdbeResolveLabel`
- 大写宏：`CHECK`
- 大写宏：`COLFLAG_VIRTUAL`
- 大写宏：`KEY`
- 大写宏：`P4_DYNAMIC`
- 大写宏：`P4_TABLEREF`
- 大写宏：`PRIMARY`
- 大写宏：`ROWID`
- 大写宏：`WITHOUT`
- 外部类型：`Check`
- 外部类型：`Decrement`
- 外部类型：`Generate`
- 外部类型：`OP_Column`
- 外部类型：`OP_Concat`
- 外部类型：`OP_Goto`
- 外部类型：`OP_Halt`
- 外部类型：`OP_IdxRowid`
- 外部类型：`OP_IfPos`
- 外部类型：`OP_IsNull`
- 外部类型：`OP_ResultRow`
- 外部类型：`OP_String8`
- 外部类型：`The`
- 外部类型：`Vdbe`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-7d47707dd8` → 本草稿移入 `cases/defect/auto-sqlite-7d47707dd8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
