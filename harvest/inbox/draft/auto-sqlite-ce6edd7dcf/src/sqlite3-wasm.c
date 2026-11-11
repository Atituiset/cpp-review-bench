// AUTO-DRAFT from sqlite/sqlite PR #670fc9f9d52bcf888b7cf515299d511f46609222
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
#define SQLITE_WASM_EXPORT __attribute__((used,visibility("default")))
/* …（同文件无关代码省略）… */
** Which sqlite3.c we're using needs to be configurable to enable
** building against a custom copy, e.g. the SEE variant. We #include
** the .c file, rather than the header, so that the WASM extensions
** have access to private API internals (namely for kvvfs and
** SQLTester pieces).
**
** The caveat here is that custom variants need to account for
** exporting any necessary symbols (e.g. sqlite3_activate_see()).  We
** cannot export them from here using SQLITE_WASM_EXPORT because that
** attribute (apparently) has to be part of the function definition.
*/
#ifndef SQLITE_C
# define SQLITE_C sqlite3.c /* yes, .c instead of .h. */
/* …（同文件无关代码省略）… */
SQLITE_WASM_EXPORT void sqlite3__wasm_pstack_restore(unsigned char * p){
  assert(p>=PStack.pBegin && p<=PStack.pEnd && p>=PStack.pPos);
  assert(0==((unsigned long long)p & 0x7) /* 8-byte aligned */);
  if(p>=PStack.pBegin && p<=PStack.pEnd /*&& p>=PStack.pPos*/){
    PStack.pPos = p;
  }
}
/*
** Allocate and zero out n bytes from the pstack. Returns a pointer to
** the memory on success, 0 on error (including a negative n value). n
** is always adjusted to be a multiple of 8 and returned memory is
** always zeroed out before returning (because this keeps the client
** JS code from having to do so, and most uses of the pstack call for
** doing so).
*/
SQLITE_WASM_EXPORT2(void *,sqlite3__wasm_pstack_alloc,(int n)){
  if( n<=0 || n>(1024 * 1024/*https://sqlite.org/bugs/forumpost/4dbd096fe3*/) ){
/* …（同文件无关代码省略）… */
** If this function returns NULL then it means that the internal
** buffer is not large enough for the generated JSON and needs to be
** increased. In debug builds that will trigger an assert().
**
** 2025-09-19: for reasons entirely not understood, building with emcc
** -sMEMORY64=2 causes this function to fail (return 0). -sMEMORY64=1
** fails to compile with "tables may not be 64-bit" but does not tell
** us where it's happening.
*/
SQLITE_WASM_EXPORT2(const char *,sqlite3__wasm_enum_json,(void)){
  static char aBuffer[1024 * 24] =
