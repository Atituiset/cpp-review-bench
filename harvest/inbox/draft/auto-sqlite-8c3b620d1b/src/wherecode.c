// AUTO-DRAFT from sqlite/sqlite PR #93f9f2ff23f9692d949e7936aaa46f038ccf0daf
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

    ** For DESC, NULL entries are scanned first.
    */
    if( (pLoop->wsFlags & (WHERE_TOP_LIMIT|WHERE_BTM_LIMIT))==0
     && (pLoop->wsFlags & WHERE_BIGNULL_SORT)!=0
    ){
      assert( bSeekPastNull==0 && nExtraReg==0 && nBtm==0 && nTop==0 );
