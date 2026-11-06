// AUTO-DRAFT from sqlite/sqlite PR #99c346fce368df37ef8eedd9b482b0748e7f752a
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  i = 0;
  if( p->nDigit==0 || (p->nDigit==1 && p->a[0]==0) ){  // <<< BUG ANCHOR
    p->sign = 0;
  }
  if( p->sign ){
    z[0] = '-';
    i = 1;
