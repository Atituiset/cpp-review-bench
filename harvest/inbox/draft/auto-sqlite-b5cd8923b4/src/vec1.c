// AUTO-DRAFT from sqlite/sqlite PR #012589ecabb5debd4b9a37e96da027e4f41b8838
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
#define VEC1_DISTANCE_COS  2
/* …（同文件无关代码省略）… */
/*
** Software version. Returned by "SELECT vec1_info()".
*/
#define VEC1_VERSION "0.7"

/*
** Values for Vec1ModelHeader.iVersion:
/* …（同文件无关代码省略）… */
typedef sqlite3_int64  i64;
typedef sqlite3_uint64 u64;
typedef unsigned char   u8;
typedef unsigned int   u32;
/* …（同文件无关代码省略）… */

#if !defined(VEC1_TEST_SCALAR)

#define VERTICAL_SUM_GENERIC(aAcc, nAcc, I) {                          \
  int c;                                                               \
  for(c=4; c<nAcc; c++) aAcc[c & 3] = I (aAcc[c & 3], aAcc[c]);        \
  for(c=2; c<4 && c<nAcc; c++) aAcc[c & 1] = I (aAcc[c & 1], aAcc[c]); \
  if( nAcc>1 ) aAcc[0] = I (aAcc[0], aAcc[1]);                         \
}

#ifdef __AVX2__
/* …（同文件无关代码省略）… */
# define VEC1_HAVE_AVX2 1
/* …（同文件无关代码省略）… */
# define VEC1_TRANSPOSE_WIDTH 32
# define VEC1_NTRANSPOSE_WIDTH 8
/* …（同文件无关代码省略）… */
# define VEC1_MULTIMATCH_NVEC  8
# define VEC1_MULTIMATCH_NCENT 8
/* …（同文件无关代码省略）… */
# define VEC1_USE_TRANSPOSITION 1
/* …（同文件无关代码省略）… */
# define VEC1_COVAR_MULTIMATCH_MULT 1
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
static u64 vec1HardwareTimer(void){
  u64 ret = 0;

#if !defined(__STRICT_ANSI__)
# if defined(__aarch64__)
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(ret));
# elif defined(__GNUC__) && defined(__x86_64__)
  unsigned int lo, hi;
  __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi) :: "%rcx");
  ret = ((u64)hi << 32) | lo;
# endif
#endif

  return ret;
}
/* …（同文件无关代码省略）… */
#define size_of_array(X) ((int)(sizeof(X)/sizeof((X)[0])))
#define MAX(a,b) ((a<b) ? (b) : (a))
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
#define sizeof_f32 4
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
  if( pBuf->n+nByte>pBuf->nAlloc ){
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
static void vec1Sub(float *aOut, const float *a1, const float *a2, int n){
  int i;
  for(i=0; i<n; i++){
    aOut[i] = a1[i] - a2[i];
  }
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
