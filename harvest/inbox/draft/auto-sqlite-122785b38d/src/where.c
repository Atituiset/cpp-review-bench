// AUTO-DRAFT from sqlite/sqlite PR #8351ef7256e58e799d2e92c9f73b082a9019501a
          }
        }
        if( isMatch && (pOrderBy->a[i].fg.sortFlags & KEYINFO_ORDER_BIGNULL) ){
          if( j==pLoop->u.btree.nEq ){  // <<< BUG ANCHOR
            pLoop->wsFlags |= WHERE_BIGNULL_SORT;
          }else{
            isMatch = 0;
