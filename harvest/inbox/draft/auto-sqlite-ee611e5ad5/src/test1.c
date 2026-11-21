// AUTO-DRAFT from sqlite/sqlite PR #7dd00550bf588547c66f1084f531d91b28805910
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
    { "SQLITE_TESTCTRL_SORTER_MMAP",        SQLITE_TESTCTRL_SORTER_MMAP     }, 
    { "SQLITE_TESTCTRL_IMPOSTER",           SQLITE_TESTCTRL_IMPOSTER        },
    { "SQLITE_TESTCTRL_INTERNAL_FUNCTIONS", SQLITE_TESTCTRL_INTERNAL_FUNCTIONS},
    { "SQLITE_TESTCTRL_FK_NO_ACTION",       SQLITE_TESTCTRL_FK_NO_ACTION},
    { 0, 0 }
  };
  int iVerb;
/* …（同文件无关代码省略）… */
      sqlite3_test_control(SQLITE_TESTCTRL_INTERNAL_FUNCTIONS, db);
      break;
    }
    case SQLITE_TESTCTRL_LOCALTIME_FAULT: {
      int val;
      if( objc!=3 ){
/* …（同文件无关代码省略）… */
      }
      if( getDbPointer(interp, Tcl_GetString(objv[2]), &db) ) return TCL_ERROR;
      if( Tcl_GetBooleanFromObj(interp, objv[3], &val) ) return TCL_ERROR;

      sqlite3_test_control(SQLITE_TESTCTRL_FK_NO_ACTION, db, val);
      break;
    }
/* …（同文件无关代码省略）… */
    { "ATTACH_CREATE",      SQLITE_DBCONFIG_ENABLE_ATTACH_CREATE },
    { "ATTACH_WRITE",       SQLITE_DBCONFIG_ENABLE_ATTACH_WRITE },
    { "COMMENTS",           SQLITE_DBCONFIG_ENABLE_COMMENTS },
    { "FP_DIGITS",          SQLITE_DBCONFIG_FP_DIGITS },
  };
  int i;
  int v = 0;
