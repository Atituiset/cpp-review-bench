// AUTO-DRAFT from sqlite/sqlite PR #c1bf3367ea188a82153b830399b00e82652db439
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
  // <<< BUG ANCHOR
Table *sqlite3FindTable(sqlite3 *db, const char *zName, const char *zDatabase){
  Table *p = 0;
  int i;

  /* All mutexes are required for schema access.  Make sure we hold them. */
  assert( zDatabase!=0 || sqlite3BtreeHoldsAllMutexes(db) );
  if( zDatabase ){
    for(i=0; i<db->nDb; i++){
      if( sqlite3StrICmp(zDatabase, db->aDb[i].zDbSName)==0 ) break;
    }
    if( i>=db->nDb ){
      /* No match against the official names.  But always match "main"
      ** to schema 0 as a legacy fallback. */
      if( sqlite3StrICmp(zDatabase,"main")==0 ){
        i = 0;
      }else{
        return 0;
      }
    }
    p = sqlite3HashFind(&db->aDb[i].pSchema->tblHash, zName);
    if( p==0 && sqlite3StrNICmp(zName, "sqlite_", 7)==0 ){
      if( i==1 ){
        if( sqlite3StrICmp(zName+7, &PREFERRED_TEMP_SCHEMA_TABLE[7])==0
         || sqlite3StrICmp(zName+7, &PREFERRED_SCHEMA_TABLE[7])==0
         || sqlite3StrICmp(zName+7, &LEGACY_SCHEMA_TABLE[7])==0
        ){
          p = sqlite3HashFind(&db->aDb[1].pSchema->tblHash,
                              LEGACY_TEMP_SCHEMA_TABLE);
        }
      }else{
        if( sqlite3StrICmp(zName+7, &PREFERRED_SCHEMA_TABLE[7])==0 ){
          p = sqlite3HashFind(&db->aDb[i].pSchema->tblHash,
                              LEGACY_SCHEMA_TABLE);
        }
      }
    }
  }else{
    /* Match against TEMP first */
    p = sqlite3HashFind(&db->aDb[1].pSchema->tblHash, zName);
    if( p ) return p;
    /* The main database is second */
    p = sqlite3HashFind(&db->aDb[0].pSchema->tblHash, zName);
    if( p ) return p;
    /* Attached databases are in order of attachment */
    for(i=2; i<db->nDb; i++){
      assert( sqlite3SchemaMutexHeld(db, i, 0) );
      p = sqlite3HashFind(&db->aDb[i].pSchema->tblHash, zName);
      if( p ) break;
    }
    if( p==0 && sqlite3StrNICmp(zName, "sqlite_", 7)==0 ){
      if( sqlite3StrICmp(zName+7, &PREFERRED_SCHEMA_TABLE[7])==0 ){
        p = sqlite3HashFind(&db->aDb[0].pSchema->tblHash, LEGACY_SCHEMA_TABLE);
      }else if( sqlite3StrICmp(zName+7, &PREFERRED_TEMP_SCHEMA_TABLE[7])==0 ){
        p = sqlite3HashFind(&db->aDb[1].pSchema->tblHash,
                            LEGACY_TEMP_SCHEMA_TABLE);
      }
    }
  }
  return p;
}
/* …（同文件无关代码省略）… */
char *sqlite3NameFromToken(sqlite3 *db, const Token *pName){
  char *zName;
  if( pName ){
    zName = sqlite3DbStrNDup(db, (const char*)pName->z, pName->n);
    sqlite3Dequote(zName);
  }else{
    zName = 0;
  }
  return zName;
}
/* …（同文件无关代码省略）… */
int sqlite3WritableSchema(sqlite3 *db){
  testcase( (db->flags&(SQLITE_WriteSchema|SQLITE_Defensive))==0 );
  testcase( (db->flags&(SQLITE_WriteSchema|SQLITE_Defensive))==
               SQLITE_WriteSchema );
  testcase( (db->flags&(SQLITE_WriteSchema|SQLITE_Defensive))==
               SQLITE_Defensive );
  testcase( (db->flags&(SQLITE_WriteSchema|SQLITE_Defensive))==
               (SQLITE_WriteSchema|SQLITE_Defensive) );
  return (db->flags&(SQLITE_WriteSchema|SQLITE_Defensive))==SQLITE_WriteSchema;
}
/* …（同文件无关代码省略）… */
** When parsing the sqlite_schema table, this routine also checks to
** make sure the "type", "name", and "tbl_name" columns are consistent
** with the SQL.
*/
int sqlite3CheckObjectName(
  Parse *pParse,            /* Parsing context */
  const char *zName,        /* Name of the object to check */
  const char *zType,        /* Type of this object */
  const char *zTblName      /* Parent table name for triggers and indexes */
){
  sqlite3 *db = pParse->db;
  if( sqlite3WritableSchema(db)
/* …（同文件无关代码省略）… */
    }
  }else{
    if( (pParse->nested==0 && 0==sqlite3StrNICmp(zName, "sqlite_", 7))
     || (sqlite3ReadOnlyShadowTables(db) && sqlite3ShadowTableName(db, zName))
    ){
      sqlite3ErrorMsg(pParse, "object name reserved for internal use: %s",
                      zName);
/* …（同文件无关代码省略）… */
    if( IN_RENAME_OBJECT ){
      sqlite3RenameTokenMap(pParse, (void*)zName, pName);
    }
  }
  pParse->sNameToken = *pName;
  if( zName==0 ) return;
  if( sqlite3CheckObjectName(pParse, zName, isView?"view":"table", zName) ){
    goto begin_table_error;
  }
  if( db->init.iDb==1 ) isTemp = 1;
/* …（同文件无关代码省略）… */
  */
  if( !IN_SPECIAL_PARSE ){
    char *zDb = db->aDb[iDb].zDbSName;
    if( SQLITE_OK!=sqlite3ReadSchema(pParse) ){
      goto begin_table_error;
    }
    pTable = sqlite3FindTable(db, zName, zDb);
    if( pTable ){
      if( !noErr ){
/* …（同文件无关代码省略）… */
int sqlite3IsShadowTableOf(sqlite3 *db, Table *pTab, const char *zName){
  int nName;                    /* Length of zName */
  Module *pMod;                 /* Module for the virtual table */

  if( !IsVirtual(pTab) ) return 0;
  nName = sqlite3Strlen30(pTab->zName);
  if( sqlite3_strnicmp(zName, pTab->zName, nName)!=0 ) return 0;
  if( zName[nName]!='_' ) return 0;
  pMod = (Module*)sqlite3HashFind(&db->aModule, pTab->u.vtab.azArg[0]);
  if( pMod==0 ) return 0;
  if( pMod->pModule->iVersion<3 ) return 0;
  if( pMod->pModule->xShadowName==0 ) return 0;
  return pMod->pModule->xShadowName(zName+nName+1);
}
/* …（同文件无关代码省略）… */

#ifndef SQLITE_OMIT_VIRTUALTABLE
/*
** Return true if zName is a shadow table name in the current database
** connection.
*/
int sqlite3ShadowTableName(sqlite3 *db, const char *zName){
  const char *zTail;            /* Pointer to the last "_" in zName */
  Table *pTab;                  /* Table that zName is a shadow of */
  char *zCopy;                  /* Transient copy of zName after last "_" */
  zTail = strrchr(zName, '_');
  if( zTail==0 ) return 0;
  zCopy = sqlite3DbStrNDup(db, zName, (int)(zTail-zName));
  pTab = zCopy ? sqlite3FindTable(db, zCopy, 0) : 0;
  sqlite3DbFree(db, zCopy);
  if( pTab==0 ) return 0;
  if( !IsVirtual(pTab) ) return 0;
  return sqlite3IsShadowTableOf(db, pTab, zName);
}
#endif /* ifndef SQLITE_OMIT_VIRTUALTABLE */

/* …（同文件无关代码省略）… */
  p = pParse->pNewTable;
  if( p==0 ) return;

  if( pSelect==0 && sqlite3ShadowTableName(db, p->zName) ){
    p->tabFlags |= TF_Shadow;
  }

/* …（同文件无关代码省略）… */
int sqlite3ReadOnlyShadowTables(sqlite3 *db){
#ifndef SQLITE_OMIT_VIRTUALTABLE
  if( (db->flags & SQLITE_Defensive)!=0
   && db->pVtabCtx==0
   && db->nVdbeExec==0
   && !sqlite3VtabInSync(db)
  ){
    return 1;
  }
#endif
  return 0;
}
/* …（同文件无关代码省略）… */
    zName = sqlite3NameFromToken(db, pName);
    if( zName==0 ) goto exit_create_index;
    assert( pName->z!=0 );
    if( SQLITE_OK!=sqlite3CheckObjectName(pParse, zName,"index",pTab->zName) ){
      goto exit_create_index;
    }
    if( !IN_RENAME_OBJECT ){
