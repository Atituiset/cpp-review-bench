// AUTO-DRAFT from sqlite/sqlite PR #98ddcd30c0e3c3f4f732ca4fc5692be0306a4224
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
#define KVRECORD_KEY_SZ 128
#endif

/*
** Which sqlite3.c we're using needs to be configurable to enable
** building against a custom copy, e.g. the SEE variant. We #include
/* …（同文件无关代码省略）… */
** us where it's happening.
*/
SQLITE_WASM_EXPORT2(const char *,sqlite3__wasm_enum_json,(void)){
  static char aBuffer[1024 * 20] =
    {0} /* where the JSON goes. 2025-09-19: output size=19295, but
           that can vary slightly from build to build, so a little
           leeway is needed here. */;
/* …（同文件无关代码省略）… */
#define lenCheck assert(zPos < zEnd - 128 \
  && "sqlite3__wasm_enum_json() buffer is too small."); \
  if( zPos >= zEnd - 128 ) return 0
/* …（同文件无关代码省略）… */
#define CloseBrace(LEVEL) \
  assert(LEVEL<5); memset(zPos, '}', LEVEL); zPos+=LEVEL; lenCheck
/* …（同文件无关代码省略）… */
#define _StructBinder CloseBrace(2)
/* …（同文件无关代码省略）… */
#define CurrentStruct sqlite3_vfs
/* …（同文件无关代码省略）… */
    } _StructBinder;
#undef CurrentStruct

    /**
     ** Workaround: in order to map the various inner structs from
     ** sqlite3_index_info, we have to uplift those into constructs we
