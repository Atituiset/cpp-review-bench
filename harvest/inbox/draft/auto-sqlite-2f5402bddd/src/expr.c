// AUTO-DRAFT from sqlite/sqlite PR #fc96a3c4530bbc66274133d3727a556821dddce6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

int sqlite3SelectWalkFail(Walker *pWalker, Select *NotUsed){
  UNUSED_PARAMETER(NotUsed);
  pWalker->eCode = 0;
  return WRC_Abort;
}
/* …（同文件无关代码省略）… */
u32 sqlite3IsTrueOrFalse(const char *zIn){
  if( sqlite3StrICmp(zIn, "true")==0  ) return EP_IsTrue;
  if( sqlite3StrICmp(zIn, "false")==0 ) return EP_IsFalse;
  return 0;
}
/* …（同文件无关代码省略）… */
int sqlite3ExprIdToTrueFalse(Expr *pExpr){
  u32 v;
  assert( pExpr->op==TK_ID || pExpr->op==TK_STRING );
  if( !ExprHasProperty(pExpr, EP_Quoted|EP_IntValue)
   && (v = sqlite3IsTrueOrFalse(pExpr->u.zToken))!=0
  ){
    pExpr->op = TK_TRUEFALSE;
    ExprSetProperty(pExpr, v);
    return 1;
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE int exprNodeIsConstantFunction(
  Walker *pWalker,
  Expr *pExpr
){
  int n;             /* Number of arguments */
  ExprList *pList;   /* List of arguments */
  FuncDef *pDef;     /* The function */
  sqlite3 *db;       /* The database */

  assert( pExpr->op==TK_FUNCTION );
  if( ExprHasProperty(pExpr, EP_TokenOnly)
   || (pList = pExpr->x.pList)==0
  ){;
    n = 0;
  }else{
    n = pList->nExpr;
    sqlite3WalkExprList(pWalker, pList);
    if( pWalker->eCode==0 ) return WRC_Abort;
  }
  db = pWalker->pParse->db;
  pDef = sqlite3FindFunction(db, pExpr->u.zToken, n, ENC(db), 0);
  if( pDef==0
   || pDef->xFinalize!=0
   || (pDef->funcFlags & (SQLITE_FUNC_CONSTANT|SQLITE_FUNC_SLOCHNG))==0
   || NEVER(ExprHasProperty(pExpr, EP_WinFunc))
  ){
    pWalker->eCode = 0;
    return WRC_Abort;
  }
  return WRC_Prune;
}
/* …（同文件无关代码省略）… */
static int exprNodeIsConstant(Walker *pWalker, Expr *pExpr){
  assert( pWalker->eCode>0 );

  /* If pWalker->eCode is 2 then any term of the expression that comes from
  ** the ON or USING clauses of an outer join disqualifies the expression
  ** from being considered constant. */
  if( pWalker->eCode==2 && ExprHasProperty(pExpr, EP_OuterON) ){
    pWalker->eCode = 0;
    return WRC_Abort;
  }

  switch( pExpr->op ){
    case TK_FUNCTION:
      if( ExprHasProperty(pExpr, EP_WinFunc) ){
        pWalker->eCode = 0;  /* Window functions are never constant */
        return WRC_Abort;
      }else if( pWalker->eCode>=4 && pWalker->eCode<=5 ){
        /* Functions used within a CREATE TABLE statement are constant as
        ** long as all of their arguments are constant */
        if( pWalker->eCode==5 ) ExprSetProperty(pExpr, EP_FromDDL);
        return WRC_Continue;
      }else if( ExprHasProperty(pExpr,EP_ConstFunc) ){
        /* Functions marked with EP_ConstFunc are constant as long as 
        ** all their arugments are constant and they are not window functions */
        return WRC_Continue;
      }else if( pWalker->pParse ){
        /* Functions are constant if they have SQLITE_FUNC_CONSTANT or
        ** SQLITE_FUNC_SLOCHNG and if all arguments are constant */
        return exprNodeIsConstantFunction(pWalker, pExpr);
      }else{
        pWalker->eCode = 0;  /* Function call is not constant */
        return WRC_Abort;
      }
    case TK_ID:
      /* Convert "true" or "false" in a DEFAULT clause into the
      ** appropriate TK_TRUEFALSE operator */
      if( sqlite3ExprIdToTrueFalse(pExpr) ){
        return WRC_Prune;
      }
      /* no break */ deliberate_fall_through
    case TK_COLUMN:
    case TK_AGG_FUNCTION:
    case TK_AGG_COLUMN:
      testcase( pExpr->op==TK_ID );
      testcase( pExpr->op==TK_COLUMN );
      testcase( pExpr->op==TK_AGG_FUNCTION );
      testcase( pExpr->op==TK_AGG_COLUMN );
      if( ExprHasProperty(pExpr, EP_FixedCol) && pWalker->eCode!=2 ){
        return WRC_Continue;
      }
      if( pWalker->eCode==3 && pExpr->iTable==pWalker->u.iCur ){
        return WRC_Continue;
      }
      /* no break */ deliberate_fall_through
    case TK_IF_NULL_ROW:
    case TK_REGISTER:
    case TK_DOT:
    case TK_RAISE:
      testcase( pExpr->op==TK_REGISTER );
      testcase( pExpr->op==TK_IF_NULL_ROW );
      testcase( pExpr->op==TK_DOT );
      testcase( pExpr->op==TK_RAISE );
      pWalker->eCode = 0;
      return WRC_Abort;
    case TK_IS:
    case TK_ISNOT:
      if( pWalker->eCode==6 ){
        pWalker->eCode = 0;
        return WRC_Abort;
      }
      return WRC_Continue;
    case TK_VARIABLE:
      if( pWalker->eCode==5 ){
        /* Silently convert bound parameters that appear inside of CREATE
        ** statements into a NULL when parsing the CREATE statement text out
        ** of the sqlite_schema table */
        pExpr->op = TK_NULL;
      }else if( pWalker->eCode==4 ){
        /* A bound parameter in a CREATE statement that originates from
        ** sqlite3_prepare() causes an error */
        pWalker->eCode = 0;
        return WRC_Abort;
      }
      /* no break */ deliberate_fall_through
    default:
      testcase( pExpr->op==TK_SELECT ); /* sqlite3SelectWalkFail() disallows */
      testcase( pExpr->op==TK_EXISTS ); /* sqlite3SelectWalkFail() disallows */
      return WRC_Continue;
  }
}
static int exprIsConst(Parse *pParse, Expr *p, int initFlag){
  Walker w;
  w.eCode = initFlag;
  w.pParse = pParse;
  w.xExprCallback = exprNodeIsConstant;
  w.xSelectCallback = sqlite3SelectWalkFail;
#ifdef SQLITE_DEBUG
  w.xSelectCallback2 = sqlite3SelectWalkAssert2;
#endif
  sqlite3WalkExpr(&w, p);
  return w.eCode;
}
/* …（同文件无关代码省略）… */
  return exprIsConst(0, p, 4+isInit);
}

#ifdef SQLITE_ENABLE_CURSOR_HINTS
/*
** Walk an expression tree.  Return 1 if the expression contains a
** subquery of some kind.  Return 0 if there are no subqueries.
/* …（同文件无关代码省略）… */
  sqlite3WalkExpr(&w, p);
  return w.eCode==0;
}
#endif

/*
** If the expression p codes a constant integer between 0 and 0x7fffffff,
