# auto-sqlite-5118a475ac

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#3d0c703e3d18b05099d90163189bca51e6820526](https://github.com/sqlite/sqlite/commit/3d0c703e3d18b05099d90163189bca51e6820526) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 33 |
| 编译错误数（gcc syntax-only） | 17（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #3d0c703e3d18b05099d90163189bca51e6820526 (https://github.com/sqlite/sqlite/commit/3d0c703e3d18b05099d90163189bca51e6820526)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 10652；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 3d0c703e3d18b05099d90163189bca51e6820526 Fix an extreme corner case OOM problem in PRAGMA integrity_check. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -10644,12 +10644,15 @@ Pager *sqlite3BtreePager(Btree *p){
 
 #ifndef SQLITE_OMIT_INTEGRITY_CHECK
 /*
-** Record an OOM error during integrity_check
+** Record an OOM error during integrity_check if rc is SQLITE_NOMEM
+** or SQLITE_IOERR_NOMEM.
 */
-static void checkOom(IntegrityCk *pCheck){
-  pCheck->rc = SQLITE_NOMEM;
-  pCheck->mxErr = 0;  /* Causes integrity_check processing to stop */
-  if( pCheck->nErr==0 ) pCheck->nErr++;
+static void checkOom(IntegrityCk *pCheck, int rc){
+  if( rc==SQLITE_NOMEM || rc==SQLITE_IOERR_NOMEM ){
+    pCheck->rc = SQLITE_NOMEM;
+    pCheck->mxErr = 0;  /* Causes integrity_check processing to stop */
+    if( pCheck->nErr==0 ) pCheck->nErr++;
+  }
 }
 
 /*
@@ -10702,7 +10705,7 @@ static void checkAppendMsg(
   sqlite3_str_vappendf(&pCheck->errMsg, zFormat, ap);
   va_end(ap);
   if( pCheck->errMsg.accError==SQLITE_NOMEM ){
-    checkOom(pCheck);
+    checkOom(pCheck, SQLITE_NOMEM);
   }
 }
 #endif /* SQLITE_OMIT_INTEGRITY_CHECK */
@@ -10768,7 +10771,7 @@ static void checkPtrmap(
 
   rc = ptrmapGet(pCheck->pBt, iChild, &ePtrmapType, &iPtrmapParent);
   if( rc!=SQLITE_OK ){
-    if( rc==SQLITE_NOMEM || rc==SQLITE_IOERR_NOMEM ) checkOom(pCheck);
+    checkOom(pCheck, rc);
     checkAppendMsg(pCheck, "Failed to read ptrmap key=%u", iChild);
     return;
   }
@@ -10964,9 +10967,9 @@ static int checkTreePage(
   pCheck->v1 = iPage;
   pCheck->nAbove++;
   if( (rc = btreeGetPage(pBt, iPage, &pPage, 0))!=0 ){
+    checkOom(pCheck, rc);
     checkAppendMsg(pCheck,
        "unable to get the page. error code=%d", rc);
-    if( rc==SQLITE_IOERR_NOMEM ) pCheck->rc = SQLITE_NOMEM;
     goto end_of_check;
   }
   if( pCheck->nAbove > BTCURSOR_MAX_DEPTH ){
@@ -11267,15 +11270,15 @@ int sqlite3BtreeIntegrityCheck(
 
   sCheck.aPgRef = sqlite3MallocZero((sCheck.nCkPage / 8)+ 1);
   if( !sCheck.aPgRef ){
-    checkOom(&sCheck);
+    checkOom(&sCheck, SQLITE_NOMEM);
     goto integrity_ck_cleanup;
   }
   sCheck.heap = (u32*)sqlite3Malloc( pBt->pageSize*2 );
 #ifdef SQLITE_DEBUG
   sCheck.mxHeap = pBt->pageSize/2 - 1;
 #endif
   if( sCheck.heap==0 ){
-    checkOom(&sCheck);
+    checkOom(&sCheck, SQLITE_NOMEM);
     goto integrity_ck_cleanup;
   }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`btreePageFromDbPage`
- 外部函数：`sqlite3Malloc`
- 外部函数：`sqlite3MallocZero`
- 外部函数：`sqlite3PagerGet`
- 外部函数：`sqlite3PagerGetData`
- 外部函数：`sqlite3PagerGetExtra`
- 外部函数：`sqlite3_mutex_held`
- 外部函数：`sqlite3_str_append`
- 外部函数：`sqlite3_str_appendf`
- 外部函数：`sqlite3_str_vappendf`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`xProgress`
- 大写宏：`BTCURSOR_MAX_DEPTH`
- 大写宏：`OOM`
- 大写宏：`PAGER_GET_NOCONTENT`
- 大写宏：`PAGER_GET_READONLY`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_INTERRUPT`
- 大写宏：`SQLITE_IOERR_NOMEM`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_OMIT_INTEGRITY_CHECK`
- 大写宏：`SQLITE_OMIT_PROGRESS_CALLBACK`
- 外部类型：`BtShared`
- 外部类型：`Causes`
- 外部类型：`DbPage`
- 外部类型：`Failed`
- 外部类型：`IntegrityCk`
- 外部类型：`MemPage`
- 外部类型：`Number`
- 外部类型：`Pgno`
- 外部类型：`Record`
- 外部类型：`Return`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-5118a475ac` → 本草稿移入 `cases/defect/auto-sqlite-5118a475ac/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
