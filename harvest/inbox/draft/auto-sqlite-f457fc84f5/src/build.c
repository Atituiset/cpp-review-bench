// AUTO-DRAFT from sqlite/sqlite PR #77a30f6648342615b1afa10ad750397f2f98e585
    for(i=0; i<db->nDb; i++){
      if( DbHasProperty(db, i, DB_ResetWanted) ){
        sqlite3SchemaClear(db->aDb[i].pSchema);
      }
    }
  }
