// AUTO-DRAFT from sqlite/sqlite PR #0e5046b30118f0ff1219c5997c18fe0e3c28bff7
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
    /* We never try to parse a trigger on a read-only database */
    assert( db->aDb[iDb].pBt==0
         || sqlite3BtreeIsReadonly(db->aDb[iDb].pBt)==0
         || sqlite3_stricmp(argv[0],"trigger")!=0 );

    if( sqlite3GetUInt32(argv[3], &db->init.newTnum)==0
     || (db->init.newTnum>pData->mxPage && pData->mxPage>0)
