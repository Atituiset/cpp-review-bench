// AUTO-DRAFT from sqlite/sqlite PR #8e1b59b6dc35197ec43b4dc2d31000fbd9d4c291
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct Qrf Qrf;
/* …（同文件无关代码省略）… */
#define qrfSpace(x) ((qrfCType[(unsigned char)x]&1)!=0)
#define qrfDigit(x) ((qrfCType[(unsigned char)x]&2)!=0)
/* …（同文件无关代码省略）… */
#  define deliberate_fall_through __attribute__((fallthrough));
/* …（同文件无关代码省略）… */
static void qrfError(
  Qrf *p,                /* Query result state */
  int iCode,             /* Error code */
  const char *zFormat,   /* Message format (or NULL) */
  ...
){
  p->iErr = iCode;
  if( p->pzErr!=0 ){
    sqlite3_free(*p->pzErr);
    *p->pzErr = 0;
    if( zFormat ){
      va_list ap;
      va_start(ap, zFormat);
      *p->pzErr = sqlite3_vmprintf(zFormat, ap);
      va_end(ap);
    }
  }
}
/* …（同文件无关代码省略）… */
static void qrfStrErr(Qrf *p, sqlite3_str *pStr){
  int rc = pStr ? sqlite3_str_errcode(pStr) : 0;
  if( rc ){
    qrfError(p, rc, sqlite3_errstr(rc));
  }
}
/* …（同文件无关代码省略）… */
  }
  if( p->u.pGraph ) p->u.pGraph->nWidth = nWidth;
  qrfStrErr(p, pLine);
  sqlite3_free(sqlite3_str_finish(pLine));
  qrfStrErr(p, pStats);
  sqlite3_free(sqlite3_str_finish(pStats));
#endif
}

/* …（同文件无关代码省略）… */
static void qrfWrite(Qrf *p){
  int n;
  if( p->spec.xWrite && (n = sqlite3_str_length(p->pOut))>0 ){
    int rc = p->spec.xWrite(p->spec.pWriteArg,
                 sqlite3_str_value(p->pOut),
                 (sqlite3_int64)n);
    sqlite3_str_reset(p->pOut);
    if( rc ){
      qrfError(p, rc, "Failed to write %d bytes of output", n);
    }
  }
}
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
/* …（同文件无关代码省略）… */
static void qrfEscape(
  int eEsc,            /* QRF_ESC_Ascii or QRF_ESC_Symbol */
  sqlite3_str *pStr,      /* String to be escaped */
  int iStart              /* Begin escapding on this byte of pStr */
){
  sqlite3_int64 i, j;     /* Loop counters */
  sqlite3_int64 nCtrl = 0;/* Number of control characters to escape */
  unsigned char *zIn;     /* Text to be escaped */
  unsigned int nIn;       /* Bytes of text to be escaped */
  unsigned char c;        /* A single character of the text */
  unsigned char *zOut;    /* Where to write the results */

  /* Find the text to be escaped */
  zIn = (unsigned char*)sqlite3_str_value(pStr);
  nIn = sqlite3_str_length(pStr);
  if( zIn==0 ) return;
  zIn += iStart;
  nIn -= iStart;

  /* Count the control characters */
  for(i=0; i<nIn; i++){
    if( (c = zIn[i])<=0x1f
     && c!='\t'
     && c!='\n'
     && (c!='\r' || zIn[i+1]!='\n')
    ){
      nCtrl++;
    }
  }
  if( nCtrl==0 ) return;  /* Early out if no control characters */

  /* Make space to hold the escapes.  Copy the original text to the end
  ** of the available space. */
  if( eEsc==QRF_ESC_Symbol ) nCtrl *= 2;
  sqlite3_str_appendchar(pStr, nCtrl, ' ');
  zOut = (unsigned char*)sqlite3_str_value(pStr);
  if( zOut==0 ) return;
  zOut += iStart;
  zIn = zOut + nCtrl;
  memmove(zIn,zOut,nIn);

  /* Convert the control characters */
  for(i=j=0; i<nIn; i++){
    if( (c = zIn[i])>0x1f
     || c=='\t'
     || c=='\n'
     || (c=='\r' && zIn[i+1]=='\n')
    ){
      continue;
    }
    if( i>0 ){
      memmove(&zOut[j], zIn, i);
      j += i;
    }
    zIn += i+1;
    nIn -= i+1;
    i = -1;
    if( eEsc==QRF_ESC_Symbol ){
      zOut[j++] = 0xe2;
      zOut[j++] = 0x90;
      zOut[j++] = 0x80+c;
    }else{
      zOut[j++] = '^';
      zOut[j++] = 0x40+c;
    }
  }
}
/* …（同文件无关代码省略）… */
static int qrfRelaxable(Qrf *p, const char *z){
  size_t i, n;
  if( z[0]=='\'' || qrfSpace(z[0]) ) return 0;
  if( z[0]==0 ){
    return (p->spec.zNull!=0 && p->spec.zNull[0]!=0);
  }
  n = strlen(z);
  if( n==0 || z[n-1]=='\'' || qrfSpace(z[n-1]) ) return 0;
  if( p->spec.zNull && strcmp(p->spec.zNull,z)==0 ) return
