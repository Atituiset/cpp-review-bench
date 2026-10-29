// AUTO-DRAFT from sqlite/sqlite PR #1e4e45df2465b56feeccc75f413be502768f95a9
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
  return c;
}


#ifndef SQLITE_OMIT_LIKE_OPTIMIZATION
/*
/* …（同文件无关代码省略）… */
    assert( pRight->op==TK_VARIABLE || pRight->op==TK_REGISTER );
  }else if( op==TK_STRING ){
    assert( !ExprHasProperty(pRight, EP_IntValue) );
     z = (u8*)pRight->u.zToken;
  }
  if( z ){
    /* Count the number of prefix bytes prior to the first wildcard,
/* …（同文件无关代码省略）… */
      }
    }

    /* The optimization is possible only if (1) the pattern does not begin
    ** with a wildcard and if (2) the non-wildcard prefix does not end with
    ** an (illegal 0xff) character, or (3) the pattern does not consist of
    ** a single escape character. The second condition is necessary so
    ** that we can increment the prefix key to find an upper bound for the
    ** range search. The third is because the caller assumes that the pattern
    ** consists of at least one character after all escapes have been
    ** removed.  */
    if( (cnt>1 || (cnt>0 && z[0]!=wc[3])) && ALWAYS(255!=(u8)z[cnt-1]) ){
/* …（同文件无关代码省略）… */

        /* If the LHS is not an ordinary column with TEXT affinity, then the
        ** pattern prefix boundaries (both the start and end boundaries) must
        ** not look like a number.  Otherwise the pattern might be treated as
        ** a number, which will invalidate the LIKE optimization.
        **
        ** Getting this right has been a persistent source of bugs in the
        ** LIKE optimization.  See, for example:
/* …（同文件无关代码省略）… */
        **    2019-06-10 https://sqlite.org/src/info/fd76310a5e843e07
        **    2019-06-14 https://sqlite.org/src/info/ce8717f0885af975
        **    2019-09-03 https://sqlite.org/src/info/0f0428096f17252a
        */
        if( pLeft->op!=TK_COLUMN
         || sqlite3ExprAffinity(pLeft)!=SQLITE_AFF_TEXT
         || (ALWAYS( ExprUseYTab(pLeft) )
             && ALWAYS(pLeft->y.pTab)
             && IsVirtual(pLeft->y.pTab))  /* Might be numeric */
        ){
          int isNum;
          double rDummy;
          assert( zNew[iTo]==0 );
          isNum = sqlite3AtoF(zNew, &rDummy);
          if( isNum<=0 ){
            if( iTo==1 && zNew[0]=='-' ){
              isNum = +1;
            }else{
              zNew[iTo-1]++;
              isNum = sqlite3AtoF(zNew, &rDummy);
              zNew[iTo-1]--;
            }
          }
          if( isNum>0 ){
            sqlite3ExprDelete(db, pPrefix);
            sqlite3ValueFree(pVal);
            return 0;
