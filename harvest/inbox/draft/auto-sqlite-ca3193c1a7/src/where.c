// AUTO-DRAFT from sqlite/sqlite PR #aba9f8ef69eb2f5cbb0dc663b377db8f16d6a858
      VdbeCoverageIf(v, pLevel->op==OP_Next);
      VdbeCoverageIf(v, pLevel->op==OP_Prev);
      VdbeCoverageIf(v, pLevel->op==OP_VNext);
      if( pLevel->regBignull ){  // <<< BUG ANCHOR
        sqlite3VdbeResolveLabel(v, pLevel->addrBignull);
        sqlite3VdbeAddOp2(v, OP_DecrJumpZero, pLevel->regBignull, pLevel->p2-1);
        VdbeCoverage(v);
      }
#ifndef SQLITE_DISABLE_SKIPAHEAD_DISTINCT
      if( addrSeek ){
        sqlite3VdbeJumpHere(v, addrSeek);
        addrSeek = 0;
      }
#endif
    }
    if( (pLoop->wsFlags & WHERE_IN_ABLE)!=0 && pLevel->u.in.nIn>0 ){
      struct InLoop *pIn;
