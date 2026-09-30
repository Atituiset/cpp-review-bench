// AUTO-DRAFT from sqlite/sqlite PR #34615b9c1f2fa513e02911bed20fe0579a9c51cb
    pLevel->iLeftJoin = ++pParse->nMem;
    sqlite3VdbeAddOp2(v, OP_Integer, 0, pLevel->iLeftJoin);
    VdbeComment((v, "init LEFT JOIN match flag"));
  }

  /* Special case of a FROM clause subquery implemented as a co-routine */
