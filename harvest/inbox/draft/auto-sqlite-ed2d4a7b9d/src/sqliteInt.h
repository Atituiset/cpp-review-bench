// AUTO-DRAFT from sqlite/sqlite PR #d54522a5b832422ab893a1e9fa7a99ab314ea4e2
typedef struct CollSeq CollSeq;
/* …（同文件无关代码省略）… */
typedef struct Expr Expr;
/* …（同文件无关代码省略）… */
typedef struct Parse Parse;
/* …（同文件无关代码省略）… */
typedef struct Token Token;
/* …（同文件无关代码省略）… */
CollSeq *sqlite3ExprCollSeq(Parse *pParse, const Expr *pExpr);
CollSeq *sqlite3ExprNNCollSeq(Parse *pParse, const Expr *pExpr);
int sqlite3ExprCollSeqMatch(Parse*,const Expr*,const Expr*);
Expr *sqlite3ExprAddCollateToken(const Parse *pParse, Expr*, const Token*, int);  // <<< BUG ANCHOR
Expr *sqlite3ExprAddCollateString(const Parse*,Expr*,const char*);
Expr *sqlite3ExprSkipCollate(Expr*);
Expr *sqlite3ExprSkipCollateAndLikely(Expr*);
int sqlite3CheckCollSeq(Parse *, CollSeq *);
