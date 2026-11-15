# auto-sqlite-1ee04c8346

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#e742a915065f86d0f271f84dc54663a7508bd12c](https://github.com/sqlite/sqlite/commit/e742a915065f86d0f271f84dc54663a7508bd12c) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-15 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 34（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e742a915065f86d0f271f84dc54663a7508bd12c (https://github.com/sqlite/sqlite/commit/e742a915065f86d0f271f84dc54663a7508bd12c)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 982；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e742a915065f86d0f271f84dc54663a7508bd12c Multiple fixes to the zipfile extension :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -429,6 +429,18 @@ static void zipfileEntryFree(ZipfileEntry *p){
 static void zipfileCleanupTransaction(ZipfileTab *pTab){
   ZipfileEntry *pEntry;
   ZipfileEntry *pNext;
+  ZipfileCsr *pCsr;
+
+  /* A cursor that is not reading from a file or from a blob steps through
+  ** the pTab->pFirstEntry list, which is about to be deleted.  Such a
+  ** cursor has nothing left to return, so leave it at EOF rather than
+  ** pointing at freed memory. */
+  for(pCsr=pTab->pCsrList; pCsr; pCsr=pCsr->pCsrNext){
+    if( pCsr->pFile==0 && pCsr->pFreeEntry==0 ){
+      pCsr->pCurrent = 0;
+      pCsr->bEof = 1;
+    }
+  }
 
   if( pTab->pWriteFd ){
     fclose(pTab->pWriteFd);
@@ -481,11 +493,15 @@ static void zipfileResetCursor(ZipfileCsr *pCsr){
 
   pCsr->bEof = 0;
   if( pCsr->pFile ){
+    /* File mode: pCurrent is a private copy owned by this cursor. */
     fclose(pCsr->pFile);
     pCsr->pFile = 0;
     zipfileEntryFree(pCsr->pCurrent);
-    pCsr->pCurrent = 0;
   }
+  /* In the other modes pCurrent belongs to pFreeEntry or to the table.
+  ** It must not be left dangling, since a later xFilter() that switches
+  ** this cursor to file mode would free it. */
+  pCsr->pCurrent = 0;
 
   for(p=pCsr->pFreeEntry; p; p=pNext){
     pNext = p->pNext;
@@ -979,7 +995,7 @@ static int zipfileNext(sqlite3_vtab_cursor *cur){
       pCsr->pCurrent = p;
     }
   }else{
-    if( !pCsr->bNoop ){
+    if( !pCsr->bNoop && !pCsr->bEof ){
       pCsr->pCurrent = pCsr->pCurrent->pNext;
     }
     if( pCsr->pCurrent==0 ){
@@ -1099,8 +1115,13 @@ static int zipfileColumn(
   int i                       /* Which column to return */
 ){
   ZipfileCsr *pCsr = (ZipfileCsr*)cur;
-  ZipfileCDS *pCDS = &pCsr->pCurrent->cds;
+  ZipfileCDS *pCDS;
   int rc = SQLITE_OK;
+  if( pCsr->pCurrent==0 ){
+    /* The transaction ended while this row was being processed. */
+    return SQLITE_OK;
+  }
+  pCDS = &pCsr->pCurrent->cds;
   switch( i ){
     case 0:   /* name */
       sqlite3_result_text(ctx, pCDS->zFile, -1, SQLITE_TRANSIENT);
@@ -1945,7 +1966,7 @@ static void zipfileFunctionCds(
   assert( argc>0 );
 
   pCsr = zipfileFindCursor(pTab, sqlite3_value_int64(argv[0]));
-  if( pCsr ){
+  if( pCsr && pCsr->pCurrent ){
     ZipfileCDS *p = &pCsr->pCurrent->cds;
     char *zRes = sqlite3_mprintf("{"
         "\"version-made-by\" : %u, "
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`fclose`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_result_text`
- 外部函数：`sqlite3_value_int64`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_TRANSIENT`
- 外部类型：`Which`
- 外部类型：`ZipfileCDS`
- 外部类型：`ZipfileCsr`
- 外部类型：`ZipfileEntry`
- 外部类型：`ZipfileTab`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-1ee04c8346` → 本草稿移入 `cases/defect/auto-sqlite-1ee04c8346/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
