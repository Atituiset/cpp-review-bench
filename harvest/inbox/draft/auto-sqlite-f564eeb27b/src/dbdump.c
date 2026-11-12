// AUTO-DRAFT from sqlite/sqlite PR #bf181220a009b89d782cf6bf9a933683255f7ead
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct DState DState;
/* …（同文件无关代码省略）… */
typedef struct DText DText;
/* …（同文件无关代码省略）… */
static void initText(DText *p){
  memset(p, 0, sizeof(*p));
}
static void freeText(DText *p){
  sqlite3_free(p->z);
  initText(p);
}
/* …（同文件无关代码省略）… */
static void appendText(DText *p, char const *zAppend, char quote){
  int len;
  int i;
  int nAppend = (int)(strlen(zAppend) & 0x3fffffff);

  len = nAppend+p->n+1;
  if( quote ){
    len += 2;
    for(i=0; i<nAppend; i++){
      if( zAppend[i]==quote ) len++;
    }
  }

  if( p->n+len>=p->nAlloc ){
    char *zNew;
    p->nAlloc = p->nAlloc*2 + len + 20;
    zNew = sqlite3_realloc64(p->z, p->nAlloc);
    if( zNew==0 ){
      freeText(p);
      return;
    }
    p->z = zNew;
  }

  if( quote ){
    char *zCsr = p->z+p->n;
    *zCsr++ = quote;
    for(i=0; i<nAppend; i++){
      *zCsr++ = zAppend[i];
      if( zAppend[i]==quote ) *zCsr++ = quote;
    }
    *zCsr++ = quote;
    p->n = (int)(zCsr - p->z);
    *zCsr = '\0';
  }else{
    memcpy(p->z+p->n, zAppend, nAppend);
    p->n += nAppend;
    p->z[p->n] = '\0';
  }
}
/* …（同文件无关代码省略）… */
static char quoteChar(const char *zName){
  int i;
  if( !isalpha((unsigned char)zName[0]) && zName[0]!='_' ) return '"';
  for(i=0; zName[i]; i++){
    if( !isalnum((unsigned char)zName[i]) && zName[i]!='_' ) return '"';
  }
  return sqlite3_keyword_check(zName, i) ? '"' : 0;
}
/* …（同文件无关代码省略）… */
static void freeColumnList(char **azCol){
  int i;
  if( azCol==0 ) return;
  for(i=1; azCol[i]; i++){
    sqlite3_free(azCol[i]);
  }
  /* azCol[0] is a static string */
  sqlite3_free(azCol);
}
/* …（同文件无关代码省略）… */
static char **tableColumnList(DState *p, const char *zTab){
  char **azCol = 0;
  sqlite3_stmt *pStmt = 0;
  char *zSql;
  sqlite3_int64 nCol = 0;
  sqlite3_int64 nAlloc = 0;
  int nPK = 0;       /* Number of PRIMARY KEY columns seen */
  int isIPK = 0;     /* True if one PRIMARY KEY column of type INTEGER */
  int preserveRowid = 1;
  int rc;

  zSql = sqlite3_mprintf("PRAGMA table_info=%Q", zTab);
  if( zSql==0 ) return 0;
  rc = sqlite3_prepare_v2(p->db, zSql, -1, &pStmt, 0);
  sqlite3_free(zSql);
  if( rc ) return 0;
  while( sqlite3_step(pStmt)==SQLITE_ROW ){
    if( nCol>=nAlloc-2 ){
      char **azNew;
      nAlloc = nAlloc*2 + nCol + 10;
      azNew = sqlite3_realloc64(azCol, nAlloc*sizeof(azCol[0]));
      if( azNew==0 ) goto col_oom;
      azCol = azNew;
      azCol[0] = 0;
    }
    azCol[++nCol] = sqlite3_mprintf("%s", sqlite3_column_text(pStmt, 1));
    if( azCol[nCol]==0 ) goto col_oom;
    if( sqlite3_column_int(pStmt, 5) ){
      nPK++;
      if( nPK==1
       && sqlite3_stricmp((const char*)sqlite3_column_text(pStmt,2),
                          "INTEGER")==0 
      ){
        isIPK = 1;
      }else{
        isIPK = 0;
      }
    }
  }
  sqlite3_finalize(pStmt);
  pStmt = 0;
  if( azCol ) azCol[nCol+1] = 0;

  /* The decision of whether or not a rowid really needs to be preserved
  ** is tricky.  We never need to preserve a rowid for a WITHOUT ROWID table
  ** or a table with an INTEGER PRIMARY KEY.  We are unable to preserve
  ** rowids on tables where the rowid is inaccessible because there are other
  ** columns in the table named "rowid", "_rowid_", and "oid".
  */
  if( isIPK ){
    /* If a single PRIMARY KEY column with type INTEGER was seen, then it
    ** might be an alise for the ROWID.  But it might also be a WITHOUT ROWID
    ** table or a INTEGER PRIMARY KEY DESC column, neither of which are
    ** ROWID aliases.  To distinguish these cases, check to see if
    ** there is a "pk" entry in "PRAGMA index_list".  There will be
    ** no "pk" index if the PRIMARY KEY really is an alias for the ROWID.
    */
    zSql = sqlite3_mprintf("SELECT 1 FROM pragma_index_list(%Q)"
                           " WHERE origin='pk'", zTab);
    if( zSql==0 ) goto col_oom;
    rc = sqlite3_prepare_v2(p->db, zSql, -1, &pStmt, 0);
    sqlite3_free(zSql);
    if( rc ){
      freeColumnList(azCol);
      return 0;
    }
    rc = sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);
    pStmt = 0;
    preserveRowid = rc==SQLITE_ROW;
  }
  if( preserveRowid ){
    /* Only preserve the rowid if we can find a name to use for the
    ** rowid */
    static char *azRowid[] = { "rowid", "_rowid_", "oid" };
    int i, j;
    for(j=0; j<3; j++){
      for(i=1; i<=nCol; i++){
        if( sqlite3_stricmp(azRowid[j],azCol[i])==0 ) break;
      }
      if( i>nCol ){
        /* At this point, we know that azRowid[j] is not the name of any
        ** ordinary column in the table.  Verify that azRowid[j] is a valid
        ** name for the rowid before adding it to azCol[0].  WITHOUT ROWID
        ** tables will fail this last check */
        rc = sqlite3_table_column_metadata(p->db,0,zTab,azRowid[j],0,0,0,0,0);
        if( rc==SQLITE_OK ) azCol[0] = azRowid[j];
        break;
      }
    }
  }
  return azCol;

