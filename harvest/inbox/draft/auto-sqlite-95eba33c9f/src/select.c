// AUTO-DRAFT from sqlite/sqlite PR #e0b8a4a743c5922ade03a43c423720cdcda0cf98
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

  assert( pParse->db->mallocFailed==0 || pParse->nErr!=0 );
  if( nReg==0 ) return;
  if( pParse->nErr ) return;
  sqlite3VdbeAddOp3(v, OP_Null, 0, pAggInfo->iFirstReg,
                    pAggInfo->iFirstReg+nReg-1);
  for(pFunc=pAggInfo->aFunc, i=0; i<pAggInfo->nFunc; i++, pFunc++){
