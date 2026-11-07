// AUTO-DRAFT from sqlite/sqlite PR #52f43226157d862f96532f1bad7292ad71641693
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef sqlite3_int64  i64;
/* …（同文件无关代码省略）… */
typedef unsigned char   u8;
typedef unsigned int   u32;
/* …（同文件无关代码省略）… */
#define VERTICAL_SUM_GENERIC(aAcc, nAcc, I) {                                 \
  int dd;                                                                     \
  for(dd=4; dd<nAcc; dd++) aAcc[dd & 3] = I (aAcc[dd & 3], aAcc[dd]);         \
  for(dd=2; dd<4 && dd<nAcc; dd++) aAcc[dd & 1] = I (aAcc[dd & 1], aAcc[dd]); \
  if( nAcc>1 ) aAcc[0] = I (aAcc[0], aAcc[1]);                                \
}
/* …（同文件无关代码省略）… */
# define VEC1_HAVE_AVX2 1
/* …（同文件无关代码省略）… */
# define VEC1_MULTIMATCH_NVEC  8
/* …（同文件无关代码省略）… */
# define VEC1_USE_TRANSPOSITION 1
/* …（同文件无关代码省略）… */
#  define FMADD(a,b,c) _mm256_fmadd_ps((a),(b),(c))
/* …（同文件无关代码省略）… */
# define LOADU(a) _mm256_loadu_ps(a)
/* …（同文件无关代码省略）… */
# define HORIZONTAL_SUM(s, vec) \
    {                                                    \
      __m128 vc_high = _mm256_extractf128_ps(vec, 1);    \
      __m128 vc_low = _mm256_castps256_ps128(vec);       \
      __m128 vc128 = _mm_add_ps(vc_high, vc_low);        \
      vc128 = _mm_hadd_ps(vc128, vc128);                 \
      vc128 = _mm_hadd_ps(vc128, vc128);                 \
      s = _mm_cvtss_f32(vc128);                          \
    }
