// AUTO-DRAFT from sqlite/sqlite PR #7c2cb02205d8b36425114ded5fd8deccf27b13f4
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdlib.h>
  // <<< BUG ANCHOR
static int getConstraintToken(const u8 *z, int *piToken){
  int iOff = 0;
  int t = 0;
  do {
    iOff += sqlite3GetToken(&z[iOff], &t);
  }while( t==TK_SPACE || t==TK_COMMENT );

  *piToken = t;

  if( t==TK_LP ){
    int nNest = 1;
    while( nNest>0 ){
      iOff += sqlite3GetToken(&z[iOff], &t);
      if( t==TK_LP ){
        nNest++;
      }else if( t==TK_RP ){
        t = TK_LP;
        nNest--;
      }else if( t==TK_ILLEGAL ){
        break;
      }
    }
  }

  *piToken = t;
  return iOff;
}
/* …（同文件无关代码省略）… */
** keyword.
**
** Argument eTok may be the token value of the first token of the constraint
** (e.g. TK_CHECK or TK_REFERENCES) or zero. If it is either TK_REFERENCES
** or TK_FOREIGN, special parsing is enabled to find the end of the foreign-key
** constraint definition.
**
** Return the number of bytes until the end of the constraint. 
*/
static int getConstraint(const u8 *z, int eTok){
  int iOff = 0;
  int t = 0;

#ifndef SQLITE_OMIT_FOREIGN_KEY
  if( eTok==TK_FOREIGN ){
/* …（同文件无关代码省略）… */
    /* REFERENCES is followed by a table name. Gobble this up here in
    ** case the table name is a fallback token like TK_GENERATED. */
    iOff += getConstraintToken(&z[iOff], &t);
  }
#endif

/* …（同文件无关代码省略）… */
  **   COLLATE, REFERENCES, FOREIGN, GENERATED, AS, RP, or COMMA
  **
  ** Also exit the loop if ILLEGAL turns up.
  */
  while( 1 ){
    int n = getConstraintToken(&z[iOff], &t);
    if( t==TK_CONSTRAINT || t==TK_PRIMARY || t==TK_NOT || t==TK_UNIQUE
     || t==TK_CHECK || t==TK_DEFAULT || t==TK_COLLATE || t==TK_REFERENCES
     || t==TK_FOREIGN || t==TK_RP || t==TK_COMMA || t==TK_ILLEGAL
     || t==TK_AS || t==TK_GENERATED
    ){
      break;
    }
    iOff += n;
  }
  
  return iOff;
/* …（同文件无关代码省略）… */
        }

      }else if( t==TK_NOT && iNotNull==ii ){
        iEnd = iOff + getConstraint(&zSql[iOff], 0);
        break;
      }else if( t==TK_RP || t==TK_ILLEGAL ){
        iEnd = -1;
        break;
