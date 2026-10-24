// AUTO-DRAFT from sqlite/sqlite PR #f879fb18cd81586e8caad7cc532575525ac1bd8e
          sqlite3_str_appendf(pDefn, ", c%d", i);
        }
      }
      if( pConfig->bLocale ){  // <<< BUG ANCHOR
        for(i=0; i<pConfig->nCol; i++){
          if( pConfig->abUnindexed[i]==0 ){
            sqlite3_str_appendf(pDefn, ", l%d", i);
