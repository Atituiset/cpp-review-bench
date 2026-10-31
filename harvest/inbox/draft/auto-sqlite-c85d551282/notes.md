# auto-sqlite-c85d551282

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#c3351529909a3ad7135b8635fe824977ff5c30bc](https://github.com/sqlite/sqlite/commit/c3351529909a3ad7135b8635fe824977ff5c30bc) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 55 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c3351529909a3ad7135b8635fe824977ff5c30bc (https://github.com/sqlite/sqlite/commit/c3351529909a3ad7135b8635fe824977ff5c30bc)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 3853；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c3351529909a3ad7135b8635fe824977ff5c30bc Improvements to the detection of partial indexes working as covering indexes. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3764,10 +3764,40 @@ typedef struct CoveringIndexCheck CoveringIndexCheck;
 struct CoveringIndexCheck {
   Index *pIdx;       /* The index */
   int iTabCur;       /* Cursor number for the corresponding table */
+  WhereClause *pWC;  /* Ignore terms of this WHERE implied by pIdx, or NULL */
   u8 bExpr;          /* Uses an indexed expression */
   u8 bUnidx;         /* Uses an unindexed column not within an indexed expr */
 };
 
+/*
+** Return true if pExpr is a term of the WHERE clause pCk->pWC (not merely
+** a copy of such a term) and is identical to one of the AND-connected
+** terms of the WHERE clause of partial index pCk->pIdx.  Such a term is
+** never evaluated when pCk->pIdx drives the loop, because the
+** whereApplyPartialIndexConstraints() routine marks it as already coded.
+** Hence any columns used by the term do not need to be in the index.
+*/
+static int whereTermImpliedByPartialIndex(
+  CoveringIndexCheck *pCk,
+  const Expr *pExpr
+){
+  WhereClause *pWC = pCk->pWC;
+  Expr *pPart;
+  int i;
+  for(i=0; i<pWC->nTerm; i++){
+    if( pWC->a[i].pExpr==pExpr ) break;
+  }
+  if( i>=pWC->nTerm ) return 0;
+  pPart = pCk->pIdx->pPartIdxWhere;
+  while( pPart->op==TK_AND ){
+    if( sqlite3ExprCompare(0, pExpr, pPart->pRight, pCk->iTabCur)==0 ){
+      return 1;
+    }
+    pPart = pPart->pLeft;
+  }
+  return sqlite3ExprCompare(0, pExpr, pPart, pCk->iTabCur)==0;
+}
+
 /*
 ** Information passed in is pWalk->u.pCovIdxCk.  Call it pCk.
 **
@@ -3794,6 +3824,9 @@ static int whereIsCoveringIndexWalkCallback(Walker *pWalk, Expr *pExpr){
 
   pCk = pWalk->u.pCovIdxCk;
   pIdx = pCk->pIdx;
+  if( pCk->pWC && whereTermImpliedByPartialIndex(pCk, pExpr) ){
+    return WRC_Prune;
+  }
   if( (pExpr->op==TK_COLUMN || pExpr->op==TK_AGG_COLUMN) ){
     /* if( pExpr->iColumn<(BMS-1) && pIdx->bHasExpr==0 ) return WRC_Continue;*/
     if( pExpr->iTable!=pCk->iTabCur ) return WRC_Continue;
