// AUTO-DRAFT from sqlite/sqlite PR #1778ff4dabcc90645f573e11aee688cbe406409d
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
int sqlite3VdbeMemIsRowSet(const Mem *pMem){
  return (pMem->flags&(MEM_Blob|MEM_Dyn))==(MEM_Blob|MEM_Dyn)
         && pMem->xDel==sqlite3RowSetDelete;
}
/* …（同文件无关代码省略）… */
  i64 nByte = n;      /* New value for pMem->n */
  int iLimit;         /* Maximum allowed string or blob size */
  u16 flags;          /* New value for pMem->flags */

  assert( pMem!=0 );
  assert( pMem->db==0 || sqlite3_mutex_held(pMem->db->mutex) );
  assert( !sqlite3VdbeMemIsRowSet(pMem) );
  assert( enc!=0 || n>=0 );

  /* If z is a NULL pointer, set pMem to contain an SQL NULL. */
  if( !z ){
/* …（同文件无关代码省略）… */
    iLimit = SQLITE_MAX_LENGTH;
  }
  if( nByte<0 ){
    assert( enc!=0 );
    if( enc==SQLITE_UTF8 ){
      nByte = strlen(z);
    }else{
      for(nByte=0; nByte<=iLimit && (z[nByte] | z[nByte+1]); nByte+=2){}
    }
    flags= MEM_Str|MEM_Term;
  }else if( enc==0 ){
    flags = MEM_Blob;
    enc = SQLITE_UTF8;
  }else{
    flags = MEM_Str;
  }
/* …（同文件无关代码省略）… */
  if( xDel==SQLITE_TRANSIENT ){
    i64 nAlloc = nByte;
    if( flags&MEM_Term ){
      nAlloc += (enc==SQLITE_UTF8?1:2);
    }
    testcase( nAlloc==0 );
    testcase( nAlloc==31 );
/* …（同文件无关代码省略）… */

  pMem->n = (int)(nByte & 0x7fffffff);
  pMem->flags = flags;
  pMem->enc = enc;

#ifndef SQLITE_OMIT_UTF16
  /* tag-20261016-1:  To retain the BOM, use enc==SQLITE_UTF8, even
  ** for UTF-16 text, and then set pMem->enc correctly after this
  ** function returns.  Note, however, that n must be non-negative
  ** for this trick to work. */
  if( enc>SQLITE_UTF8 && sqlite3VdbeMemHandleBom(pMem) ){
    return SQLITE_NOMEM_BKPT;
  }
#endif
