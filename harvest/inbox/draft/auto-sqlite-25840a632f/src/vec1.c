// AUTO-DRAFT from sqlite/sqlite PR #31e3b6ff459658c156855c9a9761370b5d02e923
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define VEC1_DISTANCE_COS  2
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
# define VEC1_TRANSPOSE_WIDTH 32
/* …（同文件无关代码省略）… */
# define VEC1_MULTIMATCH_NVEC  8
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
#define size_of_array(X) ((int)(sizeof(X)/sizeof((X)[0])))
/* …（同文件无关代码省略）… */
#define MIN(a,b) ((a>b) ? (b) : (a))
/* …（同文件无关代码省略）… */
typedef struct Vec1Tab Vec1Tab;
/* …（同文件无关代码省略）… */
#define VEC1_MODEL_ROTATE   0x02
#define VEC1_MODEL_RESIDUAL 0x04
#define VEC1_MODEL_BITQUANT 0x08
#define VEC1_MODEL_WHT      0x10
/* …（同文件无关代码省略）… */
#define sizeof_u32 4
/* …（同文件无关代码省略）… */
static int vec1CorruptError(){
  return SQLITE_CORRUPT_VTAB;
}
#define VEC1_CORRUPT vec1CorruptError();
/* …（同文件无关代码省略）… */
static u32 vec1GetU32(const u8 *aBuf){
  return (((u32)aBuf[0]) << 24)
       + (((u32)aBuf[1]) << 16)
       + (((u32)aBuf[2]) <<  8)
       + (((u32)aBuf[3]) <<  0);
}
/* …（同文件无关代码省略）… */
typedef struct Vec1Buffer Vec1Buffer;
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
static void vec1RotateVector(
  int d,
  const float* A,                 /* d x d, row-major */
  const float* x,                 /* input vector, length d */
  float* y                        /* output vector, length d */
){
  int i;
  for(i=0; i<d; i++){
    y[i] = vec1DotProduct(d, &A[i*d], x);
  }
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
static int vec1BufferSize(Vec1Buffer *pBuf, sqlite3_int64 nByte){
  if( nByte>pBuf->nAlloc ){
    u8 *aNew = sqlite3_realloc64(pBuf->a, nByte);
    if( aNew==0 ){
      return SQLITE_NOMEM;
    }
    pBuf->a = aNew;
    pBuf->nAlloc = (int)nByte;
  }
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static void vec1Sub(float *aOut, const float *a1, const float *a2, int n){
  int i;
  for(i=0; i<n; i++){
    aOut[i] = a1[i] - a2[i];
  }
}
/* …（同文件无关代码省略）… */
static void vec1SubInPlace(float *a1, const float *a2, int n){
  vec1Sub(a1, a1, a2, n);
}
/* …（同文件无关代码省略）… */
static double vec1L2Dist(
  const float *a1,
  const float *a2,
  int n
){
  double ret = 0.0f;
  int i = 0;

#ifdef VEC1_HAVE_AVX2
# ifndef VEC1_L2_NACC 
#  define VEC1_L2_NACC 4
# endif
  const int nAcc = VEC1_L2_NACC;
  int c;

  /* Create and initialize to zero nAcc accumulator registers. */
  __m256 acc[VEC1_L2_NACC];
  for(c=0; c<nAcc; c++) acc[c] = _mm256_setzero_ps();

  for(; i<=(n-(8*nAcc)); i+=(8*nAcc)){
    __m256 d[VEC1_L2_NACC];
    for(c=0; c<nAcc; c++){
      d[c] = _mm256_sub_ps(LOADU(&a1[i+c*8]), LOADU(&a2[i+c*8]));
      acc[c] = FMADD(d[c], d[c], acc[c]);
    }
  }

  /* Sum all nAcc accumulators into acc[0]. */
  VERTICAL_SUM(acc, nAcc);

  /* Process the part of the tail that can be done 8 elements at a time
  ** using acc[0] as the accumulator. Then sum all 8 elements of acc[0] 
  ** into scalar float "ret".  */
  for(; i<=(n-8); i+=8){
    __m256 d = _mm256_sub_ps(LOADU(&a1[i]), LOADU(&a2[i]));
    acc[0] = FMADD(d, d, acc[0]);
  }
  HORIZONTAL_SUM(ret, acc[0]);
#endif /* VEC1_HAVE_AVX2 */

#ifdef VEC1_HAVE_NEON
# ifndef VEC1_L2_NACC 
#  define VEC1_L2_NACC 8
# endif
  const int nAcc = VEC1_L2_NACC;
  int c, s;

  /* Create and initialize to zero nAcc accumulator registers. */
  float32x4_t acc[VEC1_L2_NACC];
  for(c=0; c<nAcc; c++) acc[c] = vdupq_n_f32(0.0f);

  for(; i<=(n-(4*nAcc)); i+=(4*nAcc)){
    float32x4_t d[VEC1_L2_NACC];
    for(c=0; c<nAcc; c++){
      d[c] = vsubq_f32(vld1q_f32(&a1[i+c*4]), vld1q_f32(&a2[i+c*4]));
      acc[c] = vmlaq_f32(acc[c], d[c], d[c]);
    }
  }

  /* Sum all nAcc accumulators into acc[0]. */
  VERTICAL_SUM(acc, nAcc);

  for(; i<=(n-4); i+=4){
    float32x4_t d = vsubq_f32(vld1q_f32(&a1[i]), vld1q_f32(&a2[i]));
    acc[0] = vmlaq_f32(acc[0], d, d);
  }
  ret = vaddvq_f32(acc[0]);
#endif /* VEC1_HAVE_NEON */

  for(; i<n; i++){
    double diff = a1[i] - a2[i];
    ret += (diff * diff);
  }

  return ret;
}
/* …（同文件无关代码省略）… */
static double vec1VectorNorm2(const float *a1, int n){
  return vec1DotProduct(n, a1, a1);
}
/* …（同文件无关代码省略）… */
#define VEC1_PQ_CODEBOOK_SZ 256
/* …（同文件无关代码省略）… */
typedef struct Vec1TrainVectors Vec1TrainVectors;
/* …（同文件无关代码省略）… */
static float *vec1TrainingVector(Vec1TrainVectors *p, int iVec){
  int iChunk = iVec / p->nVecPerChunk;
  int iOff = (iVec % p->nVecPerChunk) * p->nElem;
  return &((p->aChunk[iChunk])[iOff]);
}
/* …（同文件无关代码省略）… */
** nCode vectors, each of nCodeElem elements. Return the index of the best
** match for aSub in aCodebook[].
*/
static int vec1PqBestMatch(
  const float *aCodebook,         /* Codebook of nCodeElem element vectors */
  int nCode,                      /* Number of vectors in aCodebook */
  const float *aSub,              /* nCodeElem element vector */
  int nCodeElem,                  /* Size of vectors in elements */
  double *pfBest                  /* Distance to best match */
){
  double fBestDist = INFINITY;
  int iBest = 0;
  int ii;

  for(ii=0; ii<nCode; ii++){
    double fDist = vec1L2Dist(aSub, &aCodebook[ii*nCodeElem], nCodeElem);
    if( fDist<fBestDist ){
      iBest = ii;
      fBestDist = fDist;
    }
  }
  assert( iBest>=0 && iBest<nCode );

  if( pfBest ) *pfBest += fBestDist;
  return iBest;
}
/* …（同文件无关代码省略）… */
# define VEC1_NOINLINE __declspec(noinline)
/* …（同文件无关代码省略）… */
static VEC1_NOINLINE void vec1AddInPlaceScalar(
  float * a1, 
  const float * a2, 
  int n
){
  int i;
  for(i=0; i<n; i++){
    a1[i] += a2[i];
  }
}
/* …（同文件无关代码省略）… */
static void vec1AddInPlace(float * a1, const float * a2, int n){
  int i = 0;
#ifdef VEC1_HAVE_AVX2
  for(; i<=n-8; i+=8){
    __m256 v1 = _mm256_loadu_ps(a1 + i);
    __m256 v2 = _mm256_loadu_ps(a2 + i);
    _mm256_storeu_ps(a1 + i, _mm256_add_ps(v1, v2));
  }
#endif
#ifdef VEC1_HAVE_NEON
  for(; i<=n-4; i+=4){
    float32x4_t v1 = vld1q_f32(&a1[i]);
    float32x4_t v2 = vld1q_f32(&a2[i]);
    vst1q_f32(&a1[i], vaddq_f32(v1, v2));
  }
#endif

  if( i!=n ) vec1AddInPlaceScalar(&a1[i], &a2[i], n-i);
}
/* …（同文件无关代码省略）… */
    }

#if 0
    /* Check (assert) using vec1PqBestMatch() that the fancy code above
    ** actually worked.  */
    for(vv=0; vv<VEC1_MULTIMATCH_NVEC; vv++){
      int iBest2 = vec1PqBestMatch(
          p->aCentroid, p->nK, aVec[vv], nElem, 0
      );
      if( iBest2!=aBest[vv] ){
/* …（同文件无关代码省略）… */
  ** part of a batch of VEC1_MULTIMATCH_NVEC. */
  for(; ii<p->iEof; ii++){
    const float *aSub = vec1TrainingVector(pVec, ii);
    int iBest = vec1PqBestMatch(p->aCentroid, p->nK, aSub, nElem, &p->fTotal);
    assert( iBest>=0 && iBest<p->nK );
    p->aCount[iBest]++;
    vec1AddInPlace(&p->aSum[iBest * nElem], aSub, nElem);
/* …（同文件无关代码省略）… */
  for(; ii<nVec; ii++){
    float *aVec = vec1TrainingVector(pJob->pVec, ii);
    const float *aSub = &aVec[iSubOff];
    int iBest = vec1PqBestMatch(
        aCode, VEC1_PQ_CODEBOOK_SZ, aSub, pJob->nCodeElem, &pJob->fTotalDist
    );
    float *aSubHat = &aCode[iBest * pJob->nCodeElem];
/* …（同文件无关代码省略）… */

  for(ii=p->iFirst; ii<p->iEof; ii++){
    float *a = vec1TrainingVector(p->pVec, ii);
    int iBest = vec1PqBestMatch(p->aCentroid, p->nCentroid, a, nElem, 0);
    vec1SubInPlace(a, &p->aCentroid[iBest * nElem], nElem);
  }
}
/* …（同文件无关代码省略）… */
static int vec1IsPow2(int n) { 
  return n > 0 && (n & (n-1)) == 0; 
}
/* …（同文件无关代码省略）… */
static void vec1WHT(float *x, int n){
  assert( vec1IsPow2(n) );
  for (int stride = 1; stride < n; stride <<= 1) {
    for (int i = 0; i < n; i += stride << 1) {
      for (int j = i; j < i + stride; j++) {
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
typedef struct Vec1Model Vec1Model;
/* …（同文件无关代码省略）… */
#define VEC1_LIST_64BIT   0x01    /* Set for 64-bit rowids, clear for 32-bit */
/* …（同文件无关代码省略）… */
#define VEC1_LIST_SZHDR (3*sizeof_u32)
/* …（同文件无关代码省略）… */
#define VEC1_PQ_BLOCKSIZE 16
/* …（同文件无关代码省略）… */
static char *vec1MPrintf(int *pRc, const char *zFmt, ...){
  char *zRet = 0;
  va_list ap;
  va_start(ap, zFmt);
  zRet = sqlite3_vmprintf(zFmt, ap);
  va_end(ap);

  if( *pRc==SQLITE_OK ){
    if( zRet==0 ){
      *pRc = SQLITE_NOMEM;
    }
  }else{
    sqlite3_free(zRet);
    zRet = 0;
  }

  return zRet;
}
/* …（同文件无关代码省略）… */
static void vec1VtabError(Vec1Tab *pTab, const char *zFmt, ...){
  char *zMsg = 0;
  va_list ap;
  va_start(ap, zFmt);
  zMsg = sqlite3_vmprintf(zFmt, ap);
  va_end(ap);
  sqlite3_free(pTab->base.zErrMsg);
  pTab->base.zErrMsg = zMsg;
}
/* …（同文件无关代码省略）… */
static int vec1PrepareSql(
  Vec1Tab *pTab, 
  sqlite3_stmt **ppStmt,
  const char *zFmt, 
  ...
){
  char *zSql = 0;
  int rc = SQLITE_OK;
  va_list ap;

  va_start(ap, zFmt);
  zSql = sqlite3_vmprintf(zFmt, ap);
  va_end(ap);

  *ppStmt = 0;
  if( zSql ){
    rc = sqlite3_prepare_v2(pTab->db, zSql, -1, ppStmt, 0);
    sqlite3_free(zSql);
    if( rc!=SQLITE_OK ){
      vec1VtabError(pTab, "%s", sqlite3_errmsg(pTab->db));
    }
  }else{
    rc = SQLITE_NOMEM;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int vec1GetSql(Vec1Tab *pTab, int eSql, sqlite3_stmt **ppStmt){
  const char *azSql[] = {
#define VEC1_SQL_INSERT_BASE   0
    "INSERT INTO %Q.'%q_base' VALUES(?, ?%s)",
    "SELECT * FROM %Q.'%q_base' ORDER BY 1",
    "INSERT INTO %Q.'%q_idx'(id, bucket, first, last, val)VALUES(?,?,?,?,?)",

    "SELECT val, bucket, rowid FROM %Q.'%q_idx'",
    "SELECT sum( length(val) / ? ) FROM %Q.'%q_idx'",

    "REPLACE INTO %Q.'%q_config'(id, val) VALUES(?, ?)",
    "INSERT INTO %Q.'%q_config'(id, val) VALUES(?, 1) "
      "ON CONFLICT(id) DO UPDATE SET val=val+1",

    "SELECT id, val FROM %Q.'%q_config'",

#define VEC1_SQL_LOOKUP_BASE   8
    "SELECT * FROM %Q.'%q_base' WHERE id=?",

#define VEC1_SQL_EDIT_IDX      9
    "SELECT id, min( length(val) ) FROM %Q.'%q_idx' WHERE bucket=?",

#define VEC1_SQL_DEL_LOOKUP_BASE 10
    /* Used when removing a row by id. This statement deletes the %_base
    ** entry and returns the vector so that the user may use it to 
    ** find the %_idx entry to delete.  */
    "DELETE FROM %Q.'%q_base' WHERE id=? RETURNING vector",

#define VEC1_SQL_SEARCH_IDX 11
    "SELECT id FROM %Q.'%q_idx' WHERE bucket=? AND ? BETWEEN first AND last",

#define VEC1_SQL_SEARCH_IDX_FOR_INSERT 12
    "SELECT id, first, last, val "
      "FROM %Q.'%q_idx' WHERE bucket=? AND length(val)<? LIMIT 1",

#define VEC1_SCAN_BUCKET 13
    "SELECT val, bucket, rowid FROM %Q.'%q_idx' WHERE bucket = 0+?",

#define VEC1_SQL_UPDATE_BASE   14
    "UPDATE %Q.'%q_base' SET vector = ? WHERE id=?",

#define VEC1_SQL_ZERO_BASE_RANGE 15
    "UPDATE %Q.'%q_base' SET vector = 0, id=id WHERE id BETWEEN ? AND ?",

#define VEC1_SQL_DELETE_FROM_IDX 16
    "DELETE FROM %Q.'%q_idx' WHERE id = ?",

#define VEC1_SQL_UPDATE_BASE2   17
    "UPDATE %Q.'%q_base' SET vector = ?, id=id WHERE id=?",

#define VEC1_SQL_READ_META   18
    "SELECT val FROM %Q.'%q_meta' WHERE id=?",

#define VEC1_SQL_WRITE_META   19
    "REPLACE INTO %Q.'%q_meta' VALUES(?, ?)",

#define VEC1_SQL_DELETE_FROM_META 20
    "DELETE FROM %Q.'%q_meta' WHERE id BETWEEN (?1<<8) AND (?1<<8)+255",
  };
  int rc = SQLITE_OK;

  assert( eSql>=0 && eSql<size_of_array(azSql) );
  assert( size_of_array(azSql)==size_of_array(pTab->aStmt) );

  if( pTab->aStmt[eSql]==0 ){

    if( eSql==VEC1_SQL_INSERT_BASE ){
      int ii;
      char *zExtra = 0;
      for(ii=0; ii<pTab->nMeta; ii++){
        zExtra = vec1MPrintf(&rc, "%z,?", zExtra);
      }
      if( rc==SQLITE_OK ){
        rc = vec1PrepareSql(
            pTab, &pTab->aStmt[eSql], azSql[eSql], 
            pTab->zDb, pTab->zName, zExtra
        );
      }
      sqlite3_free(zExtra);
    }else{
      rc = vec1PrepareSql(
          pTab, &pTab->aStmt[eSql], azSql[eSql], pTab->zDb, pTab->zName
      );
    }
  }

  *ppStmt = pTab->aStmt[eSql];
  return rc;
}
/* …（同文件无关代码省略）… */
static int vec1EncodedVectorSize(const Vec1Model *pMod){
  int ret = (int)pMod->hdr.nCodebook;
  if( pMod->hdr.flags & VEC1_MODEL_BITQUANT ) ret += sizeof_f32;
  return ret;
}
/* …（同文件无关代码省略）… */
static const float *vec1TransformInputVector(
  const Vec1Model *pMod,
  float *aTmp,
  const float *aInput
){
  const float *aRet = aInput;
  if( (pMod->hdr.flags & VEC1_MODEL_ROTATE) ){
    vec1RotateVector(pMod->hdr.nElem, pMod->aRotation, aInput, aTmp);
    if( pMod->hdr.eDistance==VEC1_DISTANCE_COS ){
      vec1NormalizeVector(aTmp, pMod->hdr.nElem);
    }
    aRet = aTmp;
  }
  else if( (pMod->hdr.flags & VEC1_MODEL_WHT) ){
    memcpy(aTmp, aInput, pMod->hdr.nElem * sizeof_f32);
    vec1ApplyWHT(aTmp, pMod->hdr.nElem);
    aRet = aTmp;
  }
  else if( pMod->hdr.eDistance==VEC1_DISTANCE_COS ){
    memcpy(aTmp, aInput, pMod->hdr.nElem * sizeof_f32);
    vec1NormalizeVector(aTmp, pMod->hdr.nElem);
    aRet = aTmp;
  }

  if( aRet!=aTmp
   && pMod->hdr.nCodebook>0 
   && (pMod->hdr.nCodebook*pMod->nCodeElem)!=pMod->hdr.nElem
  ){
    memcpy(aTmp, aRet, pMod->hdr.nElem * sizeof_f32);
    aRet = aTmp;
  }

  return aRet;
}
/* …（同文件无关代码省略）… */
static void vec1ModelTDist(
  const float *pIn,
  const float *aVec,
  int nCodeElem,
  float *aOut
){
  int d;

#if defined(VEC1_HAVE_AVX2)
  __m256 acc[ VEC1_TRANSPOSE_WIDTH / 8 ];
  const int nAcc = (VEC1_TRANSPOSE_WIDTH / 8);
  int c;

  for(c=0; c<nAcc; c++) acc[c] = _mm256_setzero_ps();
  for(d=0; d<nCodeElem; d++){
    __m256 v = _mm256_set1_ps(aVec[d]);
    for(c=0; c<nAcc; c++){
      __m256 diff = _mm256_sub_ps(v, LOADU(&pIn[c*8]));
      acc[c] = FMADD(diff, diff, acc[c]);
    }
    pIn += VEC1_TRANSPOSE_WIDTH;
  }
  for(c=0; c<nAcc; c++) _mm256_storeu_ps(&aOut[c*8], acc[c]);

#elif defined(VEC1_HAVE_NEON)
  float32x4_t acc[ VEC1_TRANSPOSE_WIDTH / 4 ];
  const int nAcc = (VEC1_TRANSPOSE_WIDTH / 4);
  int c;

  for(c=0; c<nAcc; c++) acc[c] = vdupq_n_f32(0.0f);
  for(d=0; d<nCodeElem; d++){
    float32x4_t v = vdupq_n_f32( aVec[d] );
    for(c=0; c<nAcc; c++){
      float32x4_t diff = vsubq_f32(v, vld1q_f32(&pIn[c*4]));
      acc[c] = vmlaq_f32(acc[c], diff, diff);
    }
    pIn += VEC1_TRANSPOSE_WIDTH;
  }
  for(c=0; c<nAcc; c++) vst1q_f32(&aOut[c*4], acc[c]);

#else
  memset(aOut, 0, sizeof_f32 * VEC1_TRANSPOSE_WIDTH);
  for(d=0; d<nCodeElem; d++){
    float vv = aVec[d];
    const float *aVC = pIn;
    int ii;
    for(ii=0; ii<VEC1_TRANSPOSE_WIDTH; ii++){
      aOut[ii] += ((vv - aVC[ii]) * (vv - aVC[ii]));
    }
    pIn += VEC1_TRANSPOSE_WIDTH;
  }
#endif
}
/* …（同文件无关代码省略）… */
static void vec1EncodeVectorBit(
  const Vec1Model *pMod,          /* Model */
  const float *aVec,              /* Input vector */
  u8 *aCode                       /* OUT: Encoded vector */
){
  const int nElem = (int)pMod->hdr.nElem;
  float norm;
  int ii;
  u8 *pOut;

  assert( pMod->hdr.flags & VEC1_MODEL_BITQUANT );
  norm = (float)vec1VectorNorm2(aVec, nElem);
  norm = sqrtf(norm);
  memcpy(aCode, &norm, sizeof_f32);

  assert( pMod->nCodeElem==8 || pMod->nCodeElem==16 );

  pOut = &aCode[4];
  for(ii=0; ii<nElem; ii+=pMod->nCodeElem){
    u8 v = 0;
    int jj;
    if( pMod->nCodeElem==8 ){
      for(jj=0; jj<8; jj++){
        if( aVec[ii+jj]>0.0 ){
          v |= (1<<jj);
        }
      }
    }else{
      for(jj=0; jj<8; jj+=1){
        if( (aVec[ii+jj*2]+aVec[ii+jj*2+1])>0.0 ){
          v |= (1<<jj);
        }
      }
    }

    *pOut = v;
    pOut++;
  }
}
/* …（同文件无关代码省略）… */
static void vec1EncodeVectorPQ(
  const Vec1Model *pMod,
  const float *aVec,
  u8 *aCode
){
  const int nCodebook = pMod->hdr.nCodebook;
  const int nCodeElem = pMod->nCodeElem;
  const float *pIn = pMod->aModelT;
  int M, K;

  for(M=0; M<nCodebook; M++){
    float fBestDist = INFINITY;
    for(K=0; K<VEC1_PQ_CODEBOOK_SZ; K+=VEC1_TRANSPOSE_WIDTH){
      int ii;
      float aAcc[VEC1_TRANSPOSE_WIDTH];

      vec1ModelTDist(pIn, &aVec[M*nCodeElem], nCodeElem, aAcc);
      pIn += (VEC1_TRANSPOSE_WIDTH * nCodeElem);

      for(ii=0; ii<VEC1_TRANSPOSE_WIDTH; ii++){
        if( aAcc[ii]<fBestDist ){
          fBestDist = aAcc[ii];
          aCode[M] = (u8)(K+ii);
        }
      }
    }
  }
}
/* …（同文件无关代码省略）… */
  for(ii=0; ii<nCodebook; ii++){
    const float *pCodebook = &pMod->aModel[ii*nCodeElem*VEC1_PQ_CODEBOOK_SZ];
    const float *aSub = &aVec[ii*nCodeElem];
    aOut[ii] = (u8)vec1PqBestMatch(
        pCodebook, VEC1_PQ_CODEBOOK_SZ, aSub, nCodeElem, pfTotalDist
    );
  }
/* …（同文件无关代码省略）… */
static void vec1EncodeVector(
  const Vec1Model *pMod,
  const float *aVec,
  u8 *aCode
){
  if( pMod->hdr.flags & VEC1_MODEL_BITQUANT ){
    vec1EncodeVectorBit(pMod, aVec, aCode);
  }else{
    vec1EncodeVectorPQ(pMod, aVec, aCode);
  }
}
/* …（同文件无关代码省略）… */
static void vec1DecodeVector(
  const Vec1Model *pMod,          /* Model used to encode vector */
  const u8 *aCode,                /* Encoded vector */
  float *aVec                     /* Populate this buffer with full vector */
){
  const int nElem = (int)pMod->hdr.nElem;
  if( pMod->hdr.flags & VEC1_MODEL_BITQUANT ){
    float fNorm = 0.0;
    float fVal = 0.0;
    int ii;

    /* The first 4 bytes of the encoded vector store the norm of the vector,
    ** as a 32-bit float. */
    memcpy(&fNorm, aCode, sizeof_f32);

    /* The reconstructed vector uses the following value as the magnitude 
    ** of each dimension. The value is -ve if the corresponding bit is clear,
    ** or +ve if it is set.  */
    fVal = (float)sqrt((fNorm*fNorm) / nElem);

    assert( pMod->nCodeElem==8 || pMod->nCodeElem==16 );
    for(ii=0; ii<nElem; ii++){
      if( pMod->nCodeElem==8 ){
        if( aCode[sizeof_f32 + (ii/8)] & (1 << (ii%8)) ){
          aVec[ii] = fVal;
        }else{
          aVec[ii] = -1 * fVal;
        }
      }else{
        if( aCode[sizeof_f32 + (ii/16)] & (1 << ((ii/2)%8)) ){
          aVec[ii] = fVal;
        }else{
          aVec[ii] = -1 * fVal;
        }
      }
    }
  }else{
    int ii;
    for(ii=0; ii<(int)pMod->hdr.nCodebook; ii++){
      const float *aCopy = &pMod->aModel[
        (ii * VEC1_PQ_CODEBOOK_SZ + aCode[ii]) * pMod->nCodeElem
      ];
      int nCopy = MIN(pMod->nCodeElem, nElem - ii*pMod->nCodeElem);
      memcpy(&aVec[ii * pMod->nCodeElem], aCopy, nCopy*sizeof_f32);
    }
  }
}
/* …（同文件无关代码省略）… */
  
      /* Find bucket and coarse error if applicable. */
      if( pMod->hdr.nBucket>1 ){
        iBucket = vec1PqBestMatch(
            pMod->aCentroid, pMod->hdr.nBucket, aVec, nElem, &fCoarseError
        );
        if( aTransform!=pTab->aTmpVec ){
/* …（同文件无关代码省略）… */
#define VEC1_LIST_BLOCKED_OFFSET(ii, nVectorSize) (              \
  ((ii / VEC1_PQ_BLOCKSIZE) * VEC1_PQ_BLOCKSIZE * nVectorSize) + \
  (ii % VEC1_PQ_BLOCKSIZE)                                       \
)
/* …（同文件无关代码省略）… */
#define VEC1_LIST_NORM_OFFSET(ii, nCodebook) (                                \
  ((ii / VEC1_PQ_BLOCKSIZE) * VEC1_PQ_BLOCKSIZE * (nCodebook+sizeof_f32)) + \
  (ii % VEC1_PQ_BLOCKSIZE) * sizeof_f32                          \
)
/* …（同文件无关代码省略）… */
#define VEC1_LIST_CODE_OFFSET(ii, nCodebook) (                                \
  ((ii / VEC1_PQ_BLOCKSIZE) * VEC1_PQ_BLOCKSIZE * (nCodebook+sizeof_f32)) + \
  (VEC1_PQ_BLOCKSIZE * sizeof_f32) +                                          \
  (ii % VEC1_PQ_BLOCKSIZE)                                                    \
)
/* …（同文件无关代码省略）… */
  aVec = vec1TransformInputVector(pMod, aTmp, aVector);
  if( nBucket>0 ){
    int nElem = pMod->hdr.nElem;
    iBucket = vec1PqBestMatch(pMod->aCentroid, nBucket, aVec, nElem, 0);
    if( pMod->hdr.nCodebook>0 && (pMod->hdr.flags & VEC1_MODEL_RESIDUAL) ){
      vec1Sub(aTmp, aVec, &pMod->aCentroid[iBucket*nElem], nElem);
      aVec = aTmp;
/* …（同文件无关代码省略）… */
  rc = vec1GetSql(pTab, VEC1_SQL_DEL_LOOKUP_BASE, &pDelete);
  if( rc==SQLITE_OK ){
    int iBucket = 0;
    sqlite3_bind_int64(pDelete, 1, iRowid);
    if( SQLITE_ROW==sqlite3_step(pDelete) && pTab->mod.hdr.nBucket>0 ){
      if( pTab->mod.hdr.nCodebook==0 ){
/* …（同文件无关代码省略）… */
        if( nVec!=(pTab->cfg.nElem*sizeof_f32) ){
          rc = VEC1_CORRUPT;
        }else{
          iBucket = vec1PqBestMatch(
              pTab->mod.aCentroid, pTab->mod.hdr.nBucket, 
              aTransform, pTab->cfg.nElem, 0
          );
/* …（同文件无关代码省略）… */
static int vec1FloatWithinTolerance(float f1, float f2, float fTol){
  if( f1==0.0 || f2==0.0 ) return 0;
  if( f2>f1 ){
    if( ((f2-f1) / f2) > fTol ) return 0;
  }else{
    if( ((f1-f2) / f1) > fTol ) return 0;
  }
  return 1;
}
/* …（同文件无关代码省略）… */
static int vec1IntegrityCheckEncoded(
  int *pRc,                       /* IN/OUT: Error code */
  Vec1Model *pMod,                /* Model used to quantize vectors */
  const float *aVec,              /* Vector to compare quantized entry to */
  int iEntry,                     /* Entry of aBlob[] to compare to */
  const u8 *aBlob,                /* %_idx.val value */
  Vec1Buffer *pBuf                /* Buffer to use for temp space */
){
  const int nEnc = vec1EncodedVectorSize(pMod);
  const int nElem = (int)pMod->hdr.nElem;

  /* Allocate the maximum amount of space required by this function - 
  ** enough for one full vector and two encoded vectors. */
  int nReq = (nElem * sizeof_f32) + nEnc*2;
  if( vec1BufferSize(pBuf, nReq) ){
    *pRc = SQLITE_NOMEM;
    return 1;
  }else{
    const int nCodebook = pMod->hdr.nCodebook;
    float *v1 = (float*)pBuf->a;  /* Space for decoded vector */
    u8 *aEnc = (u8*)&v1[nElem];   /* Encoded version of aVec[] */
    u8 *aIdx = &aEnc[nEnc];       /* Encoded vector read from aBlob[] */

    u32 flags = vec1GetU32(&aBlob[0]);
    u32 nEntry = (int)vec1GetU32(&aBlob[4]);
    int iOut = 0;
    int iOff;
    int ii;

    /* Encode the vector into aEnc */
    vec1EncodeVector(pMod, aVec, aEnc);

    /* Read the encoded version of the vector from aBlob[] into aIdx[] */
    iOff = nEntry * ((flags & VEC1_LIST_64BIT) ? 8 : 4) + VEC1_LIST_SZHDR;
    if( pMod->hdr.flags & VEC1_MODEL_BITQUANT ){
      int iNorm = iOff + VEC1_LIST_NORM_OFFSET(iEntry, nCodebook);
      memcpy(aIdx, &aBlob[iNorm], sizeof_f32);
      iOff += VEC1_LIST_CODE_OFFSET(iEntry, nCodebook);
      iOut = sizeof_f32;
    }else{
      iOff += VEC1_LIST_BLOCKED_OFFSET(iEntry, nCodebook);
    }
    for(ii=0; ii<nCodebook; ii++){
      aIdx[iOut++] = aBlob[iOff];
      iOff += VEC1_PQ_BLOCKSIZE;
    }

    if( memcmp(aEnc, aIdx, nEnc) ){
      /* The encoded version of the vector does not exactly match the version
      ** just read from the index. This might be corruption, or it might be
      ** down to small differences in the floating point arithmetic.  
      **
      ** For BQ encodings, we require the body of the encoding to be
      ** identical, but allow a small tolerance for the norm field.
      **
      ** For PQ encodings, we measure the reconstruction errors of both
      ** the aEnc[] and aIdx[] encodings. If they are within a small tolerance
      ** of each other, then both encodings are deemed to be valid.  */
      const float fTol = 0.000001f;

      if( pMod->hdr.flags & VEC1_MODEL_BITQUANT ){
        float norm1;
        float norm2;
        memcpy(&norm1, aEnc, sizeof_f32);
        memcpy(&norm2, aIdx, sizeof_f32);

        /* Check the norms are close enough. If not, return 1. */
        if( vec1FloatWithinTolerance(norm1, norm2, fTol)==0 ) return 1;

        /* Check the arrays of bits are identical. If not, return 1. */
        if( memcmp(&aEnc[sizeof_f32], &aIdx[sizeof_f32], nEnc-sizeof_f32) ){
          return 1;
        }
      }else{
        float e1 = 0.0;           /* aEnc[] squared reconstruction error */
        float e2 = 0.0;           /* aIdx[] squared reconstruction error */
        vec1DecodeVector(pMod, aEnc, v1);
        e1 = (float)vec1L2Dist(v1, aVec, nElem);
        vec1DecodeVector(pMod, aIdx, v1);
        e2 = (float)vec1L2Dist(v1, aVec, nElem);
        if( vec1FloatWithinTolerance(e1, e2, fTol)==0 ) return 1;
      }
    }
  }

  return 0;
}

/*
** The buffer passed as the only argument contains a meta-value array. If
** it is of type GENERIC, test that it is well-formed. Return VEC1_CORRUPT
/* …（同文件无关代码省略）… */
            const u8 *aBaseVec = sqlite3_column_blob(pLookup, 1);
            const float *aTransform = 0;
            int nBaseVec = sqlite3_column_bytes(pLookup, 1);
            int iCalc = 0;
  
            assert( nCodebook>0 );

            aTransform = vec1TransformInputVector(
                &pTab->mod, pTab->aTmpVec, (const float*)aBaseVec
            ); 
  
            /* Check the vector is the right size. */
            if( nBaseVec!=nElem*sizeof_f32 ){
              const char *zFmt = "%s: vector in %%_base row %lld is wrong size";
              zErr = sqlite3_mprintf(zFmt, zTabName, iRowid);
              goto integrity_failed;
            }
  
            /* Check the vector is in the right bucket. */
            if( pMod->hdr.nBucket>0 ){
              iCalc = vec1PqBestMatch(pMod->aCentroid, 
                  pMod->hdr.nBucket, aTransform, nElem, 0
              );
            }
            if( iCalc!=iBucket ){
              const char *zFmt = "%s: vector from row %lld is in wrong bucket";
              zErr = sqlite3_mprintf(zFmt, zTabName, iRowid);
              goto integrity_failed;
/* …（同文件无关代码省略）… */
              ** aPQ[] just contains the nCodebook bytes of encoded vector. */
              const float *aEnc = (const float*)aTransform;
              if( (pMod->hdr.flags & VEC1_MODEL_RESIDUAL) ){
                vec1Sub(aResidual, aEnc, &pMod->aCentroid[iCalc*nElem], nElem);
                aEnc = aResidual;
              }
              if( vec1IntegrityCheckEncoded(&rc, pMod, aEnc, ii, aBlob, &tmp) ){
