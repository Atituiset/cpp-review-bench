// AUTO-DRAFT from sqlite/sqlite PR #7815214b6cbc7bb43fdebd65aa05caccf445815e
  db = pWalker->pParse->db;
  n = pExpr->x.pList ? pExpr->x.pList->nExpr : 0;
  pDef = sqlite3FindFunction(db, pExpr->u.zToken, n, ENC(db), 0);
  if( NEVER(pDef==0) || (pDef->funcFlags & SQLITE_RESULT_SUBTYPE)!=0 ){  // <<< BUG ANCHOR
    pWalker->eCode = 1;
    return WRC_Abort;
  }
