// AUTO-DRAFT from sqlite/sqlite PR #8a24548dfbe71bc2714079e7549d1814002f5c44
   0x7ffffffe,                /* iOnceResetThreshold */
   SQLITE_DEFAULT_SORTERREF_SIZE,   /* szSorterRef */
   0,                         /* iPrngSeed */
#ifdef SQLITE_DEBUG
   {0,0,0,0,0,0},             /* aTune */
#endif
};

/*
/* …（同文件无关代码省略）… */
sqlite3_uint64 sqlite3NProfileCnt = 0;
#endif

/*
** The value of the "pending" byte must be 0x40000000 (1 byte past the
** 1-gibabyte boundary) in a compatible database.  SQLite never uses