@@ -3821,6 +3854,12 @@ static int whereIsCoveringIndexWalkCallback(Walker *pWalk, Expr *pExpr){
 ** a covering index by using the colUsed bitmasks.  We have to do a search
 ** to see if the index is covering.  This routine does that search.
 **
+** If pWC is not NULL, then pIdx is a partial index and terms of the
+** WHERE clause pWC that are implied by the WHERE clause of pIdx are
+** ignored, as they will never be evaluated when pIdx drives the loop.
+** In this case pIdx might not cover all low-numbered columns used by
+** pWInfo->pSelect.
+**
 ** The return value is one of these:
 **
 **      0                The index is definitely not a covering index
@@ -3840,7 +3879,8 @@ static int whereIsCoveringIndexWalkCallback(Walker *pWalk, Expr *pExpr){
 static SQLITE_NOINLINE u32 whereIsCoveringIndex(
   WhereInfo *pWInfo,     /* The WHERE clause context */
   Index *pIdx,           /* Index that is being tested */
-  int iTabCur            /* Cursor for the table being indexed */
+  int iTabCur,           /* Cursor for the table being indexed */
+  WhereClause *pWC       /* Ignore terms implied by pIdx, or NULL */
 ){
   int i, rc;
   struct CoveringIndexCheck ck;
@@ -3850,7 +3890,7 @@ static SQLITE_NOINLINE u32 whereIsCoveringIndex(
     ** if pIdx is covering.  Assume it is not. */
     return 0;
   }
-  if( pIdx->bHasExpr==0 ){
+  if( pIdx->bHasExpr==0 && pWC==0 ){
     for(i=0; i<pIdx->nColumn; i++){
       if( pIdx->aiColumn[i]>=BMS-1 ) break;
     }
@@ -3863,6 +3903,7 @@ static SQLITE_NOINLINE u32 whereIsCoveringIndex(
   }
   ck.pIdx = pIdx;
   ck.iTabCur = iTabCur;
+  ck.pWC = pWC;
   ck.bExpr = 0;
   ck.bUnidx = 0;
   memset(&w, 0, sizeof(w));
@@ -4214,6 +4255,7 @@ static int whereLoopAddBtree(
       if( rc ) break;
     }else{
       Bitmask m;
+      WhereClause *pPartWC = 0;  /* WHERE terms that pProbe might imply */
       if( pProbe->isCovering ){
         m = 0;
         pNew->wsFlags = WHERE_IDX_ONLY | WHERE_INDEXED;
@@ -4225,10 +4267,23 @@ static int whereLoopAddBtree(
           wherePartIdxExpr(
               pWInfo->pParse
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`memset`
- 外部函数：`sqlite3ColumnType`
- 外部函数：`sqlite3DbFreeNN`
- 外部函数：`sqlite3DbMallocRaw`
- 外部函数：`sqlite3ExprAffinity`
- 外部函数：`sqlite3ExprAlloc`
- 外部函数：`sqlite3ExprAttachSubtrees`
- 外部函数：`sqlite3ExprCompareCollSeq`
- 外部函数：`sqlite3ExprDelete`
- 外部函数：`sqlite3ExprDup`
- 外部函数：`sqlite3ExprIsConstant`
- 外部函数：`sqlite3IsBinary`
- 外部函数：`sqlite3ParserAddCleanup`
- 外部函数：`strlen`
- 大写宏：`BMS`
- 大写宏：`FROM`
- 大写宏：`JT_LEFT`
- 大写宏：`JT_LTORJ`
- 大写宏：`JT_RIGHT`
- 大写宏：`SQLITE_AFF_BLOB`
- 大写宏：`SQLITE_AFF_TEXT`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`TK_AGG_COLUMN`
- 大写宏：`TK_AND`
- 大写宏：`TK_CAST`
- 大写宏：`TK_COLUMN`
- 大写宏：`TK_EQ`
- 大写宏：`TK_IS`
- 大写宏：`TOPBIT`
- 大写宏：`WHERE`
- 大写宏：`WHERETRACE`
- 大写宏：`WHERE_IDX_ONLY`
- 大写宏：`WHERE_INDEXED`
- 外部类型：`Assume`
- 外部类型：`Because`
- 外部类型：`Bitmask`
- 外部类型：`Call`
- 外部类型：`Column`
- 外部类型：`CoveringIndexCheck`
- 外部类型：`Cursor`
- 外部类型：`Expr`
- 外部类型：`Index`
- 外部类型：`IndexedExpr`
- 外部类型：`Information`
- 外部类型：`Mask`
- 外部类型：`Parse`
- 外部类型：`Partial`
- 外部类型：`SrcItem`
- 外部类型：`TF_Strict`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Token`
- 外部类型：`Uses`
- 外部类型：`WRC_Continue`
- 外部类型：`We`
- 外部类型：`WhereInfo`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-c85d551282` → 本草稿移入 `cases/defect/auto-sqlite-c85d551282/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
