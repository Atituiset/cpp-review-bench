// AUTO-DRAFT from sqlite/sqlite PR #317c040e352e2dcbd993588cf6d1dcfad0c01d2f
typedef struct Fts5HashEntry Fts5HashEntry;
/* …（同文件无关代码省略）… */
    **     + 5 bytes for the new position offset (32-bit max).
    */
    if( (p->nAlloc - p->nData) < (9 + 4 + 1 + 3 + 5) ){
      sqlite3_int64 nNew = (i64)p->nAlloc * 2;  // <<< BUG ANCHOR
      Fts5HashEntry *pNew;
      Fts5HashEntry **pp;
      pNew = (Fts5HashEntry*)sqlite3_realloc64(p, nNew);
      if( pNew==0 ) return SQLITE_NOMEM;
      pNew->nAlloc = (int)nNew;
      for(pp=&pHash->aSlot[iHash]; *pp!=p; pp=&(*pp)->pHashNext);
      *pp = pNew;
      p = pNew;
