// AUTO-DRAFT from sqlite/sqlite PR #95635a25d5a058dac504649debd970ba2f465801
static int alterRtrimFragment(
  sqlite3 *db,                    /* used to record OOM error */
  const char *pFrag,              /* Buffer containing SQL fragment */
  int nFrag                       /* Size of pFrag in bytes */
){
  u8 *zTmp = (u8*)sqlite3MPrintf(db, "%.*s", nFrag, pFrag);
  int iOff = 0;
  int iEnd = 0;

  if( zTmp==0 ) return 0;

  while( 1 ){
    int t = 0;
    int nToken = sqlite3GetToken(&zTmp[iOff], &t);
    if( t==TK_ILLEGAL ) break;
    if( t==TK_COMMENT ){
      /* Ignore all but properly terminated C-style comments */
      if( zTmp[iOff]=='/' && nToken>=4
       && zTmp[iOff+nToken-2]=='*' && zTmp[iOff+nToken-1]=='/'
      ){
        iEnd = iOff+nToken;
      }
    }else if( t!=TK_SPACE && t!=TK_SEMI ){
      iEnd = iOff+nToken;
    }
    iOff += nToken;
  }

  sqlite3DbFree(db, zTmp);
  return iEnd;
}
/* …（同文件无关代码省略）… */
    int isStrict = pTab->tabFlags & TF_Strict;
    int isGen = (pCol->colFlags & COLFLAG_GENERATED)!=0;

    /* Make sure the schema version is at least 3.  But do not upgrade
    ** from less than 3 to 4, as that will corrupt any preexisting DESC
    ** index.
/* …（同文件无关代码省略）… */
  pCons = pFirst->z;
  nCons = alterRtrimFragment(pParse->db, pCons, pParse->sLastToken.z - pCons);

  /* Search for a constraint violation. Throw an exception if one is found. */
  sqlite3NestedParse(pParse,
      "SELECT sqlite_fail('constraint failed', %d) "
/* …（同文件无关代码省略）… */
    sqlite3DbFree(pParse->db, zName);
  }

  /* Search for a constraint violation. Throw an exception if one is found. */
  sqlite3NestedParse(pParse,
      "SELECT sqlite_fail('constraint failed', %d) "
