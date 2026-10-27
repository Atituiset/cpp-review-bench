// AUTO-DRAFT from sqlite/sqlite PR #5d1ee888094efc63d1fb25a58af93478b103d441
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct SessionBuffer SessionBuffer;
typedef struct SessionInput SessionInput;
/* …（同文件无关代码省略）… */
struct sqlite3_changeset_iter {
  SessionInput in;                /* Input buffer or stream */
  SessionBuffer tblhdr;           /* Buffer to hold apValue/zTab/abPK/ */
  int bPatchset;                  /* True if this is a patchset */
  int bInvert;                    /* True to invert changeset */
  int bSkipEmpty;                 /* Skip noop UPDATE changes */
  int rc;                         /* Iterator error code */
  sqlite3_stmt *pConflict;        /* Points to conflicting row, if any */
  char *zTab;                     /* Current table */
  int nCol;                       /* Number of columns in zTab */
  int op;                         /* Current operation */
  int bIndirect;                  /* True if current change was indirect */
  u8 *abPK;                       /* Primary key array */
  sqlite3_value **apValue;        /* old.* and new.* values */
};
/* …（同文件无关代码省略）… */
static int sessionVarintPut(u8 *aBuf, int iVal){
  return putVarint32(aBuf, iVal);
}
/* …（同文件无关代码省略）… */
static int sessionSerializeValue(
  u8 *aBuf,                       /* If non-NULL, write serialized value here */
  sqlite3_value *pValue,          /* Value to serialize */
  sqlite3_int64 *pnWrite          /* IN/OUT: Increment by bytes written */
){
  int nByte;                      /* Size of serialized value in bytes */

  if( pValue ){
    int eType;                    /* Value type (SQLITE_NULL, TEXT etc.) */
  
    eType = sqlite3_value_type(pValue);
    if( aBuf ) aBuf[0] = eType;
  
    switch( eType ){
      case SQLITE_NULL: 
        nByte = 1;
        break;
  
      case SQLITE_INTEGER: 
      case SQLITE_FLOAT:
        if( aBuf ){
          /* TODO: SQLite does something special to deal with mixed-endian
          ** floating point values (e.g. ARM7). This code probably should
          ** too.  */
          if( eType==SQLITE_INTEGER ){
            u64 i = (u64)sqlite3_value_int64(pValue);
            sessionPutI64(&aBuf[1], i);
          }else{
            double r = sqlite3_value_double(pValue);
            sessionPutDouble(&aBuf[1], r);
          }
        }
        nByte = 9; 
        break;
  
      default: {
        u8 *z;
        int n;
        int nVarint;
  
        assert( eType==SQLITE_TEXT || eType==SQLITE_BLOB );
        if( eType==SQLITE_TEXT ){
          z = (u8 *)sqlite3_value_text(pValue);
        }else{
          z = (u8 *)sqlite3_value_blob(pValue);
        }
        n = sqlite3_value_bytes(pValue);
        if( z==0 && (eType!=SQLITE_BLOB || n>0) ) return SQLITE_NOMEM;
        nVarint = sessionVarintLen(n);
  
        if( aBuf ){
          sessionVarintPut(&aBuf[1], n);
          if( n>0 ) memcpy(&aBuf[nVarint + 1], z, n);
        }
  
        nByte = 1 + nVarint + n;
        break;
      }
    }
  }else{
    nByte = 1;
    if( aBuf ) aBuf[0] = '\0';
  }

  if( pnWrite ) *pnWrite += nByte;
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static int sessionBufferGrow(SessionBuffer *p, i64 nByte, int *pRc){
#define SESSION_MAX_BUFFER_SZ (0x7FFFFF00 - 1) 
  i64 nReq = p->nBuf + nByte;
  if( *pRc==SQLITE_OK && nReq>p->nAlloc ){
    u8 *aNew;
    i64 nNew = p->nAlloc ? p->nAlloc : 128;

    do {
      nNew = nNew*2;
    }while( nNew<nReq );

    /* The value of SESSION_MAX_BUFFER_SZ is copied from the implementation
    ** of sqlite3_realloc64(). Allocations greater than this size in bytes
    ** always fail. It is used here to ensure that this routine can always
    ** allocate up to this limit - instead of up to the largest power of
    ** two smaller than the limit.  */
    if( nNew>SESSION_MAX_BUFFER_SZ ){
      nNew = SESSION_MAX_BUFFER_SZ;
      if( nNew<nReq ){
        *pRc = SQLITE_NOMEM;
        return 1;
      }
    }

    aNew = (u8 *)sqlite3_realloc64(p->aBuf, nNew);
    if( 0==aNew ){
      *pRc = SQLITE_NOMEM;
    }else{
      p->aBuf = aNew;
      p->nAlloc = nNew;
    }
  }
  return (*pRc!=SQLITE_OK);
}
/* …（同文件无关代码省略）… */
static void sessionAppendStr(
  SessionBuffer *p, 
  const char *zStr, 
  int *pRc
){
  int nStr = sqlite3Strlen30(zStr);
  if( 0==sessionBufferGrow(p, (i64)nStr+1, pRc) ){
    memcpy(&p->aBuf[p->nBuf], zStr, nStr);
    p->nBuf += nStr;
    p->aBuf[p->nBuf] = 0x00;
  }
}
/* …（同文件无关代码省略）… */
static void sessionAppendValue(SessionBuffer *p, sqlite3_value *pVal, int *pRc){
  int rc = *pRc;
  if( rc==SQLITE_OK ){
    sqlite3_int64 nByte = 0;
    rc = sessionSerializeValue(0, pVal, &nByte);
    sessionBufferGrow(p, nByte, &rc);
    if( rc==SQLITE_OK ){
      rc = sessionSerializeValue(&p->aBuf[p->nBuf], pVal, 0);
      p->nBuf += nByte;
    }else{
      *pRc = rc;
    }
  }
}
/* …（同文件无关代码省略）… */
static void sessionAppendByte(SessionBuffer *p, u8 v, int *pRc){
  if( 0==sessionBufferGrow(p, 1, pRc) ){
    p->aBuf[p->nBuf++] = v;
  }
}
/* …（同文件无关代码省略）… */
static void sessionAppendVarint(SessionBuffer *p, int v, int *pRc){
  if( 0==sessionBufferGrow(p, 9, pRc) ){
    p->nBuf += sessionVarintPut(&p->aBuf[p->nBuf], v);
  }
}
/* …（同文件无关代码省略）… */
static void sessionAppendBlob(
  SessionBuffer *p, 
  const u8 *aBlob, 
  int nBlob, 
  int *pRc
){
  if( nBlob>0 && 0==sessionBufferGrow(p, nBlob, pRc) ){
    memcpy(&p->aBuf[p->nBuf], aBlob, nBlob);
    p->nBuf += nBlob;
  }
}
/* …（同文件无关代码省略）… */
static void sessionAppendInteger(
  SessionBuffer *p,               /* Buffer to append to */
  int iVal,                       /* Value to write the string rep. of */
  int *pRc                        /* IN/OUT: Error code */
){
  char aBuf[24];
  sqlite3_snprintf(sizeof(aBuf)-1, aBuf, "%d", iVal);
  sessionAppendStr(p, aBuf, pRc);
}
/* …（同文件无关代码省略）… */
static void sessionAppendIdent(
  SessionBuffer *p,               /* Buffer to a append to */
  const char *zStr,               /* String to quote, escape and append */
  int *pRc         
