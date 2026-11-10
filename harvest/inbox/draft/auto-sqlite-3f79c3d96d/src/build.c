// AUTO-DRAFT from sqlite/sqlite PR #2c15ec606bae5d0da2c655d4943a2d73b6211ebe
  ** by the parser, we can sometimes end up with a typename that ends
  ** with "generated always".  Check for this case and omit the surplus
  ** text. */
  if( sType.n>=16  // <<< BUG ANCHOR
   && sqlite3_strnicmp(sType.z+(sType.n-6),"always",6)==0
  ){
    sType.n -= 6;
    while( ALWAYS(sType.n>0) && sqlite3Isspace(sType.z[sType.n-1]) ) sType.n--;
    if( sType.n>=9
     && sqlite3_strnicmp(sType.z+(sType.n-9),"generated",9)==0
    ){
      sType.n -= 9;
      while( sType.n>0 && sqlite3Isspace(sType.z[sType.n-1]) ) sType.n--;
    }
  }
