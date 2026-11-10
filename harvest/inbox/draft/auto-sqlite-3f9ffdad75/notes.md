# auto-sqlite-3f9ffdad75

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#a8bdc0ba6bf7108f5ca872d0b91bf60265ab11d1](https://github.com/sqlite/sqlite/commit/a8bdc0ba6bf7108f5ca872d0b91bf60265ab11d1) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 22 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a8bdc0ba6bf7108f5ca872d0b91bf60265ab11d1 (https://github.com/sqlite/sqlite/commit/a8bdc0ba6bf7108f5ca872d0b91bf60265ab11d1)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 947；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a8bdc0ba6bf7108f5ca872d0b91bf60265ab11d1 When a VALUES clause is part of a compound SELECT, do not ignore collation sequences attached to rows or SELECTs to the  :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -619,6 +619,18 @@ static int exprListIsNoAffinity(Parse *pParse, ExprList *pRow){
 
 }
 
+/*
+** Return true if any expression in the expression-list passed as the
+** only argument contains a COLLATE operator.
+*/
+static int exprListHasCollate(ExprList *pRow){
+  int ii;
+  for(ii=0; ii<pRow->nExpr; ii++){
+    if( ExprHasProperty(pRow->a[ii].pExpr, EP_Collate) ) return 1;
+  }
+  return 0;
+}
+
 /*
 ** This function is called by the parser for the second and subsequent
 ** rows of a multi-row VALUES clause. Argument pLeft is the part of
@@ -668,7 +680,15 @@ static int exprListIsNoAffinity(Parse *pParse, ExprList *pRow){
 **       because the complex rules SQLite uses (see function 
 **       sqlite3SubqueryColumnTypes() in select.c) to determine the effective
 **       affinity of such a column for all rows require access to all values in
-**       the column simultaneously. 
+**       the column simultaneously.
+**
+**    e) The new row, or the row that would become the first row of the
+**       co-routine, contains a COLLATE operator. This ensures that the
+**       columns of a co-routine never have a collation sequence (see
+**       multiSelectCollSeq() in select.c). Each explicit COLLATE in a VALUES
+**       clause is instead part of an ordinary SELECT in a UNION ALL
+**       compound, where it is available when the collation sequence for a
+**       compound SELECT column is determined.
 */
 Select *sqlite3MultiValues(Parse *pParse, Select *pLeft, ExprList *pRow){
 
@@ -677,6 +697,8 @@ Select *sqlite3MultiValues(Parse *pParse, Select *pLeft, ExprList *pRow){
    || sqlite3ExprListIsConstant(pParse, pRow, 1)==0   /* condition (c) above */
    || (pLeft->pSrc->nSrc==0 &&
        exprListIsNoAffinity(pParse,pLeft->pEList)==0) /* condition (d) above */
+   || exprListHasCollate(pRow)                        /* condition (e) above */
+   || (pLeft->pSrc->nSrc==0 && exprListHasCollate(pLeft->pEList))
    || IN_SPECIAL_PARSE
   ){
     /* The co-routine method cannot be used. Fall back to UNION ALL. */
@@ -718,7 +740,7 @@ Select *sqlite3MultiValues(Parse *pParse, Select *pLeft, ExprList *pRow){
         pRet->pSrc->nSrc = 1;
         pRet->pPrior = pLeft->pPrior;
         pRet->op = pLeft->op;
-        if( pRet->pPrior ) pRet->selFlags |= SF_Values;
+        pRet->selFlags |= SF_Values;
         pLeft->pPrior = 0;
         pLeft->op = TK_SELECT;
         assert( pLeft->pNext==0 );
@@ -944,7 +966,11 @@ void sqlite3Insert(
   ** single row (the common case) then keep that one row of values
   ** and discard the other (unused) parts of the pSelect object
   */
-  if( pSelect && (pSelect->selFlags & SF_Values)!=0 && pSelect->pPrior==0 ){
+  if( pSelect
+   && (pSelect->selFlags & SF_Values)!=0
+   && pSelect->pPrior==0
+   && pSelect->pSrc->nSrc==0
+  ){
     pList = pSelect->pEList;
     pSelect->pEList = 0;
     sqlite3SelectDelete(db, pSelect);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`condition`
- 外部函数：`other`
- 外部函数：`sqlite3ExprAffinity`
- 外部函数：`sqlite3MultiValues`
- 外部函数：`sqlite3SelectDelete`
- 外部函数：`sqlite3SubqueryColumnTypes`
- 大写宏：`ALL`
- 大写宏：`IN_SPECIAL_PARSE`
- 大写宏：`TK_RAISE`
- 大写宏：`TK_SELECT`
- 大写宏：`UNION`
- 大写宏：`VALUES`
- 外部类型：`Argument`
- 外部类型：`Expr`
- 外部类型：`ExprList`
- 外部类型：`Fall`
- 外部类型：`Parse`
- 外部类型：`SF_Values`
- 外部类型：`SQLite`
- 外部类型：`Select`
- 外部类型：`The`
- 外部类型：`This`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-3f9ffdad75` → 本草稿移入 `cases/defect/auto-sqlite-3f9ffdad75/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
