// AUTO-DRAFT from sqlite/sqlite PR #99d15842d0a5357e6a33be2456f9c969da43fac8
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
typedef struct DateTime DateTime;
/* …（同文件无关代码省略）… */
static int getDigits(const char *zDate, const char *zFormat, ...){
  /* The aMx[] array translates the 3rd character of each format
  ** spec into a max size:    a   b   c   d   e      f */
  static const u16 aMx[] = { 12, 14, 24, 31, 59, 14712 };
  va_list ap;
  int cnt = 0;
  char nextC;
  va_start(ap, zFormat);
  do{
    char N = zFormat[0] - '0';
    char min = zFormat[1] - '0';
    int val = 0;
    u16 max;

    assert( zFormat[2]>='a' && zFormat[2]<='f' );
    max = aMx[zFormat[2] - 'a'];
    nextC = zFormat[3];
    val = 0;
    while( N-- ){
      if( !sqlite3Isdigit(*zDate) ){
        goto end_getDigits;
      }
      val = val*10 + *zDate - '0';
      zDate++;
    }
    if( val<(int)min || val>(int)max || (nextC!=0 && nextC!=*zDate) ){
      goto end_getDigits;
    }
    *va_arg(ap,int*) = val;
    zDate++;
    cnt++;
    zFormat += 4;
  }while( nextC );
end_getDigits:
  va_end(ap);
  return cnt;
}
/* …（同文件无关代码省略）… */
static int parseTimezone(const char *zDate, DateTime *p){
  int sgn = 0;
  int nHr, nMn;
  int c;
  while( sqlite3Isspace(*zDate) ){ zDate++; }
  p->tz = 0;
  c = *zDate;
  if( c=='-' ){
    sgn = -1;
  }else if( c=='+' ){
    sgn = +1;
  }else if( c=='Z' || c=='z' ){
    zDate++;
    p->isLocal = 0;
    p->isUtc = 1;
    goto zulu_time;
  }else{
    return c!=0;
  }
  zDate++;
  if( getDigits(zDate, "20b:20e", &nHr, &nMn)!=2 ){
    return 1;
  }
  zDate += 5;
  p->tz = sgn*(nMn + nHr*60);
  if( p->tz==0 ){   /* Forum post 2025-09-17T10:12:14z */
    p->isLocal = 0;
    p->isUtc = 1;
  }
zulu_time:
  while( sqlite3Isspace(*zDate) ){ zDate++; }
  return *zDate!=0;
}
/* …（同文件无关代码省略）… */
**
** Return 1 if there is a parsing error and 0 on success.
*/
static int parseHhMmSs(const char *zDate, DateTime *p){
  int h, m, s;
  double ms = 0.0;
  if( getDigits(zDate, "20c:20e", &h, &m)!=2 ){
/* …（同文件无关代码省略）… */
  p->h = h;
  p->m = m;
  p->s = s + ms;
  if( parseTimezone(zDate, p) ) return 1;
  return 0;
}
/* …（同文件无关代码省略）… */
static void datetimeError(DateTime *p){
  memset(p, 0, sizeof(*p));
  p->isError = 1;
}
/* …（同文件无关代码省略）… */
static void computeJD(DateTime *p){
  int Y, M, D, A, B, X1, X2;

  if( p->validJD ) return;
  if( p->validYMD ){
    Y = p->Y;
    M = p->M;
    D = p->D;
  }else{
    Y = 2000;  /* If no YMD specified, assume 2000-Jan-01 */
    M = 1;
    D = 1;
    if( p->validHMS && p->h==24 ){
      p->h = 0;
      D = 2;
    }
  }
  if( Y<-4713 || Y>9999 || p->rawS ){
    datetimeError(p);
    return;
  }
  if( M<=2 ){
    Y--;
    M += 12;
  }
  A = (Y+4800)/100;
  B = 38 - A + (A/4);
  X1 = 36525*(Y+4716)/100;
  X2 = 306001*(M+1)/10000;
  p->iJD = (sqlite3_int64)((X1 + X2 + D + B - 1524.5 ) * 86400000);
  p->validJD = 1;
  if( p->validHMS ){
    p->iJD += p->h*3600000 + p->m*60000 + (sqlite3_int64)(p->s*1000 + 0.5);
    if( p->tz ){
      p->iJD -= p->tz*60000;
      p->validYMD = 0;
      p->validHMS = 0;
      p->tz = 0;
      p->isUtc = 1;
      p->isLocal = 0;
    }
  }
}
/* …（同文件无关代码省略）… */
static int daysOfMonth(int Y, int M){
  if( (1<<M) & 0x15aa ){
    return 31;
  }else if( M!=2 ){
    return 30;
  }else if( Y%4==0 && (Y%100!=0 || Y%400==0) ){
    return 29;
  }else{
    return 28;
  }
}
/* …（同文件无关代码省略）… */
static void computeFloor(DateTime *p){
  int mxD;         /* Maximum day number for the current month */
  assert( p->validYMD || p->isError );
  assert( p->D>=0 && p->D<=32 );
  assert( p->M>=0 && p->M<=12 );
  mxD = daysOfMonth(p->Y, p->M);
  p->nFloor = p->D>mxD ? p->D - mxD : 0;
}
/* …（同文件无关代码省略）… */
static int parseYyyyMmDd(const char *zDate, DateTime *p){
  int Y, M, D, neg;

  if( zDate[0]=='-' ){
    zDate++;
    neg = 1;
  }else{
    neg = 0;
  }
  if( getDigits(zDate, "40f-21a-21d", &Y, &M, &D)!=3 ){
    return 1;
  }
  if( neg ) Y = -Y;
  zDate += 10;
  while( sqlite3Isspace(*zDate) || 'T'==*(u8*)zDate ){ zDate++; }
  if( parseHhMmSs(zDate, p)==0 ){
    if( p->h==24 ){
      int mxD = daysOfMonth(Y,M);
      p->h = 0;
      D++;
      if( D>mxD ){
        D -= mxD;
        M++;
        if( M>12 ){
          M = 1;
          Y++;
        }
      }
    }
  }else if( *zDate==0 ){
    p->validHMS = 0;
  }else{
    return 1;
  }
  p->validJD = 0;
  p->validYMD = 1;
  p->Y = Y;
  p->M = M;
  p->D = D;
  computeFloor(p);
  if( p->tz ){
    computeJD(p);
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static int setDateTimeToCurrent(sqlite3_context *context, DateTime *p){
  p->iJD = sqlite3StmtCurrentTime(context);
  if( p->iJD>0 ){
    p->validJD = 1;
    p->isUtc = 1;
    p->isLocal = 0;
    clearYMD_HMS_TZ(p);
    return 0;
  }else{
    return 1;
  }
}
/* …（同文件无关代码省略）… */
  double r;
  if( parseYyyyMmDd(zDate,p)==0 ){
    return 0;
  }else if( parseHhMmSs(zDate, p)==0 ){
    return 0;
  }else if( sqlite3StrICmp(zDate,"now")==0 && sqlite3NotPureFunc(context) ){
    return setDateTimeToCurrent(context, p);
/* …（同文件无关代码省略）… */
static void clearYMD_HMS_TZ(DateTime *p){
  p->validYMD = 0;
  p->validHMS = 0;
  p->tz = 0;
}
/* …（同文件无关代码省略）… */

        DateTime tx;
        sqlite3_int64 day;
        int nDay;              /* 1 for a shift of exactly 24:00 */
        int bTimeOnly = z2==z; /* This is not the HH:MM tail of a
                               ** +/-YYYY-MM-DD HH:MM shift, whose own
                               ** nFloor must survive (e0fe679e7c) */
        if( !sqlite3Isdigit(*z2) ) z2++;
        memset(&tx, 0, sizeof(tx));
        if( parseHhMmSs(z2, &tx) ) break;
        nDay = tx.h==24 && tx.tz==0;
        computeJD(&tx);
        tx.iJD -= 43200000;
        day = tx.iJD/86400000;
        tx.iJD -= day*86400000;
        tx.iJD += nDay*(sqlite3_int64)86400000;
        if( z0=='-' ) tx.iJD = -tx.iJD;
        computeJD(p);
        clearYMD_HMS_TZ(p);
