// AUTO-DRAFT from sqlite/sqlite PR #96bf376491aea863bf40d6efba4a0c2b73c6defd
    pCur->iRowid++;
    while( i<pTab->nCol ){
      sqlite3_free(pCur->azVal[i]);
      pCur->azVal[i] = 0;  // <<< BUG ANCHOR
      pCur->aLen[i] = 0;
      i++;
    }
