// AUTO-DRAFT from sqlite/sqlite PR #1f3e5e763dbee918ae8f8c44b84bb98f8d0fa2d6
typedef struct SessionChange SessionChange;
/* …（同文件无关代码省略）… */
    }
  }else if( bRebase ){
    if( pExist->op==SQLITE_DELETE && pExist->bIndirect ){
      *ppNew = pExist;
    }else{
      sqlite3_int64 nByte = nRec + pExist->nRecord + sizeof(SessionChange);
      pNew = (SessionChange*)sqlite3_malloc64(nByte);
/* …（同文件无关代码省略）… */
    }
  }
  sqlite3_free(sOut.aBuf);
  return rc;
}
