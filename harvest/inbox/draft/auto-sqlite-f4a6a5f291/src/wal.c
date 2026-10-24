// AUTO-DRAFT from sqlite/sqlite PR #9a9446f3b088bf3b8215311fbbdcc60a46045bed
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
# define WALTRACE(X)  if(sqlite3WalTrace) sqlite3DebugPrintf X
/* …（同文件无关代码省略）… */
#define WAL_WRITE_LOCK         0
/* …（同文件无关代码省略）… */
#define WAL_CKPT_LOCK          1
#define WAL_RECOVER_LOCK       2
#define WAL_READ_LOCK(I)       (3+(I))
/* …（同文件无关代码省略）… */
typedef struct WalIndexHdr WalIndexHdr;
/* …（同文件无关代码省略）… */
struct Wal {
  sqlite3_vfs *pVfs;         /* The VFS used to create pDbFd */
  sqlite3_file *pDbFd;       /* File handle for the database file */
  sqlite3_file *pWalFd;      /* File handle for WAL file */
  u32 iCallback;             /* Value to pass to log callback (or 0) */
  i64 mxWalSize;             /* Truncate WAL to this size upon reset */
  int nWiData;               /* Size of array apWiData */
  int szFirstBlock;          /* Size of first block written to WAL file */
  volatile u32 **apWiData;   /* Pointer to wal-index content in memory */
  u32 szPage;                /* Database page size */
  i16 readLock;              /* Which read lock is being held.  -1 for none */
  u8 syncFlags;              /* Flags to use to sync header writes */
  u8 exclusiveMode;          /* Non-zero if connection is in exclusive mode */
  u8 writeLock;              /* True if in a write transaction */
  u8 ckptLock;               /* True if holding a checkpoint lock */
  u8 readOnly;               /* WAL_RDWR, WAL_RDONLY, or WAL_SHM_RDONLY */
  u8 truncateOnCommit;       /* True to truncate WAL file on commit */
  u8 syncHeader;             /* Fsync the WAL header if true */
  u8 padToSectorBoundary;    /* Pad transactions out to the next sector */
  u8 bShmUnreliable;         /* SHM content is read-only and unreliable */
  WalIndexHdr hdr;           /* Wal-index header for current transaction */
  u32 minFrame;              /* Ignore wal frames before this one */
  u32 iReCksum;              /* On commit, recalculate checksums from here */
  const char *zWalName;      /* Name of WAL file */
  u32 nCkpt;                 /* Checkpoint sequence counter in the wal-header */
#ifdef SQLITE_USE_SEH
  u32 lockMask;              /* Mask of locks held */
  void *pFree;               /* Pointer to sqlite3_free() if exception thrown */
  u32 *pWiValue;             /* Value to write into apWiData[iWiPg] */
  int iWiPg;                 /* Write pWiValue into apWiData[iWiPg] */
  int iSysErrno;             /* System error code following exception */
#endif
#ifdef SQLITE_DEBUG
  int nSehTry;               /* Number of nested SEH_TRY{} blocks */
  u8 lockError;              /* True if a locking error has occurred */
#endif
#ifdef SQLITE_ENABLE_SNAPSHOT
  WalIndexHdr *pSnapshot;    /* Start transaction here if not NULL */
  int bGetSnapshot;          /* Transaction opened for sqlite3_get_snapshot() */
#endif
#ifdef SQLITE_ENABLE_SETLK_TIMEOUT
  sqlite3 *db;
#endif
};
/* …（同文件无关代码省略）… */
#define WAL_SHM_RDONLY  2    /* The SHM file is readonly */
/* …（同文件无关代码省略）… */
static const char *walLockName(int lockIdx){
  if( lockIdx==WAL_WRITE_LOCK ){
    return "WRITE-LOCK";
  }else if( lockIdx==WAL_CKPT_LOCK ){
    return "CKPT-LOCK";
  }else if( lockIdx==WAL_RECOVER_LOCK ){
    return "RECOVER-LOCK";
  }else{
    static char zName[15];
    sqlite3_snprintf(sizeof(zName), zName, "READ-LOCK[%d]",
                     lockIdx-WAL_READ_LOCK(0));
    return zName;
  }
}
/* …（同文件无关代码省略）… */
static int walLockShared(Wal *pWal, int lockIdx){
  int rc;
  if( pWal->exclusiveMode ) return SQLITE_OK;
  rc = sqlite3OsShmLock(pWal->pDbFd, lockIdx, 1,
                        SQLITE_SHM_LOCK | SQLITE_SHM_SHARED);
  WALTRACE(("WAL%p: acquire SHARED-%s %s\n", pWal,
            walLockName(lockIdx), rc ? "failed" : "ok"));
  VVA_ONLY( pWal->lockError = (u8)(rc!=SQLITE_OK && (rc&0xFF)!=SQLITE_BUSY); )
#ifdef SQLITE_USE_SEH
  if( rc==SQLITE_OK ) pWal->lockMask |= (1 << lockIdx);
#endif
  return rc;
}
static void walUnlockShared(Wal *pWal, int lockIdx){
  if( pWal->exclusiveMode ) return;
  (void)sqlite3OsShmLock(pWal->pDbFd, lockIdx, 1,
                         SQLITE_SHM_UNLOCK | SQLITE_SHM_SHARED);
#ifdef SQLITE_USE_SEH
  pWal->lockMask &= ~(1 << lockIdx);
#endif
  WALTRACE(("WAL%p: release SHARED-%s\n", pWal, walLockName(lockIdx)));
}
/* …（同文件无关代码省略）… */
  if( badHdr ){
    if( pWal->bShmUnreliable==0 && (pWal->readOnly & WAL_SHM_RDONLY) ){
      if( SQLITE_OK==(rc = walLockShared(pWal, WAL_WRITE_LOCK)) ){
        walUnlockShared(pWal, WAL_WRITE_LOCK);
        rc = SQLITE_READONLY_RECOVERY;
      }
    }else{
      int bWriteLock = pWal->writeLock;
