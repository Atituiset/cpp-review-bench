// AUTO-DRAFT from sqlite/sqlite PR #9696acb0c77f1c1a7a400446686debc1312c94bc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
typedef struct SessionBuffer SessionBuffer;
typedef struct SessionInput SessionInput;
/* …（同文件无关代码省略）… */
#define SESSIONS_ROWID "_rowid_"
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
static void sessionAppendPrintf(
  SessionBuffer *p,               /* Buffer to append to */
  int *pRc, 
  const char *zFmt,
  ...
){
  if( *pRc==SQLITE_OK ){
    char *zApp = 0;
    va_list ap;
    va_start(ap, zFmt);
    zApp = sqlite3_vmprintf(zFmt, ap);
    if( zApp==0 ){
      *pRc = SQLITE_NOMEM;
    }else{
      sessionAppendStr(p, zApp, pRc);
    }
    va_end(ap);
    sqlite3_free(zApp);
  }
}
/* …（同文件无关代码省略）… */
static void sessionFinalizeStmt(sqlite3_stmt *pStmt, int *pRc){
  int rc = sqlite3_finalize(pStmt);
  if( *pRc==SQLITE_OK ) *pRc = rc;
}
/* …（同文件无关代码省略）… */
static void sessionAppendIdent(
  SessionBuffer *p,               /* Buffer to a append to */
  const char *zStr,               /* String to quote, escape and append */
  int *pRc                        /* IN/OUT: Error code */
){
  int nStr = sqlite3Strlen30(zStr)*2 + 2 + 2;
  if( 0==sessionBufferGrow(p, nStr, pRc) ){
    char *zOut = (char *)&p->aBuf[p->nBuf];
    const char *zIn = zStr;
    *zOut++ = '"';
    if( zIn!=0 ){
      while( *zIn ){
        if( *zIn=='"' ) *zOut++ = '"';
        *zOut++ = *(zIn++);
      }
    }
    *zOut++ = '"';
    p->nBuf = (int)((u8 *)zOut - p->aBuf);
    p->aBuf[p->nBuf] = 0x00;
  }
}
/* …（同文件无关代码省略）… */
static int sessionPrepare(
  sqlite3 *db, 
  sqlite3_stmt **pp, 
  char **pzErrmsg,
  const char *zSql
){
  int rc = sqlite3_prepare_v2(db, zSql, -1, pp, 0);
  if( pzErrmsg && rc!=SQLITE_OK ){
    *pzErrmsg = sqlite3_mprintf("%s", sqlite3_errmsg(db));
  }
  return rc;
}

