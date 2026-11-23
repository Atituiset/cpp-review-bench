// AUTO-DRAFT from sqlite/sqlite PR #dc9b3226d01620a2087811a142f9cfb03b22728c
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>

/*
** Macros for testing whether or not optimizations are enabled or disabled.
*/
#define OptimizationDisabled(D,M)  (((D)->dbOptFlags&(1ull<<((M)-1)))!=0)
#define OptimizationEnabled(D,M)   (((D)->dbOptFlags&(1ull<<((M)-1)))==0)

/*
** Return true if it OK to factor constant expressions into the initialization
