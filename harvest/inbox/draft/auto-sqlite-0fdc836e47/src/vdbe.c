// AUTO-DRAFT from sqlite/sqlite PR #4303eaf22199bf952a7931068a407ce5a9e50a46
      len = (u32)pRec->n;
      serial_type = (len*2) + 12 + ((pRec->flags & MEM_Str)!=0);
      if( pRec->flags & MEM_Zero ){
        serial_type += (u32)pRec->u.nZero*2;  // <<< BUG ANCHOR
        if( nData ){
          if( sqlite3VdbeMemExpandBlob(pRec) ) goto no_mem;
          len += pRec->u.nZero;
        }else{
          nZero += pRec->u.nZero;
        }
      }
      nData += len;
      nHdr += sqlite3VarintLen(serial_type);
