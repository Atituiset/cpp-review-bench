// AUTO-DRAFT from sqlite/sqlite PR #2acb2ea9089c604d5786f56ee3e50a0479a268dd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define JSONB_NULL     0   /* "null" */
#define JSONB_TRUE     1   /* "true" */
#define JSONB_FALSE    2   /* "false" */
#define JSONB_INT      3   /* integer acceptable to JSON and SQL */
#define JSONB_INT5     4   /* integer in 0x000 notation */
#define JSONB_FLOAT    5   /* float acceptable to JSON and SQL */
#define JSONB_FLOAT5   6   /* float with JSON5 extensions */
#define JSONB_TEXT     7   /* Text compatible with both JSON and SQL */
#define JSONB_TEXTJ    8   /* Text with JSON escapes */
#define JSONB_TEXT5    9   /* Text with JSON-5 escape */
#define JSONB_TEXTRAW 10   /* SQL text that needs escaping for JSON */
#define JSONB_ARRAY   11   /* An array */
#define JSONB_OBJECT  12   /* An object */
/* …（同文件无关代码省略）… */
typedef struct JsonString JsonString;
typedef struct JsonParse JsonParse;
/* …（同文件无关代码省略）… */
#define JSTRING_OOM         0x01   /* Out of memory */
#define JSTRING_MALFORMED   0x02   /* Malformed JSONB */
#define JSTRING_TOODEEP     0x04   /* JSON nested too deep */
/* …（同文件无关代码省略）… */
# define JSON_MAX_DEPTH  1000
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
static void jsonStringTooDeep(JsonString *p){
  p->eErr |= JSTRING_TOODEEP;
  assert( p->pCtx!=0 );
  sqlite3_result_error(p->pCtx, "JSON nested too deep", -1);
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
static SQLITE_NOINLINE void jsonStringExpandAndAppend(
  JsonString *p,
  const char *zIn,
  u32 N
){
  assert( N>0 );
  if( jsonStringGrow(p,N) ) return;
  memcpy(p->zBuf+p->nUsed, zIn, N);
  p->nUsed += N;
}
static void jsonAppendRaw(JsonString *p, const char *zIn, u32 N){
  if( N==0 ) return;
  if( N+p->nUsed >= p->nAlloc ){
    jsonStringExpandAndAppend(p,zIn,N);
  }else{
    memcpy(p->zBuf+p->nUsed, zIn, N);
    p->nUsed += N;
  }
}
static void jsonAppendRawNZ(JsonString *p, const char *zIn, u32 N){
  assert( N>0 );
  if( N+p->nUsed >= p->nAlloc ){
    jsonStringExpandAndAppend(p,zIn,N);
  }else{
    memcpy(p->zBuf+p->nUsed, zIn, N);
    p->nUsed += N;
  }
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
static void jsonStringTrimOneChar(JsonString *p){
  if( p->eErr==0 ){
    assert( p->nUsed>0 );
    p->nUsed--;
  }
}
/* …（同文件无关代码省略）… */
static void jsonAppendControlChar(JsonString *p, u8 c){
  static const char aSpecial[] = {
     0, 0, 0, 0, 0, 0, 0, 0, 'b', 't', 'n', 0, 'f', 'r', 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,   0,   0,   0, 0,   0,   0, 0, 0
  };
  assert( sizeof(aSpecial)==32 );
  assert( aSpecial['\b']=='b' );
  assert( aSpecial['\f']=='f' );
  assert( aSpecial['\n']=='n' );
  assert( aSpecial['\r']=='r' );
  assert( aSpecial['\t']=='t' );
  assert( c>=0 && c<sizeof(aSpecial) );
  assert( p->nUsed+7 <= p->nAlloc );
  if( aSpecial[c] ){
    p->zBuf[p->nUsed] = '\\';
    p->zBuf[p->nUsed+1] = aSpecial[c];
    p->nUsed += 2;
  }else{
    p->zBuf[p->nUsed] = '\\';
    p->zBuf[p->nUsed+1] = 'u';
    p->zBuf[p->nUsed+2] = '0';
    p->zBuf[p->nUsed+3] = '0';
    p->zBuf[p->nUsed+4] = "0123456789abcdef"[c>>4];
    p->zBuf[p->nUsed+5] = "0123456789abcdef"[c&0xf];
    p->nUsed += 6;
  }
}
/* …（同文件无关代码省略）… */
static void jsonAppendString(JsonString *p, const char *zIn, u32 N){
  u32 k;
  u8 c;
  const u8 *z = (const u8*)zIn;
  if( z==0 ) return;
  if( (N+p->nUsed+2 >= p->nAlloc) && jsonStringGrow(p,N+2)!=0 ) return;
  p->zBuf[p->nUsed++] = '"';
  while( 1 /*exit-by-break*/ ){
    k = 0;
    /* The following while() is the 4-way unwound equivalent of
    **
    **     while( k<N && jsonIsOk[z[k]] ){ k++; }
    */
    while( 1 /* Exit by break */ ){
      if( k+3>=N ){
        while( k<N && jsonIsOk[z[k]] ){ k++; }
        break;
      }
      if( !jsonIsOk[z[k]] ){
        break;
      }
      if( !jsonIsOk[z[k+1]] ){
        k += 1;
        break;
      }
      if( !jsonIsOk[z[k+2]] ){
        k += 2;
        break;
      }
      if( !jsonIsOk[z[k+3]] ){
        k += 3;
        break;
      }else{
        k += 4;
      }
    }
    if( k>=N ){
      if( k>0 ){
        memcpy(&p->zBuf[p->nUsed], z, k);
        p->nUsed += k;
      }
      break;
    }
    if( k>0 ){
      memcpy(&p->zBuf[p->nUsed], z, k);
      p->nUsed += k;
      z += k;
      N -= k;
    }
    c = z[0];
    if( c=='"' || c=='\\' ){
      if( (p->nUsed+N+3 > p->nAlloc) && jsonStringGrow(p,N+3)!=0 ) return;
      p->zBuf[p->nUsed++] = '\\';
      p->zBuf[p->nUsed++] = c;
    }else if( c=='\'' ){
      p->zBuf[p->nUsed++] = c;
    }else{
      if( (p->nUsed+N+7 > p->nAlloc) && jsonStringGrow(p,N+7)!=0 ) return;
      jsonAppendControlChar(p, c);
    }
    z++;
    N--;
  }
  p->zBuf[p->nUsed++] = '"';
  assert( p->nUsed<p->nAlloc );
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
# define jsonbPayloadSizeInline(a,b,c) jsonbPayloadSize(a,b,c)
/* …（同文件无关代码省略）… */
static u32 jsonTranslateBlobToText(
  JsonParse *pParse,             /* the complete parse of the JSON */
  u32 i,                         /* Start rendering at this index */
  JsonString *pOut               /* Write JSON here */
){
  u32 sz, n, j, iEnd;

  n = jsonbPayloadSizeInline(pParse, i, &sz);
  if( n==0 ){
    pOut->eErr |= JSTRING_MALFORMED;
    return pParse->nBlob+1;
  }
  switch( pParse->aBlob[i] & 0x0f ){
    case JSONB_NULL: {
      jsonAppendRawNZ(pOut, "null", 4);
      return i+1;
    }
    case JSONB_TRUE: {
      jsonAppendRawNZ(pOut, "true", 4);
      return i+1;
    }
    case JSONB_FALSE: {
      jsonAppendRawNZ(pOut, "false", 5);
      return i+1;
    }
    case JSONB_INT:
    case JSONB_FLOAT: {
      if( sz==0 ) goto malformed_jsonb;
      jsonAppendRaw(pOut, (const char*)&pParse->aBlob[i+n], sz);
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
      const char *zIn = (const char*)&pParse->aBlob[i+n];
      if( sz==0 ) goto malformed_jsonb;
      if( zIn[0]=='-' ){
        jsonAppendChar(pOut, '-');
        if( sz<=1 ) goto malformed_jsonb;
        k = 1;
      }
      if( zIn[k]=='.' ){
        jsonAppendChar(pOut, '0');
      }
      for(; k<sz; k++){
        jsonAppendChar(pOut, zIn[k]);
        if( zIn[k]=='.' && (k+1==sz || !sqlite3Isdigit(zIn[k+1])) ){
          jsonAppendChar(pOut, '0');
        }
      }
      break;
    }
    case JSONB_TEXT:
    case JSONB_TEXTJ: {
      if( pOut->nUsed+sz+2<=pOut->nAlloc || jsonStringGrow(pOut, sz+2)==0 ){
        pOut->zBuf[pOut->nUsed] = '"';
        memcpy(pOut->zBuf+pOut->nUsed+1,(const char*)&pParse->aBlob[i+n],sz);
        pOut->zBuf[pOut->nUsed+sz+1] = '"';
        pOut->nUsed += sz+2;
      }
      break;
    }
    case JSONB_TEXT5: {
      const char *zIn;
      u32 k;
      u32 sz2 = sz;
      zIn = (const char*)&pParse->aBlob[i+n];
      jsonAppendChar(pOut, '"');
      while( sz2>0 ){
        for(k=0; k<sz2 && (jsonIsOk[(u8)zIn[k]] || zIn[k]=='\''); k++){}
        if( k>0 ){
          jsonAppendRawNZ(pOut, zIn, k);
          if( k>=sz2 ){
            break;
          }
          zIn += k;
          sz2 -= k;
        }
        if( zIn[0]=='"' ){
          jsonAppendRawNZ(pOut, "\\\"", 2);
          zIn++;
          sz2--;
          continue;
        }
        if( zIn[0]<=0x1f ){
          if( pOut->nUsed+7>pOut->nAlloc && jsonStringGrow(pOut,7) ) break;
          jsonAppendControlChar(pOut, zIn[0]);
          zIn++;
          sz2--;
          continue;
        }
        assert( zIn[0]=='\\' );
        assert( sz2>=1 );
        if( sz2<2 ){
          pOut->eErr |= JSTRING_MALFORMED;
          break;
        }
        switch( (u8)zIn[1] ){
          case '\'':
            jsonAppendChar(pOut, '\'');
            break;
          case 'v':
            jsonAppendRawNZ(pOut, "\\u000b", 6);
            break;
          case 'x':
            if( sz2<4 ){
              pOut->eErr |= JSTRING_MALFORMED;
              sz2 = 2;
              break;
            }
            jsonAppendRawNZ(pOut, "\\u00", 4);
            jsonAppendRawNZ(pOut, &zIn[2], 2);
            zIn += 2;
            sz2 -= 2;
            break;
          case '0':
            jsonAppendRawNZ(pOut, "\\u0000", 6);
            break;
          case '\r':
            if( sz2>2 && zIn[2]=='\n' ){
              zIn++;
              sz2--;
            }
            break;
          case '\n':
            break;
          case 0xe2:
            /* '\' followed by either U+2028 or U+2029 is ignored as
            ** whitespace.  Note that in UTF8, U+2028 is 0xe2 0x80 0x29.
            ** U+2029 is the same except for the last byte */
            if( sz2<4
             || 0x80!=(u8)zIn[2]
             || (0xa8!=(u8)zIn[3] && 0xa9!=(u8)zIn[3])
            ){
              pOut->eErr |= JSTRING_MALFORMED;
              sz2 = 2;
              break;
            }
            zIn += 2;
            sz2 -= 2;
            break;
          default:
            jsonAppendRawNZ(pOut, zIn, 2);
            break;
        }
        assert( sz2>=2 );
        zIn += 2;
        sz2 -= 2;
      }
      jsonAppendChar(pOut, '"');
      break;
    }
    case JSONB_TEXTRAW: {
      jsonAppendString(pOut, (const char*)&pParse->aBlob[i+n], sz);
      break;
    }
    case JSONB_ARRAY: {
      jsonAppendChar(pOut, '[');
      j = i+n;
      iEnd = j+sz;
      if( ++pParse->iDepth > JSON_MAX_DEPTH ){
        jsonStringTooDeep(pOut);
      }
      while( j<iEnd && pOut->eErr==0 ){
        j = jsonTranslateBlobToText(pParse, j, pOut);
        jsonAppendChar(pOut, ',');
      }
      pParse->iDepth--;
      if( j>iEnd ) pOut->eErr |= JSTRING_MALFORMED;
      if( sz>0 ) jsonStringTrimOneChar(pOut);
      jsonAppendChar(pOut, ']');
      break;
    }
    case JSONB_OBJECT: {
      int x = 0;
      jsonAppendChar(pOut, '{');
      j = i+n;
      iEnd = j+sz;
      if( ++pParse->iDepth > JSON_MAX_DEPTH ){
        jsonStringTooDeep(pOut);
      }
      while( j<iEnd && pOut->eErr==0 ){
        j = jsonTranslateBlobToText(pParse, j, pOut);
        jsonAppendChar(pOut, (x++ & 1) ? ',' : ':');
      }
      pParse->iDepth--;
      if( (x & 1)!=0 || j>iEnd ) pOut->eErr |= JSTRING_MALFORMED;
      if( sz>0 ) jsonStringTrimOneChar(pOut);
      jsonAppendChar(pOut, '}');
      break;
    }

    default: {
      malformed_jsonb:
      pOut->eErr |= JSTRING_MALFORMED;
      break;
    }
  }
  return i+n+sz;
}
/* …（同文件无关代码省略）… */
typedef struct JsonPretty JsonPretty;
/* …（同文件无关代码省略）… */
static void jsonPrettyIndent(JsonPretty *pPretty){
  u32 jj;
  for(jj=0; jj<pPretty->nIndent; jj++){
    jsonAppendRaw(pPretty->pOut, pPretty->zIndent, pPretty->szIndent);
  }
}
/* …（同文件无关代码省略）… */
static u32 jsonTranslateBlobToPrettyText(
  JsonPretty *pPretty,       /* Pretty-printing context */
  u32 i                      /* Start rendering at this index */
){
  u32 sz, n, j, iEnd;
  JsonParse *pParse = pPretty->pParse;
  JsonString *pOut = pPretty->pOut;
  n = jsonbPayloadSize(pParse, i, &sz);
  if( n==0 ){
    pOut->eErr |= JSTRING_MALFORMED;
    return pParse->nBlob+1;
  }
  switch( pParse->aBlob[i] & 0x0f ){
    case JSONB_ARRAY: {
      j = i+n;
      iEnd = j+sz;
      jsonAppendChar(pOut, '[');
      if( j<iEnd ){
        jsonAppendChar(pOut, '\n');
        pPretty->nIndent++;
        if( pPretty->nIndent >= JSON_MAX_DEPTH ){
          jsonStringTooDeep(pOut);
        }
        while( pOut->eErr==0 ){
          jsonPrettyIndent(pPretty);
          j = jsonTranslateBlobToPrettyText(pPretty, j);
          if( j>=iEnd ) break;
          jsonAppendRawNZ(pOut, ",\n", 2);
        }
        jsonAppendChar(pOut, '\n');
        pPretty->nIndent--;
        jsonPrettyIndent(pPretty);
      }
      jsonAppendChar(pOut, ']');
      i = iEnd;
      break;
    }
    case JSONB_OBJECT: {
      j = i+n;
      iEnd = j+sz;
      jsonAppendChar(pOut, '{');
      if( j<iEnd ){
        jsonAppendChar(pOut, '\n');
        pPretty->nIndent++;
        if( pPretty->nIndent >= JSON_MAX_DEPTH ){
          jsonStringTooDeep(pOut);
        }
        pParse->iDepth = pPretty->nIndent;
        while( pOut->eErr==0 ){
          jsonPrettyIndent(pPretty);
          j = jsonTranslateBlobToText(pParse, j, pOut);
          if( j>iEnd ){
            pOut->eErr |= JSTRING_MALFORMED;
            break;
          }
          jsonAppendRawNZ(pOut, ": ", 2);
          j = jsonTranslateBlobToPrettyText(pPretty, j);
          if( j>=iEnd ) break;
          jsonAppendRawNZ(pOut, ",\n", 2);
        }
        jsonAppendChar(pOut, '\n');
        pPretty->nIndent--;
        jsonPrettyIndent(pPretty);
      }
      jsonAppendChar(pOut, '}');
      i = iEnd;
      break;
    }
    default: {
      i = jsonTranslateBlobToText(pParse, i, pOut);
      break;
    }
  }
  return i;
}
