# auto-sqlite-b28f7347c1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#c8922e6f6b90eda21bf81073496efbccd44a2fe9](https://github.com/sqlite/sqlite/commit/c8922e6f6b90eda21bf81073496efbccd44a2fe9) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 142 |
| 编译错误数（gcc syntax-only） | 15（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c8922e6f6b90eda21bf81073496efbccd44a2fe9 (https://github.com/sqlite/sqlite/commit/c8922e6f6b90eda21bf81073496efbccd44a2fe9)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 7149；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c8922e6f6b90eda21bf81073496efbccd44a2fe9 Merge fixes to vec1 made on trunk into this branch. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -668,7 +668,7 @@ static void vec1HeapInsert(Vec1AnnHeap *p, sqlite3_int64 iRowid, double fDist){
 ** encountered.
 */
 static int vec1BufferGrow(Vec1Buffer *pBuf, sqlite3_int64 nByte){
-  if( pBuf->n+nByte>pBuf->nAlloc ){
+  if( nByte>pBuf->nAlloc-pBuf->n ){
     sqlite3_int64 nNew = pBuf->nAlloc;
     unsigned char *aNew = 0;
 
@@ -2020,7 +2020,7 @@ static float *vec1TrainingVector(Vec1TrainVectors *p, int iVec){
 ** nCode vectors, each of nCodeElem elements. Return the index of the best
 ** match for aSub in aCodebook[].
 */
-static int vec1PqBestMatch(
+static int vec1BestMatchSimple(
   const float *aCodebook,         /* Codebook of nCodeElem element vectors */
   int nCode,                      /* Number of vectors in aCodebook */
   const float *aSub,              /* nCodeElem element vector */
@@ -2044,6 +2044,21 @@ static int vec1PqBestMatch(
   return iBest;
 }
 
+/*
+** Relative tolerance used by vec1BucketIsNearTie().
+*/
+#define VEC1_BUCKET_TOLERANCE 0.00001
+
+/*
+** fBest is the distance from a vector to its nearest IVF centroid, as
+** found by vec1BestMatchSimple(). fDist is the distance from the same
+** vector to some other centroid. Return true if the two distances are
+** close enough that rounding differences could account for the difference.
+*/
+static int vec1BucketIsNearTie(double fBest, double fDist){
+  return (fDist - fBest) <= (fBest * VEC1_BUCKET_TOLERANCE);
+}
+
 /*
 ** Return a pseudo-random positive 32-bit value.
 */
@@ -2698,10 +2713,10 @@ static void vec1LloydsWork(void *pCtx){
     }
 
 #if 0
-    /* Check (assert) using vec1PqBestMatch() that the fancy code above
+    /* Check (assert) using vec1BestMatchSimple() that the fancy code above
     ** actually worked.  */
     for(vv=0; vv<VEC1_MULTIMATCH_NVEC; vv++){
-      int iBest2 = vec1PqBestMatch(
+      int iBest2 = vec1BestMatchSimple(
           p->aCentroid, p->nK, aVec[vv], nElem, 0
       );
       if( iBest2!=aBest[vv] ){
@@ -2722,7 +2737,9 @@ static void vec1LloydsWork(void *pCtx){
   ** part of a batch of VEC1_MULTIMATCH_NVEC. */
   for(; ii<p->iEof; ii++){
     const float *aSub = vec1TrainingVector(pVec, ii);
-    int iBest = vec1PqBestMatch(p->aCentroid, p->nK, aSub, nElem, &p->fTotal);
+    int iBest = vec1BestMatchSimple(
+        p->aCentroid, p->nK, aSub, nElem, &p->fTotal
+    );
     assert( iBest>=0 && iBest<p->nK );
     p->aCount[iBest]++;
     vec1AddInPlace(&p->aSum[iBest * nElem], aSub, nElem);
@@ -4418,7 +4435,7 @@ static void vec1CovarianceWork(void *pArg){
   for(; ii<nVec; ii++){
     float *aVec = vec1TrainingVector(pJob->pVec, ii);
     const float *aSub = &aVec[iSubOff];
-    int iBest = vec1PqBestMatch(
+    int iBest = vec1BestMatchSimple(
         aCode, VEC1_PQ_CODEBOOK_SZ, aSub, pJob->nCodeElem, &pJob->fTotalDist
     );
     float *aSubHat = &aCode[iBest * pJob->nCodeElem];
@@ -4882,7 +4899,7 @@ static void vec1TrainResidualWorker(void *pCtx){
 
   for(ii=p->iFirst; ii<p->iEof; ii++){
     float *a = vec1TrainingVector(p->pVec, ii);
-    int iBest = vec1PqBestMatch(p->aCentroid, p->nCentroid, a, nElem, 0);
+    int iBest = vec1BestMatchSimple(p->aCentroid, p->nCentroid, a, nElem, 0);
     vec1SubInPlace(a, &p->aCentroid[iBest * nElem], nElem);
   }
 }
@@ -5421,6 +5438,11 @@ static void vec1ModelFreeTCent(Vec1Model *pMod){
 #define VEC1_META_4BYTEMAX   +2147483646
 #define VEC1_META_4BYTENULL  +2147483647
 
+/* When a meta-value array is loaded from disk, at least VEC1_META_PADDING
+** extra bytes of memory are allocated. This is to allow readers to safely
+** overread the record slightly if it is corrupt.  */
+#define VEC1_META_PADDING 20
+
 typedef struct Vec1Tab Vec1Tab;
 typedef struct Vec1Csr Vec1Csr;
 
@@ -7020,13 +7042,21 @@ static int vec1PutVarint(u8 *aBuf, u64 v){
 }
 
 static int vec1GetVarint(const u8 *aBuf, u64 *piVal){
-  int nRet = 0;
   u64 out = 0;
-  do {
-    out = (out<<7) + (aBuf[nRet] & 0x7F);
-  } while( aBuf[nRet++] & 0x80 );
+  int ii = 0;
+
+  out = (aBuf[0] & 0x7F);
+  if(
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__volatile__`
- 外部函数：`_mm256_add_ps`
- 外部函数：`_mm256_castps256_ps128`
- 外部函数：`_mm256_extractf128_ps`
- 外部函数：`_mm256_fmadd_ps`
- 外部函数：`_mm256_loadu_ps`
- 外部函数：`_mm256_set1_ps`
- 外部函数：`_mm256_setzero_ps`
- 外部函数：`_mm256_storeu_ps`
- 外部函数：`_mm256_sub_ps`
- 外部函数：`_mm_add_ps`
- 外部函数：`_mm_cvtss_f32`
- 外部函数：`_mm_hadd_ps`
- 外部函数：`defined`
- 外部函数：`length`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`sqlite3_bind_int`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_blob_close`
- 外部函数：`sqlite3_blob_open`
- 外部函数：`sqlite3_blob_read`
- 外部函数：`sqlite3_column_blob`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_column_int`
- 外部函数：`sqlite3_column_int64`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_reset`
- 外部函数：`sqlite3_step`
- 外部函数：`sqlite3_value_type`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`sqrt`
- 外部函数：`sqrtf`
- 外部函数：`sum`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vaddq_f32`
- 外部函数：`vaddvq_f32`
- 外部函数：`vdupq_n_f32`
- 外部函数：`vec1AddInPlaceScalar`
- 外部函数：`vec1CorruptError`
- 外部函数：`vec1FloatWithinTolerance`
- 外部函数：`vec1MetaValueCheck`
- 外部函数：`vec1WriterMetaFromPacked`
- 外部函数：`vfmaq_f32`
- 外部函数：`vld1q_f32`
- 外部函数：`vmlaq_f32`
- 外部函数：`vst1q_f32`
- 外部函数：`vsubq_f32`
- 大写宏：`AND`
- 大写宏：`BETWEEN`
- 大写宏：`CONFLICT`
- 大写宏：`DELETE`
- 大写宏：`FROM`
- 大写宏：`INFINITY`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`LIMIT`
- 大写宏：`NDEBUG`
- 大写宏：`NULL`
- 大写宏：`ORDER`
- 大写宏：`OUT`
- 大写宏：`REPLACE`
- 大写宏：`RETURNING`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SQLITE_BLOB`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_FLOAT`
- 大写宏：`SQLITE_INTEGER`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_NULL`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`SQLITE_TEXT`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`All`
- 外部类型：`Allocate`
- 外部类型：`Also`
- 外部类型：`Bucket`
- 外部类型：`Buffer`
- 外部类型：`But`
- 外部类型：`Check`
- 外部类型：`Codebook`
- 外部类型：`Create`
- 外部类型：`Delete`
- 外部类型：`Distance`
- 外部类型：`Encode`
- 外部类型：`Encoded`
- 外部类型：`Entry`
- 外部类型：`Error`
- 外部类型：`Find`
- 外部类型：`For`
- 外部类型：`Id`
- 外部类型：`If`
- 外部类型：`Index`
- 外部类型：`Input`
- 外部类型：`Integrity`
- 外部类型：`Iterator`
- 外部类型：`Load`
- 外部类型：`Meta`
- 外部类型：`Model`
- 外部类型：`Now`
- 外部类型：`Number`
- 外部类型：`Or`
- 外部类型：`Populate`
- 外部类型：`Process`
- 外部类型：`Read`
- 外部类型：`Real`
- 外部类型：`Result`
- 外部类型：`Return`
- 外部类型：`Row`
- 外部类型：`Rowid`
- 外部类型：`Set`
- 外部类型：`Size`
- 外部类型：`So`
- 外部类型：`Space`
- 外部类型：`Sum`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Use`
- 外部类型：`Used`
- 外部类型：`Vec1`
- 外部类型：`Vec1Buffer`
- 外部类型：`Vec1Csr`
- 外部类型：`Vec1Filter`
- 外部类型：`Vec1ListBuilder`
- 外部类型：`Vec1MetaBuilder`
- 外部类型：`Vec1MetaValue`
- 外部类型：`Vec1Model`
- 外部类型：`Vec1RowidLocation`
- 外部类型：`Vec1Tab`
- 外部类型：`Vec1TrainVectors`
- 外部类型：`Vec1Writer`
- 外部类型：`Vector`
- 外部类型：`Writer`
- 外部类型：`float32x4_t`

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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-b28f7347c1` → 本草稿移入 `cases/defect/auto-sqlite-b28f7347c1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
