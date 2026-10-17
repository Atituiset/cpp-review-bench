// AUTO-DRAFT from sqlite/sqlite PR #34e5f2ee47b7267846c5a627e9ae36370461e8c2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>

          assert( ExprUseWOfst(pE->pLeft) );
          iErrOfst = pE->pRight->w.iOfst;
        }else{
          assert( ExprUseWOfst(pE) );
          iErrOfst = pE->w.iOfst;
        }
/* …（同文件无关代码省略）… */
              pLeft = sqlite3Expr(db, TK_ID, zTabName);
              pExpr = sqlite3PExpr(pParse, TK_DOT, pLeft, pRight);
              if( IN_RENAME_OBJECT && pE->pLeft ){
                sqlite3RenameTokenRemap(pParse, pLeft, pE->pLeft);
              }
              if( zSchemaName ){
