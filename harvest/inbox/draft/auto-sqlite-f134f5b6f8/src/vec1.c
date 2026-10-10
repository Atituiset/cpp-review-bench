// AUTO-DRAFT from sqlite/sqlite PR #6e55a61a68cf58544ef294f218be65fb4310c654
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef sqlite3_int64  i64;
typedef sqlite3_uint64 u64;
typedef unsigned char   u8;
typedef unsigned int   u32;
/* …（同文件无关代码省略）… */
#define VEC1_LARGEST_INT64  (0xffffffff|(((i64)0x7fffffff)<<32))
#define VEC1_SMALLEST_INT64 (((i64)-1) - VEC1_LARGEST_INT64)
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
#define MAX(a,b) ((a<b) ? (b) : (a))
#define MIN(a,b) ((a>b) ? (b) : (a))
/* …（同文件无关代码省略）… */
#define VEC1_MODEL_BITQUANT 0x08
/* …（同文件无关代码省略）… */
#define sizeof_u32 4
#define sizeof_f32 4
#define sizeof_f64 8
/* …（同文件无关代码省略）… */
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
typedef struct Vec1Buffer Vec1Buffer;
/* …（同文件无关代码省略）… */
** encountered.
*/
static int vec1BufferGrow(Vec1Buffer *pBuf, sqlite3_int64 nByte){
  if( pBuf->n+nByte>pBuf->nAlloc ){
    sqlite3_int64 nNew = pBuf->nAlloc;
    unsigned char *aNew = 0;

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
static void *vec1MallocZero(i64 nByte){
  void *pRet = sqlite3_malloc64(nByte);
  if( pRet ){
    memset(pRet, 0, nByte);
  }
  return pRet;
}
/* …（同文件无关代码省略）… */
typedef struct Vec1Model Vec1Model;
/* …（同文件无关代码省略）… */
#define VEC1_META_GENERIC         0x01
#define VEC1_META_1BYTEINT        0x02
#define VEC1_META_4BYTEINT        0x04
#define VEC1_META_REAL            0x08
#define VEC1_META_HASNULL         0x10
/* …（同文件无关代码省略）… */
#define VEC1_META_TYPEMASK ( \
  VEC1_META_GENERIC | VEC1_META_1BYTEINT | VEC1_META_4BYTEINT | VEC1_META_REAL \
)
/* …（同文件无关代码省略）… */
#define VEC1_META_SZHDR 8
/* …（同文件无关代码省略）… */
#define VEC1_META_4BYTEMAX   +2147483646
#define VEC1_META_4BYTENULL  +2147483647

typedef struct Vec1Tab Vec1Tab;
typedef struct Vec1Csr Vec1Csr;

/* …（同文件无关代码省略）… */
#define VEC1_LIST_64BIT   0x01    /* Set for 64-bit rowids, clear for 32-bit */
#define VEC1_LIST_SORTED  0x02    /* True if rowid array is in sorted order */
/* …（同文件无关代码省略）… */
#define VEC1_LIST_SZHDR (3*sizeof_u32)
/* …（同文件无关代码省略）… */
#define VEC1_TOMBSTONE_32    0xFFFFFFFF
#define VEC1_TOMBSTONE_64             0
/* …（同文件无关代码省略）… */
#define VEC1_PQ_BLOCKSIZE 16
/* …（同文件无关代码省略）… */
typedef struct Vec1MetaBuilder Vec1MetaBuilder;
/* …（同文件无关代码省略）… */
typedef struct Vec1ListBuilder Vec1ListBuilder;
/* …（同文件无关代码省略）… */
#define VEC1_BLOCKED_NONE 0
#define VEC1_BLOCKED_PQ   1
#define VEC1_BLOCKED_BIN  2
/* …（同文件无关代码省略）… */
typedef struct Vec1Writer Vec1Writer;
/* …（同文件无关代码省略）… */
# define VEC1_QINSTR_METAREAD  4
# define VEC1_QINSTR_METASCAN  5
/* …（同文件无关代码省略）… */
# define VEC1_QINSTR_START(pTab, eCounter) {         \
  (pTab)->aCycle[eCounter] -= vec1HardwareTimer();  \
}
/* …（同文件无关代码省略）… */
# define VEC1_QINSTR_STOP(pTab, eCounter) {          \
  (pTab)->aCount[eCounter]++;                       \
  (pTab)->aCycle[eCounter] += vec1HardwareTimer();  \
}
/* …（同文件无关代码省略）… */
#define VEC1_META_COLUMN_BITS 8
#define VEC1_MAX_META_COLUMNS (1<<VEC1_META_COLUMN_BITS)
/* …（同文件无关代码省略）… */
typedef struct Vec1MetaValue Vec1MetaValue;
/* …（同文件无关代码省略）… */
typedef struct Vec1Filter Vec1Filter;
/* …（同文件无关代码省略）… */
#define VEC1_OP_EQ      'A'
#define VEC1_OP_LT      'B'
#define VEC1_OP_GT      'C'
#define VEC1_OP_LE      'D'
#define VEC1_OP_GE      'E'
#define VEC1_OP_IS      'F'
/* …（同文件无关代码省略）… */
#define VEC1_OP_NOTNULL 'H'
#define VEC1_OP_IN      'I'
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
static int vec1EncodedVectorSize(const Vec1Model *pMod){
  int ret = (int)pMod->hdr.nCodebook;
  if( pMod->hdr.flags & VEC1_MODEL_BITQUANT ) ret += sizeof_f32;
  return ret;
}
/* …（同文件无关代码省略）… */
static int vec1CheckIdxSize(Vec1Tab *p, const u8 *aBlob, i64 nBlob){
  int bRet = 1;

  /* If the blob is smaller than 8 bytes, it must be corrupt */
  if( nBlob>=VEC1_LIST_SZHDR ){

    /* Read the flags and number-of-entries fields */
    u32 flags = vec1GetU32(&aBlob[0]);
    i64 nEntry = (int)vec1GetU32(&aBlob[4]);
    const i64 szRowid = ((flags & VEC1_LIST_64BIT) ? 8 : 4);

    if( p->mod.hdr.nCodebook>0 ){
      const i64 nBlk = ((nEntry+VEC1_PQ_BLOCKSIZE-1) / VEC1_PQ_BLOCKSIZE);
      const i64 szBlk = VEC1_PQ_BLOCKSIZE * vec1EncodedVectorSize(&p->mod);
      bRet = (nBlob!=(VEC1_LIST_SZHDR + nEntry*szRowid + nBlk*szBlk));
    }else{
      const i64 szVec = p->cfg.nElem*sizeof_f32;
      bRet = (nBlob!=(VEC1_LIST_SZHDR + nEntry*szRowid + nEntry*szVec));
    }
  }
  return bRet;
}
/* …（同文件无关代码省略）… */
}

