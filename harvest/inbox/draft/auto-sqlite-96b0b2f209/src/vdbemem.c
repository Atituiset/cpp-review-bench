// AUTO-DRAFT from sqlite/sqlite PR #ac429ff80483f25c0f7d1ea02f3e21040ec388e3
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
/*
** The MEM structure is already a MEM_Real or MEM_IntReal. Try to 
** make it a MEM_Int if we can.
*/
void sqlite3VdbeIntegerAffinity(Mem *pMem){
  assert( pMem!=0 );
  assert( pMem->flags & (MEM_Real|MEM_IntReal) );
  assert( !sqlite3VdbeMemIsRowSet(pMem) );
/* …（同文件无关代码省略）… */
    ** the second condition under the assumption that addition overflow causes
    ** values to wrap around.
    */
    if( pMem->u.r==ix && ix>SMALLEST_INT64 && ix<LARGEST_INT64 ){
      pMem->u.i = ix;
      MemSetTypeFlag(pMem, MEM_Int);
    }
/* …（同文件无关代码省略）… */
int sqlite3VdbeMemIsRowSet(const Mem *pMem){
  return (pMem->flags&(MEM_Blob|MEM_Dyn))==(MEM_Blob|MEM_Dyn)
         && pMem->xDel==sqlite3RowSetDelete;
}
