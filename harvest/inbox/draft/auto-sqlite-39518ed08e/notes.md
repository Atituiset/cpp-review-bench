# auto-sqlite-39518ed08e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#814fd8ef4d42d6567f1613a66f556c8194764d06](https://github.com/sqlite/sqlite/commit/814fd8ef4d42d6567f1613a66f556c8194764d06) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 102 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #814fd8ef4d42d6567f1613a66f556c8194764d06 (https://github.com/sqlite/sqlite/commit/814fd8ef4d42d6567f1613a66f556c8194764d06)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 4891；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 814fd8ef4d42d6567f1613a66f556c8194764d06 Merge branch-3.54 fixes into trunk :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -557,6 +557,9 @@ struct Fts5SegIter {
   i64 iRowid;                     /* Current rowid */
   int nPos;                       /* Number of bytes in current position list */
   u8 bDel;                        /* True if the delete flag is set */
+
+  /* True if the current entry shadows an entry that has the delete flag set */
+  u8 bDelDup;
 };
 
 static int fts5IndexCorruptRowid(Fts5Index *pIdx, i64 iRowid){
@@ -3115,6 +3118,10 @@ static int fts5MultiIterDoCompare(Fts5Iter *pIter, int iOut){
       assert_nc( i2!=0 );
       pRes->bTermEq = 1;
       if( p1->iRowid==p2->iRowid ){
+        /* p2 is about to be advanced past this entry. If it, or an entry
+        ** it shadowed, has the delete flag set, record this on p1.  */
+        p1->bDelDup |= (p2->bDel | p2->bDelDup);
+        p2->bDelDup = 0;
         return i2;
       }
       res = ((p1->iRowid > p2->iRowid)==pIter->bRev) ? -1 : +1;
@@ -4877,6 +4884,14 @@ static void fts5IndexMergeLevel(
     int nPos;                     /* position-list size field value */
     int nTerm;
     const u8 *pTerm;
+    int bDel;                     /* True to set delete flag on output */
+
+    /* If this entry has the delete flag set, or shadows an entry with the
+    ** delete flag set, set the delete flag on the output. Unless the output
+    ** will be the oldest segment in the table - in that case the delete flag
+    ** may be discarded.  */
+    bDel = bOldest ? 0 : (pSegIter->bDel || pSegIter->bDelDup);
+    pSegIter->bDelDup = 0;
 
     pTerm = fts5MultiIterTerm(pIter, &nTerm);
     if( nTerm!=term.n || fts5Memcmp(pTerm, term.p, nTerm) ){
@@ -4888,7 +4903,7 @@ static void fts5IndexMergeLevel(
     }
 
     /* Check for key annihilation. */
-    if( pSegIter->nPos==0 && (bOldest || pSegIter->bDel==0) ) continue;
+    if( pSegIter->nPos==0 && (bOldest || bDel==0) ) continue;
 
     if( p->rc==SQLITE_OK && bTermWritten==0 ){
       /* This is a new term. Append a term to the output segment. */
@@ -4901,15 +4916,15 @@ static void fts5IndexMergeLevel(
     fts5WriteAppendRowid(p, &writer, fts5MultiIterRowid(pIter));
 
     if( eDetail==FTS5_DETAIL_NONE ){
-      if( pSegIter->bDel ){
+      if( bDel ){
         fts5BufferAppendVarint(&p->rc, &writer.writer.buf, 0);
         if( pSegIter->nPos>0 ){
           fts5BufferAppendVarint(&p->rc, &writer.writer.buf, 0);
         }
       }
     }else{
       /* Append the position-list data to the output */
-      nPos = pSegIter->nPos*2 + pSegIter->bDel;
+      nPos = pSegIter->nPos*2 + bDel;
       fts5BufferAppendVarint(&p->rc, &writer.writer.buf, nPos);
       fts5ChunkIterate(p, pSegIter, (void*)&writer, fts5MergeChunkCallback);
     }
@@ -5348,7 +5363,8 @@ static void fts5DoSecureDelete(
     assert_nc( iSOP<=pSeg->iLeafOffset );
 
     if( bDetailNone ){
-      while( iSOP<pSeg->iLeafOffset ){
+      int iEnd = pSeg->iLeafOffset - (pSeg->bDel ? 1 + pSeg->nPos : 0);
+      while( iSOP<iEnd ){
         if( aPg[iSOP]==0x00 ) iSOP++;
         if( aPg[iSOP]==0x00 ) iSOP++;
         iStart = iSOP;
@@ -5413,7 +5429,7 @@ static void fts5DoSecureDelete(
 
   if( pSeg->bDel ){
     iOff += sqlite3Fts5PutVarint(&aPg[iOff], iDelta);
-    aPg[iOff++] = 0x01;
+    aPg[iOff++] = bDetailNone ? 0x00 : 0x01;
   }else if( bLastInDoclist==0 ){
     if( iNextOff!=iPgIdx ){
       u64 iNextDelta = 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert`
- 外部函数：`assert_nc`
- 外部函数：`flag`
- 外部函数：`fts5BufferAppendBlob`
- 外部函数：`fts5BufferAppendVarint`
- 外部函数：`fts5BufferZero`
- 外部函数：`fts5DataXXX`
- 外部函数：`fts5GetVarint`
- 外部函数：`fts5Memcmp`
- 外部函数：`fts5PutU16`
- 外部函数：`fts5_dri`
- 外部函数：`iRowid`
- 外部函数：`memset`
- 外部函数：`prepare`
- 外部函数：`sqlite3Fts5BufferAppendVarint`
- 外部函数：`sqlite3Fts5BufferZero`
- 外部函数：`sqlite3Fts5ConfigErrmsg`
- 外部函数：`sqlite3Fts5PutVarint`
- 外部函数：`sqlite3_bind_blob`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_bind_null`
- 外部函数：`sqlite3_blob_bytes`
- 外部函数：`sqlite3_blob_close`
- 外部函数：`sqlite3_blob_open`
- 外部函数：`sqlite3_blob_read`
- 外部函数：`sqlite3_blob_reopen`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_prepare_v3`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 外部函数：`structure`
- 外部函数：`void`
- 外部函数：`xChunk`
- 大写宏：`AND`
- 大写宏：`DELETE`
- 大写宏：`EIGHT_BYTE_ALIGNMENT`
- 大写宏：`FROM`
- 大写宏：`FTS5`
- 大写宏：`FTS5_CORRUPT`
- 大写宏：`FTS5_DETAIL_NONE`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`MIN`
- 大写宏：`NULL`
- 大写宏：`REPLACE`
- 大写宏：`SQLITE_ABORT`
- 大写宏：`SQLITE_CORRUPT`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_PREPARE_NO_VTAB`
- 大写宏：`SQLITE_PREPARE_PERSISTENT`
- 大写宏：`SQLITE_STATIC`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`All`
- 外部类型：`Also`
- 外部类型：`Append`
- 外部类型：`Call`
- 外部类型：`Check`
- 外部类型：`Context`
- 外部类型：`Current`
- 外部类型：`Doclist`
- 外部类型：`Error`
- 外部类型：`Fts5Buffer`
- 外部类型：`Fts5Config`
- 外部类型：`Fts5Data`
- 外部类型：`Fts5DlidxWriter`
- 外部类型：`Fts5Hash`
- 外部类型：`Fts5Index`
- 外部类型：`Fts5Iter`
- 外部类型：`Fts5PageWriter`
- 外部类型：`Fts5SegIter`
- 外部类型：`Fts5SegWriter`
- 外部类型：`Fts5Structure`
- 外部类型：`Hash`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Increase`
- 外部类型：`Increment`
- 外部类型：`Index`
- 外部类型：`Initialize`
- 外部类型：`Leaf`
- 外部类型：`Max`
- 外部类型：`Name`
- 外部类型：`No`
- 外部类型：`Not`
- 外部类型：`Number`
- 外部类型：`Poslist`
- 外部类型：`Push`
- 外部类型：`Read`
- 外部类型：`Rowid`
- 外部类型：`Set`
- 外部类型：`State`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Total`
- 外部类型：`True`
- 外部类型：`Variables`
- 外部类型：`Virtual`
- 外部类型：`Write`
- 外部类型：`Writer`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-39518ed08e` → 本草稿移入 `cases/defect/auto-sqlite-39518ed08e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
