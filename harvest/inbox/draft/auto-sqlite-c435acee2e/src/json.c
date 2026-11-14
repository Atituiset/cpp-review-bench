// AUTO-DRAFT from sqlite/sqlite PR #11eab668bd3180945c01a6ddf8dce8d5b9bbce54
        if( sz<2 ) return i+1;
      }
      k = i+n+sz;
      while( j<k ){
        if( sqlite3Isdigit(z[j]) ){
          j++;
/* …（同文件无关代码省略）… */
          continue;
        }
        if( z[j]=='e' || z[j]=='E' ){
          if( seen==2 ) return j+1;  // <<< BUG ANCHOR
          if( j==k-1 ) return j+1;
          if( z[j+1]=='+' || z[j+1]=='-' ){
            j++;
