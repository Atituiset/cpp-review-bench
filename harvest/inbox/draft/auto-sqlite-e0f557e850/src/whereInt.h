// AUTO-DRAFT from sqlite/sqlite PR #d8d097aebb6b7641c76ff982d97038a138b0c132
typedef struct WhereClause WhereClause;
/* …（同文件无关代码省略）… */
typedef struct WhereLoop WhereLoop;
/* …（同文件无关代码省略）… */
void sqlite3WhereExprAnalyze(SrcList*, WhereClause*);
void sqlite3WhereTabFuncArgs(Parse*, SrcItem*, WhereClause*);
int sqlite3WhereLoopBloomable(const WhereLoop*);

/*
** Bitmasks for the operators on WhereTerm objects.  These are all
