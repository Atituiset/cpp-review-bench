// AUTO-DRAFT from sqlite/sqlite PR #2023a2b37593073de0e789455b6a362ceeecd1d1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define JSONB_TEXT     7   /* Text compatible with both JSON and SQL */
/* …（同文件无关代码省略）… */
#define JSONB_TEXT5    9   /* Text with JSON-5 escape */
#define JSONB_TEXTRAW 10   /* SQL text that needs escaping for JSON */
#define JSONB_ARRAY   11   /* An array */
#define JSONB_OBJECT  12   /* An object */
/* …（同文件无关代码省略）… */
typedef struct JsonParse JsonParse;
/* …（同文件无关代码省略）… */
#define JSON_INVALID_CHAR 0x99999
/* …（同文件无关代码省略）… */
#define JEDIT_DEL   1   /* Delete if exists */
/* …（同文件无关代码省略）… */
#define JEDIT_INS   3   /* Insert if not exists */
#define JEDIT_SET   4   /* Insert or overwrite */
#define JEDIT_AINS  5   /* array_insert() */
/* …（同文件无关代码省略）… */
# define JSON_MAX_DEPTH  1000
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
/* …（同文件无关代码省略）… */
static u32 jsonbArrayCount(JsonParse *pParse, u32 iRoot){
  u32 n, sz, i, iEnd;
  u32 k = 0;
  n = jsonbPayloadSize(pParse, iRoot, &sz);
  iEnd = iRoot+n+sz;
  for(i=iRoot+n; n>0 && i<iEnd; i+=sz+n, k++){
    n = jsonbPayloadSizeSemiInline(pParse, i, &sz);
  }
  return k;
}
/* …（同文件无关代码省略）… */
static void jsonAfterEditSizeAdjust(JsonParse *pParse, u32 iRoot){
  u32 sz = 0;
  u32 nBlob;
  assert( pParse->delta!=0 );
  assert( pParse->nBlobAlloc >= pParse->nBlob );
  nBlob = pParse->nBlob;
  pParse->nBlob = pParse->nBlobAlloc;
  (void)jsonbPayloadSize(pParse, iRoot, &sz);
  pParse->nBlob = nBlob;
  sz += pParse->delta;
  pParse->delta += jsonBlobChangePayloadSize(pParse, iRoot, sz);
}
/* …（同文件无关代码省略）… */
static int jsonBlobOverwrite(
  u8 *aOut,                 /* Overwrite here */
  const u8 *aIns,           /* New content */
  u32 nIns,                 /* Bytes of new content */
  u32 d                     /* Need to expand new content by this much */
){
  u32 szPayload;       /* Bytes of payload */
  u32 i;               /* New header size, after expansion & a loop counter */
  u8 szHdr;            /* Size of header before expansion */

  /* Lookup table for finding the upper 4 bits of the first byte of the
  ** expanded aIns[], based on the size of the expanded aIns[] header:
  **
  **                             2     3  4     5  6  7  8     9 */
  static const u8 aType[] = { 0xc0, 0xd0, 0, 0xe0, 0, 0, 0, 0xf0 };

  if( (aIns[0]&0x0f)<=2 ) return 0;    /* Cannot enlarge NULL, true, false */
  switch( aIns[0]>>4 ){
    default: {                         /* aIns[] header size 1 */
      if( ((1<<d)&0x116)==0 ) return 0;  /* d must be 1, 2, 4, or 8 */
      i = d + 1;                         /* New hdr sz: 2, 3, 5, or 9 */
      szHdr = 1;
      break;
    }
    case 12: {                         /* aIns[] header size is 2 */
      if( ((1<<d)&0x8a)==0) return 0;    /* d must be 1, 3, or 7 */
      i = d + 2;                         /* New hdr sz: 2, 5, or 9 */
      szHdr = 2;
      break;
    }
    case 13: {                         /* aIns[] header size is 3 */
      if( d!=2 && d!=6 ) return 0;       /* d must be 2 or 6 */
      i = d + 3;                         /* New hdr sz: 5 or 9 */
      szHdr = 3;
      break;
    }
    case 14: {                         /* aIns[] header size is 5 */
      if( d!=4 ) return 0;               /* d must be 4 */
      i = 9;                             /* New hdr sz: 9 */
      szHdr = 5;
      break;
    }
    case 15: {                         /* aIns[] header size is 9 */
      return 0;                          /* No solution */
    }
  }
  assert( i>=2 && i<=9 && aType[i-2]!=0 );
  aOut[0] = (aIns[0] & 0x0f) | aType[i-2];
  memcpy(&aOut[i], &aIns[szHdr], nIns-szHdr);
  szPayload = nIns - szHdr;
  while( 1/*edit-by-break*/ ){
    i--;
    aOut[i] = szPayload & 0xff;
    if( i==1 ) break;
    szPayload >>= 8;
  }
  assert( (szPayload>>8)==0 );
  return 1;
}
/* …（同文件无关代码省略）… */
static void jsonBlobEdit(
  JsonParse *pParse,     /* The JSONB to be modified is in pParse->aBlob */
  u32 iDel,              /* First byte to be removed */
  u32 nDel,              /* Number of bytes to remove */
  const u8 *aIns,        /* Content to insert */
  u32 nIns               /* Bytes of content to insert */
){
  i64 d = (i64)nIns - (i64)nDel;
  assert( pParse->nBlob >= (u64)iDel + (u64)nDel );
  if( d<0 && d>=(-8) && aIns!=0
   && jsonBlobOverwrite(&pParse->aBlob[iDel], aIns, nIns, (int)-d)
  ){
    return;
  }
  if( d!=0 ){
    if( pParse->nBlob + d > pParse->nBlobAlloc ){
      jsonBlobExpand(pParse, (u64)pParse->nBlob+d);
      if( pParse->oom ) return;
    }
    memmove(&pParse->aBlob[iDel+nIns],
            &pParse->aBlob[iDel+nDel],
            pParse->nBlob - (iDel+nDel));
    pParse->nBlob += d;
    pParse->delta += d;
  }
  if( nIns && aIns ){
    memcpy(&pParse->aBlob[iDel], aIns, nIns);
  }
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
static SQLITE_NOINLINE int jsonLabelCompareEscaped(
  const char *zLeft,          /* The left label */
  u32 nLeft,                  /* Size of the left label in bytes */
  int rawLeft,                /* True if zLeft contains no escapes */
  const char *zRight,         /* The right label */
  u32 nRight,                 /* Size of the right label in bytes */
  int rawRight                /* True if zRight is escape-free */
){
  u32 cLeft, cRight;
  assert( rawLeft==0 || rawRight==0 );
  while( 1 /*exit-by-return*/ ){
    if( nLeft==0 ){
      cLeft = 0;
    }else if( rawLeft || zLeft[0]!='\\' ){
      cLeft = ((u8*)zLeft)[0];
      if( cLeft>=0xc0 ){
        int sz = sqlite3Utf8ReadLimited((u8*)zLeft, nLeft, &cLeft);
        zLeft += sz;
        nLeft -= sz;
      }else{
        zLeft++;
        nLeft--;
      }
    }else{
      u32 n = jsonUnescapeOneChar(zLeft, nLeft, &cLeft);
      zLeft += n;
      assert( n<=nLeft );
      nLeft -= n;
    }
    if( nRight==0 ){
      cRight = 0;
    }else if( rawRight || zRight[0]!='\\' ){
      cRight = ((u8*)zRight)[0];
      if( cRight>=0xc0 ){
        int sz = sqlite3Utf8ReadLimited((u8*)zRight, nRight, &cRight);
        zRight += sz;
        nRight -= sz;
      }else{
        zRight++;
        nRight--;
      }
    }else{
      u32 n = jsonUnescapeOneChar(zRight, nRight, &cRight);
      zRight += n;
      assert( n<=nRight );
      nRight -= n;
    }
    if( cLeft!=cRight ) return 0;
    if( cLeft==0 ) return 1;
  }
}
/* …（同文件无关代码省略）… */
static int jsonLabelCompare(
  const char *zLeft,          /* The left label */
  u32 nLeft,                  /* Size of the left label in bytes */
  int rawLeft,                /* True if zLeft contains no escapes */
  const char *zRight,         /* The right label */
  u32 nRight,                 /* Size of the right label in bytes */
  int rawRight                /* True if zRight is escape-free */
){
  if( rawLeft && rawRight ){
    /* Simpliest case:  Neither label contains escapes.  A simple
    ** memcmp() is sufficient. */
    if( nLeft!=nRight ) return 0;
    return memcmp(zLeft, zRight, nLeft)==0;
  }else{
    return jsonLabelCompareEscaped(zLeft, nLeft, rawLeft,
                                   zRight, nRight, rawRight);
  }
}
/* …（同文件无关代码省略）… */
#define JSON_LOOKUP_ERROR      0xffffffff
#define JSON_LOOKUP_NOTFOUND   0xfffffffe
#define JSON_LOOKUP_NOTARRAY   0xfffffffd
#define JSON_LOOKUP_TOODEEP    0xfffffffc
#define JSON_LOOKUP_PATHERROR  0xfffffffb
#define JSON_LOOKUP_ISERROR(x) ((x)>=JSON_LOOKUP_PATHERROR)
/* …（同文件无关代码省略）… */
static u32 jsonCreateEditSubstructure(
  JsonParse *pParse,  /* The original JSONB that is being edited */
  JsonParse *pIns,    /* Populate this with the blob data to insert */
  const char *zTail   /* Tail of the path that determines substructure */
){
  static const u8 emptyObject[] = { JSONB_ARRAY, JSONB_OBJECT };
  int rc;
  memset(pIns, 0, sizeof(*pIns));
  pIns->db = pParse->db;
  if( zTail[0]==0 ){
    /* No substructure.  Just insert what is given in pParse. */
    pIns->aBlob = pParse->aIns;
    pIns->nBlob = pParse->nIns;
    rc = 0;
  }else{
    /* Construct the binary substructure */
    pIns->nBlob = 1;
    pIns->aBlob = (u8*)&emptyObject[zTail[0]=='.'];
    pIns->eEdit = pParse->eEdit;
    pIns->nIns = pParse->nIns;
    pIns->aIns = pParse->aIns;
    pIns->iDepth = pParse->iDepth+1;
    if( pIns->iDepth >= JSON_MAX_DEPTH ){
      return JSON_LOOKUP_TOODEEP;
    }
    rc = jsonLookupStep(pIns, 0, zTail, 0);
    pParse->iDepth--;
    pParse->oom |= pIns->oom;
  }
  return rc;  /* Error code only */
}
/* …（同文件无关代码省略）… */
static u32 jsonLookupStep(
  JsonParse *pParse,      /* The JSON to search */
  u32 iRoot,              /* Begin the search at this element of aBlob[] */
  const char *zPath,      /* The path to search */
  u32 iLabel              /* Label if iRoot is a value of in an object */
){
  u32 i, j, k, nKey, sz, n, iEnd, rc;
  const char *zKey;
  u8 x;

  if( zPath[0]==0 ){
    if( pParse->eEdit && jsonBlobMakeEditable(pParse, pParse->nIns) ){
      n = jsonbPayloadSize(pParse, iRoot, &sz);
      sz += n;
      if( pParse->eEdit==JEDIT_DEL ){
        if( iLabel>0 ){
          sz += iRoot - iLabel;
          iRoot = iLabel;
        }
        jsonBlobEdit(pParse, iRoot, sz, 0, 0);
      }else if( pParse->eEdit==JEDIT_INS ){
        /* Already exists, so json_insert() is a no-op */
      }else if( pParse->eEdit==JEDIT_AINS ){
        /* json_array_insert() */
        if( zPath[-1]!=']' ){
          return JSON_LOOKUP_NOTARRAY;
        }else{
          jsonBlobEdit(pParse, iRoot, 0, pParse->aIns, pParse->nIns);
        }
      }else{
        /* json_set() or json_replace() */
        jsonBlobEdit(pParse, iRoot, sz, pParse->aIns, pParse->nIns);
      }
    }
    pParse->iLabel = iLabel;
    return iRoot;
  }
  if( zPath[0]=='.' ){
    int rawKey = 1;
    x = pParse->aBlob[iRoot];
    zPath++;
    if( zPath[0]=='"' ){
      zKey = zPath + 1;
      for(i=1; zPath[i] && zPath[i]!='"'; i++){
        if( zPath[i]=='\\' && zPath[i+1]!=0 ) i++;
      }
      nKey = i-1;
      if( zPath[i] ){
        i++;
      }else{
        return JSON_LOOKUP_PATHERROR;
      }
      testcase( nKey==0 );
      rawKey = memchr(zKey, '\\', nKey)==0;
    }else{
      zKey = zPath;
      for(i=0; zPath[i] && zPath[i]!='.' && zPath[i]!='['; i++){}
      nKey = i;
      if( nKey==0 ){
        return JSON_LOOKUP_PATHERROR;
      }
    }
    if( (x & 0x0f)!=JSONB_OBJECT ) return JSON_LOOKUP_NOTFOUND;
    n = jsonbPayloadSize(pParse, iRoot, &sz);
    j = iRoot + n;  /* j is the index of a label */
    iEnd = j+sz;
    while( j<iEnd ){
      int rawLabel;
      const char *zLabel;
      x = pParse->aBlob[j] & 0x0f;
      if( x<JSONB_TEXT || x>JSONB_TEXTRAW ) return JSON_LOOKUP_ERROR;
      n = jsonbPayloadSizeSemiInline(pParse, j, &sz);
      if( n==0 ) return JSON_LOOKUP_ERROR;
      k = j+n;  /* k is the index of the label text */
      if( k+sz>=iEnd ) return JSON_LOOKUP_ERROR;
      zLabel = (const char*)&pParse->aBlob[k];
      rawLabel = x==JSONB_TEXT || x==JSONB_TEXTRAW;
      if( jsonLabelCompare(zKey, nKey, rawKey, zLabel, sz, rawLabel) ){
        u32 v = k+sz;  /* v is the index of the value */
        if( ((pParse->aBlob[v])&0x0f)>JSONB_OBJECT ) return JSON_LOOKUP_ERROR;
        n = jsonbPayloadSize(pParse, v, &sz);
        if( n==0 || v+n+sz>iEnd ) return JSON_LOOKUP_ERROR;
        assert( j>0 );
        if( ++pParse->iDepth >= JSON_MAX_DEPTH ){
          return JSON_LOOKUP_TOODEEP;
        }
        rc = jsonLookupStep(pParse, v, &zPath[i], j);
        pParse->iDepth--;
        if( pParse->delta ) jsonAfterEditSizeAdjust(pParse, iRoot);
        return rc;
      }
      j = k+sz;
      if( ((pParse->aBlob[j])&0x0f)>JSONB_OBJECT ) return JSON_LOOKUP_ERROR;
      n = jsonbPayloadSizeSemiInline(pParse, j, &sz);
      if( n==0 ) return JSON_LOOKUP_ERROR;
      j += n+sz;
    }
    if( j>iEnd ) return JSON_LOOKUP_ERROR;
    if( pParse->eEdit>=JEDIT_INS ){
      u32 nIns;          /* Total bytes to insert (label+value) */
      JsonParse v;       /* BLOB encoding of the value to be inserted */
      JsonParse ix;      /* Header of the label to be inserted */
      testcase( pParse->eEdit==JEDIT_INS );
      testcase( pParse->eEdit==JEDIT_SET );
      testcase( pParse->eEdit==JEDIT_AINS );
      if( pParse->eEdit==JEDIT_AINS && sqlite3_strglob("*]",&zPath[i])!=0 ){
        return JSON_LOOKUP_NOTARRAY;
      }
      memset(&ix, 0, sizeof(ix));
      ix.db = pParse->db;
      jsonBlobAppendNode(&ix, rawKey?JSONB_TEXTRAW:JSONB_TEXT5, nKey, 0);
      pParse->oom |= ix.oom;
      rc = jsonCreateEditSubstructure(pParse, &v, &zPath[i]);
      if( !JSON_LOOKUP_ISERROR(rc)
       && jsonBlobMakeEditable(pParse, ix.nBlob+nKey+v.nBlob)
      ){
        assert( !pParse->oom );
        nIns = ix.nBlob + nKey + v.nBlob;
        jsonBlobEdit(pParse, j, 0, 0, nIns);
        if( !pParse->oom ){
          assert( pParse->aBlob!=0 ); /* Because pParse->oom!=0 */
          assert( ix.aBlob!=0 );      /* Because pPasre->oom!=0 */
          memcpy(&pParse->aBlob[j], ix.aBlob, ix.nBlob);
          k = j + ix.nBlob;
          memcpy(&pParse->aBlob[k], zKey, nKey);
          k += nKey;
          memcpy(&pParse->aBlob[k], v.aBlob, v.nBlob);
          if( ALWAYS(pParse->delta) ) jsonAfterEditSizeAdjust(pParse, iRoot);
        }
      }
      jsonParseReset(&v);
      jsonParseReset(&ix);
      return rc;
    }
  }else if( zPath[0]=='[' ){
    u64 kk = 0;
    x = pParse->aBlob[iRoot] & 0x0f;
    if( x!=JSONB_ARRAY )  return JSON_LOOKUP_NOTFOUND;
    n = jsonbPayloadSize(pParse, iRoot, &sz);
    i = 1;
    while( sqlite3Isdigit(zPath[i]) ){
      if( kk<0xffffffff ) kk = kk*10 + zPath[i] - '0';
      /*     ^^^^^^^^^^--- Allow kk to be bigger than any JSON array so that
      ** we get NOTFOUND instead of PATHERROR, without overflowing kk. */
      i++;
    }
    if( i<2 || zPath[i]!=']' ){
      if( zPath[1]=='#' ){
        kk = jsonbArrayCount(pParse, iRoot);
        i = 2;
        if( zPath[2]=='-' && sqlite3Isdigit(zPath[3]) ){
          u64 nn = 0;
          i = 3;
          do{
            if( nn<0xffffffff ) nn = nn*10 + zPath[i] - '0';
            /*     ^^^^^^^^^^--- Allow nn to be bigger than any JSON array to
            ** get NOTFOUND instead of PATHERROR, without overflowing nn. */
            i++;
          }while( sqlite3Isdigit(zPath[i]) );
          if( nn>kk ) return JSON_LOOKUP_NOTFOUND;
          kk -= nn;
        }
        if( zPath[i]!=']' ){
          return JSON_LOOKUP_PATHERROR;
        }
      }else{
        return JSON_LOOKUP_PATHERROR;
      }
    }
    j = iRoot+n;
    iEnd = j+sz;
    while( j<iEnd ){
      if( kk==0 ){
        if( ++pParse->iDepth >= JSON_MAX_DEPTH ){
          return JSON_LOOKUP_TOODEEP;
        }
        rc = jsonLookupStep(pParse, j, &zPath[i+1], 0);
        pParse->iDepth--;
        if( pParse->delta ) jsonAfterEditSizeAdjust(pParse, iRoot);
        return rc;
      }
      kk--;
      n = jsonbPayloadSizeSemiInline(pParse, j, &sz);
      if( n==0 ) return JSON_LOOKUP_ERROR;
      j += n+sz;
    }
    if( j>iEnd ) return JSON_LOOKUP_ERROR;
    if( kk>0 ) return JSON_LOOKUP_NOTFOUND;
    if( pParse->eEdit>=JEDIT_INS ){
      JsonParse v;
      testcase( pParse->eEdit==JEDIT_INS );
      testcase( pParse->eEdit==JEDIT_AINS );
      testcase( pParse->eEdit==JEDIT_SET );
      rc = jsonCreateEditSubstructure(pParse, &v, &zPath[i+1]);
      if( !JSON_LOOKUP_ISERROR(rc)
       && jsonBlobMakeEditable(pParse, v.nBlob)
      ){
        assert( !pParse->oom );
        jsonBlobEdit(pParse, j, 0, v.aBlob, v.nBlob);
      }
      jsonParseReset(&v);
      if( pParse->delta ) jsonAfterEditSizeAdjust(pParse, iRoot);
      return rc;
    }
  }else{
    return JSON_LOOKUP_PATHERROR; 
  }
  return JSON_LOOKUP_NOTFOUND;
}
/* …（同文件无关代码省略）… */
  u32 n = p->path.nUsed;
  char *z = p->path.zBuf;
  if( p->iRowid==0 && p->bRecursive && n>=2 ){
    while( n>1 ){
      n--;
      if( z[n]=='[' || z[n]=='.' ){
        u32 x, sz = 0;
        char cSaved = z[n];
        z[n] = 0;
        assert( p->sParse.eEdit==0 );
        x = jsonLookupStep(&p->sParse, 0, z+1, 0);
        z[n] = cSaved;
        if( JSON_LOOKUP_ISERROR(x) ) continue;
        if( x + jsonbPayloadSize(&p->sParse, x, &sz) == p->i ) break;
      }
    }
  }
