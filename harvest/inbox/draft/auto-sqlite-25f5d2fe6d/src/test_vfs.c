// AUTO-DRAFT from sqlite/sqlite PR #19e832b3b32a7fa0e6af88882b04907d7aac2849
      { SQLITE_FCNTL_BEGIN_ATOMIC_WRITE, "BEGIN_ATOMIC_WRITE" },
      { SQLITE_FCNTL_COMMIT_ATOMIC_WRITE, "COMMIT_ATOMIC_WRITE" },
      { SQLITE_FCNTL_ZIPVFS, "ZIPVFS" },
    };
    int i;
    for(i=0; i<sizeof(aF)/sizeof(aF[0]); i++){
