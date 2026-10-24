// AUTO-DRAFT from sqlite/sqlite PR #0bb1247fca6393c2b23cbff6c26669150d6ea412
  }
#ifndef SQLITE_OMIT_INCRBLOB
  sqlite3VdbeMemSetZeroBlob(pCtx->pOut, (int)n);
  return SQLITE_OK;
#else
  return sqlite3VdbeMemSetZeroBlob(pCtx->pOut, (int)n);
/* …（同文件无关代码省略）… */
#else
    rc = sqlite3VdbeMemSetZeroBlob(&p->aVar[i-1], n);
#endif
    sqlite3_mutex_leave(p->db->mutex);
  }
  return rc;
