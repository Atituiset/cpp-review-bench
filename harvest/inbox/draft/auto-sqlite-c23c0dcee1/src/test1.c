// AUTO-DRAFT from sqlite/sqlite PR #5ee733befea57e6a12943273fe98b4f3d722b8f6
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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
static int SQLITE_TCLAPI runAsObjProc(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  Tcl_CmdInfo cmdInfo;
  if( objc<2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "COMMAND ...");
    return TCL_ERROR;
  }
  if( !Tcl_GetCommandInfo(interp, Tcl_GetString(objv[1]), &cmdInfo) ){
    Tcl_AppendResult(interp, "command not found: ",
           Tcl_GetString(objv[1]), (char*)0);
    return TCL_ERROR;
  }
  if( cmdInfo.objProc==0 ){
    Tcl_AppendResult(interp, "command has no objProc: ",
           Tcl_GetString(objv[1]), (char*)0);
    return TCL_ERROR;
  }
  return cmdInfo.objProc(cmdInfo.objClientData, interp, objc-1, objv+1);
}
/* …（同文件无关代码省略）… */
struct win32FileLocker {
  char *evName;       /* Name of event to signal thread startup */
  sqlite3_file *pFd;  /* Handle of the file to be locked */
  int delay1;         /* Delay before locking */
  int delay2;         /* Delay before unlocking */
  int ok;             /* Finished ok */
  int err;            /* True if an error occurs */
};
/* …（同文件无关代码省略）… */
static void SQLITE_CDECL win32_file_locker(void *pAppData){
  struct win32FileLocker *p = (struct win32FileLocker*)pAppData;
  sqlite3_file *pFd = p->pFd;
  HANDLE h = INVALID_HANDLE_VALUE;
  if( p->evName ){
    HANDLE ev = OpenEvent(EVENT_MODIFY_STATE, FALSE, p->evName);
    if ( ev ){
      SetEvent(ev);
      CloseHandle(ev);
    }
  }
  if( p->delay1 ) Sleep(p->delay1);
  pFd->pMethods->xFileControl(pFd, SQLITE_FCNTL_WIN32_GET_HANDLE, (void*)&h);
  if( LockFile(h, 0, 0, 100000000, 0) ){
    Sleep(p->delay2);
    UnlockFile(h, 0, 0, 100000000, 0);
    p->ok = 1;
  }else{
    p->err = 1;
  }
  pFd->pMethods->xClose(pFd);
  sqlite3_free(pFd);
  p->pFd = 0;
  p->delay1 = 0;
  p->delay2 = 0;
}
/* …（同文件无关代码省略）… */
static int SQLITE_TCLAPI win32_file_lock(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  static struct win32FileLocker x = { "win32_file_lock", 0, 0, 0, 0, 0 };
  const char *zFilename = 0;
  Tcl_Size nFilename = 0;
  char *zTerm = 0;
  char zBuf[200];
  int retry = 0;
  HANDLE ev;
  DWORD wResult;
  sqlite3_vfs *pVfs = 0;
  int flags = SQLITE_OPEN_MAIN_DB | SQLITE_OPEN_READWRITE;
  int rc = SQLITE_OK;
  
  if( objc!=4 && objc!=1 ){
    Tcl_WrongNumArgs(interp, 1, objv, "FILENAME DELAY1 DELAY2");
    return TCL_ERROR;
  }
  if( objc==1 ){
    HANDLE h = INVALID_HANDLE_VALUE;
    if( x.pFd ){
      x.pFd->pMethods->xFileControl(
          x.pFd, SQLITE_FCNTL_WIN32_GET_HANDLE, (void*)&h
      );
    }
    sqlite3_snprintf(sizeof(zBuf), zBuf, "%d %d %d %d %d",
                     x.ok, x.err, x.delay1, x.delay2, h);
    Tcl_AppendResult(interp, zBuf, (char*)0);
    return TCL_OK;
  }
  while( x.pFd && retry<30 ){
    retry++;
    Sleep(100);
  }
  if( x.pFd ){
    Tcl_AppendResult(interp, "busy", (char*)0);
    return TCL_ERROR;
  }
  if( Tcl_GetIntFromObj(interp, objv[2], &x.delay1) ) return TCL_ERROR;
  if( Tcl_GetIntFromObj(interp, objv[3], &x.delay2) ) return TCL_ERROR;
  pVfs = sqlite3_vfs_find(0);
  x.pFd = (sqlite3_file*)sqlite3_malloc(pVfs->szOsFile);

  /* xOpen() must be passed a dual-nul-terminated string preceded in memory
  ** by 4 0x00 bytes.  */
  zFilename = Tcl_GetStringFromObj(objv[1], &nFilename);
  zTerm = (char*)sqlite3_malloc(nFilename+6);
  memset(zTerm, 0, nFilename+6);
  memcpy(&zTerm[4], zFilename, nFilename);
  rc = pVfs->xOpen(pVfs, &zTerm[4], x.pFd, flags, &flags);
  sqlite3_free(zTerm);

  if( rc!=SQLITE_OK ){
    Tcl_AppendResult(interp, "cannot open file: ", zFilename, (char*)0);
    return TCL_ERROR;
  }
  ev = CreateEvent(NULL, TRUE, FALSE, x.evName);
  if ( !ev ){
    Tcl_AppendResult(interp, "cannot create event: ", x.evName, (char*)0);
    return TCL_ERROR;
  }
  _beginthread(win32_file_locker, 0, (void*)&x);
  Sleep(0);
  if ( (wResult = WaitForSingleObject(ev, 10000))!=WAIT_OBJECT_0 ){
    sqlite3_snprintf(sizeof(zBuf), zBuf, "0x%x", wResult);
    Tcl_AppendResult(interp, "wait failed: ", zBuf, (char*)0);
    CloseHandle(ev);
    return TCL_ERROR;
  }
  CloseHandle(ev);
  return TCL_OK;
}
#endif


/*
**      optimization_control DB OPT BOOLEAN
/* …（同文件无关代码省略）… */
static int SQLITE_TCLAPI optimization_control(
  void * clientData,
  Tcl_Interp *interp,
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
    { "balanced-merge",
