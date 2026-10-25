// AUTO-DRAFT from sqlite/sqlite PR #30be364a7c4fd8e30249ceea7b5054f134656e02
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void testEnterHashMutex(){
  sqlite3_mutex_enter( sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_APP3) );
}
static void testLeaveHashMutex(){
  sqlite3_mutex_leave( sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_APP3) );
}
/* …（同文件无关代码省略）… */
static void *testStringToPointer(const char *zStr){
  void *pRet = 0;

  if( zStr[0]!='0' ){
    testEnterHashMutex();
    if( g.isInit ){
      Tcl_HashEntry *pEntry = Tcl_FindHashEntry(&g.h, zStr);
      if( pEntry ){
        pRet = Tcl_GetHashValue(pEntry);
      }
    }
    testLeaveHashMutex();
  }

  return pRet;
}
/* …（同文件无关代码省略）… */
struct SqliteDb {
  sqlite3 *db;
};
/* …（同文件无关代码省略）… */
int getDbPointer(Tcl_Interp *interp, const char *zA, sqlite3 **ppDb){
  struct SqliteDb *p;
  Tcl_CmdInfo cmdInfo;
  if( Tcl_GetCommandInfo(interp, zA, &cmdInfo) ){
    p = (struct SqliteDb*)cmdInfo.objClientData;
    *ppDb = p->db;
  }else{
    *ppDb = (sqlite3*)testStringToPointer(zA);
  }
  return TCL_OK;
}
/* …（同文件无关代码省略）… */
#define t1ErrorName sqlite3ErrName
/* …（同文件无关代码省略）… */
typedef struct TestCollationX TestCollationX;
static void testCreateCollationDel(void *pCtx){
  TestCollationX *p = (TestCollationX *)pCtx;

  int rc = Tcl_EvalObjEx(p->interp, p->pDel, TCL_EVAL_DIRECT|TCL_EVAL_GLOBAL);
  if( rc!=TCL_OK ){
    Tcl_BackgroundError(p->interp);
  }

  Tcl_DecrRefCount(p->pCmp);
  Tcl_DecrRefCount(p->pDel);
  sqlite3_free((void *)p);
}
static int testCreateCollationCmp(
  void *pCtx,
  int nLeft,
  const void *zLeft,
  int nRight,
  const void *zRight
){
  TestCollationX *p = (TestCollationX *)pCtx;
  Tcl_Obj *pScript = Tcl_DuplicateObj(p->pCmp);
  int iRes = 0;

  Tcl_IncrRefCount(pScript);
  Tcl_ListObjAppendElement(0, pScript, Tcl_NewStringObj((char *)zLeft, nLeft));
  Tcl_ListObjAppendElement(0, pScript, Tcl_NewStringObj((char *)zRight,nRight));

  if( TCL_OK!=Tcl_EvalObjEx(p->interp, pScript, TCL_EVAL_DIRECT|TCL_EVAL_GLOBAL)
   || TCL_OK!=Tcl_GetIntFromObj(p->interp, Tcl_GetObjResult(p->interp), &iRes)
  ){
    Tcl_BackgroundError(p->interp);
  }
  Tcl_DecrRefCount(pScript);

  return iRes;
}
static int SQLITE_TCLAPI test_create_collation_v2(
  ClientData clientData, /* Not used */
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int objc,              /* Number of arguments */
  Tcl_Obj *CONST objv[]  /* Command arguments */
){
  TestCollationX *p;
  sqlite3 *db;
  int rc;

  if( objc!=5 ){
    Tcl_WrongNumArgs(interp, 1, objv, "DB-HANDLE NAME CMP-PROC DEL-PROC");
    return TCL_ERROR;
  }
  if( getDbPointer(interp, Tcl_GetString(objv[1]), &db) ) return TCL_ERROR;

  p = (TestCollationX *)sqlite3_malloc(sizeof(TestCollationX));
  p->pCmp = objv[3];
  p->pDel = objv[4];
  p->interp = interp;
  Tcl_IncrRefCount(p->pCmp);
  Tcl_IncrRefCount(p->pDel);

  rc = sqlite3_create_collation_v2(db, Tcl_GetString(objv[2]), 16, 
      (void *)p, testCreateCollationCmp, testCreateCollationDel
  );
  if( rc!=SQLITE_MISUSE ){
    Tcl_AppendResult(interp, "sqlite3_create_collate_v2() failed to detect "
      "an invalid encoding", (char*)0);
    return TCL_ERROR;
  }
  rc = sqlite3_create_collation_v2(db, Tcl_GetString(objv[2]), SQLITE_UTF8, 
      (void *)p, testCreateCollationCmp, testCreateCollationDel
  );
  return TCL_OK;
}
/* …（同文件无关代码省略）… */
static int SQLITE_TCLAPI test_global_recover(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
#ifndef SQLITE_OMIT_DEPRECATED
  int rc;
  if( objc!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv, "");
    return TCL_ERROR;
  }
  rc = sqlite3_global_recover();
  Tcl_SetResult(interp, (char *)t1ErrorName(rc), TCL_STATIC);
