// AUTO-DRAFT from sqlite/sqlite PR #90ab1c833b5093be59376836fe550b01f843ffb8
          onError = OE_Ignore;  /* DO NOTHING is the same as INSERT OR IGNORE */
        }else{
          onError = OE_Update;  /* DO UPDATE */
        }
      }
      if( pUpsertClause!=pUpsert ){
