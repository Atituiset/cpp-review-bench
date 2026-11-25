// AUTO-DRAFT from sqlite/sqlite PR #f852f389434dd9e7a6376dccb5e0001dac81bcf3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
struct RenameToken {
  const void *p;         /* Parse tree element created by token t */
  Token t;               /* The token that created parse tree element p */
  RenameToken *pNext;    /* Next is a list of all RenameToken objects */
};
/* …（同文件无关代码省略）… */
typedef struct RenameCtx RenameCtx;
/* …（同文件无关代码省略）… */
  int iCol;                       /* Index of column being renamed */
  Table *pTab;                    /* Table being ALTERed */
  const char *zOld;               /* Old column name */
};

#ifdef SQLITE_DEBUG
/* …（同文件无关代码省略）… */
# define renameTokenCheckAll(x,y)
/* …（同文件无关代码省略）… */
void sqlite3RenameTokenRemap(Parse *pParse, const void *pTo, const void *pFrom){
  RenameToken *p;
  renameTokenCheckAll(pParse, pTo);
  for(p=pParse->pRename; p; p=p->pNext){
    if( p->p==pFrom ){
      p->p = pTo;
      break;
    }
  }
}
/* …（同文件无关代码省略）… */
static int renameUnmapExprCb(Walker *pWalker, Expr *pExpr){
  Parse *pParse = pWalker->pParse;
  sqlite3RenameTokenRemap(pParse, 0, (const void*)pExpr);
  if( ExprUseYTab(pExpr) ){
    sqlite3RenameTokenRemap(pParse, 0, (const void*)&pExpr->y.pTab);
  }
  return WRC_Continue;
}
/* …（同文件无关代码省略）… */
static void renameWalkWith(Walker *pWalker, Select *pSelect){
  With *pWith = pSelect->pWith;
  if( pWith ){
    Parse *pParse = pWalker->pParse;
    int i;
    With *pCopy = 0;
    assert( pWith->nCte>0 );
    if( (pWith->a[0].pSelect->selFlags & SF_Expanded)==0 ){
      /* Push a copy of the With object onto the with-stack. We use a copy
      ** here as the original will be expanded and resolved (flags SF_Expanded
      ** and SF_Resolved) below. And the parser code that uses the with-stack
      ** fails if the Select objects on it have already been expanded and
      ** resolved.  */
      pCopy = sqlite3WithDup(pParse->db, pWith);
      pCopy = sqlite3WithPush(pParse, pCopy, 1);
    }
    for(i=0; i<pWith->nCte; i++){
      Select *p = pWith->a[i].pSelect;
      NameContext sNC;
      memset(&sNC, 0, sizeof(sNC));
      sNC.pParse = pParse;
      if( pCopy ) sqlite3SelectPrep(sNC.pParse, p, &sNC);
      if( sNC.pParse->db->mallocFailed ) return;
      sqlite3WalkSelect(pWalker, p);
      sqlite3RenameExprlistUnmap(pParse, pWith->a[i].pCols);
    }
    if( pCopy && pParse->pWith==pCopy ){
      pParse->pWith = pCopy->pOuter;
    }
  }
}
/* …（同文件无关代码省略）… */
void sqlite3RenameExprlistUnmap(Parse *pParse, ExprList *pEList){
  if( pEList ){
    int i;
    Walker sWalker;
    memset(&sWalker, 0, sizeof(Walker));
    sWalker.pParse = pParse;
    sWalker.xExprCallback = renameUnmapExprCb;
    sqlite3WalkExprList(&sWalker, pEList);
    for(i=0; i<pEList->nExpr; i++){
      if( ALWAYS(pEList->a[i].fg.eEName==ENAME_NAME) ){
        sqlite3RenameTokenRemap(pParse, 0, (void*)pEList->a[i].zEName);
      }
    }
  }
}
/* …（同文件无关代码省略）… */
static void renameTokenFree(sqlite3 *db, RenameToken *pToken){
  RenameToken *pNext;
  RenameToken *p;
  for(p=pToken; p; p=pNext){
    pNext = p->pNext;
    sqlite3DbFree(db, p);
  }
}
/* …（同文件无关代码省略）… */
static RenameToken *renameTokenFind(
  Parse *pParse,
  struct RenameCtx *pCtx,
  const void *pPtr
){
  RenameToken **pp;
  if( NEVER(pPtr==0) ){
    return 0;
  }
  for(pp=&pParse->pRename; (*pp); pp=&(*pp)->pNext){
    if( (*pp)->p==pPtr ){
      RenameToken *pToken = *pp;
      if( pCtx ){
        *pp = pToken->pNext;
        pToken->pNext = pCtx->pList;
        pCtx->pList = pToken;
        pCtx->nList++;
      }
      return pToken;
    }
  }
  return 0;
}

