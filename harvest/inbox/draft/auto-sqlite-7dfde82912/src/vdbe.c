// AUTO-DRAFT from sqlite/sqlite PR #74675a9e87c45888a4ba2ea009a91fbc769539f8
    }else{
      rc = sqlite3VdbeMemSetStr(pDest, pBuf, len, 0,
                                sqlite3RCStrUnref);
    }
  }else{
    rc = sqlite3VdbeMemFromBtree(pC->uc.pCursor, iOffset, len, pDest);
