# auto-sqlite-478ff7e1f3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#9306117df843c2630d2ac5083574d5a8cbd4c40c](https://github.com/sqlite/sqlite/commit/9306117df843c2630d2ac5083574d5a8cbd4c40c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9306117df843c2630d2ac5083574d5a8cbd4c40c (https://github.com/sqlite/sqlite/commit/9306117df843c2630d2ac5083574d5a8cbd4c40c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1451；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9306117df843c2630d2ac5083574d5a8cbd4c40c Fix ALTER TABLE so that it works when a trigger contains an UPSERT with more than one ON CONFLICT clause. Report [bugs:/ :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1403,6 +1403,7 @@ static int renameResolveTrigger(Parse *pParse){
         Select *pSel = sqlite3SelectNew(
             pParse, pStep->pExprList, pSrc, 0, 0, 0, 0, 0, 0
         );
+        Upsert *pUpsert = 0;
         if( pSel==0 ){
           pStep->pExprList = 0;
           pSrc = 0;
@@ -1448,8 +1449,10 @@ static int renameResolveTrigger(Parse *pParse){
           rc = sqlite3ResolveExprListNames(&sNC, pStep->pExprList);
         }
         assert( !pStep->pUpsert || (!pStep->pWhere && !pStep->pExprList) );
-        if( pStep->pUpsert && rc==SQLITE_OK ){
-          Upsert *pUpsert = pStep->pUpsert;
+        for(pUpsert=pStep->pUpsert;
+            pUpsert && rc==SQLITE_OK;
+            pUpsert=pUpsert->pNextUpsert
+        ){
           pUpsert->pUpsertSrc = pSrc;
           sNC.uNC.pUpsert = pUpsert;
           sNC.ncFlags = NC_UUpsert;
@@ -1488,11 +1491,11 @@ static void renameWalkTrigger(Walker *pWalker, Trigger *pTrigger){
 
   /* Find tokens to edit in trigger steps */
   for(pStep=pTrigger->step_list; pStep; pStep=pStep->pNext){
+    Upsert *pUpsert = 0;
     sqlite3WalkSelect(pWalker, pStep->pSelect);
     sqlite3WalkExpr(pWalker, pStep->pWhere);
     sqlite3WalkExprList(pWalker, pStep->pExprList);
-    if( pStep->pUpsert ){
-      Upsert *pUpsert = pStep->pUpsert;
+    for(pUpsert=pStep->pUpsert; pUpsert; pUpsert=pUpsert->pNextUpsert){
       sqlite3WalkExprList(pWalker, pUpsert->pUpsertTarget);
       sqlite3WalkExprList(pWalker, pUpsert->pUpsertSet);
       sqlite3WalkExpr(pWalker, pUpsert->pUpsertWhere);
@@ -1678,9 +1681,9 @@ static void renameColumnFunc(
       if( pStep->pSrc ){
         Table *pTarget = sqlite3LocateTableItem(&sParse, 0, &pStep->pSrc->a[0]);
         if( pTarget==pTab ){
-          if( pStep->pUpsert ){
-            ExprList *pUpsertSet = pStep->pUpsert->pUpsertSet;
-            renameColumnElistNames(&sParse, &sCtx, pUpsertSet, zOld);
+          Upsert *pUpsert = 0;
+          for(pUpsert=pStep->pUpsert; pUpsert; pUpsert=pUpsert->pNextUpsert){
+            renameColumnElistNames(&sParse,&sCtx,pUpsert->pUpsertSet,zOld);
           }
           renameColumnIdlistNames(&sParse, &sCtx, pStep->pIdList, zOld);
           renameColumnElistNames(&sParse, &sCtx, pStep->pExprList, zOld);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`sqlite3LocateTableItem`
- 外部函数：`sqlite3ResolveExprListNames`
- 外部函数：`sqlite3SelectNew`
- 外部函数：`sqlite3WalkExpr`
- 外部函数：`sqlite3WalkExprList`
- 外部函数：`sqlite3WalkSelect`
- 外部函数：`sqlite3_stricmp`
- 大写宏：`ALWAYS`
- 大写宏：`ENAME_NAME`
- 大写宏：`NEVER`
- 大写宏：`SQLITE_OK`
- 外部类型：`ExprList`
- 外部类型：`Find`
- 外部类型：`IdList`
- 外部类型：`NC_UUpsert`
- 外部类型：`Next`
- 外部类型：`Parse`
- 外部类型：`RenameCtx`
- 外部类型：`RenameToken`
- 外部类型：`Select`
- 外部类型：`Table`
- 外部类型：`The`
- 外部类型：`Token`
- 外部类型：`Upsert`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-478ff7e1f3` → 本草稿移入 `cases/defect/auto-sqlite-478ff7e1f3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
