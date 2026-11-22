// AUTO-DRAFT from sqlite/sqlite PR #7dbc51b920798d4e44195e57c42a26c9c0bd2598
typedef struct Fts5HashEntry Fts5HashEntry;
/* …（同文件无关代码省略）… */
  Fts5HashEntry *p;
  u8 *pPtr;
  int nIncr = 0;                  /* Amount to increment (*pHash->pnByte) by */
  int bNew;                       /* If non-delete entry should be written */
  
  bNew = (pHash->eDetail==FTS5_DETAIL_FULL);
/* …（同文件无关代码省略）… */
  }

  nIncr += p->nData;
  *pHash->pnByte += nIncr;
  return SQLITE_OK;
}
