// AUTO-DRAFT from sqlite/sqlite PR #09a527a30d7200a06edb17690e1daed3c4bc9559
      }else if( DbMaskTest(m,i) ){
        db->bPendingSchemaReset = 1;
      }else{
        sqlite3SchemaClear(db->aDb[i].pSchema);
        db->mDbFlags &= ~DBFLAG_SchemaKnownOk;
      }
