// AUTO-DRAFT from sqlite/sqlite PR #54df57f9a328a78b3e61ab44dec6d73fdc5a1819
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR


/*
** If the SELECT references the table pWalker->u.pTab, then do two things:
**
**    (1) Mark the SELECT as as SF_Correlated.
**    (2) Set pWalker->eCode to non-zero so that the caller will know
**        that (1) has happened.
*/
static int sqlite3ReturningSubqueryCorrelated(Walker *pWalker, Select *pSelect){
  int i;
  SrcList *pSrc;
  assert( pSelect!=0 );
/* …（同文件无关代码省略）… */
  return WRC_Continue;
}

/*
** Scan the expression list that is the argument to RETURNING looking
** for subqueries that depend on the table which is being modified in the