/* …（同文件无关代码省略）… */
# define VERTICAL_SUM(aAcc, nAcc) VERTICAL_SUM_GENERIC(aAcc,nAcc,_mm256_add_ps)
/* …（同文件无关代码省略）… */
# define VEC1_HAVE_NEON 1
/* …（同文件无关代码省略）… */
#define MAX(a,b) ((a<b) ? (b) : (a))
#define MIN(a,b) ((a>b) ? (b) : (a))
/* …（同文件无关代码省略）… */
static float vec1DotProduct(
  int d,
  const float *A,
  const float *B
){
  float sum = 0.0f;
  int j = 0;

#ifdef VEC1_HAVE_AVX2
# ifndef VEC1_DOT_NACC
#  define VEC1_DOT_NACC 4
# endif
  const int nAcc = VEC1_DOT_NACC;
  int cc;

  /* Create and initialize to zero nAcc accumulator registers. */
  __m256 acc[VEC1_DOT_NACC];
  for(cc=0; cc<nAcc; cc++) acc[cc] = _mm256_setzero_ps();

  for(; j<=(d-(8*nAcc)); j+=(8*nAcc)){
    for(cc=0; cc<nAcc; cc++){
      acc[cc] = FMADD(LOADU(&A[j+cc*8]), LOADU(&B[j+cc*8]), acc[cc]);
    }
  }

  /* Sum all nAcc accumulators into acc[0]. */
  VERTICAL_SUM(acc, nAcc);

  /* Process the part of the tail that can be done 8 elements at a time
  ** using acc[0] as the accumulator. Then sum all 8 elements of acc[0]
  ** into scalar float "sum".  */
  for(; j<=(d-8); j+=8){
    acc[0] = FMADD(LOADU(&A[j]), LOADU(&B[j]), acc[0]);
  }
  HORIZONTAL_SUM(sum, acc[0]);
#endif
#ifdef VEC1_HAVE_NEON
# ifndef VEC1_DOT_NACC
#  define VEC1_DOT_NACC 8
# endif
  const int nAcc = VEC1_DOT_NACC;
  int cc;

  /* Create and initialize to zero nAcc accumulator registers. */
  float32x4_t acc[VEC1_DOT_NACC];
  for(cc=0; cc<nAcc; cc++) acc[cc] = vdupq_n_f32(0.0f);

  for(; j<=(d-(4*nAcc)); j+=(4*nAcc)){
    for(cc=0; cc<nAcc; cc++){
      float32x4_t a = vld1q_f32(&A[j+cc*4]);
      float32x4_t b = vld1q_f32(&B[j+cc*4]);
      acc[cc] = vfmaq_f32(acc[cc], a, b);
    }
  }

  /* Sum all nAcc accumulators into acc[0]. */
  VERTICAL_SUM(acc, nAcc);

  /* Process the part of the tail that can be done 4 elements at a time
  ** using acc[0] as the accumulator. Then sum all 4 elements of acc[0]
  ** into scalar float "sum".  */
  for(; j<=(d-4); j+=4){
    acc[0] = vfmaq_f32(acc[0], vld1q_f32(&A[j]), vld1q_f32(&B[j]));
  }
  sum = vaddvq_f32(acc[0]);
#endif

  for(; j<d; j++){
    sum += A[j] * B[j];
  }

  return sum;
}
/* …（同文件无关代码省略）… */
static void vec1NormalizeVector(float *aElem, int nElem){
  int ii;
  double fSum = vec1DotProduct(nElem, aElem, aElem);
  if( fSum==0.0 ) return;
  fSum = 1.0 / sqrt(fSum);
  for(ii=0; ii<nElem; ii++){
    aElem[ii] = (float)(aElem[ii] * fSum);
  }
}
/* …（同文件无关代码省略）… */
static void vec1ResultErrorF(
  sqlite3_context *pCtx,          /* Context to set error in */
  const char *zFmt,               /* printf() style format for error message */
  ...                             /* Trailing arguments for printf() */
){
  char *zMsg = 0;
  va_list ap;
  va_start(ap, zFmt);
  zMsg = sqlite3_vmprintf(zFmt, ap);
  va_end(ap);
  sqlite3_result_error(pCtx, zMsg, -1);
  sqlite3_free(zMsg);
}
/* …（同文件无关代码省略）… */
static void *vec1MallocZero(i64 nByte){
  void *pRet = sqlite3_malloc64(nByte);
  if( pRet ){
    memset(pRet, 0, nByte);
  }
  return pRet;
}
/* …（同文件无关代码省略）… */
typedef struct Vec1Thread Vec1Thread;
typedef struct Vec1Mutex Vec1Mutex;
typedef struct Vec1Cond Vec1Cond;
/* …（同文件无关代码省略）… */
static int vec1ThreadJoin(Vec1Thread *pThread, void **ppOut){
  int rc = SQLITE_OK;
  if( pthread_join(pThread->tid, ppOut)!=0 ){
    rc = SQLITE_ERROR;
    if( ppOut ) *ppOut = 0;
  }
  sqlite3_free(pThread);
  return rc;
}
/* …（同文件无关代码省略）… */
static void vec1MutexDestroy(Vec1Mutex *pMutex){
  pthread_mutex_destroy(&pMutex->mutex);
  sqlite3_free(pMutex);
}
/* …（同文件无关代码省略）… */
static void vec1MutexEnter(Vec1Mutex *pMutex){
  pthread_mutex_lock(&pMutex->mutex);
}
/* …（同文件无关代码省略）… */
static void vec1MutexLeave(Vec1Mutex *pMutex){
  pthread_mutex_unlock(&pMutex->mutex);
}
/* …（同文件无关代码省略）… */
static void vec1CondDestroy(Vec1Cond *pCond){
  pthread_cond_destroy(&pCond->cond);
  sqlite3_free(pCond);
}
/* …（同文件无关代码省略）… */
static void vec1CondWait(Vec1Mutex *pMutex, Vec1Cond *pCond){
  pthread_cond_wait(&pCond->cond, &pMutex->mutex);
}
/* …（同文件无关代码省略）… */
static void vec1CondBroadcast(Vec1Cond *pCond){
  pthread_cond_broadcast(&pCond->cond);
}
/* …（同文件无关代码省略）… */
typedef struct Vec1JobQueue Vec1JobQueue;
/* …（同文件无关代码省略）… */
typedef struct Vec1Job Vec1Job;
/* …（同文件无关代码省略）… */
static int vec1JobQueueFinishJobs(Vec1JobQueue *pQueue, int rcin){
  int rc = rcin;

  if( pQueue==0 ) return rc;

  while( 1 ){
    Vec1Job *pFin = 0;
    Vec1Job *pWork = 0;

    /* Try to find a job to help with. If there is nothing, wait on pCond2. */
    vec1MutexEnter(pQueue->pMutex);
    while( pQueue->pFinished==0 && pQueue->pWaiting==0 && pQueue->nJob>0 ){
      vec1CondWait(pQueue->pMutex, pQueue->pCond2);
    }
    if( pQueue->pFinished ){
      pFin = pQueue->pFinished;
      pQueue->pFinished = pFin->pJobNext;
      pQueue->nJob--;
    }else if( pQueue->pWaiting ){
      pWork = pQueue->pWaiting;
      pQueue->pWaiting = pWork->pJobNext;
      pQueue->nJob--;
    }
    vec1MutexLeave(pQueue->pMutex);

    if( pFin==0 && pWork==0 ) break;
    if( pWork ){
      pWork->xWork(pWork->pCtx);
      pFin = pWork;
    }

    {
      int rc2 = pFin->xFinish(pFin->pCtx, rc);
      if( rc==SQLITE_OK ) rc = rc2;
    }

    vec1MutexEnter(pQueue->pMutex);
    pFin->pJobNext = pQueue->pFree;
    pQueue->pFree = pFin;
    vec1MutexLeave(pQueue->pMutex);
  }

  return rc;
}
/* …（同文件无关代码省略）… */
static void vec1JobQueueFree(Vec1JobQueue *pQueue){
  if( pQueue ){
    if( pQueue->pMutex && pQueue->pCond1 ){
      int ii;
      vec1JobQueueFinishJobs(pQueue, SQLITE_ERROR);

      vec1MutexEnter(pQueue->pMutex);
      pQueue->bDone = 1;
      vec1CondBroadcast(pQueue->pCond1);
      vec1MutexLeave(pQueue->pMutex);
      for(ii=0; ii<pQueue->nWorker; ii++){
        void *dummy = 0;
        vec1ThreadJoin(pQueue->apWorker[ii], &dummy);
      }
    }

    if( pQueue->pMutex ) vec1MutexDestroy(pQueue->pMutex);
    if( pQueue->pCond1 ) vec1CondDestroy(pQueue->pCond1);
    if( pQueue->pCond2 ) vec1CondDestroy(pQueue->pCond2);

    assert( pQueue->pWaiting==0 );
    assert( pQueue->pFinished==0 );
    assert( pQueue->pFree!=0 );
    sqlite3_free(pQueue);
  }
}
/* …（同文件无关代码省略）… */
#define VEC1_PQ_CODEBOOK_SZ 256
/* …（同文件无关代码省略）… */
#define VEC1_PQ_TRAINING_SET_MIN (VEC1_PQ_CODEBOOK_SZ * 2)
/* …（同文件无关代码省略）… */

