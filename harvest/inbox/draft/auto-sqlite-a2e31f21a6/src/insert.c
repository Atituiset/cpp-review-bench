// AUTO-DRAFT from sqlite/sqlite PR #581417a9b6064434b68c0136fb7b66457d9d8721
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

  if( i ){
    if( iReg ){
      sqlite3VdbeAddOp4(v, OP_Affinity, iReg, i, 0, zColAff, i);
    }else{
      assert( sqlite3VdbeGetLastOp(v)->opcode==OP_MakeRecord
              || sqlite3VdbeDb(v)->mallocFailed );
