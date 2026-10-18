// AUTO-DRAFT from sqlite/sqlite PR #43a48f77585a88940c727f33682151418f544059
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
# include <stdio.h>
# include <stdlib.h>

/* Standard sized types. */
typedef sqlite3_int64  i64;
typedef sqlite3_uint64 u64;
typedef unsigned char   u8;
typedef unsigned int   u32;
typedef unsigned short u16;
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
# define HORIZONTAL_SUM64(s, vec) \
    {                                                              \
      __m128d _lo = _mm256_castpd256_pd128(vec);                   \
      __m128d _hi = _mm256_extractf128_pd(vec, 1);                 \
      __m128d _sum = _mm_add_pd(_lo, _hi);                         \
      (s) = _mm_cvtsd_f64(_mm_hadd_pd(_sum, _sum));                \
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
/* …（同文件无关代码省略）… */
#define MIN(a,b) ((a>b) ? (b) : (a))
/* …（同文件无关代码省略）… */
#define UNUSED_PARAMETER(x)    (void)(x)
#define UNUSED_PARAMETER2(x,y) (void)(x),(void)(y)
/* …（同文件无关代码省略）… */
typedef struct Vec1TabList Vec1TabList;
/* …（同文件无关代码省略）… */
#define VEC1_MODEL_BITQUANT 0x08
/* …（同文件无关代码省略）… */
#define sizeof_u32 4
/* …（同文件无关代码省略）… */
*/
#define VEC1_MAX_NTHREAD 128

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
  const char *zJson = (const char*)sqlite3_value_text(aVal[0]);
  int nJson = sqlite3_value_bytes(aVal[0]);
  const char *p = zJson;

  assert( nVal==1 );
  UNUSED_PARAMETER(nVal);

  float *aVec = 0;
  int nVec = 0;

  /* vec1_from_json(NULL) returns NULL */
  if( sqlite3_value_type(aVal[0])==SQLITE_NULL ) return;

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
      acc[c] = FMADD(d[c], d[c], a
