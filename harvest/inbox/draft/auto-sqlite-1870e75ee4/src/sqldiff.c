// AUTO-DRAFT from sqlite/sqlite PR #e2b1503ded381d24ed0edbfd8f4be9616eae995d
  }  // <<< BUG ANCHOR

  /* Build the comparison query */
  for(n2=n; az[n2]; n2++){}
  sqlite3_str_appendf(pSql, "SELECT 1, count(*)");
  if( n2==nPk2 ){
    sqlite3_str_appendf(pSql, ", 0\n");
/* …（同文件无关代码省略）… */
      sqlite3_str_appendf(pSql, "%sA.%s IS NOT B.%s", zSep, az[i], az[i]);
      zSep = " OR ";
    }
    sqlite3_str_appendf(pSql, ")\n");
  }
  sqlite3_str_appendf(pSql, "  FROM main.%s A, aux.%s B\n", zId, zId);
