// AUTO-DRAFT from postgres/postgres PR #9dcaea6edc8c3f8a658ff788c94b6a896ae254af
#if defined(WIN32) || defined(__CYGWIN__)
#include <fcntl.h>				/* ensure O_BINARY is available */
#endif
#include <locale.h>
#ifdef HAVE_XLOCALE_H
#include <xlocale.h>
/* …（同文件无关代码省略）… */
#define __has_attribute(attribute) 0
/* …（同文件无关代码省略）… */
 *   including after either member is updated.
 *
 * The attribute is ignored in C++ due to lack of compiler support.
 */
#ifndef __cplusplus
#if __has_attribute (counted_by)
#define pg_attribute_counted_by(count) __attribute__((counted_by(count)))
#else
#define pg_attribute_counted_by(count)
#endif
/* …（同文件无关代码省略）… */
#define pg_attribute_aligned(a) __attribute__((aligned(a)))
/* …（同文件无关代码省略）… */
typedef PG_INT128_TYPE int128
#if defined(pg_attribute_aligned)
			pg_attribute_aligned(MAXIMUM_ALIGNOF)
#endif
		   ;
