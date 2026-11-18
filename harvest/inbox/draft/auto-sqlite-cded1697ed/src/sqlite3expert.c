// AUTO-DRAFT from sqlite/sqlite PR #90ba7148c6d943ec5233002927c18e57cc8a5589
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct IdxScan IdxScan;
typedef struct IdxStatement IdxStatement;
typedef struct IdxTable IdxTable;
typedef struct IdxWrite IdxWrite;
/* …（同文件无关代码省略）… */
#define STRLEN  (int)strlen
/* …（同文件无关代码省略）… */
typedef struct IdxHash IdxHash;
/* …（同文件无关代码省略）… */
struct sqlite3expert {
  int iSample;                    /* Percentage of tables to sample for stat1 */
  sqlite3 *db;                    /* User database */
  sqlite3 *dbm;                   /* In-memory db for this analysis */
  sqlite3 *dbv;                   /* Vtab schema for this analysis */
  IdxTable *pTable;               /* List of all IdxTable objects */
  IdxScan *pScan;                 /* List of scan objects */
  IdxWrite *pWrite;               /* List of write objects */
  IdxStatement *pStatement;       /* List of IdxStatement objects */
  int bRun;                       /* True once analysis has run */
  char **pzErrmsg;
  int rc;                         /* Error code from whereinfo hook */
  IdxHash hIdx;                   /* Hash containing all candidate indexes */
  char *zCandidates;              /* For EXPERT_REPORT_CANDIDATES */
};
/* …（同文件无关代码省略）… */
static void idxFinalize(int *pRc, sqlite3_stmt *pStmt){
  int rc = sqlite3_finalize(pStmt);
  if( *pRc==SQLITE_OK ) *pRc = rc;
}
/* …（同文件无关代码省略）… */
  return rc;
}

static int idxProcessOneTrigger(
  sqlite3expert *p, 
  IdxWrite *pWrite, 
/* …（同文件无关代码省略）… */
  while( rc==SQLITE_OK && SQLITE_ROW==sqlite3_step(pSelect) ){
    const char *zCreate = (const char*)sqlite3_column_text(pSelect, 0);
    if( zCreate==0 ) continue;
    rc = sqlite3_exec(p->dbv, zCreate, 0, 0, pzErr);
  }
  idxFinalize(&rc, pSelect);

/* …（同文件无关代码省略）… */
  int rc = SQLITE_OK;
  char *zErr = 0;

  rc = sqlite3_exec(db, zSql, 0, 0, &zErr);
  if( rc!=SQLITE_OK && zErr ){
    int nErr = STRLEN(zErr);
    if( nErr>=15 && memcmp(zErr, "no such module:", 15)==0 ){
/* …（同文件无关代码省略）… */
** Define and possibly pretend to use a useless collation sequence.
** This pretense allows expert to accept SQL using custom collations.
*/
int dummyCompare(void *up1, int up2, const void *up3, int up4, const void *up5){
  (void)up1;
  (void)up2;
  (void)up3;
/* …（同文件无关代码省略）… */
  return 0;
}
/* And a callback to register above upon actual need */
void useDummyCS(void *up1, sqlite3 *db, int etr, const char *zName){
  (void)up1;
  sqlite3_create_collation_v2(db, zName, etr, 0, dummyCompare, 0);
}
/* …（同文件无关代码省略）… */
/*
** dummy functions for no-op implementation of UDFs during expert's work
*/
void dummyUDF(sqlite3_context *up1, int up2, sqlite3_value **up3){
  (void)up1;
  (void)up2;
  (void)up3;
  assert(0); /* VDBE should never be run. */
}
void dummyUDFvalue(sqlite3_context *up1){
  (void)up1;
  assert(0); /* VDBE should never be run. */
}

/*
** Register UDFs from user database with another.
*/
int registerUDFs(sqlite3 *dbSrc, sqlite3 *dbDst){
  sqlite3_stmt *pStmt;
  int rc = sqlite3_prepare_v2(dbSrc,
            "SELECT name,type,enc,narg,flags "
