// AUTO-DRAFT from sqlite/sqlite PR #28e05e55f68d84c08e023aca71a5ecbad274be55
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
        p->pSub = sqlite3ExprListAppend(pParse, p->pSub, pDup);
      }
      if( p->pSub ){
        int f = pExpr->flags & EP_Collate;
        assert( ExprHasProperty(pExpr, EP_Static)==0 );
        ExprSetProperty(pExpr, EP_Static);
        sqlite3ExprDelete(pParse->db, pExpr);
/* …（同文件无关代码省略）… */
        pExpr->iTable = p->pWin->iEphCsr;
        pExpr->y.pTab = p->pTab;
        pExpr->flags = f;
      }
      if( pParse->db->mallocFailed ) return WRC_Abort;
      break;
