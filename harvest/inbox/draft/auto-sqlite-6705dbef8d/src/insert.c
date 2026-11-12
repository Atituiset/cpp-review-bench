// AUTO-DRAFT from sqlite/sqlite PR #f157fe7debcda0d8513a2c10364e8fe781ee30ae
  if( pSelect->pSrc->a[0].fg.isSubquery ){
    return 0;   /* FROM clause cannot contain a subquery */
  }
  if( pSelect->pWhere ){
    return 0;   /* SELECT may not have a WHERE clause */
  }
