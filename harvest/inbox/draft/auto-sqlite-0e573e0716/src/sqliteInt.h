// AUTO-DRAFT from sqlite/sqlite PR #95635a25d5a058dac504649debd970ba2f465801
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#  define UINT32_TYPE uint32_t
/* …（同文件无关代码省略）… */
#  define UINT8_TYPE uint8_t
/* …（同文件无关代码省略）… */
typedef sqlite_int64 i64;          /* 8-byte signed integer */
typedef sqlite_uint64 u64;         /* 8-byte unsigned integer */
typedef UINT32_TYPE u32;           /* 4-byte unsigned integer */
/* …（同文件无关代码省略）… */
typedef UINT8_TYPE u8;             /* 1-byte unsigned integer */
/* …（同文件无关代码省略）… */
typedef struct BusyHandler BusyHandler;
/* …（同文件无关代码省略）… */
typedef struct CollSeq CollSeq;
/* …（同文件无关代码省略）… */
typedef struct Db Db;
typedef struct DbClientData DbClientData;
/* …（同文件无关代码省略）… */
typedef struct Expr Expr;
typedef struct ExprList ExprList;
/* …（同文件无关代码省略）… */
typedef struct Index Index;
/* …（同文件无关代码省略）… */
typedef struct Lookaside Lookaside;
/* …（同文件无关代码省略）… */
typedef struct Parse Parse;
/* …（同文件无关代码省略）… */
typedef struct PreUpdate PreUpdate;
/* …（同文件无关代码省略）… */
typedef struct Savepoint Savepoint;
/* …（同文件无关代码省略）… */
typedef struct SrcList SrcList;
/* …（同文件无关代码省略）… */
typedef struct Token Token;
/* …（同文件无关代码省略）… */
typedef struct VTable VTable;
typedef struct VtabCtx VtabCtx;
/* …（同文件无关代码省略）… */
#define SQLITE_N_LIMIT (SQLITE_LIMIT_CTES+1)
/* …（同文件无关代码省略）… */
struct sqlite3 {
  sqlite3_vfs *pVfs;            /* OS Interface */
  struct Vdbe *pVdbe;           /* List of active virtual machines */
  CollSeq *pDfltColl;           /* BINARY collseq for the database encoding */
  sqlite3_mutex *mutex;         /* Connection mutex */
  Db *aDb;                      /* All backends */
  int nDb;                      /* Number of backends currently in use */
  u32 mDbFlags;                 /* flags recording internal state */
  u64 flags;                    /* flags settable by pragmas. See below */
  i64 lastRowid;                /* ROWID of most recent insert (see above) */
  i64 szMmap;                   /* Default mmap_size setting */
  u32 nSchemaLock;              /* Do not reset the schema when non-zero */
  unsigned int openFlags;       /* Flags passed to sqlite3_vfs.xOpen() */
  int errCode;                  /* Most recent error code (SQLITE_*) */
  int errByteOffset;            /* Byte offset of error in SQL statement */
  int errMask;                  /* & result codes with this before returning */
  int iSysErrno;                /* Errno value from last system error */
  u8 enc;                       /* Text encoding */
  u8 autoCommit;                /* The auto-commit flag. */
  u8 temp_store;                /* 1: file 2: memory 0: default */
  u8 mallocFailed;              /* True if we have seen a malloc failure */
  u8 bBenignMalloc;             /* Do not require OOMs if true */
  u8 dfltLockMode;              /* Default locking-mode for attached dbs */
  signed char nextAutovac;      /* Autovac setting after VACUUM if >=0 */
  u8 suppressErr;               /* Do not issue error messages if true */
  u8 vtabOnConflict;            /* Value to return for s3_vtab_on_conflict() */
  u8 isTransactionSavepoint;    /* True if the outermost savepoint is a TS */
  u8 mTrace;                    /* zero or more SQLITE_TRACE flags */
  u8 noSharedCache;             /* True if no shared-cache backends */
  u8 nSqlExec;                  /* Number of pending OP_SqlExec opcodes */
  u8 eOpenState;                /* Current condition of the connection */
  u8 nFpDigit;                  /* Significant digits to keep on double->text */
  u8 bPendingSchemaReset;       /* 1 or more schema resets pending */
  int nextPagesize;             /* Pagesize after VACUUM if >0 */
  i64 nChange;                  /* Value returned by sqlite3_changes() */
  i64 nTotalChange;             /* Value returned by sqlite3_total_changes() */
  u64 dbOptFlags;               /* Flags to enable/disable optimizations */
  int aLimit[SQLITE_N_LIMIT];   /* Limits */
  int nMaxSorterMmap;           /* Maximum size of regions mapped by sorter */
  struct sqlite3InitInfo {      /* Information used during initialization */
    Pgno newTnum;               /* Rootpage of table being initialized */
    u8 iDb;                     /* Which db file is being initialized */
    u8 busy;                    /* TRUE if initing. 2 if due to ADD COLUMN */
    unsigned orphanTrigger : 1; /* Last statement is orphaned TEMP trigger */
    unsigned imposterTable : 2; /* Building an imposter table */
    unsigned reopenMemdb : 1;   /* ATTACH is really a reopen using MemDB */
    const char **azInit;        /* "type", "name", and "tbl_name" columns */
  } init;
  int nVdbeActive;              /* Number of VDBEs currently running */
  int nVdbeRead;                /* Number of active VDBEs that read or write */
  int nVdbeWrite;               /* Number of active VDBEs that read and write */
  int nVdbeExec;                /* Number of nested calls to VdbeExec() */
  int nVDestroy;                /* Number of active OP_VDestroy operations */
  int nExtension;               /* Number of loaded extensions */
  void **aExtension;            /* Array of shared library handles */
  union {
    void (*xLegacy)(void*,const char*);   /* mTrace==SQLITE_TRACE_LEGACY */
    int (*xV2)(u32,void*,void*,void*);    /* All other mTrace values */
  } trace;
  void *pTraceArg;                        /* Argument to the trace function */
#ifndef SQLITE_OMIT_DEPRECATED
  void (*xProfile)(void*,const char*,u64);  /* Profiling function */
  void *pProfileArg;                        /* Argument to profile function */
#endif
  void *pCommitArg;                 /* Argument to xCommitCallback() */
  int (*xCommitCallback)(void*);    /* Invoked at every commit. */
  void *pRollbackArg;               /* Argument to xRollbackCallback() */
  void (*xRollbackCallback)(void*); /* Invoked at every commit. */
  void *pUpdateArg;
  void (*xUpdateCallback)(void*,int, const char*,const char*,sqlite_int64);
  void *pAutovacPagesArg;           /* Client argument to autov
