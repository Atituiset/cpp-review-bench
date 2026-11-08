// AUTO-DRAFT from sqlite/sqlite PR #5753ef1cb769ae3088ec3a5c22b4fdbe7e6eba8d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

      assert( pParse->iSelfTab==0 );
      if( (pNC->ncFlags & NC_InAggFunc)==0 ) break;
      if( pParse->pIdxEpr==0 ) break;
      for(pIEpr=pParse->pIdxEpr; pIEpr; pIEpr=pIEpr->pIENext){
        int iDataCur = pIEpr->iDataCur;
        if( iDataCur<0 ) continue;
