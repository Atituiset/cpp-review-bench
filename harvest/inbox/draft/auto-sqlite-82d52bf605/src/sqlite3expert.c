// AUTO-DRAFT from sqlite/sqlite PR #05f5ad235f1421a593fb4605e859aefd4c494289
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
  // <<< BUG ANCHOR
# define ALWAYS(X)      (1)
# define NEVER(X)       (0)
/* …（同文件无关代码省略）… */
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
typedef struct IdxColumn IdxColumn;
typedef struct IdxConstraint IdxConstraint;
typedef struct IdxScan IdxScan;
typedef struct IdxStatement IdxStatement;
typedef struct IdxTable IdxTable;
typedef struct IdxWrite IdxWrite;
/* …（同文件无关代码省略）… */
#define STRLEN  (int)strlen
/* …（同文件无关代码省略）… */
#define IDX_HASH_SIZE 1023
typedef struct IdxHashEntry IdxHashEntry;
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
static void *idxMalloc(int *pRc, i64 nByte){
  void *pRet;
  assert( *pRc==SQLITE_OK );
  assert( nByte>0 );
  pRet = sqlite3_malloc64(nByte);
  if( pRet ){
    memset(pRet, 0, nByte);
  }else{
    *pRc = SQLITE_NOMEM;
  }
  return pRet;
}
/* …（同文件无关代码省略）… */
static int idxHashString(const char *z, int n){
  unsigned int ret = 0;
  int i;
  for(i=0; i<n; i++){
    ret += (ret<<3) + (unsigned char)(z[i]);
  }
  return (int)(ret % IDX_HASH_SIZE);
}
/* …（同文件无关代码省略）… */
static int idxHashAdd(
  int *pRc, 
  IdxHash *pHash, 
  const char *zKey,
  const char *zVal
){
  int nKey = STRLEN(zKey);
  int iHash = idxHashString(zKey, nKey);
  int nVal = (zVal ? STRLEN(zVal) : 0);
  IdxHashEntry *pEntry;
  assert( iHash>=0 );
  for(pEntry=pHash->aHash[iHash]; pEntry; pEntry=pEntry->pHashNext){
    if( STRLEN(pEntry->zKey)==nKey && 0==memcmp(pEntry->zKey, zKey, nKey) ){
      return 1;
    }
  }
  pEntry = idxMalloc(pRc, sizeof(IdxHashEntry) + (i64)nKey+1 + (i64)nVal+1);
  if( pEntry ){
    pEntry->zKey = (char*)&pEntry[1];
    memcpy(pEntry->zKey, zKey, nKey);
    if( zVal ){
      pEntry->zVal = &pEntry->zKey[nKey+1];
      memcpy(pEntry->zVal, zVal, nVal);
    }
    pEntry->pHashNext = pHash->aHash[iHash];
    pHash->aHash[iHash] = pEntry;

    pEntry->pNext = pHash->pFirst;
    pHash->pFirst = pEntry;
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static int idxPrintfPrepareStmt(
  sqlite3 *db,                    /* Database handle to compile against */
  sqlite3_stmt **ppStmt,          /* OUT: Compiled SQL statement */
  char **pzErrmsg,                /* OUT: sqlite3_malloc()ed error message */
  const char *zFmt,               /* printf() format of SQL statement */
  ...                             /* Trailing printf() arguments */
){
  va_list ap;
  int rc;
  char *zSql;
  va_start(ap, zFmt);
  zSql = sqlite3_vmprintf(zFmt, ap);
  if( zSql==0 ){
    rc = SQLITE_NOMEM;
  }else{
    rc = idxPrepareStmt(db, ppStmt, pzErrmsg, zSql);
    sqlite3_free(zSql);
  }
  va_end(ap);
  return rc;
}
/* …（同文件无关代码省略）… */
static void idxFinalize(int *pRc, sqlite3_stmt *pStmt){
  int rc = sqlite3_finalize(pStmt);
  if( *pRc==SQLITE_OK ) *pRc = rc;
}
/* …（同文件无关代码省略）… */
static char *idxAppendText(int *pRc, char *zIn, const char *zFmt, ...){
  va_list ap;
  char *zAppend = 0;
  char *zRet = 0;
  i64 nIn = zIn ? STRLEN(zIn) : 0;
  i64 nAppend = 0;
  va_start(ap, zFmt);
  if( *pRc==SQLITE_OK ){
    zAppend = sqlite3_vmprintf(zFmt, ap);
    if( zAppend ){
      nAppend = STRLEN(zAppend);
      zRet = (char*)sqlite3_malloc64(nIn + nAppend + 1);
    }
    if( zAppend && zRet ){
      if( nIn ) memcpy(zRet, zIn, nIn);
      memcpy(&zRet[nIn], zAppend, nAppend+1);
    }else{
      sqlite3_free(zRet);
      zRet = 0;
      *pRc = SQLITE_NOMEM;
    }
    sqlite3_free(zAppend);
    sqlite3_free(zIn);
  }
  va_end(ap);
  return zRet;
}
/* …（同文件无关代码省略）… */
static int idxIdentifierRequiresQuotes(const char *zId){
  int i;
  int nId = STRLEN(zId);
  
  if( sqlite3_keyword_check(zId, nId) ) return 1;

  for(i=0; zId[i]; i++){
    if( !(zId[i]=='_')
     && !(zId[i]>='0' && zId[i]<='9')
     && !(zId[i]>='a' && zId[i]<='z')
     && !(zId[i]>='A' && zId[i]<='Z')
    ){
      return 1;
    }
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static char *idxAppendColDefn(
  int *pRc,                       /* IN/OUT: Error code */
  char *zIn,                      /* Column defn accumulated so far */
  IdxTable *pTab,                 /* Table index will be created on */
  IdxConstraint *pCons
){
  char *zRet = zIn;
  IdxColumn *p = &pTab->aCol[pCons->iCol];
  if( zRet ) zRet = idxAppendText(pRc, zRet, ", ");

  if( idxIdentifierRequiresQuotes(p->zName) ){
    zRet = idxAppendText(pRc, zRet, "%Q", p->zName);
  }else{
    zRet = idxAppendText(pRc, zRet, "%s", p->zName);
  }

  if( sqlite3_stricmp(p->zColl, pCons->zColl) ){
    if( idxIdentifierRequiresQuotes(pCons->zColl) ){
      zRet = idxAppendText(pRc, zRet, " COLLATE %Q", pCons->zColl);
    }else{
      zRet = idxAppendText(pRc, zRet, " COLLATE %s", pCons->zColl);
    }
  }

  if( pCons->bDesc ){
    zRet = idxAppendText(pRc, zRet, " DESC");
  }
  return zRet;
}
/* …（同文件无关代码省略）… */
static int idxFindCompatible(
  int *pRc,                       /* OUT: Error code */
  sqlite3* dbm,                   /* Database to search */
  IdxScan *pScan,                 /* Scan for table to search for index on */
  IdxConstraint *pEq,             /* List of == constraints */
  IdxConstraint *pTail            /* List of range constraints */
){
  const char *zTbl = pScan->pTab->zName;
  sqlite3_stmt *pIdxList = 0;
  IdxConstraint *pIter;
  int nEq = 0;                    /* Number of elements in pEq */
  int rc;

  /* Count the elements in list pEq */
  for(pIter=pEq; pIter; pIter=pIter->pLink) nEq++;

  rc = idxPrintfPrepareStmt(dbm, &pIdxList, 0, "PRAGMA index_list=%Q", zTbl);
  while( rc==SQLITE_OK && sqlite3_step(pIdxList)==SQLITE_ROW ){
    int bMatch = 1;
    IdxConstraint *pT = pTail;
    sqlite3_stmt *pInfo = 0;
    const char *zIdx = (const char*)sqlite3_column_text(pIdxList, 1);
    if( zIdx==0 ) continue;

    /* Zero the IdxConstraint.bFlag values in the pEq list */
    for(pIter=pEq; pIter; pIter=pIter->pLink) pIter->bFlag = 0;

    rc = idxPrintfPrepareStmt(dbm, &pInfo, 0, "PRAGMA index_xInfo=%Q", zIdx);
    while( rc==SQLITE_OK && sqlite3_step(pInfo)==SQLITE_ROW ){
      int iIdx = sqlite3_column_int(pInfo, 0);
      int iCol = sqlite3_column_int(pInfo, 1);
      const char *zColl = (const char*)sqlite3_column_text(pInfo, 4);

      if( iIdx<nEq ){
        for(pIter=pEq; pIter; pIter=pIter->pLink){
          if( pIter->bFlag ) continue;
          if( pIter->iCol!=iCol ) continue;
          if( sqlite3_stricmp(pIter->zColl, zColl) ) continue;
          pIter->bFlag = 1;
          break;
        }
        if( pIter==0 ){
          bMatch = 0;
          break;
        }
      }else{
        if( pT ){
          if( pT->iCol!=iCol || sqlite3_stricmp(pT->zColl, zColl) ){
            bMatch = 0;
            break;
          }
          pT = pT->pLink;
        }
      }
    }
    idxFinalize(&rc, pInfo);

    if( rc==SQLITE_OK && bMatch ){
      sqlite3_finalize(pIdxList);
      return 1;
    }
  }
  idxFinalize(&rc, pIdxList);

  *pRc = rc;
  return 0;
}
/* …（同文件无关代码省略）… */
static int countNonzeros(void* pCount, int nc,
                         char* azResults[], char* azColumns[]){
  (void)azColumns;  /* Suppress unused parameter warning */
  if( nc>0 && (azResults[0][0]!='0' || azResults[0][1]!=0) ){
    *((int *)pCount) += 1;
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static int idxCreateFromCons(
  sqlite3expert *p,
  IdxScan *pScan,
  IdxConstraint *pEq, 
  IdxConstraint *pTail
){
  sqlite3 *dbm = p->dbm;
  int rc = SQLITE_OK;
  if( (pEq || pTail) && 0==idxFindCompatible(&rc, dbm, pScan, pEq, pTail) ){
    IdxTable *pTab = pScan->pTab;
    char *zCols = 0;
    char *zIdx = 0;
    IdxConstraint *pCons;
    unsigned int h = 0;
    const char *zFmt;

    for(pCons=pEq; pCons; pCons=pCons->pLink){
      zCols = idxAppendColDefn(&rc, zCols, pTab, pCons);
    }
    for(pCons=pTail; pCons; pCons=pCons->pLink){
      zCols = idxAppendColDefn(&rc, zCols, pTab, pCons);
    }

    if( rc==SQLITE_OK ){
      /* Hash the list of columns to come up with a name for the index */
      const char *zTable = pScan->pTab->zName;
      int quoteTable = idxIdentifierRequiresQuotes(zTable);
      char *zName = 0;          /* Index name */
      int collisions = 0;
      do{
        int i;
        char *zFind;

        sqlite3_free(zName);
        if( collisions<10 ){
          for(i=0; zCols[i]; i++){
            h += ((h<<3) + zCols[i]);
          }
          zName = sqlite3_mprintf("%s_idx_%08x", zTable, h);
        }else{
          /* After several hash attempts, generate a random name */
          sqlite3_uint64 u;
          sqlite3_randomness(sizeof(u), &u);
          zName = sqlite3_mprintf("%s_idx_%llx", zTable, u);
        }
        if( zName==0 ) break;
         
        /* Is is unique among table, view and index names? */
        zFmt = "SELECT count(*) FROM sqlite_schema WHERE name=%Q"
          " AND type in ('index','table','view')";
        zFind = sqlite3_mprintf(zFmt, zName);
        i = 0;
        rc = sqlite3_exec(dbm, zFind, countNonzeros, &i, 0);
        assert(rc==SQLITE_OK);
        sqlite3_free(zFind);
        if( i==0 ){
          collisions = 0;
          break;
        }
        ++collisions;
      }while( ALWAYS(collisions<500) && zName!=0 );
      if( NEVER(collisions) ){
        /* This return means "Gave up trying to find a unique index name." */
        rc = SQLITE_BUSY_TIMEOUT;
      }else if( zName==0 ){
        rc = SQLITE_NOMEM;
      }else{
        if( quoteTable ){
          zFmt = "CREATE INDEX \"%w\" ON \"%w\"(%s)";
        }else{
          zFmt = "CREATE INDEX %s ON %s(%s)";
        }
        zIdx = sqlite3_mprintf(zFmt, zName, zTable, zCols);
        if( !zIdx ){
          rc = SQLITE_NOMEM;
        }else{
          rc = sqlite3_exec(dbm, zIdx, 0, 0, p->pzErrmsg);
          if( rc!=SQLITE_OK ){
            rc = SQLITE_BUSY_TIMEOUT;
          }else{
            idxHashAdd(&rc, &p->hIdx, zName, zIdx);
          }
        }
        sqlite3_free(zIdx);
      }
      sqlite3_free(zName);
    }

    sqlite3_free(zCols);
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int idxFindConstraint(IdxConstraint *pList, IdxConstraint *p){
  IdxConstraint *pCmp;
  for(pCmp=pList; pCmp; pCmp=pCmp->pLink){
    if( p->iCol==pCmp->iCol ) return 1;
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static int idxCreateFromWhere(
  sqlite3expert *p, 
  IdxScan *pScan,                 /* Create indexes for this scan */
  IdxConstraint *pTail            /* range/ORDER BY constraints for inclusion */
){
  IdxConstraint *p1 = 0;
  IdxConstraint *pCon;
  int rc;

  /* Gather up all the == constraints. */
  for(pCon=pScan->pEq; pCon; pCon=pCon->pNext){
    if( !idxFindConstraint(p1, pCon) && !idxFindConstraint(pTail, pCon) ){
      pCon->pLink = p1;
      p1 = pCon;
    }
  }

  /* Create an index using the == constraints collected above. And the
  ** range constraint/ORDER BY terms passed in by the caller, if any. */
  rc = idxCreateFromCons(p, pScan, p1, pTail);

  /* If no range/ORDER BY passed by the caller, create a version of the
  ** index for each range constraint.  */
  if( pTail==0 ){
    for(pCon=pScan->pRange; rc==SQLITE_OK && pCon; pCon=pCon->pNext){
      assert( pCon->pLink==0 );
      if( !idxFindConstraint(p1, pCon) && !idxFindConstraint(pTail, pCon) ){
        rc = idxCreateFromCons(p, pScan, p1, pCon);
      }
    }
  }

  return rc;
}
/* …（同文件无关代码省略）… */
static int idxCreateCandidates(sqlite3expert *p){
  int rc = SQLITE_OK;
  IdxScan *pIter;

  for(pIter=p->pScan; pIter && rc==SQLITE_OK; pIter=pIter->pNextScan){
    rc = idxCreateFromWhere(p, pIter, 0);
    if( rc==SQLITE_OK && pIter->pOrder ){
      rc = idxCreateFromWhere(p, pIter, pIter->pOrder);
    }
  }

  return rc;
}
/* …（同文件无关代码省略）… */

  /* Create candidate indexes within the in-memory database file */
  if( rc==SQLITE_OK ){
    rc = idxCreateCandidates(p);
  }else if ( rc==SQLITE_BUSY_TIMEOUT ){
    if( pzErr )
      *pzErr = sqlite3_mprintf("Cannot find a unique index name to propose.");
    return rc;
  }

  /* Generate the stat1 data */