static int vec1GetVarint(const u8 *aBuf, u64 *piVal){
  int nRet = 0;
  u64 out = 0;
  do {
    out = (out<<7) + (aBuf[nRet] & 0x7F);
  } while( aBuf[nRet++] & 0x80 );
  *piVal = out;
  return nRet;
}

/*
/* …（同文件无关代码省略）… */
*/
static int vec1ReadMeta(
  Vec1Tab *pTab,                  /* Vec1 virtual table */
  Vec1Buffer *pBuf,               /* Buffer to read into */
  i64 iId,                        /* Id of %_idx row */
  int iMeta                       /* Index of meta-column */
){
  sqlite3_stmt *pStmt = 0;
  int rc = vec1GetSql(pTab, VEC1_SQL_READ_META, &pStmt);

  assert( iMeta>=0 && iMeta<VEC1_MAX_META_COLUMNS );

  if( rc==SQLITE_OK ){
    i64 iMetaId = (iId << VEC1_META_COLUMN_BITS) + iMeta;
    pBuf->n = 0;
    sqlite3_bind_int64(pStmt, 1, iMetaId);
    if( SQLITE_ROW==sqlite3_step(pStmt) ){
      int nByte = sqlite3_column_bytes(pStmt, 0);
      rc = vec1BufferGrow(pBuf, nByte);
      if( rc==SQLITE_OK ){
        const u8 *a = sqlite3_column_blob(pStmt, 0);
        memcpy(pBuf->a, a, nByte);
        pBuf->n = nByte;
      }
    }

    vec1StmtReset(&rc, pStmt);
    if( rc==SQLITE_OK && pBuf->n==0 ){
      rc = VEC1_CORRUPT;
    }
  }

  return rc;
}
/* …（同文件无关代码省略）… */
**        4: Real value. 8 byte payload.
*/
static int vec1MetaValueRead(
  Vec1Buffer *pMeta, 
  int *piOff, 
  Vec1MetaValue *pVal
){
  int iOff = *piOff;
  switch( pMeta->a[iOff] ){
    case 0: {
      pVal->eType = SQLITE_NULL;
/* …（同文件无关代码省略）… */
      u64 eType = 0;
      iOff += vec1GetVarint(&pMeta->a[iOff], &eType);
      pVal->pPtr = &pMeta->a[iOff];
      pVal->iVal = (eType-5) / 2;
      iOff += (int)pVal->iVal;
      pVal->eType = (eType & 0x01) ? SQLITE_TEXT : SQLITE_BLOB;
      break;
    }
  }

  *piOff = iOff;
  return SQLITE_OK;
}

