// AUTO-DRAFT from sqlite/sqlite PR #52a8a406eb9cda436b10842734c63050da6d8f13
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
  assert( argc==2 );

  iSlot = sqlite3_value_int(argv[0]);
  assert( iSlot<p->nSlot );
  pSlot = &p->aSlot[iSlot];

  switch( pSlot->eType ){
