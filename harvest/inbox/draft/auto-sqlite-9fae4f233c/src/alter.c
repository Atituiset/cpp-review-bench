// AUTO-DRAFT from sqlite/sqlite PR #4acc047df2e209ce749f2c5a4e19f6c471f63dbe
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
  const char *zErr      /* Error message text */
){
  sqlite3NestedParse(pParse,
     "SELECT raise(ABORT,%Q) FROM \"%w\".\"%w\"",
     zErr, zDb, zTab
  );
}
/* …（同文件无关代码省略）… */
      pParse->colNamesSet = 1;
      sqlite3NestedParse(pParse,
        "SELECT CASE WHEN quick_check GLOB 'CHECK*'"
        " THEN raise(ABORT,'CHECK constraint failed')"
        " WHEN quick_check GLOB 'non-* value in*'"
        " THEN raise(ABORT,'type mismatch on DEFAULT')"
        " ELSE raise(ABORT,'NOT NULL constraint failed')"
        " END"
        "  FROM pragma_quick_check(%Q,%Q)"
        " WHERE quick_check GLOB 'CHECK*'"
/* …（同文件无关代码省略）… */
static void errorMPrintf(sqlite3_context *pCtx, const char *zFmt, ...){
  sqlite3 *db = sqlite3_context_db_handle(pCtx);
  char *zErr = 0;
  va_list ap;
  va_start(ap, zFmt);
  zErr = sqlite3VMPrintf(db, zFmt, ap);
  va_end(ap);
  if( zErr ){
    sqlite3_result_error(pCtx, zErr, -1);
    sqlite3DbFree(db, zErr);
  }else{
    sqlite3_result_error_nomem(pCtx);
  }
}
/* …（同文件无关代码省略）… */
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
static int getWhitespace(const u8 *z){
  int nRet = 0;
  while( 1 ){
    int t = 0;
    int n = sqlite3GetToken(&z[nRet], &t);
    if( t!=TK_SPACE && t!=TK_COMMENT ) break;
    nRet += n;
  }
  return nRet;
}
/* …（同文件无关代码省略）… */
static int getConstraint(const u8 *z, int eTok){
  int iOff = 0;
  int t = 0;
  int t2 = 0;
  int prev = 0;

#ifndef SQLITE_OMIT_FOREIGN_KEY
  if( eTok==TK_FOREIGN ){
    /* For a FOREIGN KEY constraint, use getConstraint() to parse everything
    ** up to the REFERENCES keyword. Then getConstraintToken() to consume
    ** the TK_REFERENCES token itself. Then fall through to the special
    ** handling for TK_REFERENCES below.  */
    iOff = getConstraint(z, 0);
    iOff += getConstraintToken(&z[iOff], &eTok);
  }

  if( eTok==TK_REFERENCES ){
    /* REFERENCES is followed by a table name. Gobble this up here in
    ** case the table name is a fallback token like TK_GENERATED. */
    iOff += getConstraintToken(&z[iOff], &t);

    /* If the table names is followed by a parenthesized list of columns,
    ** also skip that, lest the close ")" end the constraint in the
    ** while loop below. */
    iOff += getWhitespace(&z[iOff]);
    if( z[iOff]=='(' ){
      iOff += getConstraintToken(&z[iOff], &t);
    }
  }
#endif

  /* Now, the current constraint proceeds until the next occurence of one 
  ** of the following tokens: 
  **
  **   CONSTRAINT, PRIMARY, NOT, UNIQUE, CHECK, DEFAULT, 
  **   COLLATE, REFERENCES, FOREIGN, GENERATED, AS, RP, or COMMA
  **
  ** Also exit the loop if ILLEGAL turns up.
  **
  ** Exceptions:
  **
  **   *  For NOT NULL constraints, also terminate at DEFERRABLE
  **
  **   *  Do not terminate at DEFAULT if the previous token was SET
  */
  while( 1 ){
    int n = getConstraintToken(&z[iOff], &t);
    if( t==TK_CONSTRAINT || t==TK_PRIMARY || t==TK_UNIQUE
     || t==TK_CHECK || t==TK_COLLATE || t==TK_REFERENCES
     || (t==TK_DEFAULT && prev!=TK_SET)
     || t==TK_FOREIGN || t==TK_RP || t==TK_COMMA || t==TK_ILLEGAL
     || t==TK_AS || t==TK_GENERATED
     || (eTok==TK_NOT && t==TK_DEFERRABLE)
    ){
      break;
    }
    if( t==TK_NOT
     && (eTok!=TK_REFERENCES
         || (getConstraintToken(&z[iOff+n],&t2), t2!=TK_DEFERRABLE))
    ){
      /* NOT ends the constraint unless eTok is TK_REFERENCES and the next
      ** token is DEFERRABLE */
      break;
    }
    iOff += n;
    prev = t;
  }
  
  return iOff;
}
/* …（同文件无关代码省略）… */
static int quotedCompare(
  sqlite3_context *ctx,  /* Function context on which to report errors */
  int t,                 /* Token type */
  const u8 *zQuote,      /* Possibly quoted text.  Not zero-terminated. */
  int nQuote,            /* Length of zQuote in bytes */
  const u8 *zCmp,        /* Zero-terminated, unquoted name to compare against */
  int *pRes              /* OUT: Set to 0 if equal, non-zero if unequal */
){
  char *zCopy = 0;       /* De-quoted, zero-terminated copy of zQuote[] */

  if( t==TK_ILLEGAL ){
    *pRes = 1;
    return SQLITE_OK;
  }
  zCopy = sqlite3MallocZero(nQuote+1);
  if( zCopy==0 ){
    sqlite3_result_error_nomem(ctx);
    return SQLITE_NOMEM_BKPT;
  }
  memcpy(zCopy, zQuote, nQuote);
  sqlite3Dequote(zCopy);
  *pRes = sqlite3_stricmp((const char*)zCopy, (const char*)zCmp);
  sqlite3_free(zCopy);
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static int skipCreateTable(sqlite3_context *ctx, const u8 *zSql, int *piOff){
  int iOff = 0;

  if( zSql==0 ) return SQLITE_ERROR;

  /* Jump past the "CREATE TABLE" bit. */
  while( 1 ){
    int t = 0;
    iOff += sqlite3GetToken(&zSql[iOff], &t);
    if( t==TK_LP ) break;
    if( t==TK_ILLEGAL ){
      sqlite3_result_error_code(ctx, SQLITE_CORRUPT_BKPT);
      return SQLITE_ERROR;
    }
  }

  *piOff = iOff;
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
static void dropConstraintFunc(
  sqlite3_context *ctx,
  int NotUsed,
  sqlite3_value **argv
){
  const u8 *zSql = sqlite3_value_text(argv[0]);
  const u8 *zCons = 0;
  int iNotNull = -1;
  int ii;
  int iOff = 0;
  int iStart = 0;
  int iEnd = 0;
  char *zNew = 0;
  int t = 0;
  sqlite3 *db;
  UNUSED_PARAMETER(NotUsed);

  if( zSql==0 ) return;

  /* Jump past the "CREATE TABLE" bit. */
  if( skipCreateTable(ctx, zSql, &iOff) ) return;

  if( sqlite3_value_type(argv[1])==SQLITE_INTEGER ){
    iNotNull = sqlite3_value_int(argv[1]);
  }else{
    zCons = sqlite3_value_text(argv[1]);
