// AUTO-DRAFT from sqlite/sqlite PR #e830a56ab386d0b082c1ace30c7111b19d8ee089
    char *zSql;
    if( p->zVocabLang && p->zVocabLang[0] ){
      zSql = sqlite3_mprintf(
          "SELECT \"%w\" FROM \"%w\"",  // <<< BUG ANCHOR
          " WHERE \"%w\">=?1 AND \"%w\"=?2"
          " ORDER BY 1",
          p->zVocabWord, p->zVocabTab,
