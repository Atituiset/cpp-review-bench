// AUTO-DRAFT from sqlite/sqlite PR #77f8caa273d6d30f3f0e278b2ac04715a87b37df
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdint.h>
  // <<< BUG ANCHOR
#  define UINT32_TYPE uint32_t
/* …（同文件无关代码省略）… */
typedef sqlite_uint64 u64;         /* 8-byte unsigned integer */
typedef UINT32_TYPE u32;           /* 4-byte unsigned integer */
/* …（同文件无关代码省略）… */
*/
typedef u64 tRowcnt;

/*
** The datatype used to store serial-types.  The size depends on the
** maximum length of a string or blob.
*/
#if SQLITE_MAX_LENGTH>=1073741817
typedef u64 tSerial;
#else
typedef u32 tSerial;
#endif

/*
** Estimated quantities used for query planning are stored as 16-bit
** logarithms.  For quantity X, the value stored is 10*log2(X).  This
