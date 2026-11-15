// AUTO-DRAFT from sqlite/sqlite PR #ae2104f4dfcea4a3ac147de1a8aa2b5487f4b883
      if( sqlite3_value_type(pL)==SQLITE_TEXT ){
        sqlite3_value_text(pL); /* Make sure the encoding is UTF-8 */
      }
      res = sqlite3MemCompare(pL, pR, 0) ? 2 : 0;  // <<< BUG ANCHOR
    }
    sqlite3ValueFree(pR);
    sqlite3ValueFree(pL);