/*
** When accumulating training data, allocate space in chunks of 
** VEC1_TRAINING_SZCHUNK. Allow at most VEC1_TRAINING_MAXCHUNK chunks of
** training data. With default values, the limit is 64GiB of training
** vectors.
*/
#define VEC1_TRAINING_MAXCHUNK 1024
#define VEC1_TRAINING_SZCHUNK  (32*1024*1024)

/*
/* …（同文件无关代码省略）… */
#define VEC1_QUANTIZE_PQ   1
#define VEC1_QUANTIZE_OPQ  2
/* …（同文件无关代码省略）… */
typedef struct Vec1TrainVectors Vec1TrainVectors;
struct Vec1TrainVectors {
  int nElem;                      /* Number of elements in each vector */
  int nVec;                       /* Number of vectors in chunk array */
  int nChunk;                     /* Number of chunks allocated so far */
  int nVecPerChunk;               /* Number of vectors in each chunk */
  float *aChunk[VEC1_TRAINING_MAXCHUNK];
};

/*
/* …（同文件无关代码省略）… */
typedef struct Vec1TrainCtx Vec1TrainCtx;
/* …（同文件无关代码省略）… */
** Return a pointer to the requested training vector. The results are
** undefined if parameter iVec is out of range.
*/
static float *vec1TrainingVector(Vec1TrainVectors *p, int iVec){
  int iChunk = iVec / p->nVecPerChunk;
  int iOff = (iVec % p->nVecPerChunk) * p->nElem;
  return &((p->aChunk[iChunk])[iOff]);
}

/* …（同文件无关代码省略）… */
struct LloydsJob {
  /* Data to train on */
  Vec1TrainVectors *pVec;
  int iFirst;                     /* First vector for this job */
  int iEof;                       /* Index of last vector +1 for this job */

  int nK;                         /* Number of centroids */
  const float *aCentroid;         /* Centroids */
/* …（同文件无关代码省略）… */
  /* Outputs */
  double fTotal;
  float *aSum;
  int *aCount;
};

