// AUTO-DRAFT from sqlite/sqlite PR #ad2082370a47b01484fea2579e03f909e5e50df7
      pExpr = pIdx->aColExpr->a[i].pExpr;
    }else if( j>=0 && (pTab->aCol[j].colFlags & COLFLAG_VIRTUAL)!=0 ){
      pExpr = sqlite3ColumnExpr(pTab, &pTab->aCol[j]);
    }else{
      continue;
    }
