// AUTO-DRAFT from sqlite/sqlite PR #b281238e9249f947fba3fc9aca3e21b62b88794c
          }
          cnt++;
          pMatch = pItem;
          /* Substitute the rowid (column -1) for the INTEGER PRIMARY KEY */
          pExpr->iColumn = j==pTab->iPKey ? -1 : (i16)j;
          if( pItem->fg.isNestedFrom ){
            sqlite3SrcItemColumnUsed(pItem, j);
          }
/* …（同文件无关代码省略）… */
        cntTab++;
        iCol = sqlite3ColumnIndex(pTab, zCol);
        if( iCol>=0 ){
          if( pTab->iPKey==iCol ) iCol = -1;  // <<< BUG ANCHOR
        }else{
          if( sqlite3IsRowid(zCol) && VisibleRowid(pTab) ){
            iCol = -1;
