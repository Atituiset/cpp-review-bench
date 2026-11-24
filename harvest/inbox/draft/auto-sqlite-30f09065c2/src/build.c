// AUTO-DRAFT from sqlite/sqlite PR #95635a25d5a058dac504649debd970ba2f465801
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

  return 0;
}

/*
** Create a new index for an SQL table.  pName1.pName2 is the name of the index
** and pTblList is the name of the table that is to be indexed.  Both will
/* …（同文件无关代码省略）… */
      if( v==0 ) goto exit_create_index;

      sqlite3BeginWriteOperation(pParse, 1, iDb);

      /* Create the rootpage for the index using CreateIndex. But before
      ** doing so, code a Noop instruction and store its address in
/* …（同文件无关代码省略）… */
int sqlite3OpenTempDatabase(Parse *pParse){
  sqlite3 *db = pParse->db;
  if( db->aDb[1].pBt==0 && !pParse->explain ){
    int rc;
    Btree *pBt;
    static const int flags =
          SQLITE_OPEN_READWRITE |
          SQLITE_OPEN_CREATE |
          SQLITE_OPEN_EXCLUSIVE |
          SQLITE_OPEN_DELETEONCLOSE |
          SQLITE_OPEN_TEMP_DB;

    rc = sqlite3BtreeOpen(db->pVfs, 0, db, &pBt, 0, flags);
    if( rc!=SQLITE_OK ){
      /* See proof sketch on the 2026-11-16 check-in that adds the following */
      assert( rc==SQLITE_NOMEM );
      sqlite3OomFault(db);
      return 1;
    }
    db->aDb[1].pBt = pBt;
    assert( db->aDb[1].pSchema );
    if( SQLITE_NOMEM==sqlite3BtreeSetPageSize(pBt, db->nextPagesize, 0, 0) ){
      sqlite3OomFault(db);
      return 1;
    }
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static void sqlite3CodeVerifySchemaAtToplevel(Parse *pToplevel, int iDb){
  assert( iDb>=0 && iDb<pToplevel->db->nDb );
  assert( pToplevel->db->aDb[iDb].pBt!=0 || iDb==1 );
  assert( iDb<SQLITE_MAX_DB );
  assert( sqlite3SchemaMutexHeld(pToplevel->db, iDb, 0) );
  if( DbMaskTest(pToplevel->cookieMask, iDb)==0 ){
    DbMaskSet(pToplevel->cookieMask, iDb);
    if( !OMIT_TEMPDB && iDb==1 ){
      sqlite3OpenTempDatabase(pToplevel);
    }
  }
}
/* …（同文件无关代码省略）… */
void sqlite3BeginWriteOperation(Parse *pParse, int setStatement, int iDb){
  Parse *pToplevel = sqlite3ParseToplevel(pParse);
  sqlite3CodeVerifySchemaAtToplevel(pToplevel, iDb);
  DbMaskSet(pToplevel->writeMask, iDb);
  pToplevel->isMultiWrite |= setStatement;
}