#endif
  return TCL_OK;
}
/* …（同文件无关代码省略）… */
static int SQLITE_TCLAPI working_64bit_int(
  ClientData clientData, /* Pointer to sqlite3_enable_XXX function */
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int objc,              /* Number of arguments */
  Tcl_Obj *CONST objv[]  /* Command arguments */
){
  Tcl_Obj *pTestObj;
  int working = 0;

  pTestObj = Tcl_NewWideIntObj(1000000*(i64)1234567890);
  working = strcmp(Tcl_GetString(pTestObj), "1234567890000000")==0;
  Tcl_DecrRefCount(pTestObj);
  Tcl_SetObjResult(interp, Tcl_NewBooleanObj(working));
  return TCL_OK;
}


/*
** tclcmd:   vfs_unlink_test
**
/* …（同文件无关代码省略）… */
static int SQLITE_TCLAPI vfs_unlink_test(
  ClientData clientData, /* Pointer to sqlite3_enable_XXX function */
  Tcl_Interp *interp,    /* The TCL interpreter that invoked this command */
  int objc,              /* Number of arguments */
  Tcl_Obj *CONST objv[]  /* Command arguments */
){
  int i;
  sqlite3_vfs *pMain;
  sqlite3_vfs *apVfs[20];
  sqlite3_vfs one, two;

  sqlite3_vfs_unregister(0);   /* Unregister of NULL is harmless */
  one.zName = "__one";
  two.zName = "__two";

  /* Calling sqlite3_vfs_register with 2nd argument of 0 does not
  ** change the default VFS
  */
  pMain = sqlite3_vfs_find(0);
  sqlite3_vfs_register(&one, 0);
  assert( pMain==0 || pMain==sqlite3_vfs_find(0) );
  sqlite3_vfs_register(&two, 0);
  assert( pMain==0 || pMain==sqlite3_vfs_find(0) );

  /* We can find a VFS by its name */
  assert( sqlite3_vfs_find("__one")==&one );
  assert( sqlite3_vfs_find("__two")==&two );

  /* Calling sqlite_vfs_register with non-zero second parameter changes the
  ** default VFS, even if the 1st parameter is an existing VFS that is
  ** previously registered as the non-default.
  */
  sqlite3_vfs_register(&one, 1);
  assert( sqlite3_vfs_find("__one")==&one );
  assert( sqlite3_vfs_find("__two")==&two );
  assert( sqlite3_vfs_find(0)==&one );
  sqlite3_vfs_register(&two, 1);
  assert( sqlite3_vfs_find("__one")==&one );
  assert( sqlite3_vfs_find("__two")==&two );
  assert( sqlite3_vfs_find(0)==&two );
  if( pMain ){
    sqlite3_vfs_register(pMain, 1);
    assert( sqlite3_vfs_find("__one")==&one );
    assert( sqlite3_vfs_find("__two")==&two );
    assert( sqlite3_vfs_f
