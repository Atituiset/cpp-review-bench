// AUTO-DRAFT from sqlite/sqlite PR #dac1b8dc849b379f7540f91c28191b35c0e9e86c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>

** The number of different kinds of things that can be limited
** using the sqlite3_limit() interface.
*/
#define SQLITE_N_LIMIT (SQLITE_LIMIT_TRIGGER_STEPS+1)

/*
** Lookaside malloc is a set of fixed-size buffers that can be used
