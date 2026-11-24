# auto-sqlite-e3c2d38ae9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#67a23f5ce761fcce65029c4d7d555614932e54b8](https://github.com/sqlite/sqlite/commit/67a23f5ce761fcce65029c4d7d555614932e54b8) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 26 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #67a23f5ce761fcce65029c4d7d555614932e54b8 (https://github.com/sqlite/sqlite/commit/67a23f5ce761fcce65029c4d7d555614932e54b8)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1912；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 67a23f5ce761fcce65029c4d7d555614932e54b8 Fix PRAGMA quick_check so that it verifies PRIMARY KEY order on  :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1865,7 +1865,7 @@ void sqlite3Pragma(
           if( pIdx->pPartIdxWhere ) nPartIdx++;
         }
         if( !IsOrdinaryTable(pTab) ) continue;
-        if( isQuick || HasRowid(pTab) ){
+        if( HasRowid(pTab) ){
           pPk = 0;
           r2 = 0;
         }else{
@@ -1909,24 +1909,22 @@ void sqlite3Pragma(
         sqlite3VdbeAddOp3(v, OP_Column, iDataCur, mxCol, 3);
         sqlite3VdbeTypeofColumn(v, 3);
 
-        if( !isQuick ){
-          if( pPk ){
-            /* Verify WITHOUT ROWID keys are in ascending order */
-            int a1;
-            char *zErr;
-            a1 = sqlite3VdbeAddOp4Int(v, OP_IdxGT, iDataCur, 0,r2,pPk->nKeyCol);
-            VdbeCoverage(v);
-            sqlite3VdbeAddOp1(v, OP_IsNull, r2); VdbeCoverage(v);
-            zErr = sqlite3MPrintf(db,
-                   "row not in PRIMARY KEY order for %s",
-                    pTab->zName);
-            sqlite3VdbeAddOp4(v, OP_String8, 0, 3, 0, zErr, P4_DYNAMIC);
-            integrityCheckResultRow(v, iDataCur, mxCol);
-            sqlite3VdbeJumpHere(v, a1);
-            sqlite3VdbeJumpHere(v, a1+1);
-            for(j=0; j<pPk->nKeyCol; j++){
-              sqlite3ExprCodeLoadIndexColumn(pParse, pPk, iDataCur, j, r2+j);
-            }
+        if( pPk ){
+          /* Verify WITHOUT ROWID keys are in ascending order */
+          int a1;
+          char *zErr;
+          a1 = sqlite3VdbeAddOp4Int(v, OP_IdxGT, iDataCur, 0,r2,pPk->nKeyCol);
+          VdbeCoverage(v);
+          sqlite3VdbeAddOp1(v, OP_IsNull, r2); VdbeCoverage(v);
+          zErr = sqlite3MPrintf(db,
+                 "row not in PRIMARY KEY order for %s",
+                  pTab->zName);
+          sqlite3VdbeAddOp4(v, OP_String8, 0, 3, 0, zErr, P4_DYNAMIC);
+          integrityCheckResultRow(v, iDataCur, mxCol);
+          sqlite3VdbeJumpHere(v, a1);
+          sqlite3VdbeJumpHere(v, a1+1);
+          for(j=0; j<pPk->nKeyCol; j++){
+            sqlite3ExprCodeLoadIndexColumn(pParse, pPk, iDataCur, j, r2+j);
           }
         }
         /* Verify datatypes for all columns:
@@ -2210,7 +2208,6 @@ void sqlite3Pragma(
           sqlite3ReleaseTempRange(pParse, regPartCnt, nPartIdx);
         }
         if( pPk ){
-          assert( !isQuick );
           sqlite3ReleaseTempRange(pParse, r2, pPk->nKeyCol);
         }
       }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3ExprCodeLoadIndexColumn`
- 外部函数：`sqlite3MPrintf`
- 外部函数：`sqlite3ReleaseTempRange`
- 外部函数：`sqlite3VdbeAddOp0`
- 外部函数：`sqlite3VdbeAddOp1`
- 外部函数：`sqlite3VdbeAddOp2`
- 外部函数：`sqlite3VdbeAddOp3`
- 外部函数：`sqlite3VdbeAddOp4`
- 外部函数：`sqlite3VdbeAddOp4Int`
- 外部函数：`sqlite3VdbeCurrentAddr`
- 外部函数：`sqlite3VdbeJumpHere`
- 外部函数：`sqlite3VdbeTypeofColumn`
- 大写宏：`KEY`
- 大写宏：`P4_DYNAMIC`
- 大写宏：`PRIMARY`
- 大写宏：`ROWID`
- 大写宏：`WITHOUT`
- 外部类型：`OP_Column`
- 外部类型：`OP_Halt`
- 外部类型：`OP_IdxGT`
- 外部类型：`OP_IfPos`
- 外部类型：`OP_IsNull`
- 外部类型：`OP_ResultRow`
- 外部类型：`OP_String8`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-e3c2d38ae9` → 本草稿移入 `cases/defect/auto-sqlite-e3c2d38ae9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
