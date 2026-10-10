// AUTO-DRAFT from sqlite/sqlite PR #fc96a3c4530bbc66274133d3727a556821dddce6
typedef struct Expr Expr;
typedef struct ExprList ExprList;
/* …（同文件无关代码省略）… */
typedef struct Parse Parse;
/* …（同文件无关代码省略）… */
typedef struct SrcList SrcList;
/* …（同文件无关代码省略）… */
#define SQLITE_QueryFlattener 0x00000001 /* Query flattening */
#define SQLITE_WindowFunc     0x00000002 /* Use xInverse for window functions */
#define SQLITE_GroupByOrder   0x00000004 /* GROUPBY cover of ORDERBY */
                           /* 0x00000008 -- Available for reuse */
#define SQLITE_DistinctOpt    0x00000010 /* DISTINCT using indexes */
#define SQLITE_CoverIdxScan   0x00000020 /* Covering index scans */
#define SQLITE_OrderByIdxJoin 0x00000040 /* ORDER BY of joins via index */
/* …（同文件无关代码省略）… */
int sqlite3ExprIsConstantOrGroupBy(Parse*, Expr*, ExprList*);
int sqlite3ExprIsSingleTableConstraint(Expr*,const SrcList*,int,int);
int sqlite3ExprListIsConstant(Parse *pParse, ExprList *pList, int bNoIs);
#ifdef SQLITE_ENABLE_CURSOR_HINTS
int sqlite3ExprContainsSubquery(Expr*);
#endif
int sqlite3ExprIsInteger(const Expr*, int*, Parse*, int);
int sqlite3ExprCanBeNull(const Expr*);
int sqlite3ExprNeedsNoAffinityChange(const Expr*, char);