/* …（同文件无关代码省略）… */
#define COMPARE(a, b) ((a)==(b) ? 0 : ((a)>(b) ? +1 : -1))
/* …（同文件无关代码省略）… */
static int vec1MetaValueFilter(Vec1Filter *pFilter, Vec1MetaValue *pMeta){
  int cmp = 0;

  if( pFilter->vop==VEC1_OP_IN ){
    int ii;
    for(ii=0; ii<pFilter->iVal; ii++){
      if( vec1MetaValueFilter(&pFilter[ii+1], pMeta)==0 ) return 0;
    }
    return 1;
  }

  if( pMeta->eType==SQLITE_INTEGER ){
    switch( pFilter->eType ){
      case SQLITE_NULL:  return 1;

      case SQLITE_INTEGER: {
        cmp = COMPARE(pFilter->iVal, pMeta->iVal);
        break;
      }
      case SQLITE_FLOAT: {
        cmp = COMPARE(pFilter->fVal, (double)pMeta->iVal);
        break;
      }
      default: {
        /* All text and blob are bigger than all numbers */
        cmp = +1;
        break;
      }
    }
  }
  else if( pMeta->eType==SQLITE_FLOAT ){
    switch( pFilter->eType ){
      case SQLITE_NULL:  return 1;

      case SQLITE_INTEGER: {
        cmp = COMPARE((double)pFilter->iVal, pMeta->fVal);
        break;
      }
      case SQLITE_FLOAT: {
        cmp = COMPARE(pFilter->fVal, pMeta->fVal);
        break;
      }
      default: {
        /* All text and blob are bigger than all numbers */
        cmp = +1;
        break;
      }
    }
  }else if( pMeta->eType==SQLITE_NULL ){
    if( pFilter->vop==VEC1_OP_IS && pFilter->eType==SQLITE_NULL ) return 0;
    return 1;
  }else if( pMeta->eType==SQLITE_TEXT ){
    switch( pFilter->eType ){
      case SQLITE_NULL: return 1;

      case SQLITE_INTEGER:
      case SQLITE_FLOAT: {
        cmp = -1;
        break;
      }

      case SQLITE_TEXT: {
        i64 n = MIN(pMeta->iVal, pFilter->iVal);
        if( n>0 ){
          cmp = memcmp(pFilter->pPtr, pMeta->pPtr, n);
        }
        if( cmp==0 ) cmp = COMPARE(pFilter->iVal, pMeta->iVal);
        break;
      }

      default: {
        assert( pFilter->eType==SQLITE_BLOB );
        cmp = 1;
        break;
      }
    }
  }else{
    assert( pMeta->eType==SQLITE_BLOB );
    switch( pFilter->eType ){
      case SQLITE_NULL: return 1;

      case SQLITE_INTEGER:
      case SQLITE_FLOAT: 
      case SQLITE_TEXT: {
        cmp = -1;
        break;
      }

      default: {
        i64 n = MIN(pMeta->iVal, pFilter->iVal);
        if( n>0 ) cmp = memcmp(pFilter->pPtr, pMeta->pPtr, n);
        if( cmp==0 ) cmp = COMPARE(pFilter->iVal, pMeta->iVal);
        assert( pFilter->eType==SQLITE_BLOB );
        break;
      }
    }
  }

  switch( pFilter->vop ){
    case VEC1_OP_IS:
    case VEC1_OP_EQ: return !(cmp==0);
    case VEC1_OP_LT: return !(cmp>0);
    case VEC1_OP_LE: return !(cmp>=0);
    case VEC1_OP_GT: return !(cmp<0);
    case VEC1_OP_GE: return !(cmp<=0);
  }

  assert( pFilter->vop==VEC1_OP_NOTNULL );
  return 0;
}
/* …（同文件无关代码省略）… */
  for(ii=0; rc==SQLITE_OK && ii<pQuery->nFilter; ii++){
    Vec1Filter *pFilter = &pQuery->aFilter[ii];
    VEC1_QINSTR_START(pTab, VEC1_QINSTR_METAREAD);
    rc = vec1ReadMeta(pTab, pMeta, iId, pFilter->iMeta);
    VEC1_QINSTR_STOP(pTab, VEC1_QINSTR_METAREAD);

    VEC1_QINSTR_START(pTab, VEC1_QINSTR_METASCAN);
/* …（同文件无关代码省略）… */
        int iOff = VEC1_META_SZHDR;
        for(jj=0; jj<nEntry; jj++){
          Vec1MetaValue val;
          vec1MetaValueRead(pMeta, &iOff, &val);
          if( vec1MetaValueFilter(pFilter, &val) ){
            pBitmask->a[jj / 8] |= (1 << (jj % 8));
          }
/* …（同文件无关代码省略）… */
    if( rc!=SQLITE_OK ) return rc;

    while( iIn<p->buf.n ){
      vec1MetaValueRead(&p->buf, &iIn, &val);
      if( val.eType==SQLITE_NULL ){
        val.iVal = iNull;
      }else if( p->format==VEC1_META_REAL ){
/* …（同文件无关代码省略）… */
** A serialized meta-value is stored in buffer aBuf. Return the size in
** bytes of the meta value.
*/
static int vec1MetaValueSize(const u8 *aBuf){
  switch( aBuf[0] ){
    case 0: return 1;               /* NULL */
    case 1: return 2;               /* 1-byte integer */
/* …（同文件无关代码省略）… */
      */
      i64 V = 0;
      int n = vec1GetVarint(aBuf, (u64*)&V);
      return (int)(((V-5)/2) + n);
    }
  }
}

#ifndef NDEBUG
/*
** This may be used in assert() statements to check that the meta-value
** list contains the expected number of elements.
*/
static int vec1MetaValueCheck(Vec1Buffer *pBuf, int nEntry){
  int iOff = 0;
  int ii;
  for(ii=0; ii<nEntry; ii++){
    iOff += vec1MetaValueSize(&pBuf->a[iOff]);
  }
  assert( iOff==pBuf->n );
  return 1;
}
#endif

static int vec1ListBuilderLoad(
  Vec1ListBuilder *p,
  i64 iId,
  i64 iFirst,
  i64 iLast,
  const u8 *aVal, int nVal
){
  Vec1Tab *pTab = p->pTab;
  int nEntry = 0;
  int flags = 0;
  int rc = SQLITE_OK;
  int nRowid = 0;
  int nData = 0;
  int ii;                         /* Iterator variable */

  assert( p->bufRowid.n==0 );
  assert( p->szRowid==4 );

  if( vec1CheckIdxSize(pTab, aVal, nVal) ){
    return VEC1_CORRUPT;
  }
  flags = vec1GetU32(&aVal[0]);
  nEntry = vec1GetU32(&aVal[4]);
  p->nTombstone = vec1GetU32(&aVal[8]);
  if( flags & VEC1_LIST_64BIT ) p->szRowid = 8;
  
  /* Set Vec1ListBuilder.eBlocked */
  if( pTab->mod.hdr.flags & VEC1_MODEL_BITQUANT ){
    p->eBlocked = VEC1_BLOCKED_BIN;
  }else if( pTab->mod.hdr.nCodebook>0 ){
    p->eBlocked = VEC1_BLOCKED_PQ;
  }else{
    p->eBlocked = VEC1_BLOCKED_NONE;
  }

  p->bSorted = ((flags & VEC1_LIST_SORTED)!=0);
  p->iFirst = iFirst;
  p->iLast = iLast;
  p->iId = iId;

  nRowid = nEntry * p->szRowid;
  nData = nVal - VEC1_LIST_SZHDR - nRowid;
  rc = vec1BufferGrow(&p->bufRowid, nRowid);
  if( rc==SQLITE_OK ){
    memcpy(p->bufRowid.a, &aVal[VEC1_LIST_SZHDR], nRowid);
    p->bufRowid.n = nRowid;
    rc = vec1BufferGrow(&p->bufData, nData);
  }
  if( rc==SQLITE_OK ){
    memcpy(p->bufData.a, &aVal[VEC1_LIST_SZHDR+nRowid], nData);
    p->bufData.n = nData;
  }

  /* Load the array for each meta-data column from the %_meta table */
  for(ii=0; rc==SQLITE_OK && ii<pTab->nMeta; ii++){
    Vec1MetaBuilder *pMeta = &p->aMeta[ii];
    rc = vec1ReadMeta(pTab, &pMeta->buf, iId, ii);
    if( rc==SQLITE_OK ){
      u32 f = vec1GetU32(pMeta->buf.a);
      pMeta->format = (f & VEC1_META_TYPEMASK);
      pMeta->flags = f;
    }
  }

  return rc;
}
/* …（同文件无关代码省略）… */
#define VEC1_LIST_BLOCKED_OFFSET(ii, nVectorSize) (              \
  ((ii / VEC1_PQ_BLOCKSIZE) * VEC1_PQ_BLOCKSIZE * nVectorSize) + \
  (ii % VEC1_PQ_BLOCKSIZE)                                       \
)
/* …（同文件无关代码省略）… */
#define VEC1_LIST_NORM_OFFSET(ii, nCodebook) (                                \
  ((ii / VEC1_PQ_BLOCKSIZE) * VEC1_PQ_BLOCKSIZE * (nCodebook+sizeof_f32)) + \
  (ii % VEC1_PQ_BLOCKSIZE) * sizeof_f32                          \
)
/* …（同文件无关代码省略）… */
#define VEC1_LIST_CODE_OFFSET(ii, nCodebook) (                                \
  ((ii / VEC1_PQ_BLOCKSIZE) * VEC1_PQ_BLOCKSIZE * (nCodebook+sizeof_f32)) + \
  (VEC1_PQ_BLOCKSIZE * sizeof_f32) +                                          \
  (ii % VEC1_PQ_BLOCKSIZE)                                                    \
)
/* …（同文件无关代码省略）… */
static int vec1ListBuilderCompress(
  Vec1ListBuilder *p,
  i64 iDel
){
  const int nMeta = p->pTab->nMeta;
  int ii;
  int iOut = 0;
  int nEntry = p->bufRowid.n / p->szRowid;
  int *aMetaIn = 0;               /* Meta-data input offsets */
  int *aMetaOut = 0;              /* Meta-data output offsets */

  i64 tomb1 = p->szRowid==8 ? VEC1_TOMBSTONE_64 : VEC1_TOMBSTONE_32;
  i64 tomb2 = iDel;

  /* Allocate space for aMetaIn[] and aMetaOut[] if requried */
  if( nMeta>0 ){
    aMetaIn = vec1MallocZero(nMeta * 2 * sizeof(int));
    if( aMetaIn==0 ) return SQLITE_NOMEM;
    aMetaOut = &aMetaIn[nMeta];
    for(ii=0; ii<nMeta; ii++){
      aMetaIn[ii] = VEC1_META_SZHDR;
      aMetaOut[ii] = VEC1_META_SZHDR;
    }
  }

  p->iFirst = VEC1_LARGEST_INT64;
  p->iLast = VEC1_SMALLEST_INT64;

  for(ii=0; ii<nEntry; ii++){
    int iOff = p->szRowid*ii;
    i64 iRowid;
    int bTombstone = 0;
    int iMeta;                    /* Iterator used for meta-data columns */

    if( p->szRowid==8 ){
      iRowid = vec1GetU64(&p->bufRowid.a[iOff]);
    }else{
      iRowid = vec1GetU32(&p->bufRowid.a[iOff]);
    }
    if( iRowid==tomb1 || iRowid==tomb2 ) bTombstone = 1;

    if( bTombstone==0 ){
      p->iFirst = MIN(p->iFirst, iRowid);
      p->iLast = MAX(p->iLast, iRowid);

      if( ii!=iOut ){
        int iOff2 = p->szRowid*iOut;
        u8 *a = p->bufData.a; 
        int iFrom, iTo;

        memcpy(&p->bufRowid.a[iOff2], &p->bufRowid.a[iOff], p->szRowid);
        if( p->eBlocked==VEC1_BLOCKED_NONE ){
          int i1 = p->nVectorSize * iOut;
          int i2 = p->nVectorSize * ii;
          memcpy(&p->bufData.a[i1], &p->bufData.a[i2], p->nVectorSize);
        }else{
          int nCodebook;          /* Size of quantized vector without norm */
          int jj;

          if( p->eBlocked==VEC1_BLOCKED_BIN ){
            nCodebook = p->nVectorSize - sizeof_f32;
            memcpy(&a[ VEC1_LIST_NORM_OFFSET(iOut, nCodebook) ],
                   &a[ VEC1_LIST_NORM_OFFSET(ii, nCodebook) ],
                   sizeof_f32
            );
            iFrom = VEC1_LIST_CODE_OFFSET(ii, nCodebook);
            iTo = VEC1_LIST_CODE_OFFSET(iOut, nCodebook);
          }else{
            assert( p->eBlocked==VEC1_BLOCKED_PQ );
            nCodebook = p->nVectorSize;
            iFrom = VEC1_LIST_BLOCKED_OFFSET(ii, nCodebook);
            iTo = VEC1_LIST_BLOCKED_OFFSET(iOut, nCodebook);
          }

          for(jj=0; jj<nCodebook; jj++){
            u8 v = p->bufData.a[iFrom + jj*VEC1_PQ_BLOCKSIZE];
            p->bufData.a[iTo + jj*VEC1_PQ_BLOCKSIZE] = v;
          }
        }
      }
      iOut++;
    }

    for(iMeta=0; iMeta<nMeta; iMeta++){
      Vec1MetaBuilder *pMeta = &p->aMeta[iMeta];
      u8 *a = pMeta->buf.a;


      if( pMeta->format==VEC1_META_GENERIC ){
        int nIn = vec1MetaValueSize(&a[aMetaIn[iMeta]]);
        if( bTombstone==0 ){
          if( aMetaIn[iMeta]!=aMetaOut[iMeta] ){
            memmove(&a[ aMetaOut[iMeta] ], &a[ aMetaIn[iMeta] ], nIn);
          }
          aMetaOut[iMeta] += nIn;
        }
        aMetaIn[iMeta] += nIn;
      }else{
        int szElem = 
          (pMeta->format==VEC1_META_1BYTEINT) ? 1 :
          (pMeta->format==VEC1_META_4BYTEINT) ? sizeof_u32 : sizeof_f64
        ;
        if( bTombstone==0 ){
          memmove(&a[ aMetaOut[iMeta] ], &a[ aMetaIn[iMeta] ], szElem);
          aMetaOut[iMeta] += szElem;
        }
        aMetaIn[iMeta] += szElem;
      }
    }
  }

  p->nTombstone = 0;

  p->bufRowid.n = iOut*p->szRowid;
  if( p->eBlocked==VEC1_BLOCKED_NONE ){
    p->bufData.n = p->nVectorSize * iOut;
  }else{
    p->bufData.n = p->nVectorSize * VEC1_PQ_BLOCKSIZE * (
        ((iOut+VEC1_PQ_BLOCKSIZE-1)/VEC1_PQ_BLOCKSIZE)
    );
  }
  for(ii=0; ii<nMeta; ii++){
    p->aMeta[ii].buf.n = aMetaOut[ii];
  }

  sqlite3_free(aMetaIn);
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static void vec1MetaValueUpdateFlags(const u8 *aMeta, u32 *pFlags){
  switch( aMeta[0] ){
    case 0:             /* NULL value */
      *pFlags |= VEC1_META_HASNULL;
      break;

    case 1:             /* 1-byte integer */
      break;

    case 2:             /* 4-byte integer */
      *pFlags &= ~(VEC1_META_1BYTEINT);
      break;

    case 4:             /* 8-byte real */
      *pFlags &= ~(VEC1_META_1BYTEINT|VEC1_META_4BYTEINT);
      break;

    default:
      *pFlags &= ~(VEC1_META_1BYTEINT|VEC1_META_4BYTEINT|VEC1_META_REAL);
      break;
  }
}
/* …（同文件无关代码省略）… */
/*
** Read nMeta values from buffer pBuf. Use them as the meta values associated
** with the (compressed) vector just appended to bucket iBucket.
*/
static int vec1WriterMetaFromPacked(
  Vec1Writer *pWriter,            /* Writer object accumulating changes */
  int iBucket,                    /* Bucket to write to */
  Vec1Buffer *pBuf,
  int *piOff                      /* IN/OUT: Buffer offset */
){
  int ii;
  int iOff = *piOff;
  for(ii=0; ii<pWriter->pTab->nMeta; ii++){
    Vec1MetaBuilder *pTo = &pWriter->aBld[iBucket].aMeta[ii];
    int n = vec1MetaValueSize(&pBuf->a[iOff]);
    int rc = vec1BufferGrow(&pTo->buf, n);
    vec1MetaValueUpdateFlags(&pBuf->a[iOff], &pTo->flags);
    if( rc!=SQLITE_OK ) return rc;
    memcpy(&pTo->buf.a[pTo->buf.n], &pBuf->a[iOff], n);
    pTo->buf.n += n;
    iOff += n;
  }
  *piOff = iOff;
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
          ** the threshold. So reorganize the list and remove iRowid in
          ** one go.  */
          if( loc.pList ){
            vec1ListBuilderCompress(loc.pList, iRowid);
          }else{
            /* Load the list into memory, compress it so there are no
            ** tombstones, then write it back to the database.  */
/* …（同文件无关代码省略）… */
    vec1MetaValueRead(pBuf, &iOff, &val);
  }

  if( pVal ){
    int eType = sqlite3_value_type(pVal);
    int bMatch = 0;
    switch( eType ){
/* …（同文件无关代码省略）… */
  return 0;
}

/*
** Integrity check method. If the table has been supplied with a model,
** loop through the %_idx table. For each 
/* …（同文件无关代码省略）… */

      /* Load the array for each meta-value column from disk */
      for(iMeta=0; iMeta<pTab->nMeta; iMeta++){
        rc = vec1ReadMeta(pTab, &aBufMeta[iMeta], iId, iMeta);
        if( rc!=SQLITE_OK ){
          if( rc==SQLITE_CORRUPT_VTAB ){
            const char *zFmt = "%s: error reading meta-list id=%lld,meta=%d";
