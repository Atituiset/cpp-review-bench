// AUTO-DRAFT from sqlite/sqlite PR #aa49b4609a46dc9795baa50a8d83838a3a1f24ca
#define size_of_array(X) ((int)(sizeof(X)/sizeof((X)[0])))
/* …（同文件无关代码省略）… */
  static struct AggFunc {
    const char *zName;
    int nArg;
    void (*xStep)(sqlite3_context*,int,sqlite3_value**);
    void (*xFinal)(sqlite3_context*);
  } aAgg[] = {
/* …（同文件无关代码省略）… */
  for(ii=0; rc==SQLITE_OK && ii<size_of_array(aAgg); ii++){
    struct AggFunc *p = &aAgg[ii];
    rc = sqlite3_create_function(
        db, p->zName, p->nArg, SQLITE_UTF8|SQLITE_INNOCUOUS, (void*)pList, 0,   // <<< BUG ANCHOR
        p->xStep, p->xFinal
    );
  }
