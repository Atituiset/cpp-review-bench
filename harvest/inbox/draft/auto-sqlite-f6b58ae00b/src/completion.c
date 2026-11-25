// AUTO-DRAFT from sqlite/sqlite PR #b5fb535663df6593c749cb48d1f5d5672988d959
#define COMPLETION_DATABASES     7
#define COMPLETION_TABLES        8    /* Also VIEWs and TRIGGERs */
#define COMPLETION_COLUMNS       9
/* …（同文件无关代码省略）… */
#define COMPLETION_EOF           11
/* …（同文件无关代码省略）… */
  return SQLITE_OK;
}
  // <<< BUG ANCHOR
/*
** Advance a completion_cursor to its next row of output.
**
/* …（同文件无关代码省略）… */
      }
      case COMPLETION_DATABASES: {
        if( pCur->pStmt==0 ){
          sqlite3_prepare_v2(pCur->db, "PRAGMA database_list", -1,
                             &pCur->pStmt, 0);
        }
        iCol = 1;
        eNextPhase = COMPLETION_TABLES;
/* …（同文件无关代码省略）… */
          sqlite3_str* pStr = sqlite3_str_new(pCur->db);
          char *zSql = 0;
          const char *zSep = "";
          sqlite3_prepare_v2(pCur->db, "PRAGMA database_list", -1, &pS2, 0);
          while( sqlite3_step(pS2)==SQLITE_ROW ){
            const char *zDb = (const char*)sqlite3_column_text(pS2, 1);
            sqlite3_str_appendf(pStr,
/* …（同文件无关代码省略）… */
          zSql = sqlite3_str_finish(pStr);
          if( zSql==0 ) return SQLITE_NOMEM;
          if( rc==SQLITE_OK ){
            sqlite3_prepare_v2(pCur->db, zSql, -1, &pCur->pStmt, 0);
          }
          sqlite3_free(zSql);
          if( rc ) return rc;
        }
        iCol = 0;
        eNextPhase = COMPLETION_COLUMNS;
/* …（同文件无关代码省略）… */
          sqlite3_str *pStr = sqlite3_str_new(pCur->db);
          char *zSql = 0;
          const char *zSep = "";
          sqlite3_prepare_v2(pCur->db, "PRAGMA database_list", -1, &pS2, 0);
          while( sqlite3_step(pS2)==SQLITE_ROW ){
            const char *zDb = (const char*)sqlite3_column_text(pS2, 1);
            sqlite3_str_appendf(pStr,
/* …（同文件无关代码省略）… */
          zSql = sqlite3_str_finish(pStr);
          if( zSql==0 ) return SQLITE_NOMEM;
          if( rc==SQLITE_OK ){
            sqlite3_prepare_v2(pCur->db, zSql, -1, &pCur->pStmt, 0);
          }
          sqlite3_free(zSql);
          if( rc ) return rc;
        }
        iCol = 0;
        eNextPhase = COMPLETION_EOF;
/* …（同文件无关代码省略）… */
        rc = sqlite3_finalize(pCur->pStmt);
        pCur->pStmt = 0;
        pCur->ePhase = eNextPhase;
        if( rc ) return rc;
        continue;
      }
    }
