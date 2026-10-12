// AUTO-DRAFT from sqlite/sqlite PR #c5115c777078b53e8e6fda4793e6eb50d0f38d15
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
#define jsonIsspace(x) (jsonIsSpace[(unsigned char)x])
/* …（同文件无关代码省略）… */
typedef struct JsonCache JsonCache;
/* …（同文件无关代码省略）… */
typedef struct JsonParse JsonParse;
/* …（同文件无关代码省略）… */
#define JSON_CACHE_ID    (-429938)  /* Cache entry */
#define JSON_CACHE_SIZE  4          /* Max number of cache entries */
/* …（同文件无关代码省略）… */
#define JSON_INVALID_CHAR 0x99999
/* …（同文件无关代码省略）… */
# define JSON_MAX_DEPTH  1000
/* …（同文件无关代码省略）… */
*/
#define JSON_EDITABLE  0x01   /* Generate a writable JsonParse object */
#define JSON_KEEPERROR 0x02   /* Return non-NULL even if there is an error */

/**************************************************************************
** Forward references
/* …（同文件无关代码省略）… */
static void jsonCacheDelete(JsonCache *p){
  int i;
  for(i=0; i<p->nUsed; i++){
    jsonParseFree(p->a[i]);
  }
  sqlite3DbFree(p->db, p);
}
static void jsonCacheDeleteGeneric(void *p){
  jsonCacheDelete((JsonCache*)p);
}
/* …（同文件无关代码省略）… */
static int jsonCacheInsert(
  sqlite3_context *ctx,   /* The SQL statement context holding the cache */
  JsonParse *pParse       /* The parse object to be added to the cache */
){
  JsonCache *p;

  assert( pParse->zJson!=0 );
  assert( pParse->bJsonIsRCStr );
  assert( pParse->delta==0 );
  p = sqlite3_get_auxdata(ctx, JSON_CACHE_ID);
  if( p==0 ){
    sqlite3 *db = sqlite3_context_db_handle(ctx);
    p = sqlite3DbMallocZero(db, sizeof(*p));
    if( p==0 ) return SQLITE_NOMEM;
    p->db = db;
    sqlite3_set_auxdata(ctx, JSON_CACHE_ID, p, jsonCacheDeleteGeneric);
    p = sqlite3_get_auxdata(ctx, JSON_CACHE_ID);
    if( p==0 ) return SQLITE_NOMEM;
  }
  if( p->nUsed >= JSON_CACHE_SIZE ){
    jsonParseFree(p->a[0]);
    memmove(p->a, &p->a[1], (JSON_CACHE_SIZE-1)*sizeof(p->a[0]));
    p->nUsed = JSON_CACHE_SIZE-1;
  }
  assert( pParse->nBlobAlloc>0 );
  pParse->eEdit = 0;
  pParse->nJPRef++;
  pParse->bReadOnly = 1;
  p->a[p->nUsed] = pParse;
  p->nUsed++;
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static JsonParse *jsonCacheSearch(
  sqlite3_context *ctx,    /* The SQL statement context holding the cache */
  sqlite3_value *pArg      /* Function argument containing SQL text */
){
  JsonCache *p;
  int i;
  const char *zJson;
  int nJson;

  if( sqlite3_value_type(pArg)!=SQLITE_TEXT ){
    return 0;
  }
  zJson = (const char*)sqlite3_value_text(pArg);
  if( zJson==0 ) return 0;
  nJson = sqlite3_value_bytes(pArg);

  p = sqlite3_get_auxdata(ctx, JSON_CACHE_ID);
  if( p==0 ){
    return 0;
  }
  for(i=0; i<p->nUsed; i++){
    if( p->a[i]->zJson==zJson ) break;
  }
  if( i>=p->nUsed ){
    for(i=0; i<p->nUsed; i++){
      if( p->a[i]->nJson!=nJson ) continue;
      if( memcmp(p->a[i]->zJson, zJson, nJson)==0 ) break;
    }
  }
  if( i<p->nUsed ){
    if( i<p->nUsed-1 ){
      /* Make the matching entry the most recently used entry */
      JsonParse *tmp = p->a[i];
      memmove(&p->a[i], &p->a[i+1], (p->nUsed-i-1)*sizeof(tmp));
      p->a[p->nUsed-1] = tmp;
      i = p->nUsed - 1;
    }
    assert( p->a[i]->delta==0 );
    return p->a[i];
  }else{
    return 0;
  }
}
/* …（同文件无关代码省略）… */
static void jsonParseReset(JsonParse *pParse){
  assert( pParse->nJPRef<=1 );
  if( pParse->bJsonIsRCStr ){
    sqlite3RCStrUnref(pParse->zJson);
    pParse->zJson = 0;
    pParse->nJson = 0;
    pParse->bJsonIsRCStr = 0;
  }
  if( pParse->nBlobAlloc ){
    sqlite3DbFree(pParse->db, pParse->aBlob);
    pParse->aBlob = 0;
    pParse->nBlob = 0;
    pParse->nBlobAlloc = 0;
  }
}
/* …（同文件无关代码省略）… */
static void jsonParseFree(JsonParse *pParse){
  if( pParse ){
    if( pParse->nJPRef>1 ){
      pParse->nJPRef--;
    }else{
      jsonParseReset(pParse);
      sqlite3DbFree(pParse->db, pParse);
    }
  }
}
/* …（同文件无关代码省略）… */
static u8 jsonHexToInt(int h){
#ifdef SQLITE_ASCII
  h += 9*(1&(h>>6));
#endif
#ifdef SQLITE_EBCDIC
  h += 9*(1&~(h>>4));
#endif
  return (u8)(h & 0xf);
}
/* …（同文件无关代码省略）… */
static u32 jsonHexToInt4(const char *z){
  u32 v;
  v = (jsonHexToInt(z[0])<<12)
    + (jsonHexToInt(z[1])<<8)
    + (jsonHexToInt(z[2])<<4)
    + jsonHexToInt(z[3]);
  return v;
}
/* …（同文件无关代码省略）… */
static int jsonIs2Hex(const char *z){
  return sqlite3Isxdigit(z[0]) && sqlite3Isxdigit(z[1]);
}
/* …（同文件无关代码省略）… */
static int jsonIs4Hex(const char *z){
  return jsonIs2Hex(z) && jsonIs2Hex(&z[2]);
}
/* …（同文件无关代码省略）… */
static int json5Whitespace(const char *zIn){
  int n = 0;
  const u8 *z = (u8*)zIn;
  while( 1 /*exit by "goto whitespace_done"*/ ){
    switch( z[n] ){
      case 0x09:
      case 0x0a:
      case 0x0b:
      case 0x0c:
      case 0x0d:
      case 0x20: {
        n++;
        break;
      }
      case '/': {
        if( z[n+1]=='*' && z[n+2]!=0 ){
          int j;
          for(j=n+3; z[j]!='/' || z[j-1]!='*'; j++){
            if( z[j]==0 ) goto whitespace_done;
          }
          n = j+1;
          break;
        }else if( z[n+1]=='/' ){
          int j;
          char c;
          for(j=n+2; (c = z[j])!=0; j++){
            if( c=='\n' || c=='\r' ) break;
            if( 0xe2==(u8)c && 0x80==(u8)z[j+1]
             && (0xa8==(u8)z[j+2] || 0xa9==(u8)z[j+2])
            ){
              j += 2;
              break;
            }
          }
          n = j;
          if( z[n] ) n++;
          break;
        }
        goto whitespace_done;
      }
      case 0xc2: {
        if( z[n+1]==0xa0 ){
          n += 2;
          break;
        }
        goto whitespace_done;
      }
      case 0xe1: {
        if( z[n+1]==0x9a && z[n+2]==0x80 ){
          n += 3;
          break;
        }
        goto whitespace_done;
      }
      case 0xe2: {
        if( z[n+1]==0x80 ){
          u8 c = z[n+2];
          if( c<0x80 ) goto whitespace_done;
          if( c<=0x8a || c==0xa8 || c==0xa9 || c==0xaf ){
            n += 3;
            break;
          }
        }else if( z[n+1]==0x81 && z[n+2]==0x9f ){
          n += 3;
          break;
        }
        goto whitespace_done;
      }
      case 0xe3: {
        if( z[n+1]==0x80 && z[n+2]==0x80 ){
          n += 3;
          break;
        }
        goto whitespace_done;
      }
      case 0xef: {
        if( z[n+1]==0xbb && z[n+2]==0xbf ){
          n += 3;
          break;
        }
        goto whitespace_done;
      }
      default: {
        goto whitespace_done;
      }
    }
  }
  whitespace_done:
  return n;
}
/* …（同文件无关代码省略）… */
static int jsonBlobExpand(JsonParse *pParse, u64 N){
  u8 *aNew;
  u64 t;
  assert( N>pParse->nBlobAlloc );
  if( pParse->nBlobAlloc==0 ){
    t = 100;
  }else{
    t = pParse->nBlobAlloc*2;
  }
  if( t<N ) t = N+100;
  aNew = sqlite3DbRealloc(pParse->db, pParse->aBlob, t);
  if( aNew==0 ){ pParse->oom = 1; return 1; }
  assert( t<0x7fffffff );
  pParse->aBlob = aNew;
  pParse->nBlobAlloc = (u32)t;
  return 0;
}
/* …（同文件无关代码省略）… */
static int jsonBlobMakeEditable(JsonParse *pParse, u32 nExtra){
  u8 *aOld;
  u64 nSize;
  assert( !pParse->bReadOnly );
  if( pParse->oom ) return 0;
  if( pParse->nBlobAlloc>0 ) return 1;
  aOld = pParse->aBlob;
  nSize = pParse->nBlob + nExtra;
  pParse->aBlob = 0;
  if( jsonBlobExpand(pParse, nSize) ){
    return 0;
  }
  assert( pParse->nBlobAlloc >= pParse->nBlob + nExtra );
  memcpy(pParse->aBlob, aOld, pParse->nBlob);
  return 1;
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE void jsonBlobExpandAndAppendOneByte(
  JsonParse *pParse,
  u8 c
){
  jsonBlobExpand(pParse, (u64)pParse->nBlob+1);
  if( pParse->oom==0 ){
    assert( pParse->nBlob+1<=pParse->nBlobAlloc );
    pParse->aBlob[pParse->nBlob++] = c;
  }
}
/* …（同文件无关代码省略）… */
static void jsonBlobAppendOneByte(JsonParse *pParse, u8 c){
  if( pParse->nBlob >= pParse->nBlobAlloc ){
    jsonBlobExpandAndAppendOneByte(pParse, c);
  }else{
    pParse->aBlob[pParse->nBlob++] = c;
  }
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE void jsonBlobExpandAndAppendNode(
  JsonParse *pParse,
  u8 eType,
  u64 szPayload,
  const void *aPayload
){
  if( jsonBlobExpand(pParse, (u64)pParse->nBlob+(u64)szPayload+9) ) return;
  jsonBlobAppendNode(pParse, eType, szPayload, aPayload);
}
/* …（同文件无关代码省略）… */
static void jsonBlobAppendNode(
  JsonParse *pParse,          /* The JsonParse object under construction */
  u8 eType,                   /* Node type.  One of JSONB_* */
  u64 szPayload,              /* Number of bytes of payload */
  const void *aPayload        /* The payload.  Might be NULL */
){
  u8 *a;
  if( pParse->nBlob+szPayload+9 > pParse->nBlobAlloc ){
    jsonBlobExpandAndAppendNode(pParse,eType,szPayload,aPayload);
    return;
  }
  assert( pParse->aBlob!=0 );
  a = &pParse->aBlob[pParse->nBlob];
  if( szPayload<=11 ){
    a[0] = eType | (szPayload<<4);
    pParse->nBlob += 1;
  }else if( szPayload<=0xff ){
    a[0] = eType | 0xc0;
    a[1] = szPayload & 0xff;
    pParse->nBlob += 2;
  }else if( szPayload<=0xffff ){
    a[0] = eType | 0xd0;
    a[1] = (szPayload >> 8) & 0xff;
    a[2] = szPayload & 0xff;
    pParse->nBlob += 3;
  }else{
    a[0] = eType | 0xe0;
    a[1] = (szPayload >> 24) & 0xff;
    a[2] = (szPayload >> 16) & 0xff;
    a[3] = (szPayload >> 8) & 0xff;
    a[4] = szPayload & 0xff;
    pParse->nBlob += 5;
  }
  if( aPayload ){
    pParse->nBlob += szPayload;
    memcpy(&pParse->aBlob[pParse->nBlob-szPayload], aPayload, szPayload);
  }
}
/* …（同文件无关代码省略）… */
static int jsonBlobChangePayloadSize(
  JsonParse *pParse,
  u32 i,
  u32 szPayload
){
  u8 *a;
  u8 szType;
  u8 nExtra;
  u8 nNeeded;
  int delta;
  if( pParse->oom ) return 0;
  a = &pParse->aBlob[i];
  szType = a[0]>>4;
  if( szType<=11 ){
    nExtra = 0;
  }else if( szType==12 ){
    nExtra = 1;
  }else if( szType==13 ){
    nExtra = 2;
  }else if( szType==14 ){
    nExtra = 4;
  }else{
    nExtra = 8;
  }
  if( szPayload<=11 ){
    nNeeded = 0;
  }else if( szPayload<=0xff ){
    nNeeded = 1;
  }else if( szPayload<=0xffff ){
    nNeeded = 2;
  }else{
    nNeeded = 4;
  }
  delta = nNeeded - nExtra;
  if( delta ){
    u64 newSize = (u64)pParse->nBlob + delta;
    if( delta>0 ){
      if( newSize>pParse->nBlobAlloc && jsonBlobExpand(pParse, newSize) ){
        return 0;  /* OOM error.  Error state recorded in pParse->oom. */
      }
      a = &pParse->aBlob[i];
      memmove(&a[1+delta], &a[1], pParse->nBlob - (i+1));
    }else{
      memmove(&a[1], &a[1-delta], pParse->nBlob - (i+1-delta));
    }
    pParse->nBlob = newSize;
  }
  if( nNeeded==0 ){
    a[0] = (a[0] & 0x0f) | (szPayload<<4);
  }else if( nNeeded==1 ){
    a[0] = (a[0] & 0x0f) | 0xc0;
    a[1] = szPayload & 0xff;
  }else if( nNeeded==2 ){
    a[0] = (a[0] & 0x0f) | 0xd0;
    a[1] = (szPayload >> 8) & 0xff;
    a[2] = szPayload & 0xff;
  }else{
    a[0] = (a[0] & 0x0f) | 0xe0;
    a[1] = (szPayload >> 24) & 0xff;
    a[2] = (szPayload >> 16) & 0xff;
    a[3] = (szPayload >> 8) & 0xff;
    a[4] = szPayload & 0xff;
  }
  return delta;
}
/* …（同文件无关代码省略）… */
static int jsonIs4HexB(const char *z, int *pOp){
  if( z[0]!='u' ) return 0;
  if( !jsonIs4Hex(&z[1]) ) return 0;
  *pOp = JSONB_TEXTJ;
  return 1;
}
/* …（同文件无关代码省略）… */
static u32 jsonbValidityCheck(
  const JsonParse *pParse,    /* Input JSONB.  Only aBlob and nBlob are used */
  u32 i,                      /* Start of element as pParse->aBlob[i] */
  u32 iEnd,                   /* One more than the last byte of the element */
  u32 iDepth                  /* Current nesting depth */
){
  u32 n, sz, j, k;
  const u8 *z;
  u8 x;
  if( iDepth>JSON_MAX_DEPTH ) return i+1;
  sz = 0;
  n = jsonbPayloadSize(pParse, i, &sz);
  if( NEVER(n==0) ) return i+1;          /* Checked by caller */
  if( NEVER(i+n+sz!=iEnd) ) return i+1;  /* Checked by caller */
  z = pParse->aBlob;
  x = z[i] & 0x0f;
  switch( x ){
    case JSONB_NULL:
    case JSONB_TRUE:
    case JSONB_FALSE: {
      return n+sz==1 ? 0 : i+1;
    }
    case JSONB_INT: {
      if( sz<1 ) return i+1;
      j = i+n;
      if( z[j]=='-' ){
        j++;
        if( sz<2 ) return i+1;
      }
      k = i+n+sz;
      while( j<k ){
        if( sqlite3Isdigit(z[j]) ){
          j++;
        }else{
          return j+1;
        }
      }
      return 0;
    }
    case JSONB_INT5: {
      if( sz<3 ) return i+1;
      j = i+n;
      if( z[j]=='-' ){
        if( sz<4 ) return i+1;
        j++;
      }
      if( z[j]!='0' ) return i+1;
      if( z[j+1]!='x' && z[j+1]!='X' ) return j+2;
      j += 2;
      k = i+n+sz;
      while( j<k ){
        if( sqlite3Isxdigit(z[j]) ){
          j++;
        }else{
          return j+1;
        }
      }
      return 0;
    }
    case JSONB_FLOAT:
    case JSONB_FLOAT5: {
      u8 seen = 0;   /* 0: initial.  1: '.' seen  2: 'e' seen */
      if( sz<2 ) return i+1;
      j = i+n;
      k = j+sz;
      if( z[j]=='-' ){
        j++;
        if( sz<3 ) return i+1;
      }
      if( z[j]=='.' ){
        if( x==JSONB_FLOAT ) return j+1;
        if( !sqlite3Isdigit(z[j+1]) ) return j+1;
        j += 2;
        seen = 1;
      }else if( z[j]=='0' && x==JSONB_FLOAT ){
        if( j+3>k ) return j+1;
        if( z[j+1]!='.' && z[j+1]!='e' && z[j+1]!='E' ) return j+1;
        j++;
      }
      for(; j<k; j++){
        if( sqlite3Isdigit(z[j]) ) continue;
        if( z[j]=='.' ){
          if( seen>0 ) return j+1;
          if( x==JSONB_FLOAT && (j==k-1 || !sqlite3Isdigit(z[j+1])) ){
            return j+1;
          }
          seen = 1;
          continue;
        }
        if( z[j]=='e' || z[j]=='E' ){
          if( seen==2 ) return j+1;
          if( j==k-1 ) return j+1;
          if( z[j+1]=='+' || z[j+1]=='-' ){
            j++;
            if( j==k-1 ) return j+1;
          }
          seen = 2;
          continue;
        }
        return j+1;
      }
      if( seen==0 ) return i+1;
      return 0;
    }
    case JSONB_TEXT: {
      j = i+n;
      k = j+sz;
      while( j<k ){
        if( !jsonIsOk[z[j]] && z[j]!='\'' ) return j+1;
        j++;
      }
      return 0;
    }
    case JSONB_TEXTJ:
    case JSONB_TEXT5: {
      j = i+n;
      k = j+sz;
      while( j<k ){
        if( !jsonIsOk[z[j]] && z[j]!='\'' ){
          if( z[j]=='"' ){
            if( x==JSONB_TEXTJ ) return j+1;
          }else if( z[j]<=0x1f ){
            /* Control characters in JSON5 string literals are ok */
            if( x==JSONB_TEXTJ ) return j+1;
          }else if( NEVER(z[j]!='\\') || j+1>=k ){
            return j+1;
          }else if( strchr("\"\\/bfnrt",z[j+1])!=0 ){
            j++;
          }else if( z[j+1]=='u' ){
            if( j+5>=k ) return j+1;
            if( !jsonIs4Hex((const char*)&z[j+2]) ) return j+1;
            j++;
          }else if( x!=JSONB_TEXT5 ){
            return j+1;
          }else{
            u32 c = 0;
            u32 szC = jsonUnescapeOneChar((const char*)&z[j], k-j, &c);
            if( c==JSON_INVALID_CHAR ) return j+1;
            j += szC - 1;
          }
        }
        j++;
      }
      return 0;
    }
    case JSONB_TEXTRAW: {
      return 0;
    }
    case JSONB_ARRAY: {
      u32 sub;
      j = i+n;
      k = j+sz;
      while( j<k ){
        sz = 0;
        n = jsonbPayloadSize(pParse, j, &sz);
        if( n==0 ) return j+1;
        if( j+n+sz>k ) return j+1;
        sub = jsonbValidityCheck(pParse, j, j+n+sz, iDepth+1);
        if( sub ) return sub;
        j += n + sz;
      }
      assert( j==k );
      return 0;
    }
    case JSONB_OBJECT: {
      u32 cnt = 0;
      u32 sub;
      j = i+n;
      k = j+sz;
      while( j<k ){
        sz = 0;
        n = jsonbPayloadSize(pParse, j, &sz);
        if( n==0 ) return j+1;
        if( j+n+sz>k ) return j+1;
        if( (cnt & 1)==0 ){
          x = z[j] & 0x0f;
          if( x<JSONB_TEXT || x>JSONB_TEXTRAW ) return j+1;
        }
        sub = jsonbValidityCheck(pParse, j, j+n+sz, iDepth+1);
        if( sub ) return sub;
        cnt++;
        j += n + sz;
      }
      assert( j==k );
      if( (cnt & 1)!=0 ) return j+1;
      return 0;
    }
    default: {
      return i+1;
    }
  }
}
/* …（同文件无关代码省略）… */
static int jsonTranslateTextToBlob(JsonParse *pParse, u32 i){
  char c;
  u32 j;
  u32 iThis, iStart;
  int x;
  u8 t;
  const char *z = pParse->zJson;
json_parse_restart:
  switch( (u8)z[i] ){
  case '{': {
    /* Parse object */
    iThis = pParse->nBlob;
    jsonBlobAppendNode(pParse, JSONB_OBJECT, pParse->nJson-i, 0);
    if( ++pParse->iDepth > JSON_MAX_DEPTH ){
      pParse->iErr = i;
      return -1;
    }
    iStart = pParse->nBlob;
    for(j=i+1;;j++){
      u32 iBlob = pParse->nBlob;
      x = jsonTranslateTextToBlob(pParse, j);
      if( x<=0 ){
        int op;
        if( x==(-2) ){
          j = pParse->iErr;
          if( pParse->nBlob!=(u32)iStart ) pParse->hasNonstd = 1;
          break;
        }
        j += json5Whitespace(&z[j]);
        op = JSONB_TEXT;
        if( sqlite3JsonId1(z[j]) 
         || (z[j]=='\\' && jsonIs4HexB(&z[j+1], &op))
        ){
          int k = j+1;
          while( (sqlite3JsonId2(z[k]) && json5Whitespace(&z[k])==0)
            || (z[k]=='\\' && jsonIs4HexB(&z[k+1], &op))
          ){
            k++;
          }
          assert( iBlob==pParse->nBlob );
          jsonBlobAppendNode(pParse, op, k-j, &z[j]);
          pParse->hasNonstd = 1;
          x = k;
        }else{
          if( x!=-1 ) pParse->iErr = j;
          return -1;
        }
      }
      if( pParse->oom ) return -1;
      t = pParse->aBlob[iBlob] & 0x0f;
      if( t<JSONB_TEXT || t>JSONB_TEXTRAW ){
        pParse->iErr = j;
        return -1;
      }
      j = x;
      if( z[j]==':' ){
        j++;
      }else{
        if( jsonIsspace(z[j]) ){
          /* strspn() is not helpful here */
          do{ j++; }while( jsonIsspace(z[j]) );
          if( z[j]==':' ){
            j++;
            goto parse_object_value;
          }
        }
        x = jsonTranslateTextToBlob(pParse, j);
        if( x!=(-5) ){
          if( x!=(-1) ) pParse->iErr = j;
          return -1;
        }
        j = pParse->iErr+1;
      }
    parse_object_value:
      x = jsonTranslateTextToBlob(pParse, j);
      if( x<=0 ){
        if( x!=(-1) ) pParse->iErr = j;
        return -1;
      }
      j = x;
      if( z[j]==',' ){
        continue;
      }else if( z[j]=='}' ){
        break;
      }else{
        if( jsonIsspace(z[j]) ){
          j += 1 + (u32)strspn(&z[j+1], jsonSpaces);
          if( z[j]==',' ){
            continue;
          }else if( z[j]=='}' ){
            break;
          }
        }
        x = jsonTranslateTextToBlob(pParse, j);
        if( x==(-4) ){
          j = pParse->iErr;
          continue;
        }
        if( x==(-2) ){
          j = pParse->iErr;
          break;
        }
      }
      pParse->iErr = j;
      return -1;
    }
    jsonBlobChangePayloadSize(pParse, iThis, pParse->nBlob - iStart);
    pParse->iDepth--;
    return j+1;
  }
  case '[': {
    /* Parse array */
    iThis = pParse->nBlob;
    assert( i<=(u32)pParse->nJson );
    jsonBlobAppendNode(pParse, JSONB_ARRAY, pParse->nJson - i, 0);
    iStart = pParse->nBlob;
    if( pParse->oom ) return -1;
    if( ++pParse->iDepth > JSON_MAX_DEPTH ){
      pParse->iErr = i;
      return -1;
    }
    for(j=i+1;;j++){
      x = jsonTranslateTextToBlob(pParse, j);
      if( x<=0 ){
        if( x==(-3) ){
          j = pParse->iErr;
          if( pParse->nBlob!=iStart ) pParse->hasNonstd = 1;
          break;
        }
        if( x!=(-1) ) pParse->iErr = j;
        return -1;
      }
      j = x;
      if( z[j]==',' ){
        continue;
      }else if( z[j]==']' ){
        break;
      }else{
        if( jsonIsspace(z[j]) ){
          j += 1 + (u32)strspn(&z[j+1], jsonSpaces);
          if( z[j]==',' ){
            continue;
          }else if( z[j]==']' ){
            break;
          }
        }
        x = jsonTranslateTextToBlob(pParse, j);
        if( x==(-4) ){
          j = pParse->iErr;
          continue;
        }
        if( x==(-3) ){
          j = pParse->iErr;
          break;
        }
      }
      pParse->iErr = j;
      return -1;
    }
    jsonBlobChangePayloadSize(pParse, iThis, pParse->nBlob - iStart);
    pParse->iDepth--;
    return j+1;
  }
  case '\'': {
    u8 opcode;
    char cDelim;
    pParse->hasNonstd = 1;
    opcode = JSONB_TEXT;
    goto parse_string;
  case '"':
    /* Parse string */
    opcode = JSONB_TEXT;
  parse_string:
    cDelim = z[i];
    j = i+1;
    while( 1 /*exit-by-break*/ ){
      if( jsonIsOk[(u8)z[j]] ){
        if( !jsonIsOk[(u8)z[j+1]] ){
          j += 1;
        }else if( !jsonIsOk[(u8)z[j+2]] ){
          j += 2;
        }else{
          j += 3;
          continue;
        }
      }
      c = z[j];
      if( c==cDelim ){
        break;
      }else if( c=='\\' ){
        c = z[++j];
        if( c=='"' || c=='\\' || c=='/' || c=='b' || c=='f'
           || c=='n' || c=='r' || c=='t'
           || (c=='u' && jsonIs4Hex(&z[j+1])) ){
          if( opcode==JSONB_TEXT ) opcode = JSONB_TEXTJ;
        }else if( c=='\'' ||  c=='v' || c=='\n'
#ifdef SQLITE_BUG_COMPATIBLE_20250510
           || (c=='0')                            /* Legacy bug compatible */
#else
           || (c=='0' && !sqlite3Isdigit(z[j+1])) /* Correct implementation */
#endif
           || (0xe2==(u8)c && 0x80==(u8)z[j+1]
                && (0xa8==(u8)z[j+2] || 0xa9==(u8)z[j+2]))
           || (c=='x' && jsonIs2Hex(&z[j+1])) ){
          opcode = JSONB_TEXT5;
          pParse->hasNonstd = 1;
        }else if( c=='\r' ){
          if( z[j+1]=='\n' ) j++;
          opcode = JSONB_TEXT5;
          pParse->hasNonstd = 1;
        }else{
          pParse->iErr = j;
          return -1;
        }
      }else if( c<=0x1f ){
        if( c==0 ){
          pParse->iErr = j;
          return -1;
        }
        /* Control characters are not allowed in canonical JSON string
        ** literals, but are allowed in JSON5 string literals. */
        opcode = JSONB_TEXT5;
        pParse->hasNonstd = 1;
      }else if( c=='"' ){
        opcode = JSONB_TEXT5;
      }
      j++;
    }
    jsonBlobAppendNode(pParse, opcode, j-1-i, &z[i+1]);
    return j+1;
  }
  case 't': {
    if( strncmp(z+i,"true",4)==0 && !sqlite3Isalnum(z[i+4]) ){
      jsonBlobAppendOneByte(pParse, JSONB_TRUE);
      return i+4;
    }
    pParse->iErr = i;
    return -1;
  }
  case 'f': {
    if( strncmp(z+i,"false",5)==0 && !sqlite3Isalnum(z[i+5]) ){
      jsonBlobAppendOneByte(pParse, JSONB_FALSE);
      return i+5;
    }
    pParse->iErr = i;
    return -1;
  }
  case '+': {
    u8 seenE;
    pParse->hasNonstd = 1;
    t = 0x00;            /* Bit 0x01:  JSON5.   Bit 0x02:  FLOAT */
    goto parse_number;
  case '.':
    if( sqlite3Isdigit(z[i+1]) ){
      pParse->hasNonstd = 1;
      t = 0x03;          /* Bit 0x01:  JSON5.   Bit 0x02:  FLOAT */
      seenE = 0;
      goto parse_number_2;
    }
    pParse->iErr = i;
    return -1;
  case '-':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    /* Parse number */
    t = 0x00;            /* Bit 0x01:  JSON5.   Bit 0x02:  FLOAT */
  parse_number:
    seenE = 0;
    assert( '-' < '0' );
    assert( '+' < '0' );
    assert( '.' < '0' );
    c = z[i];

    if( c<='0' ){
      if( c=='0' ){
        if( (z[i+1]=='x' || z[i+1]=='X') && sqlite3Isxdigit(z[i+2]) ){
          assert( t==0x00 );
          pParse->hasNonstd = 1;
          t = 0x01;
          for(j=i+3; sqlite3Isxdigit(z[j]); j++){}
          goto parse_number_finish;
        }else if( sqlite3Isdigit(z[i+1]) ){
          pParse->iErr = i+1;
          return -1;
        }
      }else{
        if( !sqlite3Isdigit(z[i+1]) ){
          /* JSON5 allows for "+Infinity" and "-Infinity" using exactly
          ** that case.  SQLite also allows these in any case and it allows
          ** "+inf" and "-inf". */
          if( (z[i+1]=='I' || z[i+1]=='i')
           && sqlite3StrNICmp(&z[i+1], "inf",3)==0
          ){
            pParse->hasNonstd = 1;
            if( z[i]=='-' ){
              jsonBlobAppendNode(pParse, JSONB_FLOAT, 6, "-9e999");
            }else{
              jsonBlobAppendNode(pParse, JSONB_FLOAT, 5, "9e999");
            }
            return i + (sqlite3StrNICmp(&z[i+4],"inity",5)==0 ? 9 : 4);
          }
          if( z[i+1]=='.' ){
            pParse->hasNonstd = 1;
            t |= 0x01;
            goto parse_number_2;
          }
          pParse->iErr = i;
          return -1;
        }
        if( z[i+1]=='0' ){
          if( sqlite3Isdigit(z[i+2]) ){
            pParse->iErr = i+1;
            return -1;
          }else if( (z[i+2]=='x' || z[i+2]=='X') && sqlite3Isxdigit(z[i+3]) ){
            pParse->hasNonstd = 1;
            t |= 0x01;
            for(j=i+4; sqlite3Isxdigit(z[j]); j++){}
            goto parse_number_finish;
          }
        }
      }
    }

  parse_number_2:
    for(j=i+1;; j++){
      c = z[j];
      if( sqlite3Isdigit(c) ) continue;
      if( c=='.' ){
        if( (t & 0x02)!=0 ){
          pParse->iErr = j;
          return -1;
        }
        t |= 0x02;
        continue;
      }
      if( c=='e' || c=='E' ){
        if( z[j-1]<'0' ){
          if( ALWAYS(z[j-1]=='.') && ALWAYS(j-2>=i) && sqlite3Isdigit(z[j-2]) ){
            pParse->hasNonstd = 1;
            t |= 0x01;
          }else{
            pParse->iErr = j;
            return -1;
          }
        }
        if( seenE ){
          pParse->iErr = j;
          return -1;
        }
        t |= 0x02;
        seenE = 1;
        c = z[j+1];
        if( c=='+' || c=='-' ){
          j++;
          c = z[j+1];
        }
        if( c<'0' || c>'9' ){
          pParse->iErr = j;
          return -1;
        }
        continue;
      }
      break;
    }
    if( z[j-1]<'0' ){
      if( ALWAYS(z[j-1]=='.') && ALWAYS(j-2>=i) && sqlite3Isdigit(z[j-2]) ){
        pParse->hasNonstd = 1;
        t |= 0x01;
      }else{
        pParse->iErr = j;
        return -1;
      }
    }
  parse_number_finish:
    assert( JSONB_INT+0x01==JSONB_INT5 );
    assert( JSONB_FLOAT+0x01==JSONB_FLOAT5 );
    assert( JSONB_INT+0x02==JSONB_FLOAT );
    if( z[i]=='+' ) i++;
    jsonBlobAppendNode(pParse, JSONB_INT+t, j-i, &z[i]);
    return j;
  }
  case '}': {
    pParse->iErr = i;
    return -2;  /* End of {...} */
  }
  case ']': {
    pParse->iErr = i;
    return -3;  /* End of [...] */
  }
  case ',': {
    pParse->iErr = i;
    return -4;  /* List separator */
  }
  case ':': {
    pParse->iErr = i;
    return -5;  /* Object label/value separator */
  }
  case 0: {
    return 0;   /* End of file */
  }
  case 0x09:
  case 0x0a:
  case 0x0d:
  case 0x20: {
    i += 1 + (u32)strspn(&z[i+1], jsonSpaces);
    goto json_parse_restart;
  }
  case 0x0b:
  case 0x0c:
  case '/':
  case 0xc2:
  case 0xe1:
  case 0xe2:
  case 0xe3:
  case 0xef: {
    j = json5Whitespace(&z[i]);
    if( j>0 ){
      i += j;
      pParse->hasNonstd = 1;
      goto json_parse_restart;
    }
    pParse->iErr = i;
    return -1;
  }
  case 'n': {
    if( strncmp(z+i,"null",4)==0 && !sqlite3Isalnum(z[i+4]) ){
      jsonBlobAppendOneByte(pParse, JSONB_NULL);
      return i+4;
    }
    /* fall-through into the default case that checks for NaN */
    /* no break */ deliberate_fall_through
  }
  default: {
    u32 k;
    int nn;
    c = z[i];
    for(k=0; k<sizeof(aNanInfName)/sizeof(aNanInfName[0]); k++){
      if( c!=aNanInfName[k].c1 && c!=aNanInfName[k].c2 ) continue;
      nn = aNanInfName[k].n;
      if( sqlite3StrNICmp(&z[i], aNanInfName[k].zMatch, nn)!=0 ){
        continue;
      }
      if( sqlite3Isalnum(z[i+nn]) ) continue;
      if( aNanInfName[k].eType==JSONB_FLOAT ){
        jsonBlobAppendNode(pParse, JSONB_FLOAT, 5, "9e999");
      }else{
        jsonBlobAppendOneByte(pParse, JSONB_NULL);
      }
      pParse->hasNonstd = 1;
      return i + nn;
    }
    pParse->iErr = i;
    return -1;  /* Syntax error */
  }
  } /* End switch(z[i]) */
}
/* …（同文件无关代码省略）… */
static int jsonConvertTextToBlob(
  JsonParse *pParse,           /* Initialize and fill this JsonParse object */
  sqlite3_context *pCtx        /* Report errors here */
){
  int i;
  const char *zJson = pParse->zJson;
  i = jsonTranslateTextToBlob(pParse, 0);
  if( pParse->oom ) i = -1;
  if( i>0 ){
#ifdef SQLITE_DEBUG
    assert( pParse->iDepth==0 );
    if( sqlite3Config.bJsonSelfcheck ){
      assert( jsonbValidityCheck(pParse, 0, pParse->nBlob, 0)==0 );
    }   
#endif
    while( jsonIsspace(zJson[i]) ) i++;
    if( zJson[i] ){
      i += json5Whitespace(&zJson[i]);
      if( zJson[i] ){
        if( pCtx ) sqlite3_result_error(pCtx, "malformed JSON", -1);
        jsonParseReset(pParse);
        return 1;
      }
      pParse->hasNonstd = 1;
    }
  }
  if( i<=0 ){
    if( pCtx!=0 ){
      if( pParse->oom ){
        sqlite3_result_error_nomem(pCtx);
      }else{
        sqlite3_result_error(pCtx, "malformed JSON", -1);
      }
    }
    jsonParseReset(pParse);
    return 1;
  }
  return 0;
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
static u32 jsonBytesToBypass(const char *z, u32 n){
  u32 i = 0;
  while( i+1<n ){
    if( z[i]!='\\' ) return i;
    if( z[i+1]=='\n' ){
      i += 2;
      continue;
    }
    if( z[i+1]=='\r' ){
      if( i+2<n && z[i+2]=='\n' ){
        i += 3;
      }else{
        i += 2;
      }
      continue;
    }
    if( 0xe2==(u8)z[i+1]
     && i+3<n
     && 0x80==(u8)z[i+2]
     && (0xa8==(u8)z[i+3] || 0xa9==(u8)z[i+3])
    ){
      i += 4;
      continue;
    }
    break;
  }
  return i;
}
/* …（同文件无关代码省略）… */
static u32 jsonUnescapeOneChar(const char *z, u32 n, u32 *piOut){
  assert( n>0 );
  assert( z[0]=='\\' );
  if( n<2 ){
    *piOut = JSON_INVALID_CHAR;
    return n;
  }
  switch( (u8)z[1] ){
    case 'u': {
      u32 v, vlo;
      if( n<6 ){
        *piOut = JSON_INVALID_CHAR;
        return n;
      }
      v = jsonHexToInt4(&z[2]);
      if( (v & 0xfc00)==0xd800
       && n>=12
       && z[6]=='\\'
       && z[7]=='u'
       && ((vlo = jsonHexToInt4(&z[8]))&0xfc00)==0xdc00
      ){
        *piOut = ((v&0x3ff)<<10) + (vlo&0x3ff) + 0x10000;
        return 12;
      }else{
        *piOut = v;
        return 6;
      }
    }
    case 'b': {   *piOut = '\b';  return 2; }
    case 'f': {   *piOut = '\f';  return 2; }
    case 'n': {   *piOut = '\n';  return 2; }
    case 'r': {   *piOut = '\r';  return 2; }
    case 't': {   *piOut = '\t';  return 2; }
    case 'v': {   *piOut = '\v';  return 2; }
    case '0': {
      /* JSON5 requires that the \0 escape not be followed by a digit.
      ** But SQLite did not enforce this restriction in versions 3.42.0
      ** through 3.49.2.  That was a bug.  But some applications might have
      ** come to depend on that bug.  Use the SQLITE_BUG_COMPATIBLE_20250510
      ** option to restore the old buggy behavior. */
#ifdef SQLITE_BUG_COMPATIBLE_20250510
      /* Legacy bug-compatible behavior */
      *piOut = 0;
#else
      /* Correct behavior */
      *piOut = (n>2 && sqlite3Isdigit(z[2])) ? JSON_INVALID_CHAR : 0;
#endif
      return 2;
    }
    case '\'':
    case '"':
    case '/':
    case '\\':{   *piOut = z[1];  return 2; }
    case 'x': {
      if( n<4 ){
        *piOut = JSON_INVALID_CHAR;
        return n;
      }
      *piOut = (jsonHexToInt(z[2])<<4) | jsonHexToInt(z[3]);
      return 4;
    }
    case 0xe2:
    case '\r':
    case '\n': {
      u32 nSkip = jsonBytesToBypass(z, n);
      if( nSkip==0 ){
        *piOut = JSON_INVALID_CHAR;
        return n;
      }else if( nSkip==n ){
        *piOut = 0;
        return n;
      }else if( z[nSkip]=='\\' ){
        return nSkip + jsonUnescapeOneChar(&z[nSkip], n-nSkip, piOut);
      }else{
        int sz = sqlite3Utf8ReadLimited((u8*)&z[nSkip], n-nSkip, piOut);
        return nSkip + sz;
      }
    }
    default: {
      *piOut = JSON_INVALID_CHAR;
      return 2;
    }
  }
}
/* …（同文件无关代码省略）… */
static int jsonArgIsJsonb(sqlite3_value *pArg, JsonParse *p){
  u32 n, sz = 0;
  u8 c;
  if( sqlite3_value_type(pArg)!=SQLITE_BLOB ) return 0;
  p->aBlob = (u8*)sqlite3_value_blob(pArg);
  p->nBlob = (u32)sqlite3_value_bytes(pArg);
  if( p->nBlob>0
   && ALWAYS(p->aBlob!=0)
   && ((c = p->aBlob[0]) & 0x0f)<=JSONB_OBJECT
   && (n = jsonbPayloadSize(p, 0, &sz))>0
   && sz+n==p->nBlob
   && ((c & 0x0f)>JSONB_FALSE || sz==0)
   && (sz>7 
      || (c!=0x7b && c!=0x5b && !sqlite3Isdigit(c))
      || jsonbValidityCheck(p, 0, p->nBlob, 1)==0)
  ){
    return 1;
  }
  p->aBlob = 0;
  p->nBlob = 0;
  return 0;
}
/* …（同文件无关代码省略）… */
static JsonParse *jsonParseFuncArg(
  sqlite3_context *ctx,
  sqlite3_value *pArg,
  u32 flgs
){
  int eType;                   /* Datatype of pArg */
  JsonParse *p = 0;            /* Value to be returned */
  JsonParse *pFromCache = 0;   /* Value taken from cache */
  sqlite3 *db;                 /* The database connection */
  
  assert( ctx!=0 );
  eType = sqlite3_value_type(pArg);
  if( eType==SQLITE_NULL ){
    return 0;
  }
  pFromCache = jsonCacheSearch(ctx, pArg);
  if( pFromCache ){
    pFromCache->nJPRef++;
    if( (flgs & JSON_EDITABLE)==0 ){
      return pFromCache;
    }
  }
  db = sqlite3_context_db_handle(ctx);
rebuild_from_cache:
  p = sqlite3DbMallocZero(db, sizeof(*p));
  if( p==0 ) goto json_pfa_oom;
  memset(p, 0, sizeof(*p));
  p->db = db;
  p->nJPRef = 1;
  if( pFromCache!=0 ){
    u32 nBlob = pFromCache->nBlob;
    p->aBlob = sqlite3DbMallocRaw(db, nBlob);
    if( p->aBlob==0 ) goto json_pfa_oom;
    memcpy(p->aBlob, pFromCache->aBlob, nBlob);
    p->nBlobAlloc = p->nBlob = nBlob;
    p->hasNonstd = pFromCache->hasNonstd;
    jsonParseFree(pFromCache);
    return p;
  }
  if( eType==SQLITE_BLOB ){
    if( jsonArgIsJsonb(pArg,p) ){
      if( (flgs & JSON_EDITABLE)!=0 && jsonBlobMakeEditable(p, 0)==0 ){
        goto json_pfa_oom;
      }
      return p;
    }
    /* If the blob is not valid JSONB, fall through into trying to cast
    ** the blob into text which is then interpreted as JSON.  (tag-20240123-a)
    **
    ** This goes against all historical documentation about how the SQLite
    ** JSON functions were suppose to work.  From the beginning, blob was
    ** reserved for expansion and a blob value should have raised an error.
    ** But it did not, due to a bug.  And many applications came to depend
    ** upon this buggy behavior, especially when using the CLI and reading
    ** JSON text using readfile(), which returns a blob.  For this reason
    ** we will continue to support the bug moving forward.
    ** See for example https://sqlite.org/forum/forumpost/012136abd5292b8d
    */
  }
  p->zJson = (char*)sqlite3_value_text(pArg);
  p->nJson = sqlite3_value_bytes(pArg);
  if( db->mallocFailed ) goto json_pfa_oom;
  if( p->nJson==0 ) goto json_pfa_malformed;
  assert( p->zJson!=0 );
  if( jsonConvertTextToBlob(p, (flgs & JSON_KEEPERROR) ? 0 : ctx) ){
    if( flgs & JSON_KEEPERROR ){
      p->nErr = 1;
      return p;
    }else{
      jsonParseFree(p);
      return 0;
    }
  }else{
    int isRCStr = sqlite3ValueIsOfClass(pArg, sqlite3RCStrUnref);
    int rc;
    if( !isRCStr ){
      char *zNew = sqlite3RCStrNew( p->nJson );
      if( zNew==0 ) goto json_pfa_oom;
      memcpy(zNew, p->zJson, p->nJson);
      p->zJson = zNew;
      p->zJson[p->nJson] = 0;
    }else{
      sqlite3RCStrRef(p->zJson);
    }
    p->bJsonIsRCStr = 1;
    rc = jsonCacheInsert(ctx, p);
    if( rc==SQLITE_NOMEM ) goto json_pfa_oom;
    if( flgs & JSON_EDITABLE ){
      pFromCache = p;
      p = 0;
      goto rebuild_from_cache;
    }
  }
  return p;

json_pfa_malformed:
  if( flgs & JSON_KEEPERROR ){
    p->nErr = 1;
    return p;
  }else{
    jsonParseFree(p);
    sqlite3_result_error(ctx, "malformed JSON", -1);
    return 0;
  }

json_pfa_oom:
  jsonParseFree(pFromCache);
  jsonParseFree(p);
  sqlite3_result_error_nomem(ctx);
  return 0;
}
/* …（同文件无关代码省略）… */
      if( (flags & 0x3)==0 ) break;
      memset(&px, 0, sizeof(px));
     
      p = jsonParseFuncArg(ctx, argv[0], JSON_KEEPERROR);
      if( p ){
        if( p->oom ){
          sqlite3_result_error_nomem(ctx);
