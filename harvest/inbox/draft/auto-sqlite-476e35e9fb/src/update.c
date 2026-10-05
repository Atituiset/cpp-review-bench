// AUTO-DRAFT from sqlite/sqlite PR #5e54ba5160b572c00539b1b19c05df590701a7df
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
static SQLITE_NOINLINE void columnDefaultUncommonCase(
  Vdbe *v,          /* Byte code under construction */
  Table *pTab,      /* The table */
  Column *pCol,     /* Which column of the table */
  int iReg          /* Register in which results are stored */
){
  sqlite3_value *pValue = 0;
  u8 enc = ENC(sqlite3VdbeDb(v));
/* …（同文件无关代码省略）… */
  assert( pTab->nCol>i );
  pCol = &pTab->aCol[i];
  if( pCol->iDflt ){
    columnDefaultUncommonCase(v,pTab,pCol,iReg);
  }
#ifndef SQLITE_OMIT_FLOATING_POINT
  if( pCol->affinity==SQLITE_AFF_REAL ){
