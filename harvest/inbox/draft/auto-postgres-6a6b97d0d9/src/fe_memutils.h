// AUTO-DRAFT from postgres/postgres PR #23192ac644e2a8f0597692aacb269321ee5acd99
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
 * "Safe" memory allocation functions --- these exit(1) on failure
 * (except pg_malloc_extended with MCXT_ALLOC_NO_OOM)
 */
extern char *pg_strdup(const char *in);
extern void *pg_malloc(size_t size);
extern void *pg_malloc0(size_t size);
extern void *pg_malloc_extended(size_t size, int flags);
extern void *pg_realloc(void *ptr, size_t size);
extern void pg_free(void *ptr);

/*
 * Support for safe calculation of memory request sizes
 */
extern Size add_size(Size s1, Size s2);
extern Size mul_size(Size s1, Size s2);
extern void *pg_malloc_mul(Size s1, Size s2);
extern void *pg_malloc0_mul(Size s1, Size s2);
extern void *pg_malloc_mul_extended(Size s1, Size s2, int flags);
extern void *pg_realloc_mul(void *p, Size s1, Size s2);

/*
 * Variants with easier notation and more type safety
/* …（同文件无关代码省略）… */
#define pg_realloc_array(pointer, type, count) ((type *) pg_realloc_mul(pointer, sizeof(type), count))

/* Equivalent functions, deliberately named the same as backend functions */
extern char *pstrdup(const char *in);
extern char *pnstrdup(const char *in, Size size);
extern void *palloc(Size size);
extern void *palloc0(Size size);
extern void *palloc_extended(Size size, int flags);
extern void *repalloc(void *pointer, Size size);
extern void pfree(void *pointer);
extern void *palloc_mul(Size s1, Size s2);
extern void *palloc0_mul(Size s1, Size s2);
extern void *palloc_mul_extended(Size s1, Size s2, int flags);
extern void *repalloc_mul(void *p, Size s1, Size s2);

#define palloc_object(type) ((type *) palloc(sizeof(type)))
#define palloc0_object(type) ((type *) palloc0(sizeof(type)))
/* …（同文件无关代码省略）… */
#define repalloc_array(pointer, type, count) ((type *) repalloc_mul(pointer, sizeof(type), count))

/* sprintf into a palloc'd buffer --- these are in psprintf.c */
extern char *psprintf(const char *fmt, ...) pg_attribute_printf(1, 2);
extern size_t pvsnprintf(char *buf, size_t len, const char *fmt, va_list args) pg_attribute_printf(3, 0);

#endif							/* FE_MEMUTILS_H */