/*
** Add to pCtx the names in USING clauses of pSrc that refer to the column
** being renamed.  A USING(x) term names a column in the table on its right
** and in the first table on its left that has such a column.  The name can
** only be renamed when both of those are the table being altered, as in a
** self-join.  Otherwise, the names in the two tables would no longer match,
** and the rename fails when the schema is tested afterwards.
*/
static void renameColumnUsingNames(
  Parse *pParse,
  RenameCtx *pCtx,
  const SrcList *pSrc
){
  int i;
  const char *zOld;
  if( NEVER(pSrc==0) || pCtx->pTab==0 ) return;
  zOld = pCtx->pTab->aCol[pCtx->iCol].zCnName;
  for(i=1; i<pSrc->nSrc; i++){
    const SrcItem *pItem = &pSrc->a[i];
    int j;
    if( pItem->fg.isUsing==0 ) continue;
    if( pItem->pSTab!=pCtx->pTab ) continue;
    for(j=0; ALWAYS(j<i); j++){
      Table *pLeft = pSrc->a[j].pSTab;
      if( ALWAYS(pLeft) && sqlite3ColumnIndex(pLeft, zOld)>=0 ){
        if( pLeft==pCtx->pTab ){
          const IdList *pUsing = pItem->u3.pUsing;
          int k;
          for(k=0; k<pUsing->nId; k++){
            if( 0==sqlite3_stricmp(pUsing->a[k].zName, zOld) ){
              renameTokenFind(pParse, pCtx, (const void*)pUsing->a[k].zName);
            }
          }
        }
        break;
      }
    }
  }
}
/* …（同文件无关代码省略）… */
    testcase( p->selFlags & SF_CopyCte );
    return WRC_Prune;
  }
  renameColumnUsingNames(pWalker->pParse, pWalker->u.pRename, p->pSrc);
  renameWalkWith(pWalker, p);
  return WRC_Continue;
}
/* …（同文件无关代码省略）… */
static RenameToken *renameColumnTokenNext(RenameCtx *pCtx){
  RenameToken *pBest = pCtx->pList;
  RenameToken *pToken;
  RenameToken **pp;

  for(pToken=pBest->pNext; pToken; pToken=pToken->pNext){
    if( pToken->t.z>pBest->t.z ) pBest = pToken;
  }
  for(pp=&pCtx->pList; *pp!=pBest; pp=&(*pp)->pNext);
  *pp = pBest->pNext;

  return pBest;
}
/* …（同文件无关代码省略）… */
static int renameEditSql(
  sqlite3_context *pCtx,          /* Return result here */
  RenameCtx *pRename,             /* Rename context */
  const char *zSql,               /* SQL statement to edit */
  const char *zNew,               /* New token text */
  int bQuote                      /* True to always quote token */
){
  i64 nNew = sqlite3Strlen30(zNew);
  i64 nSql = sqlite3Strlen30(zSql);
  sqlite3 *db = sqlite3_context_db_handle(pCtx);
  
