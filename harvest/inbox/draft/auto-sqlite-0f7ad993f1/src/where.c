// AUTO-DRAFT from sqlite/sqlite PR #50575da52ca09eba39e84b370e7913edc80894d2
**      direction because there is no way to run a co-routine backwards.
*/
static SQLITE_NOINLINE int wherePathMatchSubqueryOB(
  WhereInfo *pWInfo,      /* The WHERE clause */
  WhereLoop *pLoop,       /* The nested loop term that is a subquery */
  int iLoop,              /* Which level of the nested loop.  0==outermost */
  int iCur,               /* Cursor used by the this loop */
  ExprList *pOrderBy,     /* The ORDER BY clause on the whole query */
  Bitmask *pRevMask,      /* When loops need to go in reverse order */
  Bitmask *pOBSat         /* Which terms of pOrderBy are satisfied so far */
){  // <<< BUG ANCHOR
  int iOB;                /* Index into pOrderBy->a[] */
  int jSub;               /* Index into pSubOB->a[] */
/* …（同文件无关代码省略）… */
    if( pOBExpr->op!=TK_COLUMN && pOBExpr->op!=TK_AGG_COLUMN ) break;
    if( pOBExpr->iTable!=iCur ) break;
    if( pOBExpr->iColumn!=pSubOB->a[jSub].u.x.iOrderByCol-1 ) break;
    if( (pWInfo->wctrlFlags & WHERE_GROUPBY)==0 ){
      u8 sfOB = pOrderBy->a[iOB].fg.sortFlags;   /* sortFlags for iOB */
      u8 sfSub = pSubOB->a[jSub].fg.sortFlags;   /* sortFlags for jSub */
      if( (sfSub & KEYINFO_ORDER_BIGNULL) != (sfOB & KEYINFO_ORDER_BIGNULL) ){
/* …（同文件无关代码省略）… */
      if( pLoop->wsFlags & WHERE_IPK ){
        if( pLoop->u.btree.pOrderBy
         && OptimizationEnabled(db, SQLITE_OrderBySubq)
         &&  wherePathMatchSubqueryOB(pWInfo,pLoop,iLoop,iCur,
                                     pOrderBy,pRevMask, &obSat)
        ){
          nColumn = 0;
          isOrderDistinct = 0;
