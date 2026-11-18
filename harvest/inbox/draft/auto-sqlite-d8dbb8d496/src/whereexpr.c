// AUTO-DRAFT from sqlite/sqlite PR #4c8eb82e9df01db0ace92d3e1e7fb556bd521041
        if( c==0xff                             /* (1) */
         || (u = sqlite3Utf8Read(&z2))>=0xfffd  /* (2) */
         || u==0x7ff                            /* (3) */
         || ENC(db)==SQLITE_UTF16LE             /* (4) */  // <<< BUG ANCHOR
        ){
          /* Do not accept this character as part of the prefix because
          **   (1)  0xff is not part of any valid UTF-8 character and the
/* …（同文件无关代码省略）… */
          **        start getting into the surrogate pairs in UTF16BE.
          **   (3)  When incrementing this code point to the upper bound,
          **        that would require adding an extra byte to the UTF-8
          **   (4)  UTF16LE does not compare in sorted order.  Only
          **        UTF16BE and UTF8 have that property.
          */
          cnt--;
