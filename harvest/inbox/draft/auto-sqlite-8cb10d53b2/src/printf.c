// AUTO-DRAFT from sqlite/sqlite PR #7f4f5482308541a833841f1772b9fad6b61b1436
#define PREFIXOFST     2     /* Shift right to get the prefix size */
/* …（同文件无关代码省略）… */
            testcase( szPrefix==0 );
            testcase( szPrefix==1 );
            testcase( szPrefix==2 );
            width -= (infop->flags>>PREFIXOFST)&3;  // <<< BUG ANCHOR
          }
          if( precision<width-(prefix!=0) ){
            precision = width-(prefix!=0);
