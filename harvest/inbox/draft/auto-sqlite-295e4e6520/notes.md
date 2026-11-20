# auto-sqlite-295e4e6520

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#0361a3b31cc719e4ff3af8a064f33d0925c78376](https://github.com/sqlite/sqlite/commit/0361a3b31cc719e4ff3af8a064f33d0925c78376) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 25 |
| 编译错误数（gcc syntax-only） | 11（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0361a3b31cc719e4ff3af8a064f33d0925c78376 (https://github.com/sqlite/sqlite/commit/0361a3b31cc719e4ff3af8a064f33d0925c78376)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0361a3b31cc719e4ff3af8a064f33d0925c78376 Do not push-down WHERE clause terms into DISTINCT subqueries or into :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5283,6 +5283,49 @@ static int pushDownWindowCheck(Parse *pParse, Select *pSubq, Expr *pExpr){
 #endif /* !defined(SQLITE_OMIT_SUBQUERY) || !defined(SQLITE_OMIT_VIEW) */
 
 #if !defined(SQLITE_OMIT_SUBQUERY) || !defined(SQLITE_OMIT_VIEW)
+
+/*
+** pSrc is a subquery in the FROM clause that uses DISTINCT or a compound
+** operator other than UNION ALL.  Those operations discard rows that are
+** equal to another row, even though the rows may be distinguishable.
+** For example, 1 and 1.0 are equal but typeof() tells them apart.  Return
+** true if the WHERE clause term pWhere gives the same answer for every
+** member of a set of equal rows, and is therefore safe to push down into
+** the subquery.  Return false if it might not be.
+**
+** This is the same test as for moving a HAVING term to the WHERE clause,
+** with every column of the subquery playing the role of a GROUP BY term.
+** See sqlite3ExprIsConstantOrGroupBy().
+*/
+static int pushDownIsEqualityInvariant(
+  Parse *pParse,        /* Parse context */
+  Expr *pWhere,         /* Candidate term from the WHERE clause */
+  SrcItem *pSrc         /* The subquery into which pWhere may be pushed */
+){
+  sqlite3 *db = pParse->db;
+  ExprList *pList = 0;
+  Table *pTab = pSrc->pSTab;
+  int i;
+  int rc;
+  assert( pTab->nCol>0 );
+  for(i=0; i<pTab->nCol; i++){
+    Expr *pCol = sqlite3ExprAlloc(db, TK_COLUMN, 0, 0);
+    if( pCol==0 ) break;
+    pCol->iTable = pSrc->iCursor;
+    pCol->iColumn = (i16)i;
+    pCol->y.pTab = pTab;
+    pList = sqlite3ExprListAppend(pParse, pList, pCol);
+  }
+  assert( pList!=0 || db->mallocFailed );
+  if( db->mallocFailed ){
+    sqlite3ExprListDelete(db, pList);
+    return 0;
+  }
+  rc = sqlite3ExprIsConstantOrGroupBy(pParse, pWhere, pList);
+  sqlite3ExprListDelete(db, pList);
+  return rc;
+}
+
 /*
 ** Make copies of relevant WHERE clause terms of the outer query into
 ** the WHERE clause of subquery.  Example:
@@ -5388,6 +5431,16 @@ static int pushDownWindowCheck(Parse *pParse, Select *pSubq, Expr *pExpr){
 **       case only comes up if SQLite is compiled using
 **       SQLITE_ALLOW_ROWID_IN_VIEW.
 **
+**  (13) If the subquery uses DISTINCT, or is a compound that uses UNION,
+**       INTERSECT, or EXCEPT, then the WHERE clause term may refer to the
+**       result columns of the subquery only as operands of comparison and
+**       logical operators.  Those operations discard rows that are equal
+**       but not identical, such as 1 and 1.0.  A term like typeof(x)='integer'
+**       can be true for one of two equal rows and false for the other, so
+**       applying it before the rows are discarded can change the result.
+**       For EXCEPT and INTERSECT it can even add or remove rows from the
+**       answer.  See pushDownIsEqualityInvariant().
+**
 ** Return 0 if no changes are made and non-zero if one or more WHERE clause
 ** terms are duplicated into the subquery.
 */
@@ -5401,6 +5454,7 @@ static int pushDownWhereTerms(
   Expr *pNew;
   SrcItem *pSrc;        /* The subquery FROM term into which WHERE is pushed */
   int nChng = 0;
+  int bCollapse = 0;    /* Subquery discards rows that are equal */
   pSrc = &pSrcList->a[iSrc];
   if( pWhere==0 ) return 0;
   if( pSubq->selFlags & (SF_Recursive|SF_MultiPart) ){
@@ -5415,10 +5469,12 @@ static int pushDownWhereTerms(
     int notUnionAll = 0;
     for(pSel=pSubq; pSel; pSel=pSel->pPrior){
       u8 op = pSel->op;
+      if( pSel->selFlags & SF_Distinct ) bCollapse = 1;
       assert( op==TK_ALL || op==TK_SELECT
            || op==TK_UNION || op==TK_INTERSECT || op==TK_EXCEPT );
       if( op!=TK_ALL && op!=TK_SELECT ){
         notUnionAll = 1;
+        bCollapse = 1;
       }
 #ifndef SQLITE_OMIT_WINDOWFUNC
       if( pSel->pWin ) return 0;    /* restriction (6b) */
@@ -5444,6 +5500,7 @@ static int pushDownWhereTerms(
 #ifndef SQLITE_OMIT_WINDOWFUNC
     if( pSubq->pWin && pSubq->pWin->pPartition==0 ) return 0;
 #endif
+    if( pSubq->selFlags & SF_Distinct ) bColla
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`defined`
- 外部函数：`restriction`
- 外部函数：`sqlite3ExprIsSingleTableConstraint`
- 大写宏：`FROM`
- 大写宏：`SQLITE_ALLOW_ROWID_IN_VIEW`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_OMIT_SUBQUERY`
- 大写宏：`SQLITE_OMIT_VIEW`
- 大写宏：`SQLITE_OMIT_WINDOWFUNC`
- 大写宏：`TK_ALL`
- 大写宏：`TK_EXCEPT`
- 大写宏：`TK_INTERSECT`
- 大写宏：`TK_SELECT`
- 大写宏：`TK_UNION`
- 大写宏：`WHERE`
- 外部类型：`Example`
- 外部类型：`Expr`
- 外部类型：`Make`
- 外部类型：`Return`
- 外部类型：`SF_MultiPart`
- 外部类型：`SF_PushDown`
- 外部类型：`SF_Recursive`
- 外部类型：`SQLite`
- 外部类型：`SrcItem`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-295e4e6520` → 本草稿移入 `cases/defect/auto-sqlite-295e4e6520/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
