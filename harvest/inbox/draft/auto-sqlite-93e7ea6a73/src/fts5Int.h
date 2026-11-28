// AUTO-DRAFT from sqlite/sqlite PR #850037370b7715b1a177897e72f8a21fed0cebbb
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
typedef struct Fts5Storage Fts5Storage;
/* …（同文件无关代码省略）… */
int sqlite3Fts5StorageReset(Fts5Storage *p);

void sqlite3Fts5StorageReleaseDeleteRow(Fts5Storage*);
int sqlite3Fts5StorageFindDeleteRow(Fts5Storage *p, i64 iDel);

/*
