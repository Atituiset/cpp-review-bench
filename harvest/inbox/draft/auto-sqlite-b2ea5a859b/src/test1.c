// AUTO-DRAFT from sqlite/sqlite PR #4c7729af99a9fbf54c812cbd6b14572ec8680963
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <string.h>
  // <<< BUG ANCHOR
static void *testStringToPointer(const char *zStr){
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
  int objc,
  Tcl_Obj *CONST objv[]
){
  int i;
  sqlite3 *db;
  const char *zOpt;
  int onoff;
  int mask = 0;
  int cnt = 0;
  static const struct {
    const char *zOptName;
    int mask;
  } aOpt[] = {
    { "all",                 SQLITE_AllOpts        },
    { "none",                0                     },
    { "query-flattener",     SQLITE_QueryFlattener },
    { "groupby-order",       SQLITE_GroupByOrder   },
    { "distinct-opt",        SQLITE_DistinctOpt    },
    { "cover-idx-scan",      SQLITE_CoverIdxScan   },
    { "order-by-idx-join",   SQLITE_OrderByIdxJoin },
    { "order-by-subquery",   SQLITE_OrderBySubq    },
    { "transitive",          SQLITE_Transitive     },
    { "omit-noop-join",      SQLITE_OmitNoopJoin   },
    { "stat4",               SQLITE_Stat4          },
    { "skip-scan",           SQLITE_SkipScan       },
    { "push-down",           SQLITE_PushDown       },
    { "balanced-merge",      SQLITE_BalancedMerge  },
    { "propagate-const",     SQLITE_PropagateConst },
    { "one-pass",            SQLITE_OnePass        },
    { "exists-to-join",      SQLITE_ExistsToJoin   },
    { "count-of-view",       SQLITE_CountOfView    },
  };

  if( objc!=4 ){
    Tcl_WrongNumArgs(interp, 1, objv, "DB OPT BOOLEAN");
/* …（同文件无关代码省略）… */
  if( getDbPointer(interp, Tcl_GetString(objv[1]), &db) ) return TCL_ERROR;
  if( Tcl_GetBooleanFromObj(interp, objv[3], &onoff) ) return TCL_ERROR;
  zOpt = Tcl_GetString(objv[2]);
  for(i=0; i<sizeof(aOpt)/sizeof(aOpt[0]); i++){
    if( strstr(zOpt, aOpt[i].zOptName)!=0 ){
      mask |= aOpt[i].mask;
      cnt++;
    }
  }
  if( onoff ) mask = ~mask;
  if( cnt==0 ){
    Tcl_AppendResult(interp, "unknown optimization - should be one of:",
                     (char*)0);
    for(i=0; i<sizeof(aOpt)/sizeof(aOpt[0]); i++){
      Tcl_AppendResult(interp, " ", aOpt[i].zOptName, (char*)0);
    }
    return TCL_ERROR;
  }
  sqlite3_test_control(SQLITE_TESTCTRL_OPTIMIZATIONS, db, mask);
  Tcl_SetObjResult(interp, Tcl_NewIntObj(mask));
  return TCL_OK;
}
