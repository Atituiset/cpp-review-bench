# auto-sqlite-50c4ab1d53

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#b3b913b8db89055855103c6f0dd671b778c68dbb](https://github.com/sqlite/sqlite/commit/b3b913b8db89055855103c6f0dd671b778c68dbb) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 771 |
| 编译错误数（gcc syntax-only） | 193（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b3b913b8db89055855103c6f0dd671b778c68dbb (https://github.com/sqlite/sqlite/commit/b3b913b8db89055855103c6f0dd671b778c68dbb)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b3b913b8db89055855103c6f0dd671b778c68dbb Fixes to corner cases in the UNION-LIMIT optimization. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3045,6 +3045,17 @@ static int selectIsUnionAll(Select *p){
   return 1;
 }
 
+/*
+** Return TRUE if p is preceded by zero or more UNION operators, and
+** then zero or more UNION ALL operators.  If there are any EXCEPT
+** or INTERSECT operators to the left of p, or if any UNION operator
+** to the left of p comes before a UNION ALL operator, then return FALSE.
+*/
+static int selectIsUnionAndUnionAll(Select *p){
+  for(p=p->pPrior; p->pPrior && p->op==TK_UNION; p=p->pPrior){}
+  return selectIsUnionAll(p);
+}
+
 /*
 ** Return TRUE if p is a UNION with a LIMIT of exactly 1 and no OFFSET
 ** clause.  This is a precondition for a couple of related optimizations.
@@ -3156,7 +3167,8 @@ static int multiSelect(
     /* If the compound has an ORDER BY clause, then always use the merge
     ** algorithm. */
     return multiSelectByMerge(pParse, p, pDest);
-  }else if( !selectIsUnionAll(p) && !unionWithLimitOne(db,p) ){
+  }else if( !selectIsUnionAll(p) 
+         && !(unionWithLimitOne(db,p) && selectIsUnionAndUnionAll(p)) ){
     /* If the compound is EXCEPT, INTERSECT, or UNION (anything other than
     ** UNION ALL) then also always use the merge algorithm.  However, the
     ** multiSelectByMerge() routine requires that the compound have an
@@ -3169,8 +3181,10 @@ static int multiSelect(
     return multiSelectByMerge(pParse, p, pDest);
   }else{
     /* For a UNION ALL compound without ORDER BY, or for a UNION with a 
-    ** LIMIT of exactly 1 and no OFFSET and no ORDER BY, simply run the left
-    ** query, then run the right query */
+    ** LIMIT of exactly 1 and no OFFSET and no ORDER BY and with no
+    ** EXCEPT or INTERSECT operators to the left and with no UNION operators
+    ** to the left that are followed by UNION ALL, simply run the compound
+    ** to the left of p, then run p */
     int addr = 0;
     int nLimit = 0;  /* Initialize to suppress harmless compiler warning */
 
@@ -3312,9 +3326,7 @@ static int generateOutputSubroutine(
     */
     case SRT_Fifo:
     case SRT_DistFifo:
-#ifdef SQLITE_ENABLE_UPDATE_DELETE_LIMIT
     case SRT_Table:
-#endif
     case SRT_EphemTab: {
       int r1 = sqlite3GetTempReg(pParse);
       int r2 = sqlite3GetTempReg(pParse);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memset`
- 外部函数：`sqlite3DbNNFreeNN`
- 外部函数：`sqlite3ExprCodeExprList`
- 外部函数：`sqlite3ExprDelete`
- 外部函数：`sqlite3ExprListDelete`
- 外部函数：`sqlite3SrcListDelete`
- 外部函数：`sqlite3VdbeAddOp3`
- 外部函数：`sqlite3WalkExpr`
- 外部函数：`sqlite3WindowListDelete`
- 外部函数：`sqlite3WindowUnlinkFromSelect`
- 外部函数：`sqlite3WithDelete`
- 大写宏：`ALWAYS`
- 大写宏：`LIMIT`
- 大写宏：`OK_IF_ALWAYS_TRUE`
- 大写宏：`ORDER`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_ENABLE_SORTER_REFERENCES`
- 大写宏：`SQLITE_OMIT_WINDOWFUNC`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_FUNCTION`
- 外部类型：`Assembled`
- 外部类型：`DistinctCtx`
- 外部类型：`EP_CanBeNull`
- 外部类型：`EP_InnerON`
- 外部类型：`EP_NoReduce`
- 外部类型：`EP_OuterON`
- 外部类型：`EP_Reduced`
- 外部类型：`EP_TokenOnly`
- 外部类型：`End`
- 外部类型：`Expr`
- 外部类型：`Fields`
- 外部类型：`First`
- 外部类型：`In`
- 外部类型：`Info`
- 外部类型：`Information`
- 外部类型：`No`
- 外部类型：`Number`
- 外部类型：`OP_MakeRecord`
- 外部类型：`Opcode`
- 外部类型：`OpenEphemeral`
- 外部类型：`Parse`
- 外部类型：`Parser`
- 外部类型：`Record`
- 外部类型：`Regs`
- 外部类型：`RowLoadInfo`
- 外部类型：`SORTFLAG_UseSorter`
- 外部类型：`Select`
- 外部类型：`SelectDest`
- 外部类型：`SortCtx`
- 外部类型：`SorterOpen`
- 外部类型：`Statement`
- 外部类型：`Stmt`
- 外部类型：`The`
- 外部类型：`Three`
- 外部类型：`Use`
- 外部类型：`Vdbe`
- 外部类型：`WRC_Continue`
- 外部类型：`Walker`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-50c4ab1d53` → 本草稿移入 `cases/defect/auto-sqlite-50c4ab1d53/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
