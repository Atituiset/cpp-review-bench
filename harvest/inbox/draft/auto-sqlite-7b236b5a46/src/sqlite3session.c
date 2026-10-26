// AUTO-DRAFT from sqlite/sqlite PR #9902ebadb862a7af5c08e3d7c0c9869d8dad8ae4
    **
    ** Such records are technically corrupt, but the rebaser was at one
    ** point generating them. Under most circumstances this is benign, but
    ** can cause spurious SQLITE_RANGE errors when applying the changeset. */
    if( p->bPatchset==0 && p->op==SQLITE_UPDATE){
      for(i=0; i<p->nCol; i++){
        if( p->abPK[i]==0 && p->apValue[i+p->nCol]==0 ){
          sqlite3ValueFree(p->apValue[i]);
          p->apValue[i] = 0;
