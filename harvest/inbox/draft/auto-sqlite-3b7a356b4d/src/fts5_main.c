// AUTO-DRAFT from sqlite/sqlite PR #4f6dc901fec3d7a932665f3e4c805f5a9ffc8e60
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct Fts5Auxiliary Fts5Auxiliary;
typedef struct Fts5Cursor Fts5Cursor;
typedef struct Fts5FullTable Fts5FullTable;
/* …（同文件无关代码省略）… */
typedef struct Fts5TokenizerModule Fts5TokenizerModule;
/* …（同文件无关代码省略）… */
struct Fts5Global {
  fts5_api api;                   /* User visible part of object (see fts5.h) */
  sqlite3 *db;                    /* Associated database connection */ 
  i64 iNextId;                    /* Used to allocate unique cursor ids */
  Fts5Auxiliary *pAux;            /* First in list of all aux. functions */
  Fts5TokenizerModule *pTok;      /* First in list of all tokenizer modules */
  Fts5TokenizerModule *pDfltTok;  /* Default tokenizer module */
  Fts5Cursor *pCsr;               /* First in list of all open cursors */
  u32 aLocaleHdr[4];
};
/* …（同文件无关代码省略）… */
#define FTS5_LOCALE_HDR_SIZE ((int)sizeof( ((Fts5Global*)0)->aLocaleHdr ))
#define FTS5_LOCALE_HDR(pConfig) ((const u8*)(pConfig->pGlobal->aLocaleHdr))
/* …（同文件无关代码省略）… */
#define FTS5CSR_REQUIRE_CONTENT   0x02
/* …（同文件无关代码省略）… */
#define CsrFlagClear(pCsr, flag) ((pCsr)->csrflags &= ~(flag))
#define CsrFlagTest(pCsr, flag)  ((pCsr)->csrflags & (flag))
/* …（同文件无关代码省略）… */
#define FTS5_PLAN_MATCH          1       /* (<tbl> MATCH ?) */
#define FTS5_PLAN_SOURCE         2       /* A source cursor for SORTED_MATCH */
/* …（同文件无关代码省略）… */
#define FTS5_PLAN_SORTED_MATCH   4       /* (<tbl> MATCH ? ORDER BY rank) */
#define FTS5_PLAN_SCAN           5       /* No usable constraint */
#define FTS5_PLAN_ROWID          6       /* (rowid = ?) */
/* …（同文件无关代码省略）… */
static int fts5StmtType(Fts5Cursor *pCsr){
  if( pCsr->ePlan==FTS5_PLAN_SCAN ){
    return (pCsr->bDesc) ? FTS5_STMT_SCAN_DESC : FTS5_STMT_SCAN_ASC;
  }
  return FTS5_STMT_LOOKUP;
}
/* …（同文件无关代码省略）… */
static void fts5SetVtabError(Fts5FullTable *p, const char *zFormat, ...){
  va_list ap;                     /* ... printf arguments */
  va_start(ap, zFormat);
  sqlite3_free(p->p.base.zErrMsg);
  p->p.base.zErrMsg = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
}
/* …（同文件无关代码省略）… */
static void sqlite3Fts5SetLocale(
  Fts5Config *pConfig, 
  const char *zLocale, 
  int nLocale
){
  Fts5TokenizerConfig *pT = &pConfig->t;
  pT->pLocale = zLocale;
  pT->nLocale = nLocale;
}
/* …（同文件无关代码省略）… */
void sqlite3Fts5ClearLocale(Fts5Config *pConfig){
  sqlite3Fts5SetLocale(pConfig, 0, 0);
}
/* …（同文件无关代码省略）… */
int sqlite3Fts5IsLocaleValue(Fts5Config *pConfig, sqlite3_value *pVal){
  int ret = 0;
  if( sqlite3_value_type(pVal)==SQLITE_BLOB ){
    /* Call sqlite3_value_bytes() after sqlite3_value_blob() in this case.
    ** If the blob was created using zeroblob(), then sqlite3_value_blob()
    ** may call malloc(). If this malloc() fails, then the values returned
    ** by both value_blob() and value_bytes() will be 0. If value_bytes() were 
    ** called first, then the NULL pointer returned by value_blob() might
    ** be dereferenced.  */
    const u8 *pBlob = sqlite3_value_blob(pVal);
    int nBlob = sqlite3_value_bytes(pVal); 
    if( nBlob>FTS5_LOCALE_HDR_SIZE
     && 0==memcmp(pBlob, FTS5_LOCALE_HDR(pConfig), FTS5_LOCALE_HDR_SIZE)
    ){
      ret = 1;
    }
  }
  return ret;
}
/* …（同文件无关代码省略）… */
int sqlite3Fts5DecodeLocaleValue(
  sqlite3_value *pVal, 
  const char **ppText,
  int *pnText, 
  const char **ppLoc, 
  int *pnLoc
){
  const char *p = sqlite3_value_blob(pVal);
  int n = sqlite3_value_bytes(pVal);
  int nLoc = 0;

  assert( sqlite3_value_type(pVal)==SQLITE_BLOB );
  assert( n>FTS5_LOCALE_HDR_SIZE );

  for(nLoc=FTS5_LOCALE_HDR_SIZE; p[nLoc]; nLoc++){
    if( nLoc==(n-1) ){
      return SQLITE_MISMATCH;
    }
  }
  *ppLoc = &p[FTS5_LOCALE_HDR_SIZE];
  *pnLoc = nLoc - FTS5_LOCALE_HDR_SIZE;

  *ppText = &p[nLoc+1];
  *pnText = n - nLoc - 1;
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static i64 fts5CursorRowid(Fts5Cursor *pCsr){
  assert( pCsr->ePlan==FTS5_PLAN_MATCH 
       || pCsr->ePlan==FTS5_PLAN_SORTED_MATCH 
       || pCsr->ePlan==FTS5_PLAN_SOURCE 
       || pCsr->ePlan==FTS5_PLAN_SCAN 
       || pCsr->ePlan==FTS5_PLAN_ROWID 
  );
  if( pCsr->pSorter ){
    return pCsr->pSorter->iRowid;
  }else if( pCsr->ePlan>=FTS5_PLAN_SCAN ){
    return sqlite3_column_int64(pCsr->pStmt, 0);
  }else{
    return sqlite3Fts5ExprRowid(pCsr->pExpr);
  }
}
/* …（同文件无关代码省略）… */
static int fts5SeekCursor(Fts5Cursor *pCsr, int bErrormsg){
  int rc = SQLITE_OK;

  /* If the cursor does not yet have a statement handle, obtain one now. */ 
  if( pCsr->pStmt==0 ){
    Fts5FullTable *pTab = (Fts5FullTable*)(pCsr->base.pVtab);
    int eStmt = fts5StmtType(pCsr);
    rc = sqlite3Fts5StorageStmt(
        pTab->pStorage, eStmt, &pCsr->pStmt, (bErrormsg?&pTab->p.base.zErrMsg:0)
    );
    assert( rc!=SQLITE_OK || pTab->p.base.zErrMsg==0 );
    assert( CsrFlagTest(pCsr, FTS5CSR_REQUIRE_CONTENT) );
  }

  if( rc==SQLITE_OK && CsrFlagTest(pCsr, FTS5CSR_REQUIRE_CONTENT) ){
    Fts5Table *pTab = (Fts5Table*)(pCsr->base.pVtab);
    assert( pCsr->pExpr );
    sqlite3_reset(pCsr->pStmt);
    sqlite3_bind_int64(pCsr->pStmt, 1, fts5CursorRowid(pCsr));
    pTab->pConfig->bLock++;
    rc = sqlite3_step(pCsr->pStmt);
    pTab->pConfig->bLock--;
    if( rc==SQLITE_ROW ){
      rc = SQLITE_OK;
      CsrFlagClear(pCsr, FTS5CSR_REQUIRE_CONTENT);
    }else{
      rc = sqlite3_reset(pCsr->pStmt);
      if( rc==SQLITE_OK ){
        rc = FTS5_CORRUPT;
        fts5SetVtabError((Fts5FullTable*)pTab, 
            "fts5: missing row %lld from content table %s",
            fts5CursorRowid(pCsr), 
            pTab->pConfig->zContent
        );
      }else if( pTab->pConfig->pzErrmsg ){
        fts5SetVtabError((Fts5FullTable*)pTab, 
            "%s", sqlite3_errmsg(pTab->pConfig->db)
        );
      }
    }
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static int fts5TextFromStmt(
  Fts5Config *pConfig,
  sqlite3_stmt *pStmt,
  int iCol,
  const char **ppText,
  int *pnText
){
  sqlite3_value *pVal = sqlite3_column_value(pStmt, iCol+1);
  const char *pLoc = 0;
  int nLoc = 0;
  int rc = SQLITE_OK;

  if( pConfig->bLocale 
   && pConfig->eContent==FTS5_CONTENT_EXTERNAL
   && sqlite3Fts5IsLocaleValue(pConfig, pVal) 
  ){
    rc = sqlite3Fts5DecodeLocaleValue(pVal, ppText, pnText, &pLoc, &nLoc);
  }else{
    *ppText = (const char*)sqlite3_value_text(pVal);
    *pnText = sqlite3_value_bytes(pVal);
    if( pConfig->bLocale && pConfig->eContent==FTS5_CONTENT_NORMAL ){
      pLoc = (const char*)sqlite3_column_text(pStmt, iCol+1+pConfig->nCol);
      nLoc = sqlite3_column_bytes(pStmt, iCol+1+pConfig->nCol);
    }
  }
  sqlite3Fts5SetLocale(pConfig, pLoc, nLoc);
  return rc;
}
/* …（同文件无关代码省略）… */
        rc = fts5SeekCursor(pCsr, 0);
      }
      for(i=0; i<pConfig->nCol && rc==SQLITE_OK; i++){
        const char *z = 0;
        int n = 0; 
        rc = fts5TextFromStmt(pConfig, pCsr->pStmt, i, &z, &n);
        if( rc==SQLITE_OK ){
          rc = sqlite3Fts5ExprPopulatePoslists(
              pConfig, pCsr->pExpr, aPopulator, i, z, n
          );
        }
        sqlite3Fts5ClearLocale(pConfig);
      }
      sqlite3_free(aPopulator);
