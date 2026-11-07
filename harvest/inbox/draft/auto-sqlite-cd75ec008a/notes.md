# auto-sqlite-cd75ec008a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | sqlite/sqlite |
| 源 PR | [#52f43226157d862f96532f1bad7292ad71641693](https://github.com/sqlite/sqlite/commit/52f43226157d862f96532f1bad7292ad71641693) |
| 许可证 | Public-Domain |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 81 |
| 编译错误数（gcc syntax-only） | 72（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #52f43226157d862f96532f1bad7292ad71641693 (https://github.com/sqlite/sqlite/commit/52f43226157d862f96532f1bad7292ad71641693)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 2016；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 52f43226157d862f96532f1bad7292ad71641693 In vec1, avoid fixed buffers and 32-bit arithmatic when loading training vectors. Report [bugs:/info/2026-11-06T17:58:29 :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1926,11 +1926,9 @@ static int vec1JobQueueAddJob(
 
 /*
 ** When accumulating training data, allocate space in chunks of 
-** VEC1_TRAINING_SZCHUNK. Allow at most VEC1_TRAINING_MAXCHUNK chunks of
-** training data. With default values, the limit is 64GiB of training
-** vectors.
+** VEC1_TRAINING_SZCHUNK. The number of chunks is limited only by available
+** memory.
 */
-#define VEC1_TRAINING_MAXCHUNK 1024
 #define VEC1_TRAINING_SZCHUNK  (32*1024*1024)
 
 /*
@@ -1948,10 +1946,10 @@ static int vec1JobQueueAddJob(
 typedef struct Vec1TrainVectors Vec1TrainVectors;
 struct Vec1TrainVectors {
   int nElem;                      /* Number of elements in each vector */
-  int nVec;                       /* Number of vectors in chunk array */
-  int nChunk;                     /* Number of chunks allocated so far */
-  int nVecPerChunk;               /* Number of vectors in each chunk */
-  float *aChunk[VEC1_TRAINING_MAXCHUNK];
+  i64 nVec;                       /* Number of vectors in chunk array */
+  i64 nChunk;                     /* Number of chunks allocated so far */
+  i64 nVecPerChunk;               /* Number of vectors in each chunk */
+  float **aChunk;                 /* Array of nChunk allocated chunks */
 };
 
 /*
@@ -2013,9 +2011,9 @@ struct Vec1TrainCtx {
 ** Return a pointer to the requested training vector. The results are
 ** undefined if parameter iVec is out of range.
 */
