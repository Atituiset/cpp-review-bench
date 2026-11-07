// AUTO-DRAFT from sqlite/sqlite PR #32dc28665df7cf989225c6df252f3c2ec064e9bc
      }
    }
    if( rc==SQLITE_OK ){
      sqlite3_bind_blob(pReplace, 2, pBuf->p, pBuf->n, SQLITE_STATIC);  // <<< BUG ANCHOR
      sqlite3_step(pReplace);
      rc = sqlite3_reset(pReplace);
      sqlite3_bind_null(pReplace, 2);
    }
  }
  return rc;
