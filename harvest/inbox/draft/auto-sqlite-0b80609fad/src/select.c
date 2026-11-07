// AUTO-DRAFT from sqlite/sqlite PR #812558083845078bcc5924e5beb353aa8e138a6b
}

#if !defined(SQLITE_OMIT_SUBQUERY) || !defined(SQLITE_OMIT_VIEW)
/*
** This routine attempts to flatten subqueries as a performance optimization.
** This routine returns 1 if it makes changes and 0 if no flattening occurs.
/* …（同文件无关代码省略）… */
    if( pSrc->nSrc>1 ){
      if( pParse->nSelect>500 ) return 0;
      if( OptimizationDisabled(db, SQLITE_FlttnUnionAll) ) return 0;
      aCsrMap = sqlite3DbMallocZero(db, ((i64)pParse->nTab+1)*sizeof(int));
      if( aCsrMap ) aCsrMap[0] = pParse->nTab;
    }
