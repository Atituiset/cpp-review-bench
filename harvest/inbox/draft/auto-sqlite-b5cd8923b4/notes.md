# auto-sqlite-b5cd8923b4

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#012589ecabb5debd4b9a37e96da027e4f41b8838](https://github.com/sqlite/sqlite/commit/012589ecabb5debd4b9a37e96da027e4f41b8838) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 121 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #012589ecabb5debd4b9a37e96da027e4f41b8838 (https://github.com/sqlite/sqlite/commit/012589ecabb5debd4b9a37e96da027e4f41b8838)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 112；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 012589ecabb5debd4b9a37e96da027e4f41b8838 Bring the tree-tracker branch up-to-date with trunk. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -52,7 +52,7 @@
 /*
 ** Software version. Returned by "SELECT vec1_info()".
 */
-#define VEC1_VERSION "0.7"
+#define VEC1_VERSION "0.8"
 
 /*
 ** Values for Vec1ModelHeader.iVersion:
@@ -105,11 +105,11 @@ typedef char            i8;
 
 #if !defined(VEC1_TEST_SCALAR)
 
-#define VERTICAL_SUM_GENERIC(aAcc, nAcc, I) {                          \
-  int c;                                                               \
-  for(c=4; c<nAcc; c++) aAcc[c & 3] = I (aAcc[c & 3], aAcc[c]);        \
-  for(c=2; c<4 && c<nAcc; c++) aAcc[c & 1] = I (aAcc[c & 1], aAcc[c]); \
-  if( nAcc>1 ) aAcc[0] = I (aAcc[0], aAcc[1]);                         \
+#define VERTICAL_SUM_GENERIC(aAcc, nAcc, I) {                                 \
+  int dd;                                                                     \
+  for(dd=4; dd<nAcc; dd++) aAcc[dd & 3] = I (aAcc[dd & 3], aAcc[dd]);         \
+  for(dd=2; dd<4 && dd<nAcc; dd++) aAcc[dd & 1] = I (aAcc[dd & 1], aAcc[dd]); \
+  if( nAcc>1 ) aAcc[0] = I (aAcc[0], aAcc[1]);                                \
 }
 
 #ifdef __AVX2__
@@ -2098,7 +2098,7 @@ static u32 vec1RandomNext(Vec1Random *p) {
   p->state = old * 6364136223846793005ULL + p->inc;
   xorshifted = (u32)(((old >> 18u) ^ old) >> 27u);
   rot = (u32)(old >> 59u);
-  return ((xorshifted >> rot) | (xorshifted << ((-rot) & 31))) & 0x7FFFFFFF;
+  return ((xorshifted >> rot) | (xorshifted << ((32 - rot) & 31))) & 0x7FFFFFFF;
 }
 
 static void vec1RandomInit(Vec1Random *p, u64 seed, u64 id){
@@ -2138,7 +2138,7 @@ static void vec1KMeansInitWork(void *pCtx){
     float *vec = vec1TrainingVector(p->pVec, ii);
     for(jj=p->iFirstNew; jj<p->nCentroid; jj++){
       float *cent = &p->aCentroid[jj * nElem];
-      float dist = vec1L2Dist(vec, cent, nElem);
+      float dist = (float)vec1L2Dist(vec, cent, nElem);
       if( jj==0 || dist<p->aMin[ii] ){
         p->aMin[ii] = dist;
         if( p->aBest ) p->aBest[ii] = jj;
@@ -2256,7 +2256,7 @@ static int vec1KMeansInitOne(
     /* Choose new centroids */
     if( nNewCentroid==1 ){
       /* KMeans ++ case */
-      float fTarget = (fTotalMin * vec1RandomNext(&rnd)) / 0x7FFFFFFF;
+      float fTarget = (float)((fTotalMin * vec1RandomNext(&rnd)) / 0x7FFFFFFF);
       for(vv=0; vv<pVec->nVec-1; vv++){
         fTarget -= (aMin[vv] * (aWeight ? aWeight[vv] : 1));
         if( fTarget<=0.0 ) break;
@@ -2504,7 +2504,7 @@ static void vec1BestMatchTransposeN(
   const int nWidth = VEC1_NTRANSPOSE_WIDTH;
   float aDist[VEC1_MULTIMATCH_NVEC];
   int aBest[VEC1_MULTIMATCH_NVEC];
-  int vv;
+  int vv;                         /* For looping 0..VEC1_MULTIMATCH_NVEC-1 */
   int iCent;
   const float *pTrans = aCentNT;
 
@@ -2518,7 +2518,6 @@ static void vec1BestMatchTransposeN(
   for(iCent=0; iCent<=(nCent-nWidth); iCent+=nWidth){
     __m256 aAcc[VEC1_MULTIMATCH_NVEC];
     int d;
-    int vv;                       /* For looping 0..VEC1_MULTIMATCH_NVEC-1 */
 
     for(vv=0; vv<VEC1_MULTIMATCH_NVEC; vv++){
       aAcc[vv] = _mm256_setzero_ps();
@@ -2560,7 +2559,7 @@ static void vec1BestMatchTransposeN(
   for( ; iCent<nCent; iCent++){
     const float *cent = &aCent[iCent*nElem];
     for(vv=0; vv<VEC1_MULTIMATCH_NVEC; vv++){
-      float dist = vec1L2Dist(aVec[vv], cent, nElem);
+      float dist = (float)vec1L2Dist(aVec[vv], cent, nElem);
       if( dist<aDist[vv] ){
         aDist[vv] = dist;
         aBest[vv] = iCent;
@@ -2616,7 +2615,7 @@ static void vec1BestMatchN(
     for(ii=0; ii<VEC1_MULTIMATCH_NCENT; ii++){
       const float *cent = &aCent[(ii+cc)*nElem];
       for(vv=0; vv<VEC1_MULTIMATCH_NCENT; vv++){
-        float dist = vec1L2Dist(cent, aVec[vv], nElem);
+        float dist = (float)vec1L2Dist(cent, aVec[vv], nElem);
         if( dist<aDist[vv] ){
           aDist[vv] = dist;
           aBest[vv] = ii+cc;
@@ -2632,7 +2631,7 @@ static void vec1BestMatchN(
     int vv;
     const float *cent = &aCent[cc*nElem];
     for(vv=0; vv<VEC1_MULTIMATCH_NVEC; vv++){
-      float dist = vec1L2Dist(aVec[vv], ce
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__volatile__`
- 外部函数：`_mm256_castps256_ps128`
- 外部函数：`_mm256_extractf128_ps`
- 外部函数：`_mm256_fmadd_ps`
- 外部函数：`_mm256_loadu_ps`
- 外部函数：`_mm256_setzero_ps`
- 外部函数：`_mm_add_ps`
- 外部函数：`_mm_cvtss_f32`
- 外部函数：`_mm_hadd_ps`
- 外部函数：`size_of_array`
- 外部函数：`sqlite3_realloc64`
- 外部函数：`sqrt`
- 外部函数：`vaddvq_f32`
- 外部函数：`vdupq_n_f32`
- 外部函数：`vec1_info`
- 外部函数：`vfmaq_f32`
- 外部函数：`vld1q_f32`
- 大写宏：`SELECT`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 大写宏：`VEC1_TEST_SCALAR`
- 外部类型：`Create`
- 外部类型：`Process`
- 外部类型：`Returned`
- 外部类型：`Software`
- 外部类型：`Sum`
- 外部类型：`Then`
- 外部类型：`Values`
- 外部类型：`Vec1Buffer`
- 外部类型：`Vec1ModelHeader`
- 外部类型：`Vec1Tab`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-b5cd8923b4` → 本草稿移入 `cases/defect/auto-sqlite-b5cd8923b4/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
