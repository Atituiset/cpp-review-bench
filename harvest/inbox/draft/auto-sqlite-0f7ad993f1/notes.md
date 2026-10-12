# auto-sqlite-0f7ad993f1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#50575da52ca09eba39e84b370e7913edc80894d2](https://github.com/sqlite/sqlite/commit/50575da52ca09eba39e84b370e7913edc80894d2) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 18 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #50575da52ca09eba39e84b370e7913edc80894d2 (https://github.com/sqlite/sqlite/commit/50575da52ca09eba39e84b370e7913edc80894d2)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 11（原始 PR diff 行 5106；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 50575da52ca09eba39e84b370e7913edc80894d2 Another fix to the order-by-subquery optimization :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5079,13 +5079,13 @@ static int whereLoopAddAll(WhereLoopBuilder *pBuilder){
 **      direction because there is no way to run a co-routine backwards.
 */
 static SQLITE_NOINLINE int wherePathMatchSubqueryOB(
-  WhereInfo *pWInfo,      /* The WHERE clause */
   WhereLoop *pLoop,       /* The nested loop term that is a subquery */
   int iLoop,              /* Which level of the nested loop.  0==outermost */
   int iCur,               /* Cursor used by the this loop */
   ExprList *pOrderBy,     /* The ORDER BY clause on the whole query */
   Bitmask *pRevMask,      /* When loops need to go in reverse order */
-  Bitmask *pOBSat         /* Which terms of pOrderBy are satisfied so far */
+  Bitmask *pOBSat,        /* Which terms of pOrderBy are satisfied so far */
+  u16 wctrlFlags          /* WHERE_GROUPBY or _DISTINCTBY or _ORDERBY_LIMIT */
 ){
   int iOB;                /* Index into pOrderBy->a[] */
   int jSub;               /* Index into pSubOB->a[] */
@@ -5103,7 +5103,7 @@ static SQLITE_NOINLINE int wherePathMatchSubqueryOB(
     if( pOBExpr->op!=TK_COLUMN && pOBExpr->op!=TK_AGG_COLUMN ) break;
     if( pOBExpr->iTable!=iCur ) break;
     if( pOBExpr->iColumn!=pSubOB->a[jSub].u.x.iOrderByCol-1 ) break;
-    if( (pWInfo->wctrlFlags & WHERE_GROUPBY)==0 ){
+    if( (wctrlFlags & WHERE_GROUPBY)==0 ){
       u8 sfOB = pOrderBy->a[iOB].fg.sortFlags;   /* sortFlags for iOB */
       u8 sfSub = pSubOB->a[jSub].fg.sortFlags;   /* sortFlags for jSub */
       if( (sfSub & KEYINFO_ORDER_BIGNULL) != (sfOB & KEYINFO_ORDER_BIGNULL) ){
@@ -5285,8 +5285,8 @@ static i8 wherePathSatisfiesOrderBy(
       if( pLoop->wsFlags & WHERE_IPK ){
         if( pLoop->u.btree.pOrderBy
          && OptimizationEnabled(db, SQLITE_OrderBySubq)
-         &&  wherePathMatchSubqueryOB(pWInfo,pLoop,iLoop,iCur,
-                                     pOrderBy,pRevMask, &obSat)
+         &&  wherePathMatchSubqueryOB(pLoop,iLoop,iCur,
+                                     pOrderBy,pRevMask, &obSat, wctrlFlags)
         ){
           nColumn = 0;
           isOrderDistinct = 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`KEYINFO_ORDER_BIGNULL`
- 大写宏：`ORDER`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`TK_AGG_COLUMN`
- 大写宏：`TK_COLUMN`
- 大写宏：`WHERE`
- 大写宏：`WHERE_GROUPBY`
- 大写宏：`WHERE_IPK`
- 外部类型：`Bitmask`
- 外部类型：`Cursor`
- 外部类型：`ExprList`
- 外部类型：`Index`
- 外部类型：`SQLITE_OrderBySubq`
- 外部类型：`The`
- 外部类型：`When`
- 外部类型：`WhereInfo`
- 外部类型：`WhereLoop`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-0f7ad993f1` → 本草稿移入 `cases/defect/auto-sqlite-0f7ad993f1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
