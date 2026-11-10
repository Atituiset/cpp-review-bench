// AUTO-DRAFT from sqlite/sqlite PR #5909e4f54d42746463186e702fdf63395056844c
       && (p = geopolyFuncParam(0, pShape, &rc))!=0
       && rc==SQLITE_OK
      ){
        sqlite3_bind_blob(pUp, 2, p->hdr, 4+8*p->nVertex, SQLITE_TRANSIENT);  // <<< BUG ANCHOR
      }else{
        sqlite3_bind_value(pUp, 2, pShape);
      }
/* …（同文件无关代码省略）… */
      nChange++;
      sqlite3_bind_value(pUp, jj+2, aData[jj+2]);
    }
    if( nChange ){
      sqlite3_step(pUp);
      rc = sqlite3_reset(pUp);
    }
