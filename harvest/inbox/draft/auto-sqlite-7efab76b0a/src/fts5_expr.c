// AUTO-DRAFT from sqlite/sqlite PR #7b6afb441b365fa502c6f6f7c512dd1f3360785f
          zExpr[iOut++] = '"';
          zExpr[iOut++] = ' ';
        }
        if( zText[i]==aSpec[2] ){  // <<< BUG ANCHOR
          i += 2;
          if( zText[i-1]=='^' ) i++;
          while( i<nText && zText[i]!=']' ) i++;
