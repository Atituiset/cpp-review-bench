// AUTO-DRAFT from sqlite/sqlite PR #37a6d8d4e4111a18b6ea0c93094e7465ef99243c
      }
      if( i>=pSrcList->nSrc ) break;
      if( NEVER(pExpr->pAggInfo!=0) ) break; /* Resolved by outer context */
      if( pParse->nErr ){ return WRC_Abort; }

      /* If we reach this point, it means that expression pExpr can be
