// AUTO-DRAFT from sqlite/sqlite PR #0361a3b31cc719e4ff3af8a064f33d0925c78376
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

#endif /* !defined(SQLITE_OMIT_SUBQUERY) || !defined(SQLITE_OMIT_VIEW) */

#if !defined(SQLITE_OMIT_SUBQUERY) || !defined(SQLITE_OMIT_VIEW)
/*
** Make copies of relevant WHERE clause terms of the outer query into
** the WHERE clause of subquery.  Example:
/* …（同文件无关代码省略）… */
**       case only comes up if SQLite is compiled using
**       SQLITE_ALLOW_ROWID_IN_VIEW.
**
** Return 0 if no changes are made and non-zero if one or more WHERE clause
** terms are duplicated into the subquery.
*/
/* …（同文件无关代码省略）… */
  Expr *pNew;
  SrcItem *pSrc;        /* The subquery FROM term into which WHERE is pushed */
  int nChng = 0;
  pSrc = &pSrcList->a[iSrc];
  if( pWhere==0 ) return 0;
  if( pSubq->selFlags & (SF_Recursive|SF_MultiPart) ){
/* …（同文件无关代码省略）… */
    int notUnionAll = 0;
    for(pSel=pSubq; pSel; pSel=pSel->pPrior){
      u8 op = pSel->op;
      assert( op==TK_ALL || op==TK_SELECT
           || op==TK_UNION || op==TK_INTERSECT || op==TK_EXCEPT );
      if( op!=TK_ALL && op!=TK_SELECT ){
        notUnionAll = 1;
      }
#ifndef SQLITE_OMIT_WINDOWFUNC
      if( pSel->pWin ) return 0;    /* restriction (6b) */
/* …（同文件无关代码省略）… */
#ifndef SQLITE_OMIT_WINDOWFUNC
    if( pSubq->pWin && pSubq->pWin->pPartition==0 ) return 0;
#endif
  }

#ifdef SQLITE_DEBUG
/* …（同文件无关代码省略）… */
#endif

  if( sqlite3ExprIsSingleTableConstraint(pWhere, pSrcList, iSrc, 1) ){
    nChng++;
    pSubq->selFlags |= SF_PushDown;
    while( pSubq ){
