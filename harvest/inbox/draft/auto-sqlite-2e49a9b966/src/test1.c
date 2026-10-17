// AUTO-DRAFT from sqlite/sqlite PR #c3e073420ff9e35ea80c783e3664e32449b5b703
*/
static struct Test1Global {
  int isInit;
  Tcl_Mutex m;  // <<< BUG ANCHOR
  Tcl_HashTable h;
} g;

/*
** Convert a pointer to a string, and return a pointer to a buffer containing
** a copy of that string. The buffer belongs to the global hash table and
/* …（同文件无关代码省略）… */
  if( pPtr==0 ){
    zRet = "0";
  }else{
    Tcl_MutexLock(&g.m);
    if( g.isInit==0 ){
      Tcl_InitHashTable(&g.h, TCL_STRING_KEYS);
      g.isInit = 1;
/* …（同文件无关代码省略）… */
    pEntry = Tcl_CreateHashEntry(&g.h, aKey, &dummy);
    Tcl_SetHashValue(pEntry, pPtr);
    zRet = Tcl_GetHashKey(&g.h, pEntry);
    Tcl_MutexUnlock(&g.m);
  }

  return zRet;
/* …（同文件无关代码省略）… */
  void *pRet = 0;

  if( zStr[0]!='0' ){
    Tcl_MutexLock(&g.m);
    if( g.isInit ){
      Tcl_HashEntry *pEntry = Tcl_FindHashEntry(&g.h, zStr);
      if( pEntry ){
        pRet = Tcl_GetHashValue(pEntry);
      }
    }
    Tcl_MutexUnlock(&g.m);
  }

  return pRet;
