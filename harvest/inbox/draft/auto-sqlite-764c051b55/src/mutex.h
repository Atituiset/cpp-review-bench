// AUTO-DRAFT from sqlite/sqlite PR #be3dc6699ec31a45116750812a9567cdb0a84d7a
# define SQLITE_MUTEX_OMIT
#endif
#if SQLITE_THREADSAFE && !defined(SQLITE_MUTEX_NOOP)
#  if SQLITE_OS_UNIX
#    define SQLITE_MUTEX_PTHREADS
#  elif SQLITE_OS_WIN
#    define SQLITE_MUTEX_W32
/* …（同文件无关代码省略）… */
#    define SQLITE_MUTEX_NOOP
