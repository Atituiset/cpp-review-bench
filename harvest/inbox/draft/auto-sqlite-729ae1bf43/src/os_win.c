// AUTO-DRAFT from sqlite/sqlite PR #5ee733befea57e6a12943273fe98b4f3d722b8f6
#define osSetEndOfFile ((BOOL(WINAPI*)(HANDLE))aSyscall[36].pCurrent)
/* …（同文件无关代码省略）… */
#define osSetFilePointerEx ((BOOL(WINAPI*)(HANDLE,LARGE_INTEGER,\
        PLARGE_INTEGER,DWORD))aSyscall[37].pCurrent)
/* …（同文件无关代码省略）… */
static int winHandleSeek(HANDLE h, sqlite3_int64 iOffset){
  int rc = SQLITE_OK;             /* Return value */
  LARGE_INTEGER x;                /* The offset */
  // <<< BUG ANCHOR
  x.QuadPart = iOffset;
  if( osSetFilePointerEx(h, x, 0, FILE_BEGIN)==0 ){
    rc = SQLITE_IOERR_SEEK;
  }
  OSTRACE(("SEEK file=%p, offset=%lld rc=%s\n", h, iOffset,sqlite3ErrName(rc)));
  return rc;
}
/* …（同文件无关代码省略）… */
static int winHandleTruncate(HANDLE h, sqlite3_int64 nByte){ 
  int rc = SQLITE_OK;             /* Return code */
  rc = winHandleSeek(h, nByte);
  if( rc==SQLITE_OK ){
    if( 0==osSetEndOfFile(h) ){
      rc = SQLITE_IOERR_TRUNCATE;
    }
  }
  return rc;
}
/* …（同文件无关代码省略）… */
  }
}

/*
** The DMS lock has not yet been taken on the shm file associated with
** pShmNode. Take the lock. Truncate the *-shm file if required.
/* …（同文件无关代码省略）… */
    if( pShmNode->isReadonly ){
      rc = SQLITE_READONLY_CANTINIT;
    }else{
      rc = winHandleTruncate(h, 0);
    }

    /* Release the EXCLUSIVE lock acquired above. */
