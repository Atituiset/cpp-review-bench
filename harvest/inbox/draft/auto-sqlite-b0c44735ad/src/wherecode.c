// AUTO-DRAFT from sqlite/sqlite PR #4226522153ecdf8a18c9498e0f12cee469398965
  if( (pParent->pWLoop->wsFlags & WHERE_AUTO_INDEX)!=0
   && (mKey & (mKey-1))==0
  ){  // <<< BUG ANCHOR
    if( pParent->addrDeleteCont==0 ){
      pParent->addrDeleteCont = sqlite3VdbeMakeLabel(pWInfo->pParse);
    }
    return pParent->addrDeleteCont;
  }

  testcase( iParent<iLevel-1 );