/*
** Formulate and prepare a SELECT statement to retrieve a row from table
** zTab in database zDb based on its primary key. i.e.
/* …（同文件无关代码省略）… */
int sqlite3changeset_old(
  sqlite3_changeset_iter *pIter,  /* Changeset iterator */
  int iVal,                       /* Index of old.* value to retrieve */
  sqlite3_value **ppValue         /* OUT: Old value (or NULL pointer) */
){
  if( pIter->op!=SQLITE_UPDATE && pIter->op!=SQLITE_DELETE ){
    return SQLITE_MISUSE;
  }
  if( iVal<0 || iVal>=pIter->nCol ){
    return SQLITE_RANGE;
  }
  *ppValue = pIter->apValue[iVal];
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
int sqlite3changeset_new(
  sqlite3_changeset_iter *pIter,  /* Changeset iterator */
  int iVal,                       /* Index of new.* value to retrieve */
  sqlite3_value **ppValue         /* OUT: New value (or NULL pointer) */
){
  if( pIter->op!=SQLITE_UPDATE && pIter->op!=SQLITE_INSERT ){
    return SQLITE_MISUSE;
  }
  if( iVal<0 || iVal>=pIter->nCol ){
    return SQLITE_RANGE;
  }
  *ppValue = pIter->apValue[pIter->nCol+iVal];
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
int sqlite3changeset_finalize(sqlite3_changeset_iter *p){
  int rc = SQLITE_OK;
  if( p ){
    int i;                        /* Used to iterate through p->apValue[] */
    rc = p->rc;
    if( p->apValue ){
      for(i=0; i<p->nCol*2; i++) sqlite3ValueFree(p->apValue[i]);
    }
    sqlite3_free(p->tblhdr.aBuf);
    sqlite3_free(p->in.buf.aBuf);
    sqlite3_free(p);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
typedef struct SessionApplyCtx SessionApplyCtx;
/* …（同文件无关代码省略）… */
static int sessionBindValue(
  sqlite3_stmt *pStmt,            /* Statement to bind value to */
  int i,                          /* Parameter number to bind to */
  sqlite3_value *pVal             /* Value to bind */
){
  int eType = sqlite3_value_type(pVal);
  /* COVERAGE: The (pVal->z==0) branch is never true using current versions
  ** of SQLite. If a malloc fails in an sqlite3_value_xxx() function, either
  ** the (pVal->z) variable remains as it was or the type of the value is
  ** set to SQLITE_NULL.  */
  if( (eType==SQLITE_TEXT || eType==SQLITE_BLOB) && pVal->z==0 ){
    /* This condition occurs when an earlier OOM in a call to
    ** sqlite3_value_text() or sqlite3_value_blob() (perhaps from within
    ** a conflict-handler) has zeroed the pVal->z pointer. Return NOMEM. */
    return SQLITE_NOMEM;
  }
  return sqlite3_bind_value(pStmt, i, pVal);
}
/* …（同文件无关代码省略）… */
static int sessionBindRow(
  sqlite3_changeset_iter *pIter,  /* Iterator to read values from */
  int(*xValue)(sqlite3_changeset_iter *, int, sqlite3_value **),
  int nCol,                       /* Number of columns */
  u8 *abPK,                       /* If not NULL, bind only if true */
  sqlite3_stmt *pStmt             /* Bind values to this statement */
){
  int i;
  int rc = SQLITE_OK;

  /* Neither sqlite3changeset_old or sqlite3changeset_new can fail if the
  ** argument iterator points to a suitable entry. Make sure that xValue 
  ** is one of these to guarantee that it is safe to ignore the return 
  ** in the code below. */
  assert( xValue==sqlite3changeset_old || xValue==sqlite3changeset_new );

  for(i=0; rc==SQLITE_OK && i<nCol; i++){
    if( !abPK || abPK[i] ){
      sqlite3_value *pVal = 0;
      (void)xValue(pIter, i, &pVal);
      if( pVal==0 ){
        /* The value in the changeset was "undefined". This indicates a
        ** corrupt changeset blob.  */
        rc = SQLITE_CORRUPT_BKPT;
      }else{
        rc = sessionBindValue(pStmt, i+1, pVal);
      }
    }
  }
  return rc;
}
/* …（同文件无关代码省略）… */
}

/*
** Check if table zTab in the "main" database of db is a WITHOUT ROWID
** table. 
**
** If no error occurs, return SQLITE_OK and set output variable (*pbWR) to 
** true if zTab is a WITHOUT ROWID table, or false otherwise. Or, if an
** error does occur, return an SQLite error code. The final value of (*pbWR)
** is undefined in this case.
*/
static int sessionTableIsWithoutRowid(sqlite3 *db, const char *zTab, int *pbWR){
  sqlite3_stmt *pList = 0;
  char *zSql = 0;
  int rc = SQLITE_OK;

  zSql = sqlite3_mprintf("PRAGMA table_list = %Q", zTab);
  if( zSql==0 ){
    rc = SQLITE_NOMEM;
  }else{
    rc = sqlite3_prepare_v2(db, zSql, -1, &pList, 0);
    sqlite3_free(zSql);
  }

  if( rc==SQLITE_OK ){
    sqlite3_step(pList);
    *pbWR = sqlite3_column_int(pList, 4);
    rc = sqlite3_finalize(pList);
  }

  return rc;
}

/*
** Iterator pUp points to an UPDATE change. This function deletes the 
** affected row from the database and creates an INSERT statement that
** may be used to reinsert the row as it is after the UPDATE change
** has been applied.
**
** If successful, SQLITE_OK is returned and output variable (*ppInsert)
** is left pointing to a prepared INSERT statement. It is the responsibility
** of the caller to eventually free this statement using sqlite3_finalize().
** Or, if an error occurs, an SQLite error code is returned and (*ppInsert)
** set to NULL. pApply->zErr may be set to an error message in this case.
*/
static int sessionUpdateToDeleteInsert(
  sqlite3 *db,                    /* Database to write to */
  const char *zTab,               /* Table name */
  SessionApplyCtx *pApply,        /* Apply context */
  sqlite3_changeset_iter *pUp,    /* Iterator pointing to UPDATE change */
  sqlite3_stmt **ppInsert         /* OUT: INSERT statement */
){
  sqlite3_stmt *pRet = 0;         /* The INSERT statement */
  sqlite3_stmt *pSelect = 0;      /* SELECT to read current values of row */
  int rc = SQLITE_OK;
  int bWR = 0;

  rc = sessionTableIsWithoutRowid(db, zTab, &bWR);
  if( rc==SQLITE_OK ){
    char *zSelect = 0;
    char *zInsert = 0;
    SessionBuffer cols = {0, 0, 0};
    SessionBuffer insbind = {0, 0, 0};
    SessionBuffer pkcols = {0, 0, 0};
    SessionBuffer selbind = {0, 0, 0};

    const char *zComma = "";
    const char *zComma2 = "";
    int ii;
    for(ii=0; ii<pApply->nCol; ii++){
      sessionAppendStr(&cols, zComma, &rc);
      sessionAppendIdent(&cols, pApply->azCol[ii], &rc);
      sessionAppendStr(&insbind, zComma, &rc);
      sessionAppendStr(&insbind, "?", &rc);
      zComma = ", ";

      if( pApply->abPK[ii] ){
        sessionAppendStr(&pkcols, zComma2, &rc);
        sessionAppendIdent(&pkcols, pApply->azCol[ii], &rc);
        sessionAppendStr(&selbind, zComma2, &rc);
        sessionAppendPrintf(&selbind, &rc, "?%d", ii+1);
        zComma2 = ", ";
      }
    }
    if( bWR==0 ){
      sessionAppendStr(&cols, zComma, &rc);
      sessionAppendStr(&cols, SESSIONS_ROWID, &rc);
      sessionAppendStr(&insbind, zComma, &rc);
      sessionAppendStr(&insbind, "?", &rc);
    }

    if( rc==SQLITE_OK ){
      zSelect = sqlite3_mprintf("SELECT %s FROM %Q WHERE (%s) IS (%s)",
          cols.aBuf, zTab, pkcols.aBuf, selbind.aBuf
      );
      if( zSelect==0 ) rc = SQLITE_NOMEM;
    }
    if( rc==SQLITE_OK ){
      zInsert = sqlite3_mprintf("INSERT INTO %Q(%s) VALUES(%s)",
          zTab, cols.aBuf, insbind.aBuf
      );
      if( zInsert==0 ) rc = SQLITE_NOMEM;
    }

    if( rc==SQLITE_OK ){
      rc = sessionPrepare(db, &pSelect, &pApply->zErr, zSelect);
    }
    if( rc==SQLITE_OK ){
      rc = sessionPrepare(db, &pRet, &pApply->zErr, zInsert);
    }

    sqlite3_free(zSelect);
    sqlite3_free(zInsert);
    sqlite3_free(cols.aBuf);
    sqlite3_free(insbind.aBuf);
    sqlite3_free(pkcols.aBuf);
    sqlite3_free(selbind.aBuf);
  }

  if( rc==SQLITE_OK ){
    rc = sessionBindRow(
        pUp, sqlite3changeset_old, pApply->nCol, pApply->abPK, pSelect
    );
  }

  if( rc==SQLITE_OK && sqlite3_step(pSelect)==SQLITE_ROW ){
    int iCol;
    for(iCol=0; iCol<pApply->nCol; iCol++){
      sqlite3_value *pVal = pUp->apValue[iCol+pApply->nCol];
      if( pVal==0 ){
        pVal = sqlite3_column_value(pSelect, iCol);
      }
      rc = sqlite3_bind_value(pRet, iCol+1, pVal);
    }
    if( bWR==0 ){
      sqlite3_bind_int64(pRet, iCol+1, sqlite3_column_int64(pSelect, iCol));
    }
  }
  sessionFinalizeStmt(pSelect, &rc);

  /* Delete the row from the database. */
  if( rc==SQLITE_OK ){
    rc = sessionBindRow(
        pUp, sqlite3changeset_old, pApply->nCol, pApply->abPK, pApply->pDelete
    );
    sqlite3_bind_int(pApply->pDelete, pApply->nCol+1, 1);
  }
  if( rc==SQLITE_OK ){
    sqlite3_step(pApply->pDelete);
    rc = sqlite3_reset(pApply->pDelete);
  }

  if( rc!=SQLITE_OK ){
    sqlite3_finalize(pRet);
    pRet = 0;
  }

  *ppInsert = pRet;
  return rc;
}

/* …（同文件无关代码省略）… */
**   2) For each UPDATE change in the buffer, try the following in a
**      savepoint transaction:
**
**      a) DELETE the affected row,
**      b) Attempt step (1) with remaining changes,
**      c) Attempt to INSERT a row equivalent to the one that would be
**         created by applying this UPDATE change.
**
**      If the INSERT in (c) succeeds, the savepoint is committed and all
**      successfully applied changes are removed from the buffer. Step (2)
**      is then repeated.
**
**   3) Once step (2) has been attempted for each UPDATE in the change,
**      a final attempt is made to apply each remaining change. This time,
/* …（同文件无关代码省略）… */
  while( rc==SQLITE_OK && pApply->constraints.nBuf && !pApply->bNoUpdateLoop ){
    SessionBuffer cons = {0, 0, 0};
    sqlite3_changeset_iter *pUp = 0;
    sqlite3_stmt *pInsert = 0;
    int iSkip = 0;

    rc = sessionRetryIterInit(
        &pApply->constraints, bPatchset, zTab, pApply, &pUp
/* …（同文件无关代码省略）… */
      if( iThis==iUpdate ){
        rc = sqlite3_exec(db, "SAVEPOINT update_op", 0, 0, 0);
        if( rc==SQLITE_OK ){
          rc = sessionUpdateToDeleteInsert(db, zTab, pApply, pUp, &pInsert);
        }
      }
      sqlite3changeset_finalize(pUp);
/* …（同文件无关代码省略）… */

    if( rc==SQLITE_OK ){
      cons = pApply->constraints;

      while( rc==SQLITE_OK && pApply->constraints.nBuf>0 ){
        SessionBuffer app = pApply->constraints;
/* …（同文件无关代码省略）… */

    iUpdate++;
    if( rc==SQLITE_OK ){
      sqlite3_step(pInsert);
      rc = sqlite3_finalize(pInsert);
      if( (rc&0xff)==SQLITE_CONSTRAINT ){
        rc = sqlite3_exec(db, "ROLLBACK TO update_op", 0, 0, 0);
        sqlite3_free(pApply->constraints.aBuf);
        pApply->constraints = cons;
/* …（同文件无关代码省略）… */
      if( rc==SQLITE_OK ){
        rc = sqlite3_exec(db, "RELEASE update_op", 0, 0, 0);
      }
    }else{
      sqlite3_finalize(pInsert);
    }

    sqlite3_free(cons.aBuf);
  }
