// AUTO-DRAFT from sqlite/sqlite PR #8a24548dfbe71bc2714079e7549d1814002f5c44
        /* TUNING: For a UNION CTE, because UNION is implies DISTINCT,
        ** reduce the estimated output row count by 8 (LogEst 30). 
        ** Search for tag-20250414a to see other cases */
        p->nSelectRow -= 30;  // <<< BUG ANCHOR
      }
    }
    if( sDistinct.isTnct && sqlite3WhereIsDistinct(pWInfo) ){