/*
/* …（同文件无关代码省略）… */
typedef struct Vec1Random Vec1Random;
/* …（同文件无关代码省略）… */

typedef struct KMeansInitJob KMeansInitJob;
struct KMeansInitJob {
  int iFirst;                     /* First vector for this job */
  int iEof;                       /* Index of last vector +1 for this job */
  double fTotalMin;               /* Total of aMin[iFirst..iEof-1] */

  Vec1TrainVectors *pVec;
/* …（同文件无关代码省略）… */
  KMeansInitJob *p = (KMeansInitJob*)pCtx;
  const int nElem = p->pVec->nElem;
  double fTotalMin = 0.0;
  int ii;

  for(ii=p->iFirst; ii<p->iEof; ii++){
    int jj;
/* …（同文件无关代码省略）… */
**   struct XyzJob {
**     ...
**     Vec1TrainVectors *pVec;    // Training vectors to use a subset of
**     int iFirst;                // index of first vector in pVec to use
**     int iEof;                  // 1 past index of last vector in pVec to use
**     ...
**   };
*/
/* …（同文件无关代码省略）… */

  Vec1Random rnd;

  int nByte;                      /* Bytes of space to allocate */
  KMeansInitJob *aJob = 0;
  float *aMin = 0;
  int *aBest = 0;
/* …（同文件无关代码省略）… */
  nCentroid = nNewCentroid = 1;

  for(rr=1; rr<nK; rr+=nNewCentroid){
    int vv;
    double fTotalMin = 0.0;

    /* Step 2: */
/* …（同文件无关代码省略）… */
  }

  if( aHist ){
    int vv;
    for(vv=0; vv<pVec->nVec; vv++){
      aHist[aBest[vv]]++;
    }
/* …（同文件无关代码省略）… */
  LloydsJob *p = (LloydsJob*)pCtx;
  Vec1TrainVectors *pVec = p->pVec;
  int nElem = pVec->nElem;
  int ii;

  p->fTotal = 0.0f;
  memset(p->aSum, 0, sizeof_f32 * p->nK * nElem);
  memset(p->aCount, 0, sizeof(int) * p->nK);

  /* Loop through those vectors that are our responsibility, 
  ** VEC1_MULTIMATCH_NVEC vectors at a time. Each iteration of this outer 
/* …（同文件无关代码省略）… */
  int ii;
  int iIter;
  int nElem = pVec->nElem;
  int *pCsr = 0;
  double fPrevTotal = 0.0;

  int *aCount = 0;
  float *aSum = 0;

  LloydsJob *aJob = 0;
/* …（同文件无关代码省略）… */
  **
  ** Each job requires the following working space:
  **
  **   + aCount      -> (sizeof(int) * K) bytes
  **   + aSum        -> (sizeof_f32 * nElem * K) bytes
  */
  nByte = nThread * (
      sizeof(LloydsJob) +
      nElem * nK * sizeof_f32 +       /* aSum[] */
      nK * sizeof(int)                      /* aCount[] */
  );

  aJob = (LloydsJob*)sqlite3_malloc64(nByte);
  if( aJob==0 ) return SQLITE_NOMEM;
  memset(aJob, 0, sizeof(LloydsJob) * nThread);

  pCsr = (int*)&aJob[nThread];

  /* Initialize each job object in the array */
  for(ii=0; ii<nThread; ii++){
/* …（同文件无关代码省略）… */

    pJob->nK = nK;
    pJob->aCentroid = aCode;
    pJob->aSum = (float*)pCsr;
    pCsr += (nK * nElem);
    pJob->aCount = pCsr;
    pCsr += nK;
  }
  aCount = aJob[0].aCount;
  aSum = aJob[0].aSum;
/* …（同文件无关代码省略）… */
  }
#endif

  assert( (u8*)pCsr==((u8*)aJob)+nByte );

  /* If pQueue is not NULL, then this is training the coarse quantizer
  ** and so we have exclusive access to the Vec1TrainCtx object. So
/* …（同文件无关代码省略）… */
  Vec1TrainCtx *p = 0;
  int n = 0;
  const u8 *a = 0;
  int iChunk = 0;
  char *zErr = 0;

  assert( nArg==1 || nArg==2 );
/* …（同文件无关代码省略）… */

  iChunk = (p->tv.nVec+1)/p->tv.nVecPerChunk;
  if( iChunk>=p->tv.nChunk ){
    p->tv.aChunk[iChunk] = (float*)sqlite3_malloc(VEC1_TRAINING_SZCHUNK);
    if( p->tv.aChunk[iChunk]==0 ){
      sqlite3_result_error_nomem(pCtx);
      return;
    }
    p->tv.nChunk++;
/* …（同文件无关代码省略）… */
** Normalize all the training vectors.
*/
static void vec1TrainNormalizeAll(Vec1TrainCtx *p){
  int ii;
  for(ii=0; ii<p->tv.nVec; ii++){
    vec1NormalizeVector(vec1TrainingVector(&p->tv, ii), p->tv.nElem);
  }
}

static void vec1FreeVectors(Vec1TrainVectors *pFree){
  if( pFree ){
    int ii;
    for(ii=0; ii<pFree->nChunk; ii++){
      sqlite3_free(pFree->aChunk[ii]);
    }
/* …（同文件无关代码省略）… */
typedef struct Vec1CodebookJob Vec1CodebookJob;
/* …（同文件无关代码省略）… */
static void vec1CodebookWork(void *pCtx){
  Vec1CodebookJob *pJob = (Vec1CodebookJob*)pCtx;
  Vec1TrainCtx *p = pJob->p;
  int nByte = sizeof(Vec1TrainVectors);

  Vec1TrainVectors *pVec = (Vec1TrainVectors*)vec1MallocZero(nByte);

  if( pVec==0 ){
    pJob->rc = SQLITE_NOMEM;
  }else{
    int ii;
    int nShort = 0;
    int nBytePerVec = p->nCodeElem * sizeof_f32;
    pVec->nElem = p->nCodeElem;
    pVec->nVec = pJob->pTrain->nVec;
    pVec->nVecPerChunk = (VEC1_TRAINING_SZCHUNK / nBytePerVec);
    pVec->nChunk = (pVec->nVec+pVec->nVecPerChunk-1) / pVec->nVecPerChunk;

    /* If the training vector size is not an integer multiple of the 
    ** sub-vector size and this is the rightmost sub-vector (the one that
/* …（同文件无关代码省略）… */
    for(ii=0; ii<pVec->nChunk; ii++){
      /* Allocate space for this chunk. Either VEC1_TRAINING_SZCHUNK or
      ** enough space for all remaining sub-vectors, whichever is smaller. */
      int nAlloc = MIN(VEC1_TRAINING_SZCHUNK, 
          (pVec->nVec - (ii * pVec->nVecPerChunk)) * nBytePerVec
      );
      pVec->aChunk[ii] = (float*)sqlite3_malloc(nAlloc);

      if( pVec->aChunk[ii]==0 ){
        pJob->rc = SQLITE_NOMEM;
/* …（同文件无关代码省略）… */
typedef struct Vec1CovarianceJob Vec1CovarianceJob;
/* …（同文件无关代码省略）… */
  Vec1CovarianceJob *pJob = (Vec1CovarianceJob*)pArg;
  const int iSubOff = pJob->iSub*pJob->nCodeElem;
  float *aCode = &pJob->aBook[iSubOff * VEC1_PQ_CODEBOOK_SZ];
  int ii, kk;

  int nVec = pJob->pVec->nVec;
  int nElem = pJob->pVec->nElem;
  int nKK = MIN(pJob->nCodeElem, nElem - iSubOff);

/* …（同文件无关代码省略）… */
typedef struct Vec1RotationJob Vec1RotationJob;
/* …（同文件无关代码省略）… */
  Vec1TrainVectors *pVec;         /* Input vectors */
  Vec1TrainVectors *pOut;         /* Output vectors */

  int iFirst;
  int iEof;

  const float *aRotation;         /* nElem*nElem rotation matrix */
#ifdef VEC1_USE_TRANSPOSITION
/* …（同文件无关代码省略）… */
  Vec1RotationJob *pJob = (Vec1RotationJob*)pArg;
  int nElem = pJob->pVec->nElem;

  int ii = pJob->iFirst;
  for(; ii<=(pJob->iEof-VEC1_MULTIMATCH_NVEC); ii+=VEC1_MULTIMATCH_NVEC){
    const float *aIn[VEC1_MULTIMATCH_NVEC];
    float *aOut[VEC1_MULTIMATCH_NVEC];
/* …（同文件无关代码省略）… */
*/
static Vec1TrainVectors *vec1CopyVectors(Vec1TrainVectors *pIn){
  Vec1TrainVectors *pRet = 0;
  int ii;

  pRet = (Vec1TrainVectors*)vec1MallocZero(sizeof(Vec1TrainVectors));
  if( pRet==0 ) return pRet;
  pRet->nElem = pIn->nElem;
  pRet->nVec = pIn->nVec;
  pRet->nChunk = pIn->nChunk;
  pRet->nVecPerChunk = pIn->nVecPerChunk;

  for(ii=0; ii<pIn->nChunk; ii++){
    int nByte = pRet->nVecPerChunk * pIn->nElem * sizeof_f32;
    pRet->aChunk[ii] = (float*)sqlite3_malloc(VEC1_TRAINING_SZCHUNK);
    if( pRet->aChunk[ii]==0 ){
      vec1FreeVectors(pRet);
/* …（同文件无关代码省略）… */
  const int nElem = p->tv.nElem;
  const int nMatrixByte = sizeof_f32 * nElem * nElem;
  int iRound = 0;                 /* Current OPQ round */
  int ii;
  float *aRot = (float*)sqlite3_malloc(nMatrixByte * 2);
  float *aRot2 = &aRot[nElem*nElem];
  Vec1TrainVectors *pTrain = vec1CopyVectors(&p->tv);
/* …（同文件无关代码省略）… */
typedef struct Vec1ResidualJob Vec1ResidualJob;
/* …（同文件无关代码省略）… */
  /* Data to train on */
  const Vec1TrainCtx *p;
  Vec1TrainVectors *pVec;
  int iFirst;
  int iEof;
  const float *aCentroid;
  int nCentroid;
};

static void vec1TrainResidualWorker(void *pCtx){
  Vec1ResidualJob *p = (Vec1ResidualJob*)pCtx;
  int ii;
  int nElem = p->pVec->nElem;

  for(ii=p->iFirst; ii<p->iEof; ii++){
/* …（同文件无关代码省略）… */
static int vec1IsPow2(int n) { 
  return n > 0 && (n & (n-1)) == 0; 
}
/* …（同文件无关代码省略）… */
static void vec1WHT(float *x, int n){
  int stride, i, j;
  assert( vec1IsPow2(n) );
  for (stride = 1; stride < n; stride <<= 1) {
    for (i = 0; i < n; i += stride << 1) {
      for (j = i; j < i + stride; j++) {
        float a = x[j];
        float b = x[j + stride];
        x[j]        = a + b;
        x[j+stride] = a - b;
      }
    }
  }
}
/* …（同文件无关代码省略）… */
static void vec1ApplyWHT(float *aVec, int nElem){
  if (vec1IsPow2(nElem)) {
    vec1WHT(aVec, nElem);
  }else{
    int p = 1;
    while( (p<<1)<nElem ) p = p<<1;
    vec1WHT(aVec, p);
    vec1WHT(aVec + nElem - p, p);
  }
}
/* …（同文件无关代码省略）… */
** the training context.
*/
static void vec1TrainApplyWHT(Vec1TrainCtx *p){
  int ii;
  for(ii=0; ii<p->tv.nVec; ii++){
    vec1ApplyWHT(vec1TrainingVector(&p->tv, ii), p->tv.nElem);
  }
/* …（同文件无关代码省略）… */
  float *pCsr = 0;
  Vec1JobQueue *pQueue = 0;
  u32 flags = 0;
  int ii;
  int bPQ = 0;
  i64 nElem = 0;                  /* 64-bit copy of p->tv.nElem */

/* …（同文件无关代码省略）… */
  bPQ = (p->eQuant==VEC1_QUANTIZE_PQ || p->eQuant==VEC1_QUANTIZE_OPQ);
  nElem = p->tv.nElem;

  if( p->tv.nVec==0 ){
    vec1ResultErrorF(pCtx,
        "too few training vectors (have 0, require more than that)"
/* …（同文件无关代码省略）… */
    if( bPQ ) nReq = MAX(VEC1_PQ_TRAINING_SET_MIN, nReq);
    if( p->tv.nVec<nReq ){
      vec1ResultErrorF(pCtx,
          "too few training vectors (require %d, have %d)",
          nReq, p->tv.nVec
      );
      goto train_final_out;
/* …（同文件无关代码省略）… */
  for(ii=0; ii<p->tv.nChunk; ii++){
    sqlite3_free(p->tv.aChunk[ii]);
  }
  vec1JobQueueFree(pQueue);
  sqlite3_free(aByte);
}
