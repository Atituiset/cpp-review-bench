// AUTO-DRAFT from sqlite/sqlite PR #bc766d966a3690f848e8c9ace9f01380f98ab9cd
        idxStr[iIdxStr++] = '=';
        bSeenEq = 1;
        pInfo->aConstraintUsage[i].argvIndex = ++iCons;
        pInfo->aConstraintUsage[i].omit = 1;  // <<< BUG ANCHOR
      }
    }
  }
