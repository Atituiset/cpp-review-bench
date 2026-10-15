// AUTO-DRAFT from sqlite/sqlite PR #fff257d3ed2c2d5f77a052d14e931b260d4828a5
#define FTS5_STMT_LOOKUP2         3
/* …（同文件无关代码省略）… */
#define FTS5_STMT_SCAN           11
/* …（同文件无关代码省略）… */
      rc = sqlite3_prepare_v3(pC->db, zSql, -1, f, &p->aStmt[eStmt], 0);
      p->pConfig->bLock--;
      sqlite3_free(zSql);
      if( rc!=SQLITE_OK && pzErrMsg ){  // <<< BUG ANCHOR
        *pzErrMsg = sqlite3_mprintf("%s", sqlite3_errmsg(pC->db));
      }
      if( rc==SQLITE_ERROR && eStmt>FTS5_STMT_LOOKUP2 && eStmt<FTS5_STMT_SCAN ){
        /* One of the internal tables - not the %_content table - is missing.
        ** This counts as a corrupted table.  */
       rc = SQLITE_CORRUPT;
      }
    }
  }
