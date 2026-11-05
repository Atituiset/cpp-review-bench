// AUTO-DRAFT from sqlite/sqlite PR #edb49992bb532644f696f8e488a51860acf9439a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
  // <<< BUG ANCHOR
#define JSONB_INT      3   /* integer acceptable to JSON and SQL */
#define JSONB_INT5     4   /* integer in 0x000 notation */
/* …（同文件无关代码省略）… */
#define JSONB_FLOAT5   6   /* float with JSON5 extensions */
/* …（同文件无关代码省略）… */
typedef struct JsonString JsonString;
typedef struct JsonParse JsonParse;
/* …（同文件无关代码省略）… */
#define JSTRING_OOM         0x01   /* Out of memory */
#define JSTRING_MALFORMED   0x02   /* Malformed JSONB */
/* …（同文件无关代码省略）… */
static void jsonStringZero(JsonString *p){
  p->zBuf = p->zSpace;
  p->nAlloc = sizeof(p->zSpace);
  p->nUsed = 0;
  p->bStatic = 1;
}
/* …（同文件无关代码省略）… */
static void jsonStringReset(JsonString *p){
  if( !p->bStatic ) sqlite3RCStrUnref(p->zBuf);
  jsonStringZero(p);
}
/* …（同文件无关代码省略）… */
static void jsonStringOom(JsonString *p){
  p->eErr |= JSTRING_OOM;
  if( p->pCtx ) sqlite3_result_error_nomem(p->pCtx);
  jsonStringReset(p);
}
/* …（同文件无关代码省略）… */
static int jsonStringGrow(JsonString *p, u32 N){
  u64 nTotal = N<p->nAlloc ? p->nAlloc*2 : p->nAlloc+N+10;
  char *zNew;
  if( p->bStatic ){
    if( p->eErr ) return 1;
    zNew = sqlite3RCStrNew(nTotal);
    if( zNew==0 ){
      jsonStringOom(p);
      return SQLITE_NOMEM;
    }
    memcpy(zNew, p->zBuf, (size_t)p->nUsed);
    p->zBuf = zNew;
    p->bStatic = 0;
  }else{
    p->zBuf = sqlite3RCStrResize(p->zBuf, nTotal);
    if( p->zBuf==0 ){
      p->eErr |= JSTRING_OOM;
      jsonStringZero(p);
      return SQLITE_NOMEM;
    }
  }
  p->nAlloc = nTotal;
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static void jsonPrintf(int N, JsonString *p, const char *zFormat, ...){
  va_list ap;
  if( (p->nUsed + N >= p->nAlloc) && jsonStringGrow(p, N) ) return;
  va_start(ap, zFormat);
  sqlite3_vsnprintf(N, p->zBuf+p->nUsed, zFormat, ap);
  va_end(ap);
  p->nUsed += (int)strlen(p->zBuf+p->nUsed);
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE void jsonAppendCharExpand(JsonString *p, char c){
  if( jsonStringGrow(p,1) ) return;
  p->zBuf[p->nUsed++] = c;
}
static void jsonAppendChar(JsonString *p, char c){
  if( p->nUsed>=p->nAlloc ){
    jsonAppendCharExpand(p,c);
  }else{
    p->zBuf[p->nUsed++] = c;
  }
}
/* …（同文件无关代码省略）… */
static u32 jsonbPayloadSizeWide(
  const JsonParse *pParse,   /* JSON parsing context */
  u32 i,                     /* Index of the node type-code */
  u32 *pSz,                  /* Write payload size here */
  u8 x                       /* pParse->aBlob[i]>>4 */
){
  u32 sz;
  u32 n;
  assert( i<pParse->nBlob );
  assert( x==(pParse->aBlob[i]>>4) );
  assert( x>=13 );
  if( x==13 ){
    if( i+2>=pParse->nBlob ){
      *pSz = 0;
      return 0;
    }
    sz = (pParse->aBlob[i+1]<<8) + pParse->aBlob[i+2];
    n = 3;
  }else if( x==14 ){
    if( i+4>=pParse->nBlob ){
      *pSz = 0;
      return 0;
    }
    sz = ((u32)pParse->aBlob[i+1]<<24) + (pParse->aBlob[i+2]<<16) +
         (pParse->aBlob[i+3]<<8) + pParse->aBlob[i+4];
    n = 5;
  }else{
    if( i+8>=pParse->nBlob
     || pParse->aBlob[i+1]!=0
     || pParse->aBlob[i+2]!=0
     || pParse->aBlob[i+3]!=0
     || pParse->aBlob[i+4]!=0
    ){
      *pSz = 0;
      return 0;
    }
    sz = ((u32)pParse->aBlob[i+5]<<24) + (pParse->aBlob[i+6]<<16) +
         (pParse->aBlob[i+7]<<8) + pParse->aBlob[i+8];
    n = 9;
  }
  testcase( (i64)i+sz+n > pParse->nBlob );
  testcase( (i64)i+sz+n > pParse->nBlob-pParse->delta
         && (i64)i+sz+n > pParse->nBlob );
  testcase( pParse->delta>0 );
  testcase( pParse->delta<0 );
  /* Quirks with -Os and gcov cause the NO_TEST line below to show up as
  ** false-positive coverage miss.  The testcases() macros above are
  ** sufficient to prove that the branch is in fact covered */
  if( (i64)i+sz+n > pParse->nBlob
   && (i64)i+sz+n > pParse->nBlob-pParse->delta  /*NO_TEST*/
  ){
    *pSz = 0;
    return 0;
  }
  *pSz = sz;
  return n;
}
/* …（同文件无关代码省略）… */
static u32 jsonbPayloadSize(
  const JsonParse *pParse,   /* JSON parsing context */
  u32 i,                     /* Index of the node type-code */
  u32 *pSz                   /* Write payload size here */
){
  u8 x;
  u32 sz;
  u32 n;
  if( i>=pParse->nBlob ){
    *pSz = 0;
    return 0;
  }else if( (x = pParse->aBlob[i]>>4)<=11 ){
    sz = x;
    n = 1;
  }else if( x==12 ){
    if( i+1>=pParse->nBlob ){
      *pSz = 0;
      return 0;
    }
    sz = pParse->aBlob[i+1];
    n = 2;
  }else{
    return jsonbPayloadSizeWide(pParse, i, pSz, x);
  }
  if( (i64)i+sz+n > pParse->nBlob
   && (i64)i+sz+n > pParse->nBlob-pParse->delta
  ){
    *pSz = 0;
    return 0;
  }
  *pSz = sz;
  return n;
}
/* …（同文件无关代码省略）… */
# define jsonbPayloadSizeSemiInline(a,b,c) jsonbPayloadSize(a,b,c)
#endif

/*
** Translate the binary JSONB representation of JSON beginning at
** pParse->aBlob[i] into a JSON text string.  Append the JSON
/* …（同文件无关代码省略）… */
      break;
    }
    case JSONB_INT5: {  /* Integer literal in hexadecimal notation */
      u32 k = 2;
      sqlite3_uint64 u = 0;
      const char *zIn = (const char*)&pParse->aBlob[i+n];
      int bOverflow = 0;
      if( sz==0 ) goto malformed_jsonb;
      if( zIn[0]=='-' ){
        jsonAppendChar(pOut, '-');
        k++;
      }else if( zIn[0]=='+' ){
        k++;
      }
      for(; k<sz; k++){
        if( !sqlite3Isxdigit(zIn[k]) ){
          pOut->eErr |= JSTRING_MALFORMED;
          break;
        }else if( (u>>60)!=0 ){
          bOverflow = 1;
        }else{
          u = u*16 + sqlite3HexToInt(zIn[k]);
        }
      }
      jsonPrintf(100,pOut,bOverflow?"9.0e999":"%llu", u);
      break;
    }
    case JSONB_FLOAT5: { /* Float literal missing digits beside "." */
      u32 k = 0;
/* …（同文件无关代码省略）… */
    }
    case JSONB_INT5:
    case JSONB_INT: {
      sqlite3_int64 iRes = 0;
      char *z;
      int bNeg = 0;
      char x;
      if( sz==0 ) goto returnfromblob_malformed;
      x = (char)pParse->aBlob[i+n];
      if( x=='-' ){
        if( sz<2 ) goto returnfromblob_malformed;
        n++;
     
