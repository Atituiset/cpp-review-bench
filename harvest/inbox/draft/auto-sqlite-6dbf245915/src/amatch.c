// AUTO-DRAFT from sqlite/sqlite PR #90ba7148c6d943ec5233002927c18e57cc8a5589
**
** returns the allowed vocabulary for the match.  If the "vocabulary_language"
** or L columnname is left unspecified or is an empty string, then no
** filtering of the vocabulary by language is performed. 
**
** For efficiency, it is essential that the vocabulary table be indexed:
**
/* …（同文件无关代码省略）… */
}
  // <<< BUG ANCHOR

/*
** Advance a cursor to its next row of output
*/
/* …（同文件无关代码省略）… */

  if( p->pVCheck==0 ){
    char *zSql;
    if( p->zVocabLang && p->zVocabLang[0] ){
      zSql = sqlite3_mprintf(
          "SELECT \"%w\" FROM \"%w\"",
          " WHERE \"%w\">=?1 AND \"%w\"=?2"
          " ORDER BY 1",
          p->zVocabWord, p->zVocabTab,
          p->zVocabWord, p->zVocabLang
      );
    }else{
      zSql = sqlite3_mprintf(
          "SELECT \"%w\" FROM \"%w\""
          " WHERE \"%w\">=?1"
          " ORDER BY 1",
          p->zVocabWord, p->zVocabTab,
          p->zVocabWord
      );
    }
