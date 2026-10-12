// AUTO-DRAFT from postgres/postgres PR #c6b8af429b3592b00d3240a24c5a27077ff0dd82
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
  // <<< BUG ANCHOR
typedef struct Tuplestorestate Tuplestorestate;
/* …（同文件无关代码省略）… */

extern void tuplestore_trim(Tuplestorestate *state);

extern void tuplestore_get_stats(Tuplestorestate *state, char **max_storage_type,
								 int64 *max_space);

extern bool tuplestore_in_memory(Tuplestorestate *state);
