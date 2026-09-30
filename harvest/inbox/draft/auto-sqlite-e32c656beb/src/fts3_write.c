// AUTO-DRAFT from sqlite/sqlite PR #a7d4cd04b596542b71865aa0c3eb258042efce97
          }
        }else{
          iPos += (iVal - 2);
          if( iPos<0 || iPos>0x7FFFFFFF ){  // <<< BUG ANCHOR
            rc = SQLITE_CORRUPT_VTAB;
            break;
          }else{
