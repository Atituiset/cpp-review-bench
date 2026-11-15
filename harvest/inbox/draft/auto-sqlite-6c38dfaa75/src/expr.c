// AUTO-DRAFT from sqlite/sqlite PR #4042a19a2d8c495fe9a242252ce391bb54770e45
    sqlite3VdbeReprepareOnBind(pParse->pVdbe, iVar, 0);
    pL = sqlite3VdbeGetBoundValue(pParse->pReprepare, iVar, SQLITE_AFF_BLOB);
    if( pL ){
      if( sqlite3_value_type(pL)==SQLITE_TEXT ){
        sqlite3_value_text(pL); /* Make sure the encoding is UTF-8 */
      }
      /* The bound value and the literal must be interchangeable, not
      ** just compare equal. For example, integer 1 and real 1.0 are equal
      ** according to MemCompare(), but are different if TEXT affinity
      ** is applied to them. Similarly, 0.0 and -0.0.  */
      int eType = sqlite3_value_type(pL);  // <<< BUG ANCHOR
      if( eType==sqlite3_value_type(pR) && sqlite3MemCompare(pL, pR, 0)==0 ){
        res = 0;
        if( eType==SQLITE_FLOAT ){
