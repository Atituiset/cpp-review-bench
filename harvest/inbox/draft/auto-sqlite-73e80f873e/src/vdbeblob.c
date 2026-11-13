// AUTO-DRAFT from sqlite/sqlite PR #b4b5ba2afb750999741ebd658d926af2cca3fe64
  int nByte;              /* Size of open blob, in bytes */
  int iOffset;            /* Byte offset of blob in cursor data */
  u16 iCol;               /* Table column this handle is open on */
  BtCursor *pCsr;         /* Cursor pointing at blob row */
  sqlite3_stmt *pStmt;    /* Statement holding cursor open */
  sqlite3 *db;            /* The associated database */
/* …（同文件无关代码省略）… */
  return rc;
}
  // <<< BUG ANCHOR
/*
** Perform a read or write operation on a blob
*/
/* …（同文件无关代码省略）… */
#endif

    sqlite3BtreeLeaveCursor(p->pCsr);
    if( rc==SQLITE_ABORT ){
      sqlite3VdbeFinalize(v);
      p->pStmt = 0;
    }else{
      v->rc = rc;
    }
  }
  sqlite3Error(db, rc);