-static float *vec1TrainingVector(Vec1TrainVectors *p, int iVec){
-  int iChunk = iVec / p->nVecPerChunk;
-  int iOff = (iVec % p->nVecPerChunk) * p->nElem;
+static float *vec1TrainingVector(Vec1TrainVectors *p, i64 iVec){
+  i64 iChunk = iVec / p->nVecPerChunk;
+  i64 iOff = (iVec % p->nVecPerChunk) * p->nElem;
   return &((p->aChunk[iChunk])[iOff]);
 }
 
@@ -2086,8 +2084,8 @@ typedef struct LloydsJob LloydsJob;
 struct LloydsJob {
   /* Data to train on */
   Vec1TrainVectors *pVec;
-  int iFirst;                     /* First vector for this job */
-  int iEof;                       /* Index of last vector +1 for this job */
+  i64 iFirst;                     /* First vector for this job */
+  i64 iEof;                       /* Index of last vector +1 for this job */
 
   int nK;                         /* Number of centroids */
   const float *aCentroid;         /* Centroids */
@@ -2098,7 +2096,7 @@ struct LloydsJob {
   /* Outputs */
   double fTotal;
   float *aSum;
-  int *aCount;
+  i64 *aCount;
 };
 
 /*
@@ -2131,8 +2129,8 @@ static void vec1RandomInit(Vec1Random *p, u64 seed, u64 id){
 
 typedef struct KMeansInitJob KMeansInitJob;
 struct KMeansInitJob {
-  int iFirst;                     /* First vector for this job */
-  int iEof;                       /* Index of last vector +1 for this job */
+  i64 iFirst;                     /* First vector for this job */
+  i64 iEof;                       /* Index of last vector +1 for this job */
   double fTotalMin;               /* Total of aMin[iFirst..iEof-1] */
 
   Vec1TrainVectors *pVec;
@@ -2150,7 +2148,7 @@ static void vec1KMeansInitWork(void *pCtx){
   KMeansInitJob *p = (KMeansInitJob*)pCtx;
   const int nElem = p->pVec->nElem;
   double fTotalMin = 0.0;
-  int ii;
+  i64 ii;
 
   for(ii=p->iFirst; ii<p->iEof; ii++){
     int jj;
@@ -2177,8 +2175,8 @@ static void vec1KMeansInitWork(void *pCtx){
 **   struct XyzJob {
 **     ...
 **     Vec1TrainVectors *pVec;    // Training vectors to use a subset of
-**     int iFirst;                // index of first vector in pVec to use
-**     int iEof;                  // 1 past index of last vector in pVec to use
+**     i64 iFirst;                // index of first vector in pVec to use
+**     i64 iEof;                  // 1 past index of last vector in pVec to use
 **     ...
 **   };
 */
@@ -2208,7 +2206,7 @@ static int vec1KMeansInitOne(
 
   Vec1Random rnd;
 
-  int nByte;                      /* Bytes of space to allocate */
+  i64 nByte;                      /* Bytes of space to allocate */
   KMeansInitJob *aJob = 0;
   float *aMin
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_mm256_castps256_ps128`
- 外部函数：`_mm256_extractf128_ps`
- 外部函数：`_mm256_fmadd_ps`
- 外部函数：`_mm256_loadu_ps`
- 外部函数：`_mm256_setzero_ps`
- 外部函数：`_mm_add_ps`
- 外部函数：`_mm_cvtss_f32`
- 外部函数：`_mm_hadd_ps`
- 外部函数：`assert`
- 外部函数：`memset`
- 外部函数：`printf`
- 外部函数：`pthread_cond_broadcast`
- 外部函数：`pthread_cond_destroy`
- 外部函数：`pthread_cond_wait`
- 外部函数：`pthread_join`
- 外部函数：`pthread_mutex_destroy`
- 外部函数：`pthread_mutex_lock`
- 外部函数：`pthread_mutex_unlock`
- 外部函数：`sqlite3_free`
- 外部函数：`sqlite3_malloc`
- 外部函数：`sqlite3_malloc64`
- 外部函数：`sqlite3_result_error`
- 外部函数：`sqlite3_result_error_nomem`
- 外部函数：`sqlite3_vmprintf`
- 外部函数：`sqrt`
- 外部函数：`va_end`
- 外部函数：`va_start`
- 外部函数：`vaddvq_f32`
- 外部函数：`vdupq_n_f32`
- 外部函数：`vector`
- 外部函数：`vectors`
- 外部函数：`vfmaq_f32`
- 外部函数：`vld1q_f32`
- 外部函数：`xFinish`
- 外部函数：`xWork`
- 大写宏：`NULL`
- 大写宏：`OPQ`
- 大写宏：`SQLITE_ERROR`
- 大写宏：`SQLITE_NOMEM`
- 大写宏：`SQLITE_OK`
- 外部类型：`Allocate`
- 外部类型：`Allow`
- 外部类型：`Bytes`
- 外部类型：`Centroids`
- 外部类型：`Context`
- 外部类型：`Create`
- 外部类型：`Current`
- 外部类型：`Data`
- 外部类型：`Each`
- 外部类型：`Either`
- 外部类型：`First`
- 外部类型：`If`
- 外部类型：`Index`
- 外部类型：`Initialize`
- 外部类型：`Input`
- 外部类型：`LloydsJob`
- 外部类型：`Loop`
- 外部类型：`Normalize`
- 外部类型：`Number`
- 外部类型：`Output`
- 外部类型：`Outputs`
- 外部类型：`Process`
- 外部类型：`Return`
- 外部类型：`So`
- 外部类型：`Step`
- 外部类型：`Sum`
- 外部类型：`The`
- 外部类型：`Then`
- 外部类型：`Total`
- 外部类型：`Trailing`
- 外部类型：`Training`
- 外部类型：`Try`
- 外部类型：`Vec1CodebookJob`
- 外部类型：`Vec1Cond`
- 外部类型：`Vec1CovarianceJob`
- 外部类型：`Vec1Job`
- 外部类型：`Vec1JobQueue`
- 外部类型：`Vec1Mutex`
- 外部类型：`Vec1Random`
- 外部类型：`Vec1ResidualJob`
- 外部类型：`Vec1RotationJob`
- 外部类型：`Vec1Thread`
- 外部类型：`Vec1TrainCtx`
- 外部类型：`When`
- 外部类型：`With`
- 外部类型：`XyzJob`
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

1. 完成上面检查清单后评论 `/case accept auto-sqlite-cd75ec008a` → 本草稿移入 `cases/defect/auto-sqlite-cd75ec008a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
