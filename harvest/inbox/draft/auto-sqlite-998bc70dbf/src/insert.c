// AUTO-DRAFT from sqlite/sqlite PR #d785a1234e632131ea4f5643eb148a3824747abb
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

    VdbeNoopComment((v, "prep index %s", pIdx->zName));
    iThisCur = iIdxCur+ix;


    /* Skip partial indices for which the WHERE clause is not true */
    if( pIdx->pPartIdxWhere ){
