# auto-sqlite-25840a632f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#31e3b6ff459658c156855c9a9761370b5d02e923](https://github.com/sqlite/sqlite/commit/31e3b6ff459658c156855c9a9761370b5d02e923) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 95 |
| 编译错误数（gcc syntax-only） | 34（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #31e3b6ff459658c156855c9a9761370b5d02e923 (https://github.com/sqlite/sqlite/commit/31e3b6ff459658c156855c9a9761370b5d02e923)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 12321；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 31e3b6ff459658c156855c9a9761370b5d02e923 Update vec1 integrity-check method to tolerate small rounding errors when checking that a vector has been assigned to th :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2020,7 +2020,7 @@ static float *vec1TrainingVector(Vec1TrainVectors *p, int iVec){
 ** nCode vectors, each of nCodeElem elements. Return the index of the best
 ** match for aSub in aCodebook[].
 */
-static int vec1PqBestMatch(
+static int vec1BestMatchSimple(
   const float *aCodebook,         /* Codebook of nCodeElem element vectors */
   int nCode,                      /* Number of vectors in aCodebook */
   const float *aSub,              /* nCodeElem element vector */
@@ -2698,10 +2698,10 @@ static void vec1LloydsWork(void *pCtx){
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
@@ -2722,7 +2722,9 @@ static void vec1LloydsWork(void *pCtx){
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
@@ -4418,7 +4420,7 @@ static void vec1CovarianceWork(void *pArg){
   for(; ii<nVec; ii++){
     float *aVec = vec1TrainingVector(pJob->pVec, ii);
     const float *aSub = &aVec[iSubOff];
-    int iBest = vec1PqBestMatch(
+    int iBest = vec1BestMatchSimple(
         aCode, VEC1_PQ_CODEBOOK_SZ, aSub, pJob->nCodeElem, &pJob->fTotalDist
     );
     float *aSubHat = &aCode[iBest * pJob->nCodeElem];
@@ -4882,7 +4884,7 @@ static void vec1TrainResidualWorker(void *pCtx){
 
   for(ii=p->iFirst; ii<p->iEof; ii++){
     float *a = vec1TrainingVector(p->pVec, ii);
-    int iBest = vec1PqBestMatch(p->aCentroid, p->nCentroid, a, nElem, 0);
+    int iBest = vec1BestMatchSimple(p->aCentroid, p->nCentroid, a, nElem, 0);
     vec1SubInPlace(a, &p->aCentroid[iBest * nElem], nElem);
   }
 }
@@ -8091,7 +8093,7 @@ static void vec1EncodeVectorPQSimple(
   for(ii=0; ii<nCodebook; ii++){
     const float *pCodebook = &pMod->aModel[ii*nCodeElem*VEC1_PQ_CODEBOOK_SZ];
     const float *aSub = &aVec[ii*nCodeElem];
-    aOut[ii] = (u8)vec1PqBestMatch(
+    aOut[ii] = (u8)vec1BestMatchSimple(
         pCodebook, VEC1_PQ_CODEBOOK_SZ, aSub, nCodeElem, pfTotalDist
     );
   }
@@ -9525,7 +9527,7 @@ static void vec1DistanceStats(sqlite3_context *ctx, Vec1Csr *pCsr){
   
       /* Find bucket and coarse error if applicable. */
       if( pMod->hdr.nBucket>1 ){
-        iBucket = vec1PqBestMatch(
+        iBucket = vec1BestMatchSimple(
             pMod->aCentroid, pMod->hdr.nBucket, aVec, nElem, &fCoarseError
         );
         if( aTransform!=pTab->aTmpVec ){
@@ -10417,7 +10419,7 @@ static void vec1QuantizeVector(
   aVec = vec1TransformInputVector(pMod, aTmp, aVector);
   if( nBucket>0 ){
     int nElem = pMod->hdr.nElem;
-    iBucket = vec1PqBestMatch(pMod->aCentroid, nBucket, aVec, nElem, 0);
+    iBucket = vec1BestMatchSimple(pMod->aCentroid, nBucket, aVec, nElem, 0);
     if( pMod->hdr.nCodebook>0 && (pMod->hdr.flags & VEC1_MODEL_RESIDUAL) ){
       vec1Sub(aTmp, aVec, &pMod->aCentroid[iBucket*nElem], nElem);
       aVec = aTmp;
@@ -11552,6 +11554,9 @@ static int vec1DeleteByRowid(Vec1Tab *pTab, i64 iRowid){
   rc = vec1GetSql(pTab, VEC1_SQL_DEL_LOOKUP_BASE, &pDelete);
   if( rc==SQLITE_OK ){
     int iBucket = 0;
+
+    /* Delete the row from the %_base table. Also set variable iBucket to
+    ** the index of the bucket containing the vector.  */
     sqlite3_bind_int64(pDelete, 1, iRowid);
     if( SQLITE_ROW==sqlite3_step(pDelete) && pTab->mod.hdr.nBucket>0 ){
       if( pTab->mod.hdr.nCodebook==0 ){
@@ -11568,7 +11573,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

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
- 外部函数：`assert`
- 外部函数：`defined`
- 外部函数：`length`
- 外部函数：`memcmp`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`size_of_array`
- 外部函数：`sqlite3_bind_int64`
- 外部函数：`sqlite3_column_blob`
- 外部函数：`sqlite3_column_bytes`
- 外部函数：`sqlite3_errmsg`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_mprintf`
- 外部函数：`sqlite3_prepare_v2`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqlite3_step`
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
- 大写宏：`GENERIC`
- 大写宏：`INFINITY`
- 大写宏：`INSERT`
- 大写宏：`INTO`
- 大写宏：`LIMIT`
- 大写宏：`ORDER`
- 大写宏：`OUT`
- 大写宏：`REPLACE`
- 大写宏：`RETURNING`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`SQLITE_ROW`
- 大写宏：`UPDATE`
- 大写宏：`VALUES`
- 大写宏：`WHERE`
- 外部类型：`Allocate`
- 外部类型：`Buffer`
- 外部类型：`Check`
- 外部类型：`Codebook`
- 外部类型：`Create`
- 外部类型：`Distance`
- 外部类型：`Encode`
- 外部类型：`Encoded`
- 外部类型：`Entry`
- 外部类型：`Error`
- 外部类型：`Find`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Input`
- 外部类型：`Model`
- 外部类型：`Number`
- 外部类型：`Populate`
- 外部类型：`Process`
- 外部类型：`Read`
- 外部类型：`Return`
- 外部类型：`Set`
- 外部类型：`Size`
- 外部类型：`Space`
- 外部类型：`Sum`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`This`
- 外部类型：`Used`
- 外部类型：`Vec1Buffer`
- 外部类型：`Vec1Model`
- 外部类型：`Vec1Tab`
- 外部类型：`Vec1TrainVectors`
- 外部类型：`Vector`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-25840a632f` → 本草稿移入 `cases/defect/auto-sqlite-25840a632f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
