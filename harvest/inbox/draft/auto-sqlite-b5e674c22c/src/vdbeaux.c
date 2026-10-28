// AUTO-DRAFT from sqlite/sqlite PR #076ab6c28636bb73ff84c41d3ddbb59c78d46a7c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

#define checkActiveVdbeCnt(x)
#endif

/*
** If the Vdbe passed as the first argument opened a statement-transaction,
** close it now. Argument eOp must be either SAVEPOINT_ROLLBACK or
/* …（同文件无关代码省略）… */
    sqlite3ConnectionUnlocked(db);
  }

  assert( db->nVdbeActive>0 || db->autoCommit==0 || db->nStatement==0 );
  return (p->rc==SQLITE_BUSY ? SQLITE_BUSY : SQLITE_OK);
}
/* …（同文件无关代码省略）… */
  }
}

/*
** Return the database associated with the Vdbe.
*/
