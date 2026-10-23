// AUTO-DRAFT from sqlite/sqlite PR #20822819adf299e3a7609a26206bf2b43ac402ff
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
#  define safe_isdigit(x)  sqlite3Isdigit(x)
/* …（同文件无关代码省略）… */
#define fast_isspace(x) (geopolyIsSpace[(unsigned char)x])
/* …（同文件无关代码省略）… */
typedef float GeoCoord;
/* …（同文件无关代码省略）… */
typedef struct GeoPoly GeoPoly;
/* …（同文件无关代码省略）… */
#define GEOPOLY_SZ(N)  (sizeof(GeoPoly) + sizeof(GeoCoord)*2*((N)-4))
/* …（同文件无关代码省略）… */
#define GeoX(P,I)  (((GeoCoord*)(P)->a)[(I)*2])
#define GeoY(P,I)  (((GeoCoord*)(P)->a)[(I)*2+1])
/* …（同文件无关代码省略）… */
typedef struct GeoParse GeoParse;
/* …（同文件无关代码省略）… */
static void geopolySwab32(unsigned char *a){
  unsigned char t = a[0];
  a[0] = a[3];
  a[3] = t;
  t = a[1];
  a[1] = a[2];
  a[2] = t;
}
/* …（同文件无关代码省略）… */
static char geopolySkipSpace(GeoParse *p){
  while( fast_isspace(p->z[0]) ) p->z++;
  return p->z[0];
}
/* …（同文件无关代码省略）… */
static int geopolyParseNumber(GeoParse *p, GeoCoord *pVal){
  char c = geopolySkipSpace(p);
  const unsigned char *z = p->z;
  int j = 0;
  int seenDP = 0;
  int seenE = 0;
  if( c=='-' ){
    j = 1;
    c = z[j];
  }
  if( c=='0' && z[j+1]>='0' && z[j+1]<='9' ) return 0;
  for(;; j++){
    c = z[j];
    if( safe_isdigit(c) ) continue;
    if( c=='.' ){
      if( z[j-1]=='-' ) return 0;
      if( seenDP ) return 0;
      seenDP = 1;
      continue;
    }
    if( c=='e' || c=='E' ){
      if( z[j-1]<'0' ) return 0;
      if( seenE ) return -1;
      seenDP = seenE = 1;
      c = z[j+1];
      if( c=='+' || c=='-' ){
        j++;
        c = z[j+1];
      }
      if( c<'0' || c>'9' ) return 0;
      continue;
    }
    break;
  }
  if( z[j-1]<'0' ) return 0;
  if( pVal ){
#ifdef SQLITE_AMALGAMATION
     /* The sqlite3AtoF() routine is much much faster than atof(), if it
     ** is available */
     double r;
     (void)sqlite3AtoF((const char*)p->z, &r);
     *pVal = r;
#else
     *pVal = (GeoCoord)atof((const char*)p->z);
#endif
  }
  p->z += j;
  return 1;
}
/* …（同文件无关代码省略）… */
static GeoPoly *geopolyParseJson(const unsigned char *z, int *pRc){
  GeoParse s;
  int rc = SQLITE_OK;
  memset(&s, 0, sizeof(s));
  s.z = z;
  if( geopolySkipSpace(&s)=='[' ){
    s.z++;
    while( geopolySkipSpace(&s)=='[' ){
      int ii = 0;
      char c;
      s.z++;
      if( s.nVertex>=s.nAlloc ){
        GeoCoord *aNew;
        s.nAlloc = s.nAlloc*2 + 16;
        aNew = sqlite3_realloc64(s.a, s.nAlloc*sizeof(GeoCoord)*2 );
        if( aNew==0 ){
          rc = SQLITE_NOMEM;
          s.nErr++;
          break;
        }
        s.a = aNew;
      }
      while( geopolyParseNumber(&s, ii<=1 ? &s.a[s.nVertex*2+ii] : 0) ){
        ii++;
        if( ii==2 ) s.nVertex++;
        c = geopolySkipSpace(&s);
        s.z++;
        if( c==',' ) continue;
        if( c==']' && ii>=2 ) break;
        s.nErr++;
        rc = SQLITE_ERROR;
        goto parse_json_err;
      }
      if( geopolySkipSpace(&s)==',' ){
        s.z++;
        continue;
      }
      break;
    }
    if( geopolySkipSpace(&s)==']'
     && s.nVertex>=4 && s.nVertex<16777216
     && s.a[0]==s.a[s.nVertex*2-2]
     && s.a[1]==s.a[s.nVertex*2-1]
     && (s.z++, geopolySkipSpace(&s)==0)
    ){
      GeoPoly *pOut;
      int x = 1;
      s.nVertex--;  /* Remove the redundant vertex at the end */
      pOut = sqlite3_malloc64( GEOPOLY_SZ((sqlite3_int64)s.nVertex) );
      x = 1;
      if( pOut==0 ) goto parse_json_err;
      pOut->nVertex = s.nVertex;
      memcpy(pOut->a, s.a, s.nVertex*2*sizeof(GeoCoord));
      pOut->hdr[0] = *(unsigned char*)&x;
      pOut->hdr[1] = (s.nVertex>>16)&0xff;
      pOut->hdr[2] = (s.nVertex>>8)&0xff;
      pOut->hdr[3] = s.nVertex&0xff;
      sqlite3_free(s.a);
      if( pRc ) *pRc = SQLITE_OK;
      return pOut;
    }else{
      s.nErr++;
      rc = SQLITE_ERROR;
    }
  }
parse_json_err:
  if( pRc ) *pRc = rc;
  sqlite3_free(s.a);
  return 0;
}
/* …（同文件无关代码省略）… */
static GeoPoly *geopolyFuncParam(
  sqlite3_context *pCtx,      /* Context for error messages */
  sqlite3_value *pVal,        /* The value to decode */
  int *pRc                    /* Write error here */
){
  GeoPoly *p = 0;
  int nByte;
  testcase( pCtx==0 );
  if( sqlite3_value_type(pVal)==SQLITE_BLOB
   && (nByte = sqlite3_value_bytes(pVal))>=(int)(4+6*sizeof(GeoCoord))
  ){
    const unsigned char *a = sqlite3_value_blob(pVal);
    int nVertex;
    if( a==0 ){
      if( pCtx ) sqlite3_result_error_nomem(pCtx);
      return 0;
    }
    nVertex = (a[1]<<16) + (a[2]<<8) + a[3];
    if( (a[0]==0 || a[0]==1)
     && (nVertex*2*sizeof(GeoCoord) + 4)==(unsigned int)nByte
    ){
      p = sqlite3_malloc64( sizeof(*p) + (nVertex-1)*2*sizeof(GeoCoord) );
      if( p==0 ){
        if( pRc ) *pRc = SQLITE_NOMEM;
        if( pCtx ) sqlite3_result_error_nomem(pCtx);
      }else{
        int x = 1;
        p->nVertex = nVertex;
        memcpy(p->hdr, a, nByte);
        if( a[0] != *(unsigned char*)&x ){
          int ii;
          for(ii=0; ii<nVertex; ii++){
            geopolySwab32((unsigned char*)&GeoX(p,ii));
            geopolySwab32((unsigned char*)&GeoY(p,ii));
          }
          p->hdr[0] ^= 1;
        }
      }
    }
    if( pRc ) *pRc = SQLITE_OK;
    return p;
  }else if( sqlite3_value_type(pVal)==SQLITE_TEXT ){
    const unsigned char *zJson = sqlite3_value_text(pVal);
    if( zJson==0 ){
      if( pRc ) *pRc = SQLITE_NOMEM;
      return 0;
    }
    return geopolyParseJson(zJson, pRc);
  }else{
    if( pRc ) *pRc = SQLITE_ERROR;
    return 0;
  }
}
/* …（同文件无关代码省略）… */
static GeoPoly *geopolyBBox(
  sqlite3_context *context,   /* For recording the error */
  sqlite3_value *pPoly,       /* The polygon */
  RtreeCoord *aCoord,         /* Results here */
  int *pRc                    /* Error code here */
){
  GeoPoly *pOut = 0;
  GeoPoly *p;
  float mnX, mxX, mnY, mxY;
  if( pPoly==0 && aCoord!=0 ){
    p = 0;
    mnX = aCoord[0].f;
    mxX = aCoord[1].f;
    mnY = aCoord[2].f;
    mxY = aCoord[3].f;
    goto geopolyBboxFill;
  }else{
    p = geopolyFuncParam(context, pPoly, pRc);
  }
  if( p ){
    int ii;
    mnX = mxX = GeoX(p,0);
    mnY = mxY = GeoY(p,0);
    for(ii=1; ii<p->nVertex; ii++){
      double r = GeoX(p,ii);
      if( r<mnX ) mnX = (float)r;
      else if( r>mxX ) mxX = (float)r;
      r = GeoY(p,ii);
      if( r<mnY ) mnY = (float)r;
      else if( r>mxY ) mxY = (float)r;
    }
    if( pRc ) *pRc = SQLITE_OK;
    if( aCoord==0 ){
      geopolyBboxFill:
      pOut = sqlite3_realloc64(p, GEOPOLY_SZ(4));
      if( pOut==0 ){
        sqlite3_free(p);
        if( context ) sqlite3_result_error_nomem(context);
        if( pRc ) *pRc = SQLITE_NOMEM;
        return 0;
      }
      pOut->nVertex = 4;
      ii = 1;
      pOut->hdr[0] = *(unsigned char*)&ii;
      pOut->hdr[1] = 0;
      pOut->hdr[2] = 0;
      pOut->hdr[3] = 4;
      GeoX(pOut,0) = mnX;
      GeoY(pOut,0) = mnY;
      GeoX(pOut,1) = mxX;
      GeoY(pOut,1) = mnY;
      GeoX(pOut,2) = mxX;
      GeoY(pOut,2) = mxY;
      GeoX(pOut,3) = mnX;
      GeoY(pOut,3) = mxY;
    }else{
      sqlite3_free(p);
      aCoord[0].f = mnX;
      aCoord[1].f = mxX;
      aCoord[2].f = mnY;
      aCoord[3].f = mxY;
    }
  }else if( aCoord ){
    memset(aCoord, 0, sizeof(RtreeCoord)*4);
  }
  return pOut;
}
/* …（同文件无关代码省略）… */
    /* Special case - lookup by rowid. */
    RtreeNode *pLeaf;        /* Leaf on which the required cell resides */
    RtreeSearchPoint *p;     /* Search point for the leaf */
    i64 iRowid = sqlite3_value_int64(argv[0]);
    i64 iNode = 0;
    rc = findLeafNode(pRtree, iRowid, &pLeaf, &iNode);
    if( rc==SQLITE_OK && pLeaf!=0 ){
      p = rtreeSearchPointNew(pCsr, RTREE_ZERO, 0);
      assert( p!=0 );  /* Always returns pCsr->sPoint */
/* …（同文件无关代码省略）… */
  i64 newRowid;                   /* The new rowid */
  int newRowidValid;              /* True if newRowid is valid */
  int coordChange = 0;            /* Change in coordinates */

  if( pRtree->nNodeRef ){
    /* Unable to write to the btree while another cursor is reading from it,
/* …（同文件无关代码省略）… */
  newRowid = newRowidValid ? sqlite3_value_int64(aData[1]) : 0;
  cell.iRowid = newRowid;

  if( nData>1                                 /* not a DELETE */
   && (!oldRowidValid                         /* INSERT */
        || !sqlite3_value_nochange(aData[2])  /* UPDATE _shape */
        || oldRowid!=newRowid)                /* Rowid change */
  ){
    assert( aData[2]!=0 );
    geopolyBBox(0, aData[2], cell.aCoord, &rc);
    if( rc ){
      if( rc==SQLITE_ERROR ){
        pVtab->zErrMsg =
/* …（同文件无关代码省略）… */
    int nChange = 0;
    sqlite3_bind_int64(pUp, 1, cell.iRowid);
    assert( pRtree->nAux>=1 );
    if( sqlite3_value_nochange(aData[2]) ){
      sqlite3_bind_null(pUp, 2);
    }else{
      GeoPoly *p = 0;
      if( sqlite3_value_type(aData[2])==SQLITE_TEXT
       && (p = geopolyFuncParam(0, aData[2], &rc))!=0
       && rc==SQLITE_OK
      ){
        sqlite3_bind_blob(pUp, 2, p->hdr, 4+8*p->nVertex, SQLITE_TRANSIENT);
      }else{
        sqlite3_bind_value(pUp, 2, aData[2]);
      }
      sqlite3_free(p);
      nChange = 1;
/* …（同文件无关代码省略）… */
  }

geopoly_update_end:
  rtreeRelease(pRtree);
  return rc;
}
