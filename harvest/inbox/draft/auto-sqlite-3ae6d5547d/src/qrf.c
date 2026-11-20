// AUTO-DRAFT from sqlite/sqlite PR #4ca7ac35faafec303ca05cd8e2f96b08fde59784
int sqlite3_qrf_wcwidth(int c){
  int iFirst, iLast;
  // <<< BUG ANCHOR
  /* Fast path for common characters */
  if( c<0x300 ) return 1;

  /* The general case */
  iFirst = 0;
  iLast = sizeof(aQrfUWidth)/sizeof(aQrfUWidth[0]) - 1;
  while( iFirst<iLast-1 ){
    int iMid = (iFirst+iLast)/2;
    int cMid = aQrfUWidth[iMid].iFirst;
    if( cMid < c ){
      iFirst = iMid;
    }else if( cMid > c ){
      iLast = iMid - 1;
    }else{
      return aQrfUWidth[iMid].w;
    }
  }
  if( aQrfUWidth[iLast].iFirst > c ) return aQrfUWidth[iFirst].w;
  return aQrfUWidth[iLast].w;
}
/* …（同文件无关代码省略）… */
int sqlite3_qrf_decode_utf8(const unsigned char *z, int *pU){
  if( (z[0] & 0xe0)==0xc0 && (z[1] & 0xc0)==0x80 ){
    *pU = ((z[0] & 0x1f)<<6) | (z[1] & 0x3f);
    return 2;
  }
  if( (z[0] & 0xf0)==0xe0 && (z[1] & 0xc0)==0x80 && (z[2] & 0xc0)==0x80 ){
    *pU = ((z[0] & 0x0f)<<12) | ((z[1] & 0x3f)<<6) | (z[2] & 0x3f);
    return 3;
  }
  if( (z[0] & 0xf8)==0xf0 && (z[1] & 0xc0)==0x80 && (z[2] & 0xc0)==0x80
   && (z[3] & 0xc0)==0x80
  ){
    *pU = ((z[0] & 0x0f)<<18) | ((z[1] & 0x3f)<<12) | ((z[2] & 0x3f))<<6
                              | (z[3] & 0x3f);
    return 4;
  }
  *pU = 0;
  return 1;
}
/* …（同文件无关代码省略）… */
static int qrfHasWideChar(const char *zIn){
  const unsigned char *z = (const unsigned char*)zIn;
  while( z[0] ){
    if( z[0]>=0xc0 ){
      int u = 0;
      z += sqlite3_qrf_decode_utf8(z, &u);
      if( sqlite3_qrf_wcwidth(u)>1 ) return 1;
    }else{
      z++;
    }
  }
  return 0;
}
/* …（同文件无关代码省略）… */
        if( w==1 ){
          /* A double-wide character needs a column at least 2 wide, so
          ** that qrfWrapLine() always makes progress. */
          int j;
          for(j=i; j<data.n; j+=nColumn){
            if( qrfHasWideChar(data.az[j]) ){
              w = 2;
              break;
            }
