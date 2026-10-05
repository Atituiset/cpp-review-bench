// AUTO-DRAFT from sqlite/sqlite PR #d54522a5b832422ab893a1e9fa7a99ab314ea4e2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
** and the pExpr parameter is returned unchanged.
*/
Expr *sqlite3ExprAddCollateToken(
  const Parse *pParse,     /* Parsing context */
  Expr *pExpr,             /* Add the "COLLATE" clause to this expression */
  const Token *pCollName,  /* Name of collating sequence */
  int dequote              /* True to dequote pCollName */
){
  if( pCollName->n>0 ){
    Expr *pNew = sqlite3ExprAlloc(pParse->db, TK_COLLATE, pCollName, dequote);
    if( pNew ){
      pNew->pLeft = pExpr;
      pNew->flags |= EP_Collate|EP_Skip;
      pExpr = pNew;
    }
  }
  return pExpr;
}
Expr *sqlite3ExprAddCollateString(
  const Parse *pParse,  /* Parsing context */
  Expr *pExpr,          /* Add the "COLLATE" clause to this expression */
  const char *zC        /* The collating sequence name */
){
/* …（同文件无关代码省略）… */
Expr *sqlite3ExprAlloc(
  sqlite3 *db,            /* Handle for sqlite3DbMallocRawNN() */
  int op,                 /* Expression opcode */
  const Token *pToken,    /* Token argument.  Might be NULL */
  int dequote             /* True to dequote */
){
  Expr *pNew;
  int nExtra = pToken ? pToken->n+1 : 0;

  assert( db!=0 );
  pNew = sqlite3DbMallocRawNN(db, sizeof(Expr)+nExtra);
  if( pNew ){
    memset(pNew, 0, sizeof(Expr));
    pNew->op = (u8)op;
    pNew->iAgg = -1;
    if( nExtra ){
      assert( pToken!=0 );
      pNew->u.zToken = (char*)&pNew[1];
      assert( pToken->z!=0 || pToken->n==0 );
      if( pToken->n ) memcpy(pNew->u.zToken, pToken->z, pToken->n);
      pNew->u.zToken[pToken->n] = 0;
      if( dequote && sqlite3Isquote(pNew->u.zToken[0]) ){
        sqlite3DequoteExpr(pNew);
      }
    }
#if SQLITE_MAX_EXPR_DEPTH>0
    pNew->nHeight = 1;
#endif 
  }
  return pNew;
}
