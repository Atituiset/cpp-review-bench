// AUTO-DRAFT from sqlite/sqlite PR #9d50c7ebfc02a8f71661797cef69d457bb22493c
static int inAnyUsingClause(
  const char *zName, /* Name we are looking for */
  SrcItem *pBase,    /* The base SrcItem.  Looking at pBase[1] and following */
  int N              /* How many SrcItems to check */
){
  while( N>0 ){
    N--;
    pBase++;
    if( pBase->fg.isUsing==0 ) continue;
    if( NEVER(pBase->u3.pUsing==0) ) continue;
    if( sqlite3IdListIndex(pBase->u3.pUsing, zName)>=0 ) return 1;
  }
  return 0;
}
/* …（同文件无关代码省略）… */
            pRight = sqlite3Expr(db, TK_ID, zName);
            if( (pTabList->nSrc>1
                 && (  (pFrom->fg.jointype & JT_LTORJ)==0
                     || (selFlags & SF_NestedFrom)!=0
                     || !inAnyUsingClause(zName,pFrom,pTabList->nSrc-i-1)
                    )
