// AUTO-DRAFT from sqlite/sqlite PR #bc8b0e766db7ad8f47677d49fbabedbae53896e0
# define SQLITE_C sqlite3.c /* yes, .c instead of .h. */
/* …（同文件无关代码省略）… */
#define INC__STRINGIFY_(f) #f
#define INC__STRINGIFY(f) INC__STRINGIFY_(f)
/* …（同文件无关代码省略）… */
#undef INC__STRINGIFY
#undef SQLITE_C

/*
** State for the "pseudo-stack" allocator implemented in
** sqlite3__wasm_pstack_xyz(). In order to avoid colliding with
