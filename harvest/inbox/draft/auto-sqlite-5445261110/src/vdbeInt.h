// AUTO-DRAFT from sqlite/sqlite PR #ac429ff80483f25c0f7d1ea02f3e21040ec388e3
double sqlite3VdbeRealValue(Mem*);
int sqlite3MemRealValueRC(Mem*, double*);
int sqlite3VdbeBooleanValue(Mem*, int ifNull);
void sqlite3VdbeIntegerAffinity(Mem*);  // <<< BUG ANCHOR
int sqlite3VdbeMemRealify(Mem*);
int sqlite3VdbeMemNumerify(Mem*);
int sqlite3VdbeMemCast(Mem*,u8,u8);
