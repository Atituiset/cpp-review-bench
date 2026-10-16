# auto-sqlite-4ffc0baf73

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#e4facb4585c7ef4d1e314e1314b81512408aeab1](https://github.com/sqlite/sqlite/commit/e4facb4585c7ef4d1e314e1314b81512408aeab1) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 142 |
| 编译错误数（gcc syntax-only） | 34（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e4facb4585c7ef4d1e314e1314b81512408aeab1 (https://github.com/sqlite/sqlite/commit/e4facb4585c7ef4d1e314e1314b81512408aeab1)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2096；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e4facb4585c7ef4d1e314e1314b81512408aeab1 When a single loop uses an IN(...) expression and a LIKE/GLOB expression with an index, nest the 2-iteration loop that s :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2093,9 +2093,6 @@ Bitmask sqlite3WhereCodeOneLoopStart(
         assert( pRangeStart!=0 );                     /* LIKE opt constraints */
         assert( pRangeStart->wtFlags & TERM_LIKEOPT );   /* occur in pairs */
         pLevel->iLikeRepCntr = (u32)++pParse->nMem;
-        sqlite3VdbeAddOp2(v, OP_Integer, 1, (int)pLevel->iLikeRepCntr);
-        VdbeComment((v, "LIKE loop counter"));
-        pLevel->addrLikeRep = sqlite3VdbeCurrentAddr(v);
         /* iLikeRepCntr actually stores 2x the counter register number.  The
         ** bottom bit indicates whether the search order is ASC or DESC. */
         testcase( bRev );
@@ -2159,6 +2156,20 @@ Bitmask sqlite3WhereCodeOneLoopStart(
     codeCursorHint(pTabItem, pWInfo, pLevel, pRangeEnd);
     regBase = codeAllEqualityTerms(pParse,pLevel,bRev,nExtraReg,&zStartAff);
     assert( zStartAff==0 || sqlite3Strlen30(zStartAff)>=nEq );
+#ifndef SQLITE_LIKE_DOESNT_MATCH_BLOBS
+    if( pLevel->iLikeRepCntr ){
+      /* The LIKE optimization loop must be the innermost of any IN(...)
+      ** or skip-scan loops. Otherwise rows may not be visited in index 
+      ** order. This means it needs an addrNxt label for the same reason
+      ** IN(...) scans do.  */
+      if( pLevel->addrNxt==pLevel->addrBrk ){
+        pLevel->addrNxt = sqlite3VdbeMakeLabel(pParse);
+      }
+      sqlite3VdbeAddOp2(v, OP_Integer, 1, (int)(pLevel->iLikeRepCntr>>1));
+      VdbeComment((v, "LIKE loop counter"));
+      pLevel->addrLikeRep = sqlite3VdbeCurrentAddr(v);
+    }
+#endif
     if( zStartAff && nTop ){
       zEndAff = sqlite3DbStrDup(db, &zStartAff[nEq]);
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`and`
- 外部函数：`assert`
- 外部函数：`be`
- 外部函数：`codeCursorHint`
- 外部函数：`order`
- 外部函数：`printf`
- 外部函数：`sqlite3CompareAffinity`
- 外部函数：`sqlite3DbFree`
- 外部函数：`sqlite3DbMallocZero`
- 外部函数：`sqlite3DbStrDup`
- 外部函数：`sqlite3DebugPrintf`
- 外部函数：`sqlite3ExprCanBeNull`
- 外部函数：`sqlite3ExprCodeTarget`
- 外部函数：`sqlite3ExprDelete`
- 外部函数：`sqlite3ExprDup`
- 外部函数：`sqlite3ExprListAppend`
- 外部函数：`sqlite3ExprListDelete`
- 外部函数：`sqlite3ExprNeedsNoAffinityChange`
- 外部函数：`sqlite3FindInIndex`
- 外部函数：`sqlite3IndexAffinityStr`
- 外部函数：`sqlite3Strlen30`
- 外部函数：`sqlite3TreeViewExpr`
- 外部函数：`sqlite3VdbeAddOp0`
- 外部函数：`sqlite3VdbeAddOp1`
- 外部函数：`sqlite3VdbeAddOp2`
- 外部函数：`sqlite3VdbeAddOp3`
- 外部函数：`sqlite3VdbeAddOp4Int`
- 外部函数：`sqlite3VdbeCurrentAddr`
- 外部函数：`sqlite3VdbeJumpHere`
- 外部函数：`sqlite3VdbeMakeLabel`
- 外部函数：`sqlite3WhereRealloc`
- 外部函数：`sqlite3WhereTermPrint`
- 外部函数：`strlen`
- 外部函数：`t1`
- 外部函数：`testcase`
- 外部函数：`that`
- 大写宏：`AND`
- 大写宏：`ASC`
- 大写宏：`CPU`
- 大写宏：`CREATE`
- 大写宏：`DESC`
- 大写宏：`DISABLE`
- 大写宏：`FROM`
- 大写宏：`GROUP`
- 大写宏：`INDEX`
- 大写宏：`IN_INDEX_INDEX_DESC`
- 大写宏：`IN_INDEX_LOOP`
- 大写宏：`IN_INDEX_NOOP`
- 大写宏：`IN_INDEX_ROWID`
- 大写宏：`LHS`
- 大写宏：`LIKE`
- 大写宏：`NEVER`
- 大写宏：`ORDER`
- 大写宏：`OUT`
- 大写宏：`RHS`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_AFF_BLOB`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_OMIT_SUBQUERY`
- 大写宏：`TERM_CODED`
- 大写宏：`TERM_IS`
- 大写宏：`TERM_LIKE`
- 大写宏：`TERM_LIKECOND`
- 大写宏：`TERM_LIKEOPT`
- 大写宏：`TERM_VIRTUAL`
- 大写宏：`TK_EQ`
- 大写宏：`TK_IN`
- 大写宏：`TK_IS`
- 大写宏：`TK_ISNULL`
- 大写宏：`TK_VECTOR`
- 大写宏：`WHERE`
- 大写宏：`WHERETRACE_ENABLED`
- 大写宏：`WHERE_IN_ABLE`
- 大写宏：`WHERE_IN_EARLYOUT`
- 大写宏：`WHERE_IN_SEEKSCAN`
- 大写宏：`WHERE_MULTI_OR`
- 大写宏：`WHERE_TRANSCONS`
- 大写宏：`WHERE_VIRTUALTABLE`
- 大写宏：`WO_AND`
- 大写宏：`WO_EQUIV`
- 大写宏：`WO_IN`
- 大写宏：`WO_ISNULL`
- 大写宏：`WO_OR`
- 大写宏：`XN_EXPR`
- 大写宏：`XN_ROWID`
- 外部类型：`Affinity`
- 外部类型：`As`
- 外部类型：`Attempt`
- 外部类型：`Base`
- 外部类型：`But`
- 外部类型：`Duplicate`
- 外部类型：`EP_OuterON`
- 外部类型：`EP_Subrtn`
- 外部类型：`EP_xIsSelect`
- 外部类型：`Evaluate`
- 外部类型：`Ex`
- 外部类型：`Expr`
- 外部类型：`ExprList`
- 外部类型：`Figure`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`InLoop`
- 外部类型：`Index`
- 外部类型：`Into`
- 外部类型：`Look`
- 外部类型：`Loop`
- 外部类型：`New`
- 外部类型：`No`
- 外部类型：`Number`
- 外部类型：`OP_Column`
- 外部类型：`OP_Copy`
- 外部类型：`OP_Goto`
- 外部类型：`OP_Integer`
- 外部类型：`OP_IsNull`
- 外部类型：`OP_Last`
- 外部类型：`OP_Next`
- 外部类型：`OP_Noop`
- 外部类型：`OP_Null`
- 外部类型：`OP_Prev`
- 外部类型：`OP_Rewind`
- 外部类型：`OP_Rowid`
- 外部类型：`OP_SeekGT`
- 外部类型：`OP_SeekHit`
- 外部类型：`OP_SeekLT`
- 外部类型：`Original`
- 外部类型：`Parse`
- 外部类型：`Parsing`
- 外部类型：`Pointer`
- 外部类型：`Register`
- 外部类型：`Req`
- 外部类型：`Reverse`
- 外部类型：`Select`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`SubrtnSig`
- 外部类型：`Take`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Vdbe`
- 外部类型：`WhereLevel`
- 外部类型：`WhereLoop`
- 外部类型：`WhereTerm`
- 外部类型：`Which`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-4ffc0baf73` → 本草稿移入 `cases/defect/auto-sqlite-4ffc0baf73/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