col_oom:
  sqlite3_finalize(pStmt);
  freeColumnList(azCol);
  p->nErr++;
  p->rc = SQLITE_NOMEM;
  return 0;
}

/*
** Send mprintf-formatted content to the output callback.
*/
static void output_formatted(DState *p, const char *zFormat, ...){
  va_list ap;
  char *z;
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  p->xCallback(z, p->pArg);
  sqlite3_free(z);
}
/* …（同文件无关代码省略）… */
static const char *unused_string(
  const char *z,                    /* Result must not appear anywhere in z */
  const char *zA, const char *zB,   /* Try these first */
  char *zBuf                        /* Space to store a generated string */
){
  unsigned i = 0;
  if( strstr(z, zA)==0 ) return zA;
  if( strstr(z, zB)==0 ) return zB;
  do{
    sqlite3_snprintf(20,zBuf,"(%s%u)", zA, i++);
  }while( strstr(z,zBuf)!=0 );
  return zBuf;
}
/* …（同文件无关代码省略）… */
static void output_quoted_escaped_string(DState *p, const char *z){
  int i;
  char c;
  for(i=0; (c = z[i])!=0 && c!='\'' && c!='\n' && c!='\r'; i++){}
  if( c==0 ){
    output_formatted(p,"'%s'",z);
  }else{
    const char *zNL = 0;
    const char *zCR = 0;
    int nNL = 0;
    int nCR = 0;
    char zBuf1[20], zBuf2[20];
    for(i=0; z[i]; i++){
      if( z[i]=='\n' ) nNL++;
      if( z[i]=='\r' ) nCR++;
    }
    if( nNL ){
      p->xCallback("replace(", p->pArg);
      zNL = unused_string(z, "\\n", "\\012", zBuf1);
    }
    if( nCR ){
      p->xCallback("replace(", p->pArg);
      zCR = unused_string(z, "\\r", "\\015", zBuf2);
    }
    p->xCallback("'", p->pArg);
    while( *z ){
      for(i=0; (c = z[i])!=0 && c!='\n' && c!='\r' && c!='\''; i++){}
      if( c=='\'' ) i++;
      if( i ){
        output_formatted(p, "%.*s", i, z);
        z += i;
      }
      if( c=='\'' ){
        p->xCallback("'", p->pArg);
        continue;
      }
      if( c==0 ){
        break;
      }
      z++;
      if( c=='\n' ){
        p->xCallback(zNL, p->pArg);
        continue;
      }
      p->xCallback(zCR, p->pArg);
    }
    p->xCallback("'", p->pArg);
    if( nCR ){
      output_formatted(p, ",'%s',char(13))", zCR);
    }
    if( nNL ){
      output_formatted(p, ",'%s',char(10))", zNL);
    }
  }
}
/* …（同文件无关代码省略）… */
static int dump_callback(void *pArg, int nArg, char **azArg, char **azCol){
  int rc;
  const char *zTable;
  const char *zType;
  const char *zSql;
  DState *p = (DState*)pArg;
  sqlite3_stmt *pStmt;

  (void)azCol;
  if( nArg!=3 ) return 1;
  if( azArg==0 ) return 1;
  zTable = azArg[0];
  zType = azArg[1];
  zSql = azArg[2];

  if( strcmp(zTable, "sqlite_sequence")==0 ){
    p->xCallback("DELETE FROM sqlite_sequence;\n", p->pArg);
  }else if( sqlite3_strglob("sqlite_stat?", zTable)==0 ){
    p->xCallback("ANALYZE sqlite_schema;\n", p->pArg);
  }else if( strncmp(zTable, "sqlite_", 7)==0 ){
    return 0;
  }else if( strncmp(zSql, "CREATE VIRTUAL TABLE", 20)==0 ){
    if( !p->writableSchema ){
      p->xCallback("PRAGMA writable_schema=ON;\n", p->pArg);
      p->writableSchema = 1;
    }
    output_formatted(p,
       "INSERT INTO sqlite_schema(type,name,tbl_name,rootpage,sql)"
       "VALUES('table','%q','%q',0,'%q');",
       zTable, zTable, zSql);
    return 0;
  }else{
    if( sqlite3_strglob("CREATE TABLE ['\"]*", zSql)==0 ){
      p->xCallback("CREATE TABLE IF NOT EXISTS ", p->pArg);
      p->xCallback(zSql+13, p->pArg);
    }else{
      p->xCallback(zSql, p->pArg);
    }
    p->xCallback(";\n", p->pArg);
  }

  if( strcmp(zType, "table")==0 ){
    DText sSelect;
    DText sTable;
    char **azTCol;
    int i;
    int nCol;

    azTCol = tableColumnList(p, zTable);
    if( azTCol==0 ) return 0;

    initText(&sTable);
    appendText(&sTable, "INSERT INTO ", 0);

    /* Always quote the table name, even if it appears to be pure ascii,
    ** in case it is a keyword. Ex:  INSERT INTO "table" ... */
    appendText(&sTable, zTable, quoteChar(zTable));

    /* If preserving the rowid, add a column list after the table name.
    ** In other words:  "INSERT INTO tab(rowid,a,b,c,...) VALUES(...)"
    ** instead of the usual "INSERT INTO tab VALUES(...)".
    */
    if( azTCol[0] ){
      appendText(&sTable, "(", 0);
      appendText(&sTable, azTCol[0], 0);
      for(i=1; azTCol[i]; i++){
        appendText(&sTable, ",", 0);
        appendText(&sTable, azTCol[i], quoteChar(azTCol[i]));
      }
      appendText(&sTable, ")", 0);
    }
    appendText(&sTable, " VALUES(", 0);

    /* Build an appropriate SELECT statement */
    initText(&sSelect);
    appendText(&sSelect, "SELECT ", 0);
    if( azTCol[0] ){
      appendText(&sSelect, azTCol[0], 0);
      appendText(&sSelect, ",", 0);
    }
    for(i=1; azTCol[i]; i++){
      appendText(&sSelect, azTCol[i], quoteChar(azTCol[i]));
      if( azTCol[i+1] ){
        appendText(&sSelect, ",", 0);
      }
    }
    nCol = i;
    if( azTCol[0]==0 ) nCol--;
    freeColumnList(azTCol);
    appendText(&sSelect, " FROM ", 0);
    appendText(&sSelect, zTable, quoteChar(zTable));

    rc = sqlite3_prepare_v2(p->db, sSelect.z, -1, &pStmt, 0);
    if( rc!=SQLITE_OK ){
      p->nErr++;
      if( p->rc==SQLITE_OK ) p->rc = rc;
    }else{
      while( SQLITE_ROW==sqlite3_step(pStmt) ){
        p->xCallback(sTable.z, p->pArg);
        for(i=0; i<nCol; i++){
          if( i ) p->xCallback(",", p->pArg);
          switch( sqlite3_column_type(pStmt,i) ){
            case SQLITE_INTEGER: {
              output_formatted(p, "%lld", sqlite3_column_int64(pStmt,i));
              break;
            }
            case SQLITE_FLOAT: {
              double r = sqlite3_column_double(pStmt,i);
              sqlite3_uint64 ur;
              memcpy(&ur,&r,sizeof(r));
              if( ur==0x7ff0000000000000LL ){
                p->xCallback("1e999", p->pArg);
              }else if( ur==0xfff0000000000000LL ){
                p->xCallback("-1e999", p->pArg);
              }else{
                output_formatted(p, "%!.20g", r);
              }
              break;
            }
            case SQLITE_NULL: {
              p->xCallback("NULL", p->pArg);
              break;
            }
            case SQLITE_TEXT: {
              output_quoted_escaped_string(p, 
                   (const char*)sqlite3_column_text(pStmt,i));
              break;
            }
            case SQLITE_BLOB: {
              int nByte = sqlite3_column_bytes(pStmt,i);
              unsigned char *a = (unsigned char*)sqlite3_column_blob(pStmt,i);
              int j;
              p->xCallback("x'", p->pArg);
              for(j=0; j<nByte; j++){
                char zWord[3];
                zWord[0] = "0123456789abcdef"[(a[j]>>4)&15];
                zWord[1] = "0123456789abcdef"[a[j]&15];
                zWord[2] = 0;
                p->xCallback(zWord, p->pArg);
              }
              p->xCallback("'", p->pArg);
              break;
            }
          }
        }
        p->xCallback(");\n", p->pArg);
      }
    }
    sqlite3_finalize(pStmt);
    freeText(&sTable);
    freeText(&sSelect);
  }
  return 0;
}
/* …（同文件无关代码省略）… */
  rc = sqlite3_prepare_v2(p->db, zSql, -1, &pSelect, 0);
  sqlite3_free(zSql);
  if( rc!=SQLITE_OK || !pSelect ){
    output_formatted(p, "/**** ERROR: (%d) %s *****/\n", rc,
                sqlite3_errmsg(p->db));
    p->nErr++;
    return;
  }
/* …（同文件无关代码省略）… */
  }
  rc = sqlite3_finalize(pSelect);
  if( rc!=SQLITE_OK ){
    output_formatted(p, "/**** ERROR: (%d) %s *****/\n", rc,
                     sqlite3_errmsg(p->db));
    if( (rc&0xff)!=SQLITE_CORRUPT ) p->nErr++;
  }
}
/* …（同文件无关代码省略）… */
  sqlite3_exec(p->db, z, dump_callback, p, &zErr);
  sqlite3_free(z);
  if( zErr ){
    output_formatted(p, "/****** %s ******/\n", zErr);
    sqlite3_free(zErr);
    p->nErr++;
    zErr = 0;
