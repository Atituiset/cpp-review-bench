// AUTO-DRAFT from sqlite/sqlite PR #f2375631927a6893f9fcdb6a7c05bade769f53b8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
#define etFLOAT       1 /* Floating point.  %f */
/* …（同文件无关代码省略）… */
#define etGENERIC     3 /* Floating or exponential, depending on exponent. %g */
/* …（同文件无关代码省略）… */
void sqlite3StrAccumSetError(StrAccum *p, u8 eError){
  assert( eError==SQLITE_NOMEM || eError==SQLITE_TOOBIG );
  p->accError = eError;
  if( p->mxAlloc ) sqlite3_str_reset(p);
  if( eError==SQLITE_TOOBIG ) sqlite3ErrorToParser(p->db, eError);
}
/* …（同文件无关代码省略）… */
# define SQLITE_PRINT_BUF_SIZE 70
/* …（同文件无关代码省略）… */
#define etBUFSIZE SQLITE_PRINT_BUF_SIZE  /* Size of the output buffer */

/*
** Hard limit on the precision of floating-point conversions.
*/
#ifndef SQLITE_PRINTF_PRECISION_LIMIT
# define SQLITE_FP_PRECISION_LIMIT 100000000
#endif

/* …（同文件无关代码省略）… */
          realvalue = va_arg(ap,double);
        }
        if( precision<0 ) precision = 6;         /* Set default precision */
#ifdef SQLITE_FP_PRECISION_LIMIT
        if( precision>SQLITE_FP_PRECISION_LIMIT ){
          precision = SQLITE_FP_PRECISION_LIMIT;
        }
#endif
        if( xtype==etFLOAT ){
          iRound = -precision;
        }else if( xtype==etGENERIC ){
/* …（同文件无关代码省略）… */
            /* Unable to allocate space in pAccum, perhaps because it
            ** is coming from sqlite3_snprintf() or similar.  We'll have
            ** to render into temporary space and the memcpy() it over. */
            bufpt = sqlite3_malloc(szBufNeeded);
            if( bufpt==0 ){
              sqlite3StrAccumSetError(pAccum, SQLITE_NOMEM);
              return;
/* …（同文件无关代码省略）… */
void sqlite3_str_reset(StrAccum *p){
  if( isMalloced(p) ){
    sqlite3DbFree(p->db, p->zText);
    p->printfFlags &= ~SQLITE_PRINTF_MALLOCED;
  }else if( p==(sqlite3_str*)&sqlite3OomStr ){
    return;
  }
  p->nAlloc = 0;
  p->nChar = 0;
  p->zText = 0;
}
