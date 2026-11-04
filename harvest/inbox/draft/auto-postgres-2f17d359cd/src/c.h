// AUTO-DRAFT from postgres/postgres PR #a625e153b7d2f0e1c81955d13f2fc8de472bc082
#define pg_attribute_returns_nonnull
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
