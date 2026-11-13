// AUTO-DRAFT from sqlite/sqlite PR #cb6a35b9c2b10b8663958e612acb8a82b04829fd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
typedef struct Incrblob Incrblob;
/* …（同文件无关代码省略）… */
static int blobSeekToRow(Incrblob *p, sqlite3_int64 iRow, char **pzErr){
  int rc;                         /* Error code */
  char *zErr = 0;                 /* Error message */
  Vdbe *v = (Vdbe *)p->pStmt;

  /* Set the value of register r[1] in the SQL statement to integer iRow. 
  ** This is done directly as a performance optimization
  */
  sqlite3VdbeMemSetInt64(&v->aMem[1], iRow);

  /* If the statement has been run before (and is paused at the OP_ResultRow)
  ** then back it up to the point where it does the OP_NotExists.  This could
  ** have been down with an extra OP_Goto, but simply setting the program
  ** counter is faster. */
  if( v->pc>4 ){
    v->pc = 4;
    assert( v->aOp[v->pc].opcode==OP_NotExists );
    rc = sqlite3VdbeExec(v);
  }else{
    rc = sqlite3_step(p->pStmt);
  }
  if( rc==SQLITE_ROW ){
    VdbeCursor *pC = v->apCsr[0];
    u32 type;
    assert( pC!=0 );
    assert( pC->eCurType==CURTYPE_BTREE );
    type = pC->nHdrParsed>p->iCol ? pC->aType[p->iCol] : 0;
    testcase( pC->nHdrParsed==p->iCol );
    testcase( pC->nHdrParsed==p->iCol+1 );
    if( type<12 ){
      zErr = sqlite3MPrintf(p->db, "cannot open value of type %s",
          type==0?"null": type==7?"real": "integer"
      );
      rc = SQLITE_ERROR;
      sqlite3_finalize(p->pStmt);
      p->pStmt = 0;
    }else{
      p->iOffset = pC->aType[p->iCol + pC->nField];
      p->nByte = sqlite3VdbeSerialTypeLen(type);
      p->pCsr =  pC->uc.pCursor;
      sqlite3BtreeIncrblobCursor(p->pCsr);
    }
  }

  if( rc==SQLITE_ROW ){
    rc = SQLITE_OK;
  }else if( p->pStmt ){
    rc = sqlite3_finalize(p->pStmt);
    p->pStmt = 0;
    if( rc==SQLITE_OK ){
      zErr = sqlite3MPrintf(p->db, "no such rowid: %lld", iRow);
      rc = SQLITE_ERROR;
    }else{
      zErr = sqlite3MPrintf(p->db, "%s", sqlite3_errmsg(p->db));
    }
  }

  assert( rc!=SQLITE_OK || zErr==0 );
  assert( rc!=SQLITE_ROW && rc!=SQLITE_DONE );

  *pzErr = zErr;
  return rc;
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE int blobReadWriteRetry(
  Incrblob *p,
  void *z, 
  int n, 
  int iOffset, 
  int (*xCall)(BtCursor*, u32, u32, void*)
){
  int rc;
  sqlite3_int64 iRow;
  char *zErr = 0;
  sqlite3 *db = p->db;

  sqlite3BtreeEnterCursor(p->pCsr);
  iRow = sqlite3BtreeIntegerKey(p->pCsr);
  sqlite3BtreeLeaveCursor(p->pCsr);
  ((Vdbe*)p->pStmt)->rc = SQLITE_OK;
  if( NEVER(p->bRetry) || sqlite3FaultSim(210) ){
    /* It should not be possible for blobReadWrite() and blobReadWriteRetry()
    ** to get into a loop, but we check anyway, for safety. */
    rc = SQLITE_ABORT;
    sqlite3VdbeFinalize((Vdbe*)p->pStmt);
    p->pStmt = 0;
  }else{
    rc = blobSeekToRow(p, iRow, &zErr);
  }
  sqlite3DbFree(db, zErr);
  assert( rc==SQLITE_OK || p->pStmt==0 );
  if( rc==SQLITE_OK ){
    p->bRetry = 1;
    rc = blobReadWrite((sqlite3_blob*)p,z,n,iOffset,xCall);
    p->bRetry = 0;
  }else{
    rc = sqlite3ApiExit(db, rc);
  }
  sqlite3_mutex_leave(db->mutex);
  return rc;
}
/* …（同文件无关代码省略）… */
static int blobReadWrite(
  sqlite3_blob *pBlob, 
  void *z, 
  int n, 
  int iOffset, 
  int (*xCall)(BtCursor*, u32, u32, void*)
){
  int rc = SQLITE_OK;
  Incrblob *p = (Incrblob *)pBlob;
  Vdbe *v;
  sqlite3 *db;

  if( p==0 ) return SQLITE_MISUSE_BKPT;
  db = p->db;
  sqlite3_mutex_enter(db->mutex);
  v = (Vdbe*)p->pStmt;

  if( n<0 || iOffset<0 || ((sqlite3_int64)iOffset+n)>p->nByte ){
    /* Request is out of range. Return a transient error. */
    rc = SQLITE_ERROR;
  }else if( v==0 ){
    /* If there is no statement handle, then the blob-handle has
    ** already been invalidated. Return SQLITE_ABORT in this case.
    */
    rc = SQLITE_ABORT;
  }else{
    /* Call either BtreeData() or BtreePutData(). If SQLITE_ABORT is
    ** returned, clean-up the statement handle.
    */
    assert( db == v->db );
    sqlite3BtreeEnterCursor(p->pCsr);

#ifdef SQLITE_ENABLE_PREUPDATE_HOOK
    if( xCall==sqlite3BtreePutData && db->xPreUpdateCallback ){
      /* If a pre-update hook is registered and this is a write cursor, 
      ** invoke it here. 
      ** 
      ** TODO: The preupdate-hook is passed SQLITE_DELETE, even though this
      ** operation should really be an SQLITE_UPDATE. This is probably
      ** incorrect, but is convenient because at this point the new.* values 
      ** are not easily obtainable. And for the sessions module, an 
      ** SQLITE_UPDATE where the PK columns do not change is handled in the 
      ** same way as an SQLITE_DELETE (the SQLITE_DELETE code is actually
      ** slightly more efficient). Since you cannot write to a PK column
      ** using the incremental-blob API, this works. For the sessions module
      ** anyhow.
      */
      if( sqlite3BtreeCursorIsValidNN(p->pCsr)==0 ){
        /* If the cursor is not currently valid, try to reseek it. This 
        ** always either fails or finds the correct row - the cursor will
        ** have been marked permanently CURSOR_INVALID if the open row has
        ** been deleted.  */
        int bDiff = 0;
        rc = sqlite3BtreeCursorRestore(p->pCsr, &bDiff);
        assert( bDiff==0 || sqlite3BtreeCursorIsValidNN(p->pCsr)==0 );
      }
      if( sqlite3BtreeCursorIsValidNN(p->pCsr) ){
        sqlite3_int64 iKey;
        iKey = sqlite3BtreeIntegerKey(p->pCsr);
        assert( v->apCsr[0]!=0 );
        assert( v->apCsr[0]->eCurType==CURTYPE_BTREE );
        sqlite3VdbePreUpdateHook(
            v, v->apCsr[0], SQLITE_DELETE, p->zDb, p->pTab, iKey, -1, p->iCol
        );
      }
    }
    if( rc==SQLITE_OK ){
      rc = xCall(p->pCsr, iOffset+p->iOffset, n, z);
    }
#else
    rc = xCall(p->pCsr, iOffset+p->iOffset, n, z);
#endif

    sqlite3BtreeLeaveCursor(p->pCsr);
    if( rc==SQLITE_OK ){
      v->rc = SQLITE_OK;
    }else{
      if( rc==SQLITE_MOVED ){
        /* The cursor was restored and no data was read.  Refresh the
        ** handle and try the read again. */
        return bl
