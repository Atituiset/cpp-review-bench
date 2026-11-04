// AUTO-DRAFT from sqlite/sqlite PR #936f40746b6a4b14ed3a5428ca7fa5bf2d7d9a2a
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stdlib.h>

typedef struct PagerSavepoint PagerSavepoint;
/* …（同文件无关代码省略）… */
struct Pager {
  sqlite3_vfs *pVfs;          /* OS functions to use for IO */
  u8 exclusiveMode;           /* Boolean. True if locking_mode==EXCLUSIVE */
  u8 journalMode;             /* One of the PAGER_JOURNALMODE_* values */
  u8 useJournal;              /* Use a rollback journal on this file */
  u8 noSync;                  /* Do not sync the journal if true */
  u8 fullSync;                /* Do extra syncs of the journal for robustness */
  u8 extraSync;               /* sync directory after journal delete */
  u8 syncFlags;               /* SYNC_NORMAL or SYNC_FULL otherwise */
  u8 walSyncFlags;            /* See description above */
  u8 tempFile;                /* zFilename is a temporary or immutable file */
  u8 noLock;                  /* Do not lock (except in WAL mode) */
  u8 readOnly;                /* True for a read-only database */
  u8 memDb;                   /* True to inhibit all file I/O */
  u8 memVfs;                  /* VFS-implemented memory database */

  /**************************************************************************
  ** The following block contains those class members that change during
  ** routine operation.  Class members not in this block are either fixed
  ** when the pager is first created or else only change when there is a
  ** significant mode change (such as changing the page_size, locking_mode,
  ** or the journal_mode).  From another view, these class members describe
  ** the "state" of the pager, while other class members describe the
  ** "configuration" of the pager.
  */
  u8 eState;                  /* Pager state (OPEN, READER, WRITER_LOCKED..) */
  u8 eLock;                   /* Current lock held on database file */
  u8 changeCountDone;         /* Set after incrementing the change-counter */
  u8 setSuper;                /* Super-jrnl name is written into jrnl */
  u8 doNotSpill;              /* Do not spill the cache when non-zero */
  u8 subjInMemory;            /* True to use in-memory sub-journals */
  u8 bUseFetch;               /* True to use xFetch() */
  u8 hasHeldSharedLock;       /* True if a shared lock has ever been held */
  Pgno dbSize;                /* Number of pages in the database */
  Pgno dbOrigSize;            /* dbSize before the current transaction */
  Pgno dbFileSize;            /* Number of pages in the database file */
  Pgno dbHintSize;            /* Value passed to FCNTL_SIZE_HINT call */
  int errCode;                /* One of several kinds of errors */
  int nRec;                   /* Pages journalled since last j-header written */
  u32 cksumInit;              /* Quasi-random value added to every checksum */
  u32 nSubRec;                /* Number of records written to sub-journal */
  Bitvec *pInJournal;         /* One bit for each page in the database file */
  sqlite3_file *fd;           /* File descriptor for database */
  sqlite3_file *jfd;          /* File descriptor for main journal */
  sqlite3_file *sjfd;         /* File descriptor for sub-journal */
  i64 journalOff;             /* Current write offset in the journal file */
  i64 journalHdr;             /* Byte offset to previous journal header */
  sqlite3_backup *pBackup;    /* Pointer to list of ongoing backup processes */
  PagerSavepoint *aSavepoint; /* Array of active savepoints */
  int nSavepoint;             /* Number of elements in aSavepoint[] */
  u32 iDataVersion;           /* Changes whenever database content changes */
  char dbFileVers[16];        /* Changes whenever database file changes */

  int nMmapOut;               /* Number of mmap pages currently outstanding */
  sqlite3_int64 szMmap;       /* Desired maximum mmap size */
  PgHdr *pMmapFreelist;       /* List of free mmap page headers (pDirty) */
  /*
  ** End of the routinely-changing class members
  ***************************************************************************/

  u16 nExtra;                 /* Add this many bytes to each in-memory page */
  i16 nReserve;               /* Number of unused bytes at end of each page */
  u32 vfsFlags;               /* Flags for sqlite3_vfs.xOpen() */
  u32 sectorSize;             /* Assumed sector size during rollback */
  Pgno mxPgno;                /* Maximum allowed size of the database */
  Pgno lckPgno;               /* Page number for the locking page */
  i64 pageSize;               /* Number of bytes in a page */
  i64 journalSizeLimit;       /* Size limit for persistent journal files */
  char *zFilename;            /* Name of the database file */
  char *zJournal;             /* Name of the journal file */
  int (*xBusyHandler)(void*); /* Function to call when busy */
  void *pBusyHandlerArg;      /* Context argument for xBusyHandler */
  u32 aStat[4];               /* Total cache hits, misses, writes, spills */
#ifdef SQLITE_TEST
  int nRead;                  /* Database pages read */
#endif
  void (*xReiniter)(DbPage*); /* Call this routine when reloading pages */
  int (*xGet)(Pager*,Pgno,DbPage**,int); /* Routine to fetch a patch */
  char *pTmpSpace;            /* Pager.pageSize bytes of space for tmp use */
  PCache *pPCache;            /* Pointer to page cache object */
#ifndef SQLITE_OMIT_WAL
  Wal *pWal;                  /* Write-ahead log used by "journal_mode=wal" */
  char *zWal;                 /* File name for write-ahead log */
#endif
#ifdef SQLITE_ENABLE_SETLK_TIMEOUT
  sqlite3 *dbWal;
#endif
};
/* …（同文件无关代码省略）… */
**      file has been created already (via a spill on pagerStress()) and
**      when the number of dirty pages in memory exceeds 25% of the total
**      cache size.
*/
static int pagerFlushOnCommit(Pager *pPager, int bCommit){
  if( pPager->tempFile==0 ) return 1;
  if( !bCommit ) return 0;
  if( !isOpen(pPager->fd) ) return 0;
  return (sqlite3PCachePercentDirty(pPager->pPCache)>=25);
}
