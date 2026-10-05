// AUTO-DRAFT from sqlite/sqlite PR #e6ff280c839fdfc7418fd5add12e8f32702a17fb
  pTab->szTabRow = 1; /* Any non-zero value works */
}
  // <<< BUG ANCHOR
/*
** Given a SELECT statement, generate a Table structure that describes
** the result set of that SELECT.
/* …（同文件无关代码省略）… */
  u64 savedFlags;

  pParse->nNestSel++;
#if SQLITE_MAX_EXPR_DEPTH>0
  if( pParse->nNestSel >= db->aLimit[SQLITE_LIMIT_EXPR_DEPTH] ){
    sqlite3ErrorMsg(pParse, "VIEWs and/or subqueries nested too deep");
    return 0;
  }
#endif
  savedFlags = db->flags;
  db->flags &= ~(u64)SQLITE_FullColNames;
  db->flags |= SQLITE_ShortColNames;
/* …（同文件无关代码省略）… */
}

/*
** This function checks if argument pFrom refers to a CTE declared by
** a WITH clause on the stack currently maintained by the parser (on the
** pParse->pWith linked list).  And if currently processing a CTE
** CTE expression, through routine checks to see if the reference is
** a recursive reference to the CTE.
**
** If pFrom matches a CTE according to either of these two above, pFrom->pSTab
** and other fields are populated accordingly.
**
** Return 0 if no match is found.
** Return 1 if a match is found.
/* …（同文件无关代码省略）… */
      pRecTerm = pRecTerm->pPrior;
    }

    pCte->zCteErr = "circular reference: %s";
    pSavedWith = pParse->pWith;
    pParse->pWith = pWith;
/* …（同文件无关代码省略）… */
      }
    }
    pParse->pWith = pWith;

    for(pLeft=pSel; pLeft->pPrior; pLeft=pLeft->pPrior);
    pEList = pLeft->pEList;
