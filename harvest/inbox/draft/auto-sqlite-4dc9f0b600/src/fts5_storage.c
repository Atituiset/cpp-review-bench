// AUTO-DRAFT from sqlite/sqlite PR #4259cf526426208b1e5514e5f04c056e68958ebc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdio.h>
  // <<< BUG ANCHOR
static int fts5ExecPrintf(
  sqlite3 *db,
  char **pzErr,
  const char *zFormat,
  ...
){
  int rc;
  va_list ap;                     /* ... printf arguments */
  char *zSql;

  va_start(ap, zFormat);
  zSql = sqlite3_vmprintf(zFormat, ap);

  if( zSql==0 ){
    rc = SQLITE_NOMEM;
  }else{
    rc = sqlite3_exec(db, zSql, 0, 0, pzErr);
    sqlite3_free(zSql);
  }

  va_end(ap);
  return rc;
}
/* …（同文件无关代码省略）… */
        pConfig->zDb, pConfig->zName
    );
  }
  if( rc==SQLITE_OK && pConfig->eContent==FTS5_CONTENT_NORMAL ){
    rc = fts5ExecPrintf(pConfig->db, 0, 
        "DROP TABLE IF EXISTS %Q.'%q_content';",
        pConfig->zDb, pConfig->zName
    );
  }
  return rc;
}
/* …（同文件无关代码省略）… */
static void fts5StorageRenameOne(
  Fts5Config *pConfig,            /* Current FTS5 configuration */
  int *pRc,                       /* IN/OUT: Error code */
  const char *zTail,              /* Tail of table name e.g. "data", "config" */
  const char *zName               /* New name of FTS5 table */
){
  if( *pRc==SQLITE_OK ){
    *pRc = fts5ExecPrintf(pConfig->db, 0, 
        "ALTER TABLE %Q.'%q_%s' RENAME TO '%q_%s';",
        pConfig->zDb, pConfig->zName, zTail, zName, zTail
    );
  }
}
/* …（同文件无关代码省略）… */
  if( pConfig->bColumnsize ){
    fts5StorageRenameOne(pConfig, &rc, "docsize", zName);
  }
  if( pConfig->eContent==FTS5_CONTENT_NORMAL ){
    fts5StorageRenameOne(pConfig, &rc, "content", zName);
  }
  return rc;
