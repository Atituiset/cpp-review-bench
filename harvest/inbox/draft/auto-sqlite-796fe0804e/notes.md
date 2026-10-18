# auto-sqlite-796fe0804e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#43a48f77585a88940c727f33682151418f544059](https://github.com/sqlite/sqlite/commit/43a48f77585a88940c727f33682151418f544059) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 207 |
| 编译错误数（gcc syntax-only） | 234（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #43a48f77585a88940c727f33682151418f544059 (https://github.com/sqlite/sqlite/commit/43a48f77585a88940c727f33682151418f544059)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 362；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 43a48f77585a88940c727f33682151418f544059 Fix uses of C99 and other minor issues in ext/vec1/vec1.c. Report [bugs:/info/2026-10-17T09:17:48Z | 2026-10-17T09:17:48 :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -93,6 +93,11 @@
 # include <stdio.h>
 # include <stdlib.h>
 
+/* INFINITY is C99. Fall back to HUGE_VAL for C89 compilers. */
+#ifndef INFINITY
+# define INFINITY HUGE_VAL
+#endif
+
 /* Standard sized types. */
 typedef sqlite3_int64  i64;
 typedef sqlite3_uint64 u64;
@@ -359,7 +364,7 @@ struct Vec1ModelHeader {
 */
 #define VEC1_MAX_NTHREAD 128
 
-static int vec1CorruptError(){
+static int vec1CorruptError(void){
   return SQLITE_CORRUPT_VTAB;
 }
 #define VEC1_CORRUPT vec1CorruptError();
@@ -807,13 +812,12 @@ static void vec1FromJsonFunc(
   const char *zJson = (const char*)sqlite3_value_text(aVal[0]);
   int nJson = sqlite3_value_bytes(aVal[0]);
   const char *p = zJson;
+  float *aVec = 0;
+  int nVec = 0;
 
   assert( nVal==1 );
   UNUSED_PARAMETER(nVal);
 
-  float *aVec = 0;
-  int nVec = 0;
-
   /* vec1_from_json(NULL) returns NULL */
   if( sqlite3_value_type(aVal[0])==SQLITE_NULL ) return;
 
@@ -2062,7 +2066,7 @@ static int vec1BucketIsNearTie(double fBest, double fDist){
 /*
 ** Return a pseudo-random positive 32-bit value.
 */
-static int vec1Rand31(){
+static int vec1Rand31(void){
   int val;
   sqlite3_randomness(sizeof(val), (void*)&val);
   return (val & 0x7FFFFFFF);
@@ -2110,7 +2114,7 @@ struct Vec1Random {
 static u32 vec1RandomNext(Vec1Random *p) {
   u32 xorshifted, rot;
   u64 old = p->state;
-  p->state = old * 6364136223846793005ULL + p->inc;
+  p->state = old * ((((u64)0x5851F42D)<<32)|0x4C957F2D) + p->inc;
   xorshifted = (u32)(((old >> 18u) ^ old) >> 27u);
   rot = (u32)(old >> 59u);
   return ((xorshifted >> rot) | (xorshifted << ((32 - rot) & 31))) & 0x7FFFFFFF;
@@ -2210,7 +2214,6 @@ static int vec1KMeansInitOne(
   int *aBest = 0;
   int ii;                         /* Loop counter for per-job loop */
   int rr;                         /* Loop counter for per-round loop */
-  int iNext;
   int nCentroid = 0;
   int nNewCentroid = 0;
   float *vec = 0;
@@ -2230,7 +2233,7 @@ static int vec1KMeansInitOne(
     aBest = (int*)&aMin[pVec->nVec];
   }
 
-  for(ii=0, iNext=0; ii<nJob; ii++){
+  for(ii=0; ii<nJob; ii++){
     KMeansInitJob *pJob = &aJob[ii];
     ASSIGN_VECTORS_TO_JOB(pVec, pJob, ii, nJob);
     pJob->aMin = aMin;
@@ -2308,9 +2311,11 @@ static int vec1KMeansInitOne(
   return SQLITE_OK;
 }
 
+
 /*
 ** Implementation of KMeans || initialization.
 */
+#if 0
 static int vec1KMeansParallel(
   Vec1TrainVectors *pVec,
   Vec1JobQueue *pQueue,           /* Job queue to use (NULL -> 1 thread) */
@@ -2343,6 +2348,7 @@ static int vec1KMeansParallel(
 
   return rc;
 }
+#endif
 
 static int vec1KMeansPlusPlus(
   Vec1TrainVectors *pVec,
@@ -2357,8 +2363,10 @@ static int vec1KMeansPlusPlus(
 
 #if defined(_MSC_VER)
 # define VEC1_NOINLINE __declspec(noinline)
-#else
+#elif defined(__GNUC__)
 # define VEC1_NOINLINE __attribute__((noinline))
+#else
+# define VEC1_NOINLINE
 #endif
 
 static VEC1_NOINLINE void vec1AddInPlaceScalar(
@@ -2398,7 +2406,7 @@ static void vec1AddInPlace(float * a1, const float * a2, int n){
 }
 
 #ifdef VEC1_HAVE_NEON
-static inline void vec1MultiBestMatch(
+static void vec1MultiBestMatch(
   int nElem,                      /* Size of vectors in elements */
   int iCentroid,                  /* Centroid offset */
   const float *aVec[VEC1_MULTIMATCH_NVEC],
@@ -3340,11 +3348,11 @@ static void vec1TrainStep(
 ){
   Vec1TabList *pList = (Vec1TabList*)sqlite3_user_data(pCtx);
   Vec1TrainCtx *p = 0;
-  assert( nArg==1 || nArg==2 );
   int n = 0;
   const u8 *a = 0;
   int iChunk = 0;
 
+  assert( nArg==1 || nArg==2 );
   p = (Vec1TrainCtx*)sqlite3_aggregate_context(pCtx, sizeof(*p));
   if( !p ) return;
 
@@ -3601,8 +3609,9 @@ static void vec1TrainPQCodebooks(
 }
 
 static void vec1TransposeMatrix(double *aMatrix, int nElem) {
-  for (int i = 0; i < nElem - 1; i++) {
-    for (int j = i + 1; j < nElem; j++) {
+  int i, j;
+  for (i = 0; i < nElem - 1; i++) {
+    for (j = i + 1; j < nElem; j++) {
       double tmp = aMatrix[i * nElem + j];
       aMatrix[i * nElem + j] = aMatrix[j 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__volatile__`
- 外部函数：`_mm256_castpd256_pd128`
- 外部函数：`_mm256_castps256_ps128`
- 外部函数：`_mm256_extractf128_pd`
- 外部函数：`_mm256_extractf128_ps`
- 外部函数：`_mm256_fmadd_ps`
- 外部函数：`_mm256_loadu_ps`
- 外部函数：`_mm256_setzero_ps`
- 外部函数：`_mm256_sub_ps`
- 外部函数：`_mm_add_pd`
- 外部函数：`_mm_add_ps`
- 外部函数：`_mm_cvtsd_f64`
- 外部函数：`_mm_cvtss_f32`
- 外部函数：`_mm_hadd_pd`
- 外部函数：`_mm_hadd_ps`
- 外部函数：`assert`
- 外部函数：`defined`
- 外部函数：`size_of_array`
- 外部函数：`sqlite3_value_bytes`
- 外部函数：`sqlite3_value_text`
- 外部函数：`sqlite3_value_type`
- 外部函数：`vaddvq_f32`
- 外部函数：`vdupq_n_f32`
- 外部函数：`vec1_from_json`
- 外部函数：`vfmaq_f32`
- 外部函数：`vld1q_f32`
- 大写宏：`NULL`
- 大写宏：`SQLITE_CORRUPT_VTAB`
- 大写宏：`SQLITE_NULL`
- 外部类型：`Create`
- 外部类型：`Process`
- 外部类型：`Standard`
- 外部类型：`Sum`
- 外部类型：`Then`
- 外部类型：`Vec1TabList`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-796fe0804e` → 本草稿移入 `cases/defect/auto-sqlite-796fe0804e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
