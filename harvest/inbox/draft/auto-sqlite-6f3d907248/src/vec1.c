// AUTO-DRAFT from sqlite/sqlite PR #a27bcfe21c9b4e0989e86eec8fb34a3832cd23c7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define VEC1_DISTANCE_COS  2
/* …（同文件无关代码省略）… */
typedef sqlite3_int64  i64;
typedef sqlite3_uint64 u64;
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
typedef struct Vec1Tab Vec1Tab;
/* …（同文件无关代码省略）… */
#define VEC1_MODEL_INDEX    0x01
#define VEC1_MODEL_ROTATE   0x02
/* …（同文件无关代码省略）… */
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
static int vec1BufferGrow(Vec1Buffer *pBuf, sqlite3_int64 nByte){
  if( nByte>pBuf->nAlloc-pBuf->n ){
    sqlite3_int64 nNew = pBuf->nAlloc;
    unsigned char *aNew = 0;

    if( nNew==0 ) nNew = nByte + 100;
    while( nNew < (pBuf->n + nByte) ){
      nNew = nNew * 2;
    }
    aNew = sqlite3_realloc64(pBuf->a, nNew);
    if( aNew==0 ){
      return SQLITE_NOMEM;
    }
    pBuf->a = aNew;
    pBuf->nAlloc = (int)nNew;
  }

  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static void vec1BufferFree(Vec1Buffer *pBuf){
  sqlite3_free(pBuf->a);
  memset(pBuf, 0, sizeof(Vec1Buffer));
}
/* …（同文件无关代码省略）… */
static u64 vec1GetU64(const u8 *aBuf){
  return (((u64)aBuf[0]) << 56)
       + (((u64)aBuf[1]) << 48)
       + (((u64)aBuf[2]) << 40)
       + (((u64)aBuf[3]) << 32) 
       + (((u64)aBuf[4]) << 24)
       + (((u64)aBuf[5]) << 16)
       + (((u64)aBuf[6]) <<  8)
       + (((u64)aBuf[7]) <<  0);
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
static int vec1BestMatchSimple(
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

/*
** Return a pseudo-random positive 32-bit value.
*/
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
#define VEC1_TOMBSTONE_32    0xFFFFFFFF
#define VEC1_TOMBSTONE_64             0
/* …（同文件无关代码省略）… */
typedef struct Vec1ListBuilder Vec1ListBuilder;
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
static void vec1StmtReset(int *pRc, sqlite3_stmt *pStmt){
  int rc = sqlite3_reset(pStmt);
  if( *pRc==SQLITE_OK ) *pRc = rc;
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
typedef struct Vec1RowidLocation Vec1RowidLocation;
/* …（同文件无关代码省略）… */
static int vec1FindByRowid(
  Vec1Tab *pTab, 
  int iBucket,                    /* Row belongs to this bucket */
  int bWrite,                     /* Use writable blob-handle */
  i64 iRowid,                     /* Rowid to locate */
  Vec1RowidLocation *pOut         /* OUT: Result */
){
  int rc = SQLITE_OK;
  Vec1Buffer buf = {0, 0, 0};
  sqlite3_stmt *pSearch = 0;      /* For looping through index entries */
  sqlite3_blob *pBlob = 0;

  assert( (pTab->mod.hdr.flags & VEC1_MODEL_INDEX) );

  memset(pOut, 0, sizeof(*pOut));

  /* Check to see if the index entry for this vector is currently 
  ** in main memory. */
  if( pTab->pWriter ){
    Vec1ListBuilder *pBld = &pTab->pWriter->aBld[iBucket];
    int ii;
    for(ii=0; ii<pBld->bufRowid.n; ii+=pBld->szRowid){
      i64 iRead;
      if( pBld->szRowid==4 ){
        iRead = vec1GetU32(&pBld->bufRowid.a[ii]);
      }else{
        iRead = vec1GetU64(&pBld->bufRowid.a[ii]);
      }
      if( iRead==iRowid ){
        pOut->iEntry = (ii / pBld->szRowid);
        pOut->nEntry = (pBld->bufRowid.n / pBld->szRowid);
        pOut->nTombstone = pBld->nTombstone;
        pOut->szRowid = pBld->szRowid;
        pOut->pList = pBld;
        return SQLITE_OK;
      }
    }
  }

  rc = vec1GetSql(pTab, VEC1_SQL_SEARCH_IDX, &pSearch);
  if( rc==SQLITE_OK ){
    sqlite3_bind_int(pSearch, 1, iBucket);
    sqlite3_bind_int64(pSearch, 2, iRowid);
  }

  while( rc==SQLITE_OK && SQLITE_ROW==sqlite3_step(pSearch) ){
    int nEntry = 0;
    u32 nTombstone = 0;
    u32 flags = 0;
    int szRowid = 4;
    i64 iIdx = sqlite3_column_int64(pSearch, 0);

    rc = sqlite3_blob_open(
        pTab->db, pTab->zDb, pTab->zIdxTbl, "val", iIdx, bWrite, &pBlob
    );
    if( rc==SQLITE_OK ){
      u8 a[VEC1_LIST_SZHDR];
      rc = sqlite3_blob_read(pBlob, a, VEC1_LIST_SZHDR, 0);
      flags = vec1GetU32(&a[0]);
      nEntry = (int)vec1GetU32(&a[4]);
      nTombstone = vec1GetU32(&a[8]);
      if( flags & VEC1_LIST_64BIT ) szRowid = 8;
    }

    /* Now scan through the array of rowids at the start of the %_idx entry
    ** to find the rowid to be deleted. But, do not do this if the rowid
    ** happens to have the same value as the tombstone for this %_idx entry. */
    if( (szRowid==4 && iRowid!=VEC1_TOMBSTONE_32) 
     || (szRowid==8 && iRowid!=VEC1_TOMBSTONE_64)
    ){
      if( rc==SQLITE_OK ){
        buf.n = 0;
        rc = vec1BufferGrow(&buf, VEC1_LIST_SZHDR+szRowid*nEntry);
      }
      if( rc==SQLITE_OK ){
        rc = sqlite3_blob_read(
            pBlob, &buf.a[VEC1_LIST_SZHDR], nEntry*szRowid, VEC1_LIST_SZHDR
        );
      }

      if( rc==SQLITE_OK ){
        int ii;
        for(ii=0; ii<nEntry; ii++){
          i64 iRead = 0;
          if( szRowid==4 ){
            iRead = vec1GetU32(&buf.a[VEC1_LIST_SZHDR + ii*szRowid]);
          }else{
            iRead = vec1GetU64(&buf.a[VEC1_LIST_SZHDR + ii*szRowid]);
          }
          if( iRead==iRowid ){
            pOut->iEntry = ii;
            pOut->nEntry = nEntry;
            pOut->nTombstone = nTombstone;
            pOut->szRowid = szRowid;
            pOut->pBlob = pBlob;
            pOut->iIdx = iIdx;
            pBlob = 0;
            goto findbyrowid_done;
          }
        }
      }
    }
    (void)sqlite3_blob_close(pBlob);
  }
  
 findbyrowid_done:
  vec1StmtReset(&rc, pSearch);
  vec1BufferFree(&buf);
  return rc;
}
/* …（同文件无关代码省略）… */
*/
#define VEC1_TOMBSTONE_THRESHOLD(nEntry) ((nEntry * 2) / 10)

/*
** Delete the %_base entry for the row with rowid=iRowid. Also zero-out
** the corresponding %_idx entry.
/* …（同文件无关代码省略）… */

  rc = vec1GetSql(pTab, VEC1_SQL_DEL_LOOKUP_BASE, &pDelete);
  if( rc==SQLITE_OK ){
    int iBucket = 0;

    /* Delete the row from the %_base table. Also set variable iBucket to
    ** the index of the bucket containing the vector.  */
    sqlite3_bind_int64(pDelete, 1, iRowid);
    if( SQLITE_ROW==sqlite3_step(pDelete) && pTab->mod.hdr.nBucket>0 ){
      if( pTab->mod.hdr.nCodebook==0 ){
        iBucket = sqlite3_column_int(pDelete, 0);
      }else{
        const u8 *aVec = sqlite3_column_blob(pDelete, 0);
        int nVec = sqlite3_column_bytes(pDelete, 0);
        const float *aTransform = 0;

        aTransform = vec1TransformInputVector(
            &pTab->mod, pTab->aTmpVec, (const float*)aVec
        );

        if( nVec!=(pTab->cfg.nElem*sizeof_f32) ){
          rc = VEC1_CORRUPT;
        }else{
          iBucket = vec1BestMatchSimple(
              pTab->mod.aCentroid, pTab->mod.hdr.nBucket, 
              aTransform, pTab->cfg.nElem, 0
          );
        }
      }
    }

    vec1StmtReset(&rc, pDelete);

    if( rc==SQLITE_OK && (pTab->mod.hdr.flags & VEC1_MODEL_INDEX) ){
      Vec1RowidLocation loc;

      rc = vec1FindByRowid(pTab, iBucket, 1, iRowid, &loc);

      if( rc==SQLITE_OK ){
        if( loc.pList==0 && loc.pBlob==0 ){
/* …（同文件无关代码省略）… */
        }
      }
    }
  }

  return rc;
/* …（同文件无关代码省略）… */
}

/*
** Return true if the difference between f1 and f2 is greater than 
** (fTol * max(f1, f2)). Or false if they differ by more than this.
*/
static int vec1FloatWithinTolerance(float f1, float f2, float fTol){
/* …（同文件无关代码省略）… */
){
  const int nElem = (int)pMod->hdr.nElem;
  const int nBucket = (int)pMod->hdr.nBucket;
  const float fTol = 0.00001f;
  double fBest = 0.0;
  double fDist = 0.0;
  int iBest = 0;
/* …（同文件无关代码省略）… */
  if( iBest==iBucket ) return 0;

  fDist = vec1L2Dist(aVec, &pMod->aCentroid[iBucket*nElem], nElem);
  if( fDist==fBest ) return 0;
  return vec1FloatWithinTolerance((float)fBest, (float)fDist, fTol)==0;
}

/*
