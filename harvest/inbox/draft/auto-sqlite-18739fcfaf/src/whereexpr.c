// AUTO-DRAFT from sqlite/sqlite PR #cb5e5b117eb4581d0bd32825e62e100bc61faa0b
        cnt++;
      }else if( c>=0x80 ){
        const u8 *z2 = z+cnt-1;
        if( c==0xff || sqlite3Utf8Read(&z2)==0xfffd  /* bad utf-8 */  // <<< BUG ANCHOR
         || ENC(db)==SQLITE_UTF16LE 
        ){
          cnt--;
          break;
        }else{
