// AUTO-DRAFT from sqlite/sqlite PR #b642a1d56d3e9dea126a05be03813a99589192eb
          ** pointers to the schema object just freed by sqlite3BtreeClose() */
          sqlite3VtabEponymousTableClearAll(db);
        }
      }else{
        sqlite3BtreeClose(pNewBt);
        rc = SQLITE_NOMEM;
