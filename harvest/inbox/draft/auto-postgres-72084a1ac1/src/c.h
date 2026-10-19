// AUTO-DRAFT from postgres/postgres PR #23192ac644e2a8f0597692aacb269321ee5acd99
#define pg_attribute_nonnull(...)
#endif

/*
 * pg_attribute_target allows specifying different target options that the
 * function should be compiled with (e.g., for using special CPU instructions).
/* …（同文件无关代码省略）… */
#define pg_attribute_aligned(a) __attribute__((aligned(a)))
/* …（同文件无关代码省略）… */
typedef PG_INT128_TYPE int128
#if defined(pg_attribute_aligned)
			pg_attribute_aligned(MAXIMUM_ALIGNOF)
#endif
		   ;
