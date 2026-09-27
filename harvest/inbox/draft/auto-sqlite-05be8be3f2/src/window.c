// AUTO-DRAFT from sqlite/sqlite PR #ac70487e12adc0633dda9ef1598a6803d819f614
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

struct CallCount {
  i64 nValue;
  i64 nStep;
  i64 nTotal;
};
/* …（同文件无关代码省略）… */
  UNUSED_PARAMETER(nArg); assert( nArg==0 );
  UNUSED_PARAMETER(apArg);
  p = (struct CallCount*)sqlite3_aggregate_context(pCtx, sizeof(*p));
  p->nStep++;
}
static void percent_rankValueFunc(sqlite3_context *pCtx){
/* …（同文件无关代码省略）… */
  UNUSED_PARAMETER(nArg); assert( nArg==0 );
  UNUSED_PARAMETER(apArg);
  p = (struct CallCount*)sqlite3_aggregate_context(pCtx, sizeof(*p));
  p->nStep++;
}
static void cume_distValueFunc(sqlite3_context *pCtx){
/* …（同文件无关代码省略）… */
struct NtileCtx {
  i64 nTotal;                     /* Total rows in partition */
  i64 nParam;                     /* Parameter passed to ntile(N) */
  i64 iRow;                       /* Current row */
};
/* …（同文件无关代码省略）… */
  assert( nArg==1 ); UNUSED_PARAMETER(nArg);
  UNUSED_PARAMETER(apArg);
  p = (struct NtileCtx*)sqlite3_aggregate_context(pCtx, sizeof(*p));
  p->iRow++;
}
static void ntileValueFunc(sqlite3_context *pCtx){
