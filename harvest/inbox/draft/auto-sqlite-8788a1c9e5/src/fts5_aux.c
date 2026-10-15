// AUTO-DRAFT from sqlite/sqlite PR #fff257d3ed2c2d5f77a052d14e931b260d4828a5
  return rc;
}
  // <<< BUG ANCHOR


/*************************************************************************
** Start of highlight() implementation.
/* …（同文件无关代码省略）… */
static int fts5ValueClampedInt(sqlite3_value *pVal){
  return (int)MIN(MAX(sqlite3_value_int64(pVal), -1), 0x7FFFFFFF);
}
/* …（同文件无关代码省略）… */
    sqlite3_free(ctx.zOut);
  }
  if( rc!=SQLITE_OK ){
    sqlite3_result_error_code(pCtx, rc);
  }
}
/*
/* …（同文件无关代码省略）… */
  if( rc==SQLITE_OK ){
    sqlite3_result_text(pCtx, (const char*)ctx.zOut, -1, SQLITE_TRANSIENT);
  }else{
    sqlite3_result_error_code(pCtx, rc);
  }
  sqlite3_free(ctx.zOut);
  sqlite3_free(aSeen);
/* …（同文件无关代码省略）… */
    }
    sqlite3_result_double(pCtx, -1.0 * score);
  }else{
    sqlite3_result_error_code(pCtx, rc);
  }
}

/* …（同文件无关代码省略）… */

  iCol = fts5ValueClampedInt(apVal[0]);
  if( iCol<0 || iCol>=pApi->xColumnCount(pFts) ){
    sqlite3_result_error_code(pCtx, SQLITE_RANGE);
    return;
  }

  rc = pApi->xColumnLocale(pFts, iCol, &zLocale, &nLocale);
  if( rc!=SQLITE_OK ){
    sqlite3_result_error_code(pCtx, rc);
    return;
  }
