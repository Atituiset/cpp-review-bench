// AUTO-DRAFT from sqlite/sqlite PR #3e9779900906ef1fc36a5cdfe9969b7be30ecb14
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct MemFile MemFile;
typedef struct MemStore MemStore;
/* …（同文件无关代码省略）… */
static void memdbEnter(MemStore *p){
  UNUSED_PARAMETER(p);
}
static void memdbLeave(MemStore *p){
  UNUSED_PARAMETER(p);
}
/* …（同文件无关代码省略）… */
static MemFile *memdbFromDbSchema(sqlite3 *db, const char *zSchema){
  MemFile *p = 0;
  MemStore *pStore;
  int rc = sqlite3_file_control(db, zSchema, SQLITE_FCNTL_FILE_POINTER, &p);
  if( rc ) return 0;
  if( p->base.pMethods!=&memdb_io_methods ) return 0;
  pStore = p->pStore;
  memdbEnter(pStore);
  if( pStore->zFName!=0 ) p = 0;
  memdbLeave(pStore);
  return p;
}

/*
** Return the serialization of a database
*/
unsigned char *sqlite3_serialize(
  sqlite3 *db,              /* The database connection */
  const char *zSchema,      /* Which database within the connection */
  sqlite3_int64 *piSize,    /* Write size here, if not NULL */
  unsigned int mFlags       /* Maybe SQLITE_SERIALIZE_NOCOPY */
){
  MemFile *p;
  int iDb;
  Btree *pBt;
  sqlite3_int64 sz;
  int szPage = 0;
  sqlite3_stmt *pStmt = 0;
  unsigned char *pOut = 0;
  char *zSql;
  int rc;

#ifdef SQLITE_ENABLE_API_ARMOR
  if( !sqlite3SafetyCheckOk(db) ){
    (void)SQLITE_MISUSE_BKPT;
    return 0;
  }
#endif
  sqlite3_mutex_enter(db->mutex);

  if( zSchema==0 ) zSchema = db->aDb[0].zDbSName;
  p = memdbFromDbSchema(db, zSchema);
  iDb = sqlite3FindDbName(db, zSchema);
  if( piSize ) *piSize = -1;
  if( iDb<0 ) goto serialize_out;
  if( p ){
    MemStore *pStore = p->pStore;
    assert( pStore->pMutex==0 );
    if( piSize ) *piSize = pStore->sz;
    if( mFlags & SQLITE_SERIALIZE_NOCOPY ){
      pOut = pStore->aData;
    }else{
      pOut = sqlite3_malloc64( pStore->sz );
      if( pOut ) memcpy(pOut, pStore->aData, pStore->sz);
    }
    goto serialize_out;
  }
  pBt = db->aDb[iDb].pBt;
  if( pBt==0 ) goto serialize_out;
  szPage = sqlite3BtreeGetPageSize(pBt);
  zSql = sqlite3_mprintf("PRAGMA \"%w\".page_count", zSchema);
  rc = zSql ? sqlite3_prepare_v2(db, zSql, -1, &pStmt, 0) : SQLITE_NOMEM;
  sqlite3_free(zSql);
  if( rc ) goto serialize_out;
  rc = sqlite3_step(pStmt);
  if( rc==SQLITE_ROW ){
    sz = sqlite3_column_int64(pStmt, 0)*szPage;
    if( sz==0 ){
      sqlite3_reset(pStmt);
      sqlite3_exec(db, "BEGIN IMMEDIATE; COMMIT;", 0, 0, 0);
      rc = sqlite3_step(pStmt);
      if( rc==SQLITE_ROW ){
        sz = sqlite3_column_int64(pStmt, 0)*szPage;
      }
    }
    if( piSize ) *piSize = sz;
    if( mFlags & SQLITE_SERIALIZE_NOCOPY ){
      pOut = 0;
    }else{
      pOut = sqlite3_malloc64( sz );
      if( pOut ){
        int nPage = sqlite3_column_int(pStmt, 0);
        Pager *pPager = sqlite3BtreePager(pBt);
        int pgno;
        for(pgno=1; pgno<=nPage; pgno++){
          DbPage *pPage = 0;
          unsigned char *pTo = pOut + szPage*(sqlite3_int64)(pgno-1);
          rc = sqlite3PagerGet(pPager, pgno, (DbPage**)&pPage, 0);
          if( rc==SQLITE_OK ){
            memcpy(pTo, sqlite3PagerGetData(pPage), szPage);
          }else{
            memset(pTo, 0, szPage);
          }
          sqlite3PagerUnref(pPage);       
        }
      }
    }
  }
  sqlite3_finalize(pStmt);

 serialize_out:
  sqlite3_mutex_leave(db->mutex);
  return pOut;
}
