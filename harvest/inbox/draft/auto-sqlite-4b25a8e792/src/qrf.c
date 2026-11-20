// AUTO-DRAFT from sqlite/sqlite PR #59932cc0c81387b108add8f2568a2b9b6e82313d
#define qrfSpace(x) ((qrfCType[(unsigned char)x]&1)!=0)
/* …（同文件无关代码省略）… */
#define qrfAlnum(x) ((qrfCType[(unsigned char)x]&6)!=0)
/* …（同文件无关代码省略）… */
int sqlite3_qrf_wcwidth(int c){
  int iFirst, iLast;

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
static int qrfIsVt100(const unsigned char *z){
  int i;
  if( z[1]!='[' ) return 0;
  i = 2;
  while( z[i]>=0x30 && z[i]<=0x3f ){ i++; }
  while( z[i]>=0x20 && z[i]<=0x2f ){ i++; }
  if( z[i]<0x40 || z[i]>0x7e ) return 0;
  return i+1;
}
/* …（同文件无关代码省略）… */
static int qrfDisplayWidth(const char *zIn, sqlite3_int64 nByte, int *pnNL){
  const unsigned char *z;
  const unsigned char *zEnd;
  int mx = 0;
  int n = 0;
  int nNL = 0;
  if( zIn==0 ) zIn = "";
  z = (const unsigned char*)zIn;
  zEnd = &z[nByte];
  while( z<zEnd ){
    if( z[0]<' ' ){
      int k;
      if( z[0]=='\033' && (k = qrfIsVt100(z))>0 ){
        z += k;
      }else{
        if( z[0]=='\t' ){
          n = (n+8)&~7;
        }else if( z[0]=='\n' || z[0]=='\r' ){
          nNL++;
          if( n>mx ) mx = n;
          n = 0;
        }else{
          /* Any other control character is shown by qrfAppendWithTabs()
          ** as a one-column-wide Control Picture symbol, and is counted as
          ** one column by qrfWrapLine().  Count it as one column here too,
          ** so that columns are wide enough to hold it. */
          n++;
        }
        z++;
      }
    }else if( (0x80&z[0])==0 ){
      n++;
      z++;
    }else{
      int u = 0;
      int len = sqlite3_qrf_decode_utf8(z, &u);
      z += len;
      n += sqlite3_qrf_wcwidth(u);
    }
  }
  if( mx>n ) n = mx;
  if( pnNL ) *pnNL = nNL;
  return n;
}

/*
** Escape the text starting at byte iStart of pStr, if needed, using the
** escape encoding of eEsc, which is either QRF_ESC_Ascii or QRF_ESC_Symbol.
/* …（同文件无关代码省略）… */
static void qrfWrapLine(
  const char *zIn,   /* Input text to be displayed */
  int w,             /* Column width in characters (not bytes) */
  int bWrap,         /* True if we should do word-wrapping */
  int *pnThis,       /* OUT: How many bytes of z[] for the current line */
  int *pnWide,       /* OUT: How wide is the text of this line */
  int *piNext        /* OUT: Offset into z[] to start of the next line */
){
  int i;                 /* Input bytes consumed */
  int k;                 /* Bytes in a VT100 code */
  int n;                 /* Output column number */
  const unsigned char *z = (const unsigned char*)zIn;
  unsigned char c = 0;

  if( z[0]==0 ){
    *pnThis = 0;
    *pnWide = 0;
    *piNext = 0;
    return;
  }
  n = 0;
  for(i=0; n<=w; i++){
    c = z[i];
    if( c>=0xc0 ){
      int u;
      int len = sqlite3_qrf_decode_utf8(&z[i], &u);
      int wcw = sqlite3_qrf_wcwidth(u);
      if( wcw+n>w ) break;
      i += len-1;
      n += wcw;
      continue;
    }
    if( c>=' ' ){
      if( n==w ) break;
      n++;
      continue;
    }
    if( c==0 || c=='\n' ) break;
    if( c=='\r' && z[i+1]=='\n' ){ c = z[++i]; break; }
    if( c=='\t' ){
      int wcw = 8 - (n&7);
      if( n+wcw>w ) break;
      n += wcw;
      continue;
    }
    if( c==0x1b && (k = qrfIsVt100(&z[i]))>0 ){
      i += k-1;
    }else if( n==w ){
      break;
    }else{
      n++;
    }
  }
  if( c==0 ){
    *pnThis = i;
    *pnWide = n;
    *piNext = i;
    return;
  }
  if( c=='\n' ){
    *pnThis = i;
    *pnWide = n;
    *piNext = i+1;
    return;
  }

  /* If we get this far, that means the current line will end at some
  ** point that is neither a "\n" or a 0x00.  Figure out where that
  ** split should occur
  */
  if( bWrap && z[i]!=0 && !qrfSpace(z[i]) && qrfAlnum(c)==qrfAlnum(z[i]) ){
    /* Perhaps try to back up to a better place to break the line */
    for(k=i-1; k>=i/2; k--){
      if( qrfSpace(z[k]) ) break;
    }
    if( k<i/2 && i/2>0 ){
      for(k=i; k>=i/2; k--){
        if( qrfAlnum(z[k-1])!=qrfAlnum(z[k]) && (z[k]&0xc0)!=0x80 ) break;
      }
    }
    if( k>=i/2 ){
      i = k;
      n = qrfDisplayWidth((const char*)z, k, 0);
    }
  }
  *pnThis = i;
  *pnWide = n;
  while( zIn[i]==' ' || zIn[i]=='\t' || zIn[i]=='\r' ){ i++; }
  *piNext = i;
}
/* …（同文件无关代码省略）… */
static void qrfAppendWithTabs(
  sqlite3_str *pOut,       /* Append text here */
  const char *zVal,        /* Text to append */
  int nVal                 /* Use only the first nVal bytes of zVal[] */
){
  int i = 0;
  unsigned int col = 0;
  unsigned char *z = (unsigned char *)zVal;
  while( i<nVal ){
    unsigned char c = z[i];
    if( c<' ' ){
      int k;
      sqlite3_str_append(pOut, (const char*)z, i);
      nVal -= i;
      z += i;
      i = 0;
      if( c=='\033' && (k = qrfIsVt100(z))>0 ){
        sqlite3_str_append(pOut, (const char*)z, k);
        z += k;
        nVal -= k;
      }else if( c=='\t' ){
        k = 8 - (col&7);
        sqlite3_str_appendchar(pOut, k, ' ');
        col += k;
        z++;
        nVal--;
      }else if( 
