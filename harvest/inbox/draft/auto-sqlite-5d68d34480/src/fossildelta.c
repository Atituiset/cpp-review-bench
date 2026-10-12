// AUTO-DRAFT from sqlite/sqlite PR #891eb44d4dfe8c1afc304d6bf0c17fe7376fd8c1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
** that contain deltas.
**
** Typical build commands assuming
** 
**    DIR=ext/misc
*     NAME=fossildelta
**
/* …（同文件无关代码省略）… */
typedef sqlite3_uint64 u64;
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
static unsigned int deltaGetInt(const char **pz, int *pLen){
  static const signed char zValue[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,    8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, 16,   17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32,   33, 34, 35, -1, -1, -1, -1, 36,
    -1, 37, 38, 39, 40, 41, 42, 43,   44, 45, 46, 47, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 58, 59,   60, 61, 62, -1, -1, -1, 63, -1,

    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,   -1, -1, -1, -1, -1, -1, -1, -1,
  };
  unsigned int v = 0;
  int c;
  unsigned char *z = (unsigned char*)*pz;
  unsigned char *zEnd = z + (*pLen);
  while( z<zEnd && (c = zValue[*z])>=0 ){
    v = (v<<6) + c;
    z++;
  }

  *pLen -= (int)(z - (unsigned char*)*pz);
  *pz = (char*)z;
  return v;
}
/* …（同文件无关代码省略）… */
# define GCC_VERSION (__GNUC__*1000000+__GNUC_MINOR__*1000+__GNUC_PATCHLEVEL__)
/* …（同文件无关代码省略）… */
static unsigned int checksum(const char *zIn, size_t N){
  static const int byteOrderTest = 1;
  const unsigned char *z = (const unsigned char *)zIn;
  const unsigned char *zEnd = (const unsigned char*)&zIn[N&~3];
  unsigned sum = 0;
  assert( (z - (const unsigned char*)0)%4==0 );  /* Four-byte alignment */
  if( 0==*(char*)&byteOrderTest ){
    /* This is a big-endian machine */
    while( z<zEnd ){
      sum += *(unsigned*)z;
      z += 4;
    }
  }else{
    /* A little-endian machine */
#if GCC_VERSION>=4003000
    while( z<zEnd ){
      sum += __builtin_bswap32(*(unsigned*)z);
      z += 4;
    }
#elif defined(_MSC_VER) && _MSC_VER>=1300
    while( z<zEnd ){
      sum += _byteswap_ulong(*(unsigned*)z);
      z += 4;
    }
#else
    unsigned sum0 = 0;
    unsigned sum1 = 0;
    unsigned sum2 = 0;
    while(N >= 16){
      sum0 += ((unsigned)z[0] + z[4] + z[8] + z[12]);
      sum1 += ((unsigned)z[1] + z[5] + z[9] + z[13]);
      sum2 += ((unsigned)z[2] + z[6] + z[10]+ z[14]);
      sum  += ((unsigned)z[3] + z[7] + z[11]+ z[15]);
      z += 16;
      N -= 16;
    }
    while(N >= 4){
      sum0 += z[0];
      sum1 += z[1];
      sum2 += z[2];
      sum  += z[3];
      z += 4;
      N -= 4;
    }
    sum += (sum2 << 8) + (sum1 << 16) + (sum0 << 24);
#endif
  }
  switch(N&3){
    case 3:   sum += (z[2] << 8);
    case 2:   sum += (z[1] << 16);
    case 1:   sum += (z[0] << 24);
    default:  ;
  }
  return sum;
}
/* …（同文件无关代码省略）… */

/*
** Return the size (in bytes) of the output from applying
** a delta.
**
** This routine is provided so that an procedure that is able
** to call delta_apply() can learn how much space is required
** for the output and hence allocate nor more space that is really
** needed.
*/
static int delta_output_size(const char *zDelta, int lenDelta){
  int size;
  size = deltaGetInt(&zDelta, &lenDelta);
  if( lenDelta<=0 || *zDelta!='\n' ){
    /* ERROR: size integer not terminated by "\n" */
/* …（同文件无关代码省略）… */
static int delta_apply(
  const char *zSrc,      /* The source or pattern file */
  int lenSrc,            /* Length of the source file */
  const char *zDelta,    /* Delta to apply to the pattern */
  int lenDelta,          /* Length of the delta */
  char *zOut             /* Write the output into this preallocated buffer */
){
  sqlite3_uint64 limit;
  sqlite3_uint64 total = 0;
#ifdef FOSSIL_ENABLE_DELTA_CKSUM_TEST
  char *zOrigOut = zOut;
#endif

  limit = deltaGetInt(&zDelta, &lenDelta);
  if( lenDelta<=0 || *zDelta!='\n' ){
    /* ERROR: size integer not terminated by "\n" */
    return -1;
  }
  zDelta++; lenDelta--; /* Skip the \n */
  while( lenDelta>0 && zDelta[0] ){
    unsigned int cnt, ofst;
    cnt = deltaGetInt(&zDelta, &lenDelta);
    if( lenDelta<=0 ) return -1;
    switch( zDelta[0] ){
      case '@': {
        zDelta++; lenDelta--;
        ofst = deltaGetInt(&zDelta, &lenDelta);
        if( lenDelta>0 && zDelta[0]!=',' ){
          /* ERROR: copy command not terminated by ',' */
          return -1;
        }
        zDelta++; lenDelta--;
        total += cnt;
        if( total>limit ){
          /* ERROR: copy exceeds output file size */
          return -1;
        }
        if( (u64)ofst+(u64)cnt > (u64)lenSrc ){
          /* ERROR: copy extends past end of input */
          return -1;
        }
        memcpy(zOut, &zSrc[ofst], cnt);
        zOut += cnt;
        break;
      }
      case ':': {
        zDelta++; lenDelta--;
        total += cnt;
        if( total>limit ){
          /* ERROR:  insert command gives an output larger than predicted */
          return -1;
        }
        if( cnt>lenDelta ){
          /* ERROR: insert count exceeds size of delta */
          return -1;
        }
        memcpy(zOut, zDelta, cnt);
        zOut += cnt;
        zDelta += cnt;
        lenDelta -= cnt;
        break;
      }
      case ';': {
        zDelta++; lenDelta--;
        zOut[0] = 0;
#ifdef FOSSIL_ENABLE_DELTA_CKSUM_TEST
        if( cnt!=checksum(zOrigOut, total) ){
          /* ERROR:  bad checksum */
          return -1;
        }
#endif
        if( total!=limit 
