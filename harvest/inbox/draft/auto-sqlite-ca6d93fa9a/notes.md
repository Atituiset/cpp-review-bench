# auto-sqlite-ca6d93fa9a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#a5ae4bd098c4d727c6b237fd0994e949c7aa1d15](https://github.com/sqlite/sqlite/commit/a5ae4bd098c4d727c6b237fd0994e949c7aa1d15) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a5ae4bd098c4d727c6b237fd0994e949c7aa1d15 (https://github.com/sqlite/sqlite/commit/a5ae4bd098c4d727c6b237fd0994e949c7aa1d15)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 663；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a5ae4bd098c4d727c6b237fd0994e949c7aa1d15 Return an error if the arguments of a table-valued function that is on the left of a RIGHT JOIN depend on tables to the  :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -660,7 +660,10 @@ static int sqlite3ProcessJoin(Parse *pParse, Select *p){
       p->selFlags |= SF_OnToWhere;
     }
 
-    if( pRight->fg.isTabFunc && joinType==EP_OuterON && pRight->u1.pFuncArg ){
+    if( (pRight->fg.isTabFunc && joinType==EP_OuterON && pRight->u1.pFuncArg)
+     || (pLeft->fg.isTabFunc && pLeft->u1.pFuncArg
+         && pLeft->fg.jointype & JT_LTORJ)
+    ){
       p->selFlags |= SF_OnToWhere;
     }
   }
@@ -7468,7 +7471,7 @@ static SQLITE_NOINLINE void existsToJoin(
 typedef struct CheckOnCtx CheckOnCtx;
 struct CheckOnCtx {
   SrcList *pSrc;       /* SrcList for this context */
-  int iJoin;           /* Cursors must be left of this one, if not zero */
+  int iJoin;           /* Cursors must be left of this one, if not negative */
   int bFuncArg;        /* True for table-function arg */
   CheckOnCtx *pParent; /* Parent context */
 };
@@ -7501,11 +7504,11 @@ static int selectCheckOnClausesExpr(Walker *pWalker, Expr *pExpr){
     ** set it to the cursor number of the RHS of the join to which this
     ** ON expression was attached and then iterate through the entire 
     ** expression.  */
-    assert( pCtx->iJoin==0 || pCtx->iJoin==pExpr->w.iJoin );
-    if( pCtx->iJoin==0 ){
+    assert( pCtx->iJoin<0 || pCtx->iJoin==pExpr->w.iJoin );
+    if( pCtx->iJoin<0 ){
       pCtx->iJoin = pExpr->w.iJoin;
       sqlite3WalkExprNN(pWalker, pExpr);
-      pCtx->iJoin = 0;
+      pCtx->iJoin = -1;
       return WRC_Prune;
     }
   }
@@ -7523,7 +7526,7 @@ static int selectCheckOnClausesExpr(Walker *pWalker, Expr *pExpr){
       for(ii=0; ii<nSrc && pSrc->a[ii].iCursor!=iTab; ii++){}
       if( ii<nSrc ){
         /* pSrc is the FROM clause that contains iTab */
-        if( pCtx->iJoin ){
+        if( pCtx->iJoin>=0 ){
           for(ii--; ii>=0 && pSrc->a[ii].iCursor!=pCtx->iJoin; ii--){}
           if( ii>=0 ){
             /* Table iJoin appears to the left of table iTab in the SrcList.
@@ -7555,6 +7558,7 @@ static int selectCheckOnClausesSelect(Walker *pWalker, Select *pSelect){
     memset(&sCtx, 0, sizeof(sCtx));
     sCtx.pSrc = pSelect->pSrc;
     sCtx.pParent = pCtx;
+    sCtx.iJoin = -1;
     pWalker->u.pCheckOnCtx = &sCtx;
     sqlite3WalkSelect(pWalker, pSelect);
     pWalker->u.pCheckOnCtx = pCtx;
@@ -7580,6 +7584,7 @@ void sqlite3SelectCheckOnClauses(Parse *pParse, Select *pSelect){
   w.u.pCheckOnCtx = &sCtx;
   memset(&sCtx, 0, sizeof(sCtx));
   sCtx.pSrc = pSelect->pSrc;
+  sCtx.iJoin = -1;
   sqlite3WalkExpr(&w, pSelect->pWhere);
   pSelect->selFlags &= ~SF_OnToWhere;
 
@@ -7590,7 +7595,7 @@ void sqlite3SelectCheckOnClauses(Parse *pParse, Select *pSelect){
   for(ii=0; ii<pSelect->pSrc->nSrc; ii++){
     SrcItem *pItem = &pSelect->pSrc->a[ii];
     if( pItem->fg.isTabFunc
-     && (pItem->fg.jointype & JT_OUTER)
+     && (pItem->fg.jointype & (JT_OUTER|JT_LTORJ))
     ){
       sCtx.iJoin = pItem->iCursor;
       sqlite3WalkExprList(&w, pItem->u1.pFuncArg);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memset`
- 外部函数：`sqlite3WalkExpr`
- 外部函数：`sqlite3WalkExprList`
- 外部函数：`sqlite3WalkExprNN`
- 外部函数：`sqlite3WalkSelect`
- 大写宏：`FROM`
- 大写宏：`JT_OUTER`
- 大写宏：`RHS`
- 外部类型：`Cursors`
- 外部类型：`EP_OuterON`
- 外部类型：`Parent`
- 外部类型：`SF_OnToWhere`
- 外部类型：`SrcItem`
- 外部类型：`SrcList`
- 外部类型：`Table`
- 外部类型：`True`
- 外部类型：`WRC_Prune`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-ca6d93fa9a` → 本草稿移入 `cases/defect/auto-sqlite-ca6d93fa9a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
