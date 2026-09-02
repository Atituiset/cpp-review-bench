# auto-sqlite-b0d7185e18

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#09a55130084c903bb0d1b6a2a8a672660b4bce37](https://github.com/sqlite/sqlite/commit/09a55130084c903bb0d1b6a2a8a672660b4bce37) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 89 |
| 编译错误数（gcc syntax-only） | 38（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #09a55130084c903bb0d1b6a2a8a672660b4bce37 (https://github.com/sqlite/sqlite/commit/09a55130084c903bb0d1b6a2a8a672660b4bce37)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1972；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 09a55130084c903bb0d1b6a2a8a672660b4bce37 Within a checkpoint, avoid allocating memory for *-shm pages that will not be processed as they have already been comple :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1952,6 +1952,9 @@ static void walIteratorFree(WalIterator *p){
 ** WalIterator object when it has finished with it.
 */
 static int walIteratorInit(Wal *pWal, u32 nBackfill, WalIterator **pp){
+  int iFirstPage;                 /* First 32KB *-shm page to visit */
+  int iLastPage;                  /* Last 32KB *-shm page to visit */
+  u32 iZero;                      /* Frame before first frame to visit */
   WalIterator *p;                 /* Return value */
   int nSegment;                   /* Number of segments to merge */
   u32 iLast;                      /* Last frame in log */
@@ -1961,15 +1964,17 @@ static int walIteratorInit(Wal *pWal, u32 nBackfill, WalIterator **pp){
   int rc = SQLITE_OK;             /* Return Code */
 
   /* This routine only runs while holding the checkpoint lock. And
-  ** it only runs if there is actually content in the log (mxFrame>0).
-  */
+  ** it only runs if there is actually content in the log (mxFrame>0).  */
   assert( pWal->ckptLock && pWal->hdr.mxFrame>0 );
+
   iLast = pWal->hdr.mxFrame;
+  iFirstPage = walFramePage(nBackfill+1);
+  iLastPage = walFramePage(iLast);
+  iZero = iFirstPage ? (HASHTABLE_NPAGE_ONE+(iFirstPage-1)*HASHTABLE_NPAGE) : 0;
 
   /* Allocate space for the WalIterator object. */
-  nSegment = walFramePage(iLast) + 1;
-  nByte = SZ_WALITERATOR(nSegment)
-        + iLast*sizeof(ht_slot);
+  nSegment = iLastPage - iFirstPage + 1;
+  nByte = SZ_WALITERATOR(nSegment) + (iLast-iZero)*sizeof(ht_slot);
   p = (WalIterator *)sqlite3_malloc64(nByte
       + sizeof(ht_slot) * (iLast>HASHTABLE_NPAGE?HASHTABLE_NPAGE:iLast)
   );
@@ -1980,7 +1985,7 @@ static int walIteratorInit(Wal *pWal, u32 nBackfill, WalIterator **pp){
   p->nSegment = nSegment;
   aTmp = (ht_slot*)&(((u8*)p)[nByte]);
   SEH_FREE_ON_ERROR(0, p);
-  for(i=walFramePage(nBackfill+1); rc==SQLITE_OK && i<nSegment; i++){
+  for(i=iFirstPage; rc==SQLITE_OK && i<=iLastPage; i++){
     WalHashLoc sLoc;
 
     rc = walHashGet(pWal, i, &sLoc);
@@ -1989,22 +1994,23 @@ static int walIteratorInit(Wal *pWal, u32 nBackfill, WalIterator **pp){
       int nEntry;                 /* Number of entries in this segment */
       ht_slot *aIndex;            /* Sorted index for this segment */
 
-      if( (i+1)==nSegment ){
+      if( i==iLastPage ){
         nEntry = (int)(iLast - sLoc.iZero);
       }else{
         nEntry = (int)((u32*)sLoc.aHash - (u32*)sLoc.aPgno);
       }
-      aIndex = &((ht_slot *)&p->aSegment[p->nSegment])[sLoc.iZero];
+      assert( i!=iFirstPage || iZero==sLoc.iZero );
+      aIndex = &((ht_slot *)&p->aSegment[p->nSegment])[sLoc.iZero - iZero];
       sLoc.iZero++;
 
       for(j=0; j<nEntry; j++){
         aIndex[j] = (ht_slot)j;
       }
       walMergesort((u32 *)sLoc.aPgno, aTmp, aIndex, &nEntry);
-      p->aSegment[i].iZero = sLoc.iZero;
-      p->aSegment[i].nEntry = nEntry;
-      p->aSegment[i].aIndex = aIndex;
-      p->aSegment[i].aPgno = (u32 *)sLoc.aPgno;
+      p->aSegment[i-iFirstPage].iZero = sLoc.iZero;
+      p->aSegment[i-iFirstPage].nEntry = nEntry;
+      p->aSegment[i-iFirstPage].aIndex = aIndex;
+      p->aSegment[i-iFirstPage].aPgno = (u32 *)sLoc.aPgno;
     }
   }
   if( rc!=SQLITE_OK ){
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`callback`
- 外部函数：`log`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`sehInjectFault`
- 外部函数：`sqlite3FaultSim`
- 外部函数：`sqlite3MallocZero`
- 外部函数：`sqlite3OsShmMap`
- 外部函数：`sqlite3Realloc`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_get_snapshot`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`testcase`
- 大写宏：`EXCEPTION_IN_PAGE_ERROR`
- 大写宏：`FLEXARRAY`
- 大写宏：`NEVER`
- 大写宏：`NULL`
- 大写宏：`OUT`
- 大写宏：`SEH_TRY`
- 大写宏：`SHM`
- 大写宏：`SQLITE_DEBUG`
- 大写宏：`SQLITE_ENABLE_SETLK_TIMEOUT`
- 大写宏：`SQLITE_ENABLE_SNAPSHOT`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_NOINLINE`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NOMEM_BKPT`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_READONLY`
- 大写宏：`SQLITE_USE_SEH`
- 大写宏：`ULONG_PTR`
- 大写宏：`VFS`
- 大写宏：`WAL`
- 大写宏：`WAL_RDONLY`
- 大写宏：`WAL_RDWR`
- 外部类型：`Allocate`
- 外部类型：`And`
- 外部类型：`Array`
- 外部类型：`Buffer`
- 外部类型：`Checkpoint`
- 外部类型：`Code`
- 外部类型：`Current`
- 外部类型：`Database`
- 外部类型：`Elements`
- 外部类型：`Enlarge`
- 外部类型：`File`
- 外部类型：`Find`
- 外部类型：`Flags`
- 外部类型：`Frame`
- 外部类型：`Fsync`
- 外部类型：`Hash`
- 外部类型：`Ignore`
- 外部类型：`Index`
- 外部类型：`Last`
- 外部类型：`Left`
- 外部类型：`List`
- 外部类型：`Mask`
- 外部类型：`Must`
- 外部类型：`Name`
- 外部类型：`Next`
- 外部类型：`Non`
- 外部类型：`Nr`
- 外部类型：`Number`
- 外部类型：`On`
- 外部类型：`One`
- 外部类型：`Pad`
- 外部类型：`Pages`
- 外部类型：`Pgno`
- 外部类型：`Pointer`
- 外部类型：`Request`
- 外部类型：`Return`
- 外部类型：`Right`
- 外部类型：`Size`
- 外部类型：`Sorted`
- 外部类型：`Start`
- 外部类型：`Sublist`
- 外部类型：`System`
- 外部类型：`Temporary`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Transaction`
- 外部类型：`True`
- 外部类型：`Truncate`
- 外部类型：`Value`
- 外部类型：`Wal`
- 外部类型：`WalCkptInfo`
- 外部类型：`WalHashLoc`
- 外部类型：`WalIndexHdr`
- 外部类型：`WalIterator`
- 外部类型：`WalSegment`
- 外部类型：`Which`
- 外部类型：`Write`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-b0d7185e18` → 本草稿移入 `cases/defect/auto-sqlite-b0d7185e18/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
