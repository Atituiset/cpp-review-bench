// AUTO-DRAFT from sqlite/sqlite PR #b8f3f5e1d5d475ba07945ec86e3255e5efdc0135
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
#define FTS5_DEFAULT_AUTOMERGE      4
/* …（同文件无关代码省略）… */
#define FTS5_DEFAULT_CRISISMERGE   16
/* …（同文件无关代码省略）… */
#define FTS5_DEFAULT_DELETE_AUTOMERGE 10      /* default 10% */
/* …（同文件无关代码省略）… */
#define FTS5_MAX_PAGE_SIZE (64*1024)
/* …（同文件无关代码省略）… */
static int fts5_iswhitespace(char x){
  return (x==' ');
}
/* …（同文件无关代码省略）… */
static const char *fts5ConfigSkipWhitespace(const char *pIn){
  const char *p = pIn;
  if( p ){
    while( fts5_iswhitespace(*p) ){ p++; }
  }
  return p;
}
/* …（同文件无关代码省略）… */
static const char *fts5ConfigSkipBareword(const char *pIn){
  const char *p = pIn;
  while ( sqlite3Fts5IsBareword(*p) ) p++;
  if( p==pIn ) p = 0;
  return p;
}
/* …（同文件无关代码省略）… */
static int fts5_isdigit(char a){
  return (a>='0' && a<='9');
}
/* …（同文件无关代码省略）… */
static const char *fts5ConfigSkipLiteral(const char *pIn){
  const char *p = pIn;
  switch( *p ){
    case 'n': case 'N':
      if( sqlite3_strnicmp("null", p, 4)==0 ){
        p = &p[4];
      }else{
        p = 0;
      }
      break;

    case 'x': case 'X':
      p++;
      if( *p=='\'' ){
        p++;
        while( (*p>='a' && *p<='f') 
            || (*p>='A' && *p<='F') 
            || (*p>='0' && *p<='9') 
            ){
          p++;
        }
        if( *p=='\'' && 0==((p-pIn)%2) ){
          p++;
        }else{
          p = 0;
        }
      }else{
        p = 0;
      }
      break;

    case '\'':
      p++;
      while( p ){
        if( *p=='\'' ){
          p++;
          if( *p!='\'' ) break;
        }else if( *p==0 ){
          p = 0;
          break;
        }
        p++;
        if( *p==0 ) p = 0;
      }
      break;

    default:
      /* maybe a number */
      if( *p=='+' || *p=='-' ) p++;
      while( fts5_isdigit(*p) ) p++;

      /* At this point, if the literal was an integer, the parse is 
      ** finished. Or, if it is a floating point value, it may continue
      ** with either a decimal point or an 'E' character. */
      if( *p=='.' && fts5_isdigit(p[1]) ){
        p += 2;
        while( fts5_isdigit(*p) ) p++;
      }
      if( p==pIn ) p = 0;

      break;
  }

  return p;
}
/* …（同文件无关代码省略）… */
static const char *fts5ConfigSkipArgs(const char *pIn){
  const char *p = pIn;
  
  while( 1 ){
    p = fts5ConfigSkipWhitespace(p);
    p = fts5ConfigSkipLiteral(p);
    p = fts5ConfigSkipWhitespace(p);
    if( p==0 || *p==')' ) break;
    if( *p!=',' ){
      p = 0;
      break;
    }
    p++;
  }

  return p;
}
/* …（同文件无关代码省略）… */
int sqlite3Fts5ConfigParseRank(
  const char *zIn,                /* Input string */
  char **pzRank,                  /* OUT: Rank function name */
  char **pzRankArgs               /* OUT: Rank function arguments */
){
  const char *p = zIn;
  const char *pRank;
  char *zRank = 0;
  char *zRankArgs = 0;
  int rc = SQLITE_OK;

  *pzRank = 0;
  *pzRankArgs = 0;

  if( p==0 ){
    rc = SQLITE_ERROR;
  }else{
    p = fts5ConfigSkipWhitespace(p);
    pRank = p;
    p = fts5ConfigSkipBareword(p);

    if( p ){
      zRank = sqlite3Fts5MallocZero(&rc, 1 + p - pRank);
      if( zRank ) memcpy(zRank, pRank, p-pRank);
    }else{
      rc = SQLITE_ERROR;
    }

    if( rc==SQLITE_OK ){
      p = fts5ConfigSkipWhitespace(p);
      if( *p!='(' ) rc = SQLITE_ERROR;
      p++;
    }
    if( rc==SQLITE_OK ){
      const char *pArgs; 
      p = fts5ConfigSkipWhitespace(p);
      pArgs = p;
      if( *p!=')' ){
        p = fts5ConfigSkipArgs(p);
        if( p==0 ){
          rc = SQLITE_ERROR;
        }else{
          zRankArgs = sqlite3Fts5MallocZero(&rc, 1 + p - pArgs);
          if( zRankArgs ) memcpy(zRankArgs, pArgs, p-pArgs);
        }
      }
    }
  }

  if( rc!=SQLITE_OK ){
    sqlite3_free(zRank);
    assert( zRankArgs==0 );
  }else{
    *pzRank = zRank;
    *pzRankArgs = zRankArgs;
  }
  return rc;
}
/* …（同文件无关代码省略）… */
int sqlite3Fts5ConfigSetValue(
  Fts5Config *pConfig, 
  const char *zKey, 
  sqlite3_value *pVal,
  int *pbBadkey
){
  int rc = SQLITE_OK;

  if( 0==sqlite3_stricmp(zKey, "pgsz") ){
    int pgsz = 0;
    if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
      pgsz = sqlite3_value_int(pVal);
    }
    if( pgsz<32 || pgsz>FTS5_MAX_PAGE_SIZE ){
      *pbBadkey = 1;
    }else{
      pConfig->pgsz = pgsz;
    }
  }

  else if( 0==sqlite3_stricmp(zKey, "hashsize") ){
    int nHashSize = -1;
    if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
      nHashSize = sqlite3_value_int(pVal);
    }
    if( nHashSize<=0 ){
      *pbBadkey = 1;
    }else{
      pConfig->nHashSize = nHashSize;
    }
  }

  else if( 0==sqlite3_stricmp(zKey, "automerge") ){
    int nAutomerge = -1;
    if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
      nAutomerge = sqlite3_value_int(pVal);
    }
    if( nAutomerge<0 || nAutomerge>64 ){
      *pbBadkey = 1;
    }else{
      if( nAutomerge==1 ) nAutomerge = FTS5_DEFAULT_AUTOMERGE;
      pConfig->nAutomerge = nAutomerge;
    }
  }

  else if( 0==sqlite3_stricmp(zKey, "usermerge") ){
    int nUsermerge = -1;
    if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
      nUsermerge = sqlite3_value_int(pVal);
    }
    if( nUsermerge<2 || nUsermerge>16 ){
      *pbBadkey = 1;
    }else{
      pConfig->nUsermerge = nUsermerge;
    }
  }

  else if( 0==sqlite3_stricmp(zKey, "crisismerge") ){
    int nCrisisMerge = -1;
    if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
      nCrisisMerge = sqlite3_value_int(pVal);
    }
    if( nCrisisMerge<0 ){
      *pbBadkey = 1;
    }else{
      if( nCrisisMerge<=1 ) nCrisisMerge = FTS5_DEFAULT_CRISISMERGE;
      if( nCrisisMerge>=FTS5_MAX_SEGMENT ) nCrisisMerge = FTS5_MAX_SEGMENT-1;
      pConfig->nCrisisMerge = nCrisisMerge;
    }
  }

  else if( 0==sqlite3_stricmp(zKey, "deletemerge") ){
    int nVal = -1;
    if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
      nVal = sqlite3_value_int(pVal);
    }else{
      *pbBadkey = 1;
    }
    if( nVal<0 ) nVal = FTS5_DEFAULT_DELETE_AUTOMERGE;
    if( nVal>100 ) nVal = 0;
    pConfig->nDeleteMerge = nVal;
  }

  else if( 0==sqlite3_stricmp(zKey, "rank") ){
    const char *zIn = (const char*)sqlite3_value_text(pVal);
    char *zRank;
    char *zRankArgs;
    rc = sqlite3Fts5ConfigParseRank(zIn, &zRank, &zRankArgs);
    if( rc==SQLITE_OK ){
      sqlite3_free(pConfig->zRank);
      sqlite3_free(pConfig->zRankArgs);
      pConfig->zRank = zRank;
      pConfig->zRankArgs = zRankArgs;
    }else if( rc==SQLITE_ERROR ){
      rc = SQLITE_OK;
      *pbBadkey = 1;
    }
  }

  else if( 0==sqlite3_stricmp(zKey, "secure-delete") ){
    int bVal = -1;
    if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
      bVal = sqlite3_value_int(pVal);
    }
    if( bVal<0 ){
      *pbBadkey = 1;
    }else{
      pConfig->bSecureDelete = (bVal ? 1 : 0);
    }
  }

  else if( 0==sqlite3_stricmp(zKey, "insttoken") ){
    int bVal = -1;
    if( SQLITE_INTEGER==sqlite3_value_numeric_type(pVal) ){
      bVal = sqlite3_value_int(pVal);
    }
    if( bVal<0 ){
      *pbBadkey = 1;
    }else{
      pConfig->bPrefixInsttoken = (bVal ? 1 : 0);
    }

  }else{
    *pbBadkey = 1;
  }
  return rc;
}

/* …（同文件无关代码省略）… */
      if( 0==sqlite3_stricmp(zK, "version") ){
        iVersion = sqlite3_value_int(pVal);
      }else{
        int bDummy = 0;
        sqlite3Fts5ConfigSetValue(pConfig, zK, pVal, &bDummy);
      }
    }
    rc = sqlite3_finalize(p);
