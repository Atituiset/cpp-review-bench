// AUTO-DRAFT from sqlite/sqlite PR #a595e9daaa8ec4d4bd238a94cf705103380e9379
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static void sqlite3ErrorIfNotEmpty(
  Parse *pParse,        /* Parsing context */
  const char *zDb,      /* Schema holding the table */
  const char *zTab,     /* Table to check for empty */
  const char *zErr      /* Error message text */
){
  sqlite3NestedParse(pParse,
     "SELECT raise(ABORT,%Q) FROM \"%w\".\"%w\"",
     zErr, zDb, zTab
  );
}
/* …（同文件无关代码省略）… */
static int isRowidAlias(
  Parse *pParse, 
  Table *pTab, 
  const char *zCol, 
  const char *zOp
){
  if( HasRowid(pTab) && sqlite3IsRowid(zCol) ){
    sqlite3ErrorMsg(pParse, "cannot %s rowid alias: %s", zOp, zCol);
    return SQLITE_ERROR;
  }
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
  }
  if( isRowidAlias(pParse, pTab, pCol->zCnName, "add") ) return;
  if( (pCol->colFlags & COLFLAG_GENERATED)==0 ){
    /* If the default value for the new column was specified with a
    ** literal NULL, then set pDflt to 0. This simplifies checking
    ** for an SQL NULL default below.
    */
    assert( pDflt==0 || pDflt->op==TK_SPAN );
    if( pDflt && pDflt->pLeft->op==TK_NULL ){
      pDflt = 0;
    }
    assert( IsOrdinaryTable(pNew) );
    if( (db->flags&SQLITE_ForeignKeys) && pNew->u.tab.pFKey && pDflt ){
      sqlite3ErrorIfNotEmpty(pParse, zDb, zTab,
          "Cannot add a REFERENCES column with non-NULL default value");
    }
    if( pCol->notNull && !pDflt ){
      sqlite3ErrorIfNotEmpty(pParse, zDb, zTab,
          "Cannot add a NOT NULL column with default value NULL");
    }


    /* Ensure the default expression is something that sqlite3ValueFromExpr()
    ** can handle (i.e. not CURRENT_TIME etc.)
    */
    if( pDflt ){
      sqlite3_value *pVal = 0;
      int rc;
/* …（同文件无关代码省略）… */
      if( !pVal ){
        sqlite3ErrorIfNotEmpty(pParse, zDb, zTab,
           "Cannot add a column with non-constant default");
      }
      sqlite3ValueFree(pVal);
    }
  }else if( pCol->colFlags & COLFLAG_STORED ){
    sqlite3ErrorIfNotEmpty(pParse, zDb, zTab, "cannot add a STORED column");
  }
