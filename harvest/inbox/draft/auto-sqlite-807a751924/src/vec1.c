// AUTO-DRAFT from sqlite/sqlite PR #cc758d2fba50f108d479c642f68a5ebb70252df3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef sqlite3_int64  i64;
typedef sqlite3_uint64 u64;
typedef unsigned char   u8;
typedef unsigned int   u32;
/* …（同文件无关代码省略）… */
#define SWAP(T, a, b) { T tmp; tmp = a; a = b; b = tmp;  }
/* …（同文件无关代码省略）… */
#define size_of_array(X) ((int)(sizeof(X)/sizeof((X)[0])))
/* …（同文件无关代码省略）… */
typedef struct Vec1Tab Vec1Tab;
/* …（同文件无关代码省略）… */
#define sizeof_u32 4
/* …（同文件无关代码省略）… */
#define sizeof_f64 8
/* …（同文件无关代码省略）… */
#define VEC1_CORRUPT vec1CorruptError();
/* …（同文件无关代码省略）… */
static void vec1PutU32(u8 *aBuf, u32 val){
  aBuf[0] = (val >> 24) & 0xFF;
  aBuf[1] = (val >> 16) & 0xFF;
  aBuf[2] = (val >>  8) & 0xFF;
  aBuf[3] = (val >>  0) & 0xFF;
}
/* …（同文件无关代码省略）… */
static u32 vec1GetU32(const u8 *aBuf){
  return (((u32)aBuf[0]) << 24)
       + (((u32)aBuf[1]) << 16)
       + (((u32)aBuf[2]) <<  8)
       + (((u32)aBuf[3]) <<  0);
}
/* …（同文件无关代码省略）… */
typedef struct Vec1Buffer Vec1Buffer;
/* …（同文件无关代码省略）… */
static int vec1BufferSize(Vec1Buffer *pBuf, sqlite3_int64 nByte){
  if( nByte>pBuf->nAlloc ){
    u8 *aNew = sqlite3_realloc64(pBuf->a, nByte);
    if( aNew==0 ){
      return SQLITE_NOMEM;
    }
    pBuf->a = aNew;
    pBuf->nAlloc = (int)nByte;
  }
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static void vec1BufferFree(Vec1Buffer *pBuf){
  sqlite3_free(pBuf->a);
  memset(pBuf, 0, sizeof(Vec1Buffer));
}
/* …（同文件无关代码省略）… */
static void vec1PutU64(u8 *aBuf, u64 val){
  aBuf[0] = (val >> 56) & 0xFF;
  aBuf[1] = (val >> 48) & 0xFF;
  aBuf[2] = (val >> 40) & 0xFF;
  aBuf[3] = (val >> 32) & 0xFF;
  aBuf[4] = (val >> 24) & 0xFF;
  aBuf[5] = (val >> 16) & 0xFF;
  aBuf[6] = (val >>  8) & 0xFF;
  aBuf[7] = (val >>  0) & 0xFF;
}
/* …（同文件无关代码省略）… */
static u64 vec1GetU64(const u8 *aBuf){
  return (((u64)aBuf[0]) << 56)
       + (((u64)aBuf[1]) << 48)
       + (((u64)aBuf[2]) << 40)
       + (((u64)aBuf[3]) << 32) 
       + (((u64)aBuf[4]) << 24)
       + (((u64)aBuf[5]) << 16)
       + (((u64)aBuf[6]) <<  8)
       + (((u64)aBuf[7]) <<  0);
}
/* …（同文件无关代码省略）… */
#define VEC1_META_GENERIC         0x01
#define VEC1_META_1BYTEINT        0x02
#define VEC1_META_4BYTEINT        0x04
#define VEC1_META_REAL            0x08
#define VEC1_META_HASNULL         0x10
/* …（同文件无关代码省略）… */
#define VEC1_META_SZHDR 8
#define VEC1_META_1BYTENULL 255
/* …（同文件无关代码省略）… */
#define VEC1_META_REALNULL  0x7FF8000000000001
/* …（同文件无关代码省略）… */
#define VEC1_META_4BYTENULL  +2147483647
/* …（同文件无关代码省略）… */
#define VEC1_LIST_64BIT   0x01    /* Set for 64-bit rowids, clear for 32-bit */
#define VEC1_LIST_SORTED  0x02    /* True if rowid array is in sorted order */
/* …（同文件无关代码省略）… */
#define VEC1_LIST_SZHDR (3*sizeof_u32)
/* …（同文件无关代码省略）… */
typedef struct Vec1MetaBuilder Vec1MetaBuilder;
/* …（同文件无关代码省略）… */
typedef struct Vec1ListBuilder Vec1ListBuilder;
/* …（同文件无关代码省略）… */
typedef struct Vec1Writer Vec1Writer;
/* …（同文件无关代码省略）… */
#define VEC1_META_COLUMN_BITS 8
#define VEC1_MAX_META_COLUMNS (1<<VEC1_META_COLUMN_BITS)
/* …（同文件无关代码省略）… */
typedef struct Vec1MetaValue Vec1MetaValue;
/* …（同文件无关代码省略）… */
static char *vec1MPrintf(int *pRc, const char *zFmt, ...){
  char *zRet = 0;
  va_list ap;
  va_start(ap, zFmt);
  zRet = sqlite3_vmprintf(zFmt, ap);
  va_end(ap);

  if( *pRc==SQLITE_OK ){
    if( zRet==0 ){
      *pRc = SQLITE_NOMEM;
    }
  }else{
    sqlite3_free(zRet);
    zRet = 0;
  }

  return zRet;
}
/* …（同文件无关代码省略）… */
static void vec1VtabError(Vec1Tab *pTab, const char *zFmt, ...){
  char *zMsg = 0;
  va_list ap;
  va_start(ap, zFmt);
  zMsg = sqlite3_vmprintf(zFmt, ap);
  va_end(ap);
  sqlite3_free(pTab->base.zErrMsg);
  pTab->base.zErrMsg = zMsg;
}
/* …（同文件无关代码省略）… */
static int vec1PrepareSql(
  Vec1Tab *pTab, 
  sqlite3_stmt **ppStmt,
  const char *zFmt, 
  ...
){
  char *zSql = 0;
  int rc = SQLITE_OK;
  va_list ap;

  va_start(ap, zFmt);
  zSql = sqlite3_vmprintf(zFmt, ap);
  va_end(ap);

  *ppStmt = 0;
  if( zSql ){
    rc = sqlite3_prepare_v2(pTab->db, zSql, -1, ppStmt, 0);
    sqlite3_free(zSql);
    if( rc!=SQLITE_OK ){
      vec1VtabError(pTab, "%s", sqlite3_errmsg(pTab->db));
    }
  }else{
    rc = SQLITE_NOMEM;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
#define VEC1_SQL_INSERT_IDX    2
/* …（同文件无关代码省略）… */
static int vec1GetSql(Vec1Tab *pTab, int eSql, sqlite3_stmt **ppStmt){
  const char *azSql[] = {
#define VEC1_SQL_INSERT_BASE   0
    "INSERT INTO %Q.'%q_base' VALUES(?, ?%s)",
    "SELECT * FROM %Q.'%q_base' ORDER BY 1",
    "INSERT INTO %Q.'%q_idx'(id, bucket, first, last, val)VALUES(?,?,?,?,?)",

    "SELECT val, bucket, rowid FROM %Q.'%q_idx'",
    "SELECT sum( length(val) / ? ) FROM %Q.'%q_idx'",

    "REPLACE INTO %Q.'%q_config'(id, val) VALUES(?, ?)",
    "INSERT INTO %Q.'%q_config'(id, val) VALUES(?, 1) "
      "ON CONFLICT(id) DO UPDATE SET val=val+1",

    "SELECT id, val FROM %Q.'%q_config'",

#define VEC1_SQL_LOOKUP_BASE   8
    "SELECT * FROM %Q.'%q_base' WHERE id=?",

#define VEC1_SQL_EDIT_IDX      9
    "SELECT id, min( length(val) ) FROM %Q.'%q_idx' WHERE bucket=?",

#define VEC1_SQL_DEL_LOOKUP_BASE 10
    /* Used when removing a row by id. This statement deletes the %_base
    ** entry and returns the vector so that the user may use it to 
    ** find the %_idx entry to delete.  */
    "DELETE FROM %Q.'%q_base' WHERE id=? RETURNING vector",

#define VEC1_SQL_SEARCH_IDX 11
    "SELECT id FROM %Q.'%q_idx' WHERE bucket=? AND ? BETWEEN first AND last",

#define VEC1_SQL_SEARCH_IDX_FOR_INSERT 12
    "SELECT id, first, last, val "
      "FROM %Q.'%q_idx' WHERE bucket=? AND length(val)<? LIMIT 1",

#define VEC1_SCAN_BUCKET 13
    "SELECT val, bucket, rowid FROM %Q.'%q_idx' WHERE bucket = 0+?",

#define VEC1_SQL_UPDATE_BASE   14
    "UPDATE %Q.'%q_base' SET vector = ? WHERE id=?",

#define VEC1_SQL_ZERO_BASE_RANGE 15
    "UPDATE %Q.'%q_base' SET vector = 0, id=id WHERE id BETWEEN ? AND ?",

#define VEC1_SQL_DELETE_FROM_IDX 16
    "DELETE FROM %Q.'%q_idx' WHERE id = ?",

#define VEC1_SQL_UPDATE_BASE2   17
    "UPDATE %Q.'%q_base' SET vector = ?, id=id WHERE id=?",

#define VEC1_SQL_READ_META   18
    "SELECT val FROM %Q.'%q_meta' WHERE id=?",

#define VEC1_SQL_WRITE_META   19
    "REPLACE INTO %Q.'%q_meta' VALUES(?, ?)",

#define VEC1_SQL_DELETE_FROM_META 20
    "DELETE FROM %Q.'%q_meta' WHERE id BETWEEN (?1<<8) AND (?1<<8)+255",
  };
  int rc = SQLITE_OK;

  assert( eSql>=0 && eSql<size_of_array(azSql) );
  assert( size_of_array(azSql)==size_of_array(pTab->aStmt) );

  if( pTab->aStmt[eSql]==0 ){

    if( eSql==VEC1_SQL_INSERT_BASE ){
      int ii;
      char *zExtra = 0;
      for(ii=0; ii<pTab->nMeta; ii++){
        zExtra = vec1MPrintf(&rc, "%z,?", zExtra);
      }
      if( rc==SQLITE_OK ){
        rc = vec1PrepareSql(
            pTab, &pTab->aStmt[eSql], azSql[eSql], 
            pTab->zDb, pTab->zName, zExtra
        );
      }
      sqlite3_free(zExtra);
    }else{
      rc = vec1PrepareSql(
          pTab, &pTab->aStmt[eSql], azSql[eSql], pTab->zDb, pTab->zName
      );
    }
  }

  *ppStmt = pTab->aStmt[eSql];
  return rc;
}
/* …（同文件无关代码省略）… */
static void vec1StmtReset(int *pRc, sqlite3_stmt *pStmt){
  int rc = sqlite3_reset(pStmt);
  if( *pRc==SQLITE_OK ) *pRc = rc;
}
/* …（同文件无关代码省略）… */
static int vec1GetVarint(const u8 *aBuf, u64 *piVal){
  u64 out = 0;
  int ii = 0;

  out = (aBuf[0] & 0x7F);
  if( aBuf[0] & 0x80 ){
    for(ii=1; ii<9; ii++){
      out = (out<<7) + (aBuf[ii] & 0x7F);
      if( (aBuf[ii] & 0x80)==0 ){
        break;
      }
    }
  }

  *piVal = out;
  return ii+1;
}
/* …（同文件无关代码省略）… */
static int vec1MetaValueRead(
  Vec1Buffer *pMeta,
  int *piOff,
  Vec1MetaValue *pVal
){
  i64 iOff = (i64)*piOff;

  switch( pMeta->a[iOff] ){
    case 0: {
      pVal->eType = SQLITE_NULL;
      iOff++;
      break;
    }

    case 1: {
      pVal->eType = SQLITE_INTEGER;
      pVal->iVal = (i64)(u8)pMeta->a[iOff+1];
      iOff += 2;
      break;
    }

    case 2: {
      pVal->eType = SQLITE_INTEGER;
      pVal->iVal = (int)vec1GetU32(&pMeta->a[iOff+1]);
      iOff += 5;
      break;
    }

    case 3: {
      pVal->eType = SQLITE_INTEGER;
      pVal->iVal = (i64)vec1GetU64(&pMeta->a[iOff+1]);
      iOff += 9;
      break;
    }

    case 4: {
      u64 iVal = vec1GetU64(&pMeta->a[iOff+1]);
      memcpy(&pVal->fVal, &iVal, sizeof(u64));
      pVal->eType = SQLITE_FLOAT;
      iOff += 9;
      break;
    }

    default: {
      u64 eType = 0;
      iOff += vec1GetVarint(&pMeta->a[iOff], &eType);
      pVal->pPtr = &pMeta->a[iOff];
      pVal->iVal = (((eType-5) / 2) & (u64)0xFFFFFFFF);
      iOff += pVal->iVal;
      pVal->eType = (eType & 0x01) ? SQLITE_TEXT : SQLITE_BLOB;
      break;
    }
  }

  if( iOff>pMeta->n || iOff<0 ){
    return VEC1_CORRUPT;
  }

  *piOff = (int)iOff;
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static int vec1WriteMetaBlob(
  Vec1Tab *pTab,                  /* Vec1 virtual table */
  Vec1Buffer *pBuf,               /* Buffer to write from */
  i64 iId,                        /* Id of %_idx row */
  int iMeta                       /* Index of meta-column */
){
  sqlite3_stmt *pStmt = 0;
  int rc = vec1GetSql(pTab, VEC1_SQL_WRITE_META, &pStmt);

  assert( iMeta>=0 && iMeta<VEC1_MAX_META_COLUMNS );
  if( rc==SQLITE_OK ){
    i64 iMetaId = (iId << VEC1_META_COLUMN_BITS) + iMeta;
    sqlite3_bind_int64(pStmt, 1, iMetaId);
    rc = sqlite3_bind_blob(pStmt, 2, pBuf->a, pBuf->n, SQLITE_STATIC);
    sqlite3_step(pStmt);
    vec1StmtReset(&rc, pStmt);
    sqlite3_bind_null(pStmt, 2);
  }

  return rc;
}
/* …（同文件无关代码省略）… */
static int vec1WriteMeta(
  Vec1Tab *pTab,                  /* Vec1 virtual table */
  Vec1MetaBuilder *p,             /* Buffer to write from */
  int nEntry,
  i64 iId,                        /* Id of %_idx row */
  int iMeta                       /* Index of meta-column */
){
  u32 f = 0;

  if( (p->format & VEC1_META_GENERIC)
   && (p->flags & (VEC1_META_1BYTEINT|VEC1_META_4BYTEINT|VEC1_META_REAL))
  ){
    int iIn = VEC1_META_SZHDR;
    int iOut = VEC1_META_SZHDR;
    Vec1MetaValue val = {0,0,0,0};
    Vec1Buffer buf = {0, 0, 0};

    int rc = SQLITE_OK;
    int szElem;
    i64 iNull;
    if( p->flags & VEC1_META_1BYTEINT ){
      szElem = 1;
      iNull = VEC1_META_1BYTENULL;
      p->format = VEC1_META_1BYTEINT;
    }else if( p->flags & VEC1_META_4BYTEINT ){
      szElem = sizeof_u32;
      iNull = VEC1_META_4BYTENULL;
      p->format = VEC1_META_4BYTEINT;
    }else{
      szElem = sizeof_f64;
      iNull = VEC1_META_REALNULL;
      p->format = VEC1_META_REAL;
    }

    rc = vec1BufferSize(&buf, VEC1_META_SZHDR + nEntry * szElem);
    if( rc!=SQLITE_OK ) return rc;

    while( iIn<p->buf.n ){
      rc = vec1MetaValueRead(&p->buf, &iIn, &val);
      if( rc!=SQLITE_OK ) break;
      if( val.eType==SQLITE_NULL ){
        val.iVal = iNull;
      }else if( p->format==VEC1_META_REAL ){
        if( val.eType==SQLITE_INTEGER ){
          val.fVal = (double)val.iVal;
        }
        memcpy(&val.iVal, &val.fVal, sizeof_f64);
      }
      if( szElem==1 ){
        buf.a[iOut] = (val.iVal & 0xFF);
      }else if( szElem==sizeof_u32 ){
        vec1PutU32(&buf.a[iOut], (u32)val.iVal);
      }else{
        vec1PutU64(&buf.a[iOut], (u64)val.iVal);
      }
      iOut += szElem;
    }
    buf.n = iOut;

    SWAP(Vec1Buffer, p->buf, buf);
    vec1BufferFree(&buf);
  }

  f = (p->format | (p->flags & VEC1_META_HASNULL));
  vec1PutU32(&p->buf.a[0], f);
  vec1PutU32(&p->buf.a[4], nEntry);
  return vec1WriteMetaBlob(pTab, &p->buf, iId, iMeta);
}
/* …（同文件无关代码省略）… */
static int vec1ListBuilderFlush(Vec1ListBuilder *p){
  int rc = SQLITE_OK;
  if( p->bufRowid.n>0 ){
    int nEntry = p->bufRowid.n / p->szRowid;
    i64 iId = 0;
    int ii;
    sqlite3_stmt *pStmt = 0;
    u8 *aBlob = 0;
    int nBlob = 0;

    nBlob = VEC1_LIST_SZHDR + p->bufRowid.n + p->bufData.n;
    aBlob = (u8*)sqlite3_malloc(nBlob);
    if( aBlob==0 ){
      rc = SQLITE_NOMEM;
    }else{
      int flags = 0;
      if( p->bSorted ) flags |= VEC1_LIST_SORTED;
      if( p->szRowid==8 ) flags |= VEC1_LIST_64BIT;
      vec1PutU32(&aBlob[0], flags);
      vec1PutU32(&aBlob[4], nEntry);
      vec1PutU32(&aBlob[8], p->nTombstone);
      memcpy(&aBlob[VEC1_LIST_SZHDR], p->bufRowid.a, p->bufRowid.n);
      memcpy(&aBlob[VEC1_LIST_SZHDR+p->bufRowid.n], p->bufData.a, p->bufData.n);

      if( p->iId>0 ){
        /* If there is an existing entry, remove it */
        rc = vec1GetSql(p->pTab, VEC1_SQL_DELETE_FROM_IDX, &pStmt);
        if( rc==SQLITE_OK ){
          sqlite3_bind_int64(pStmt, 1, p->iId);
          sqlite3_step(pStmt);
          vec1StmtReset(&rc, pStmt);
        }
      }

      if( rc==SQLITE_OK ){
        rc = vec1GetSql(p->pTab, VEC1_SQL_INSERT_IDX, &pStmt);
      }
      if( rc==SQLITE_OK ){
        if( p->iId==0 ){
          sqlite3_bind_null(pStmt, 1);
        }else{
          sqlite3_bind_int64(pStmt, 1, p->iId);
        }
        sqlite3_bind_int(pStmt, 2, p->iBucket);
        sqlite3_bind_int64(pStmt, 3, p->iFirst);
        sqlite3_bind_int64(pStmt, 4, p->iLast);

        rc = sqlite3_bind_blob(pStmt, 5, aBlob, nBlob, SQLITE_STATIC);
        sqlite3_step(pStmt);
        vec1StmtReset(&rc, pStmt);
        if( rc==SQLITE_OK ){
          iId = sqlite3_last_insert_rowid(p->pTab->db);
        }
      }
      sqlite3_free(aBlob);

      /* If an error occurred within the sqlite3_step() call above, then the
      ** transaction may have been rolled back and (*p) freed. So take
      ** care not to access it again if rc!=SQLITE_OK.  */
      if( rc==SQLITE_OK ){
        p->bufRowid.n = 0;
        p->bufData.n = 0;
        p->iId = 0;
        p->iId = 0;
      }
    }

    for(ii=0; rc==SQLITE_OK && ii<p->pTab->nMeta; ii++){
      rc = vec1WriteMeta(p->pTab, &p->aMeta[ii], nEntry, iId, ii);
      if( rc==SQLITE_OK ) p->aMeta[ii].buf.n = VEC1_META_SZHDR;
    }
    
    /* Same reasoning as above - if rc!=SQLITE_OK, then the transaction
    ** may have been rolled back and (*p) freed already. */
    if( rc==SQLITE_OK ){
      p->bSorted = 1;
    }
  }else if( p->iId>0 ){
    sqlite3_stmt *pDelete = 0;
    rc = vec1GetSql(p->pTab, VEC1_SQL_DELETE_FROM_IDX, &pDelete);
    if( rc==SQLITE_OK ){
      sqlite3_bind_int64(pDelete, 1, p->iId);
      sqlite3_step(pDelete);
      vec1StmtReset(&rc, pDelete);
    }
    if( p->pTab->nMeta>0 && rc==SQLITE_OK ){
      rc = vec1GetSql(p->pTab, VEC1_SQL_DELETE_FROM_META, &pDelete);
      if( rc==SQLITE_OK ){
        sqlite3_bind_int64(pDelete, 1, p->iId);
        sqlite3_step(pDelete);
        vec1StmtReset(&rc, pDelete);
      }
    }
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static void vec1ListBuilderFree(Vec1ListBuilder *p){
  int ii;
  vec1BufferFree(&p->bufRowid);
  vec1BufferFree(&p->bufData);
  if( p->aMeta ){
    for(ii=0; ii<p->pTab->nMeta; ii++){
      vec1BufferFree(&p->aMeta[ii].buf);
    }
  }
  sqlite3_free(p->aMeta);
}
/* …（同文件无关代码省略）… */
static int vec1WriterFinish(Vec1Writer *p, int rcin){
  int rc = rcin;
  if( p ){
    int ii;
    for(ii=0; ii<p->nBld; ii++){
      if( rc==SQLITE_OK ){
        rc = vec1ListBuilderFlush(&p->aBld[ii]);
      }
      vec1ListBuilderFree(&p->aBld[ii]);
    }
    sqlite3_free(p);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int vec1InterpretCmdName(Vec1Tab *pTab, const char *zCmd, int *piCmd){
  const char *azCmd[] = {
    "blocksize",
    "rebuild",
    "blocksize-min",
    0
  };
  int iCmd;

  for(iCmd=0; iCmd<size_of_array(azCmd); iCmd++){
    if( 0==sqlite3_stricmp(zCmd, azCmd[iCmd]) ){
      *piCmd = iCmd;
      return SQLITE_OK;
    }
  }

  vec1VtabError(pTab, "vec1: unknown command: %s", zCmd);
  return SQLITE_ERROR;
}
/* …（同文件无关代码省略）… */
}


/*
** Handle a "special" INSERT command on a vec1 table. A special INSERT takes
** the form:
/* …（同文件无关代码省略）… */
  int iCmd = 0;

  rc = vec1InterpretCmdName(pTab, zCmd, &iCmd);

  if( rc==SQLITE_OK ){
    switch( iCmd ){
/* …（同文件无关代码省略）… */
  return rc;
}

static int vec1FinishWriter(Vec1Tab *pTab, int bDiscard){
  int rc = SQLITE_OK;
  if( bDiscard ){
    vec1WriterFinish(pTab->pWriter, SQLITE_ERROR);
  }else{
    rc = vec1WriterFinish(pTab->pWriter, SQLITE_OK);
  }
  pTab->pWriter = 0;
  return rc;
}

static int vec1SyncMethod(sqlite3_vtab *pVtab){
  int rc = SQLITE_OK;
  Vec1Tab *pTab = (Vec1Tab*)pVtab;
