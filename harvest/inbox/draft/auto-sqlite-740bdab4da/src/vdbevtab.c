// AUTO-DRAFT from sqlite/sqlite PR #3044c8b3b2be8f2fff54bb8357aa5377c01570c6
      if( i<=2 && pCur->zType==0 ){
        Schema *pSchema;
        HashElem *k;
        int iDb = pOp->p3;  // <<< BUG ANCHOR
        Pgno iRoot = (Pgno)pOp->p2;
        sqlite3 *db = pVTab->db;
        pSchema = db->aDb[iDb].pSchema;
        pCur->zSchema = db->aDb[iDb].zDbSName;
/* …（同文件无关代码省略）… */
      sqlite3_result_text(ctx, pCur->zName, -1, SQLITE_STATIC);
      break;
    case 23:  /* tables_used.wr */
      sqlite3_result_int(ctx, pOp->opcode==OP_OpenWrite);
      break;
  }
  return SQLITE_OK;
