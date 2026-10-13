// AUTO-DRAFT from sqlite/sqlite PR #4226522153ecdf8a18c9498e0f12cee469398965
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdint.h>

#  define UINT8_TYPE uint8_t
/* …（同文件无关代码省略）… */
typedef UINT8_TYPE u8;             /* 1-byte unsigned integer */
/* …（同文件无关代码省略）… */
typedef struct Expr Expr;
typedef struct ExprList ExprList;
/* …（同文件无关代码省略）… */
typedef struct Parse Parse;
/* …（同文件无关代码省略）… */
typedef struct SrcList SrcList;
/* …（同文件无关代码省略）… */
int sqlite3ExprIsConstant(Parse*,Expr*);
int sqlite3ExprIsConstantOrFunction(Expr*, u8);
int sqlite3ExprIsConstantOrGroupBy(Parse*, Expr*, ExprList*);
int sqlite3ExprIsSingleTableConstraint(Expr*,const SrcList*,int,int);
int sqlite3ExprListIsConstant(Parse *pParse, ExprList *pList, int bNoIs);
int sqlite3ExprContainsSubquery(Expr*);
