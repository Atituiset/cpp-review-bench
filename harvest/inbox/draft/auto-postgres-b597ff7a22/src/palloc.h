// AUTO-DRAFT from postgres/postgres PR #23192ac644e2a8f0597692aacb269321ee5acd99
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct MemoryContextData *MemoryContext;
/* …（同文件无关代码省略）… */
/*
 * Fundamental memory-allocation operations (more are in utils/memutils.h)
 */
extern void *MemoryContextAlloc(MemoryContext context, Size size);
extern void *MemoryContextAllocZero(MemoryContext context, Size size);
extern void *MemoryContextAllocExtended(MemoryContext context,
										Size size, int flags);
extern void *MemoryContextAllocAligned(MemoryContext context,
									   Size size, Size alignto, int flags);

extern void *palloc(Size size);
extern void *palloc0(Size size);
extern void *palloc_extended(Size size, int flags);
extern void *palloc_aligned(Size size, Size alignto, int flags);
pg_nodiscard extern void *repalloc(void *pointer, Size size);
pg_nodiscard extern void *repalloc_extended(void *pointer,
											Size size, int flags);
pg_nodiscard extern void *repalloc0(void *pointer, Size oldsize, Size size);
extern void pfree(void *pointer);

/*
 * Support for safe calculation of memory request sizes
 */
extern Size add_size(Size s1, Size s2);
extern Size mul_size(Size s1, Size s2);
extern void *palloc_mul(Size s1, Size s2);
extern void *palloc0_mul(Size s1, Size s2);
extern void *palloc_mul_extended(Size s1, Size s2, int flags);
pg_nodiscard extern void *repalloc_mul(void *p, Size s1, Size s2);
pg_nodiscard extern void *repalloc_mul_extended(void *p, Size s1, Size s2,
												int flags);

/* …（同文件无关代码省略）… */
#define repalloc_array_extended(pointer, type, count, flags) ((type *) repalloc_mul_extended(pointer, sizeof(type), count, flags))

/* Higher-limit allocators. */
extern void *MemoryContextAllocHuge(MemoryContext context, Size size);
pg_nodiscard extern void *repalloc_huge(void *pointer, Size size);

/*
 * Although this header file is nominally backend-only, certain frontend
/* …（同文件无关代码省略）… */
 * These are like standard strdup() except the copied string is
 * allocated in a context, not with malloc().
 */
extern char *MemoryContextStrdup(MemoryContext context, const char *string);
extern char *pstrdup(const char *in);
extern char *pnstrdup(const char *in, Size len);

extern char *pchomp(const char *in);

/* sprintf into a palloc'd buffer --- these are in psprintf.c */
extern char *psprintf(const char *fmt, ...) pg_attribute_printf(1, 2);
extern size_t pvsnprintf(char *buf, size_t len, const char *fmt, va_list args) pg_attribute_printf(3, 0);

#endif							/* PALLOC_H */
