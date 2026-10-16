// AUTO-DRAFT from sqlite/sqlite PR #e9183c57f2f8544b35f82b163a140af4bd35d1cc
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <time.h>
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
static int parseHhMmSs(const char *zDate, DateTime *p){
  int h, m, s;
  double ms = 0.0;
  if( getDigits(zDate, "20c:20e", &h, &m)!=2 ){
    return 1;
  }
  zDate += 5;
  if( *zDate==':' ){
    zDate++;
    if( getDigits(zDate, "20e", &s)!=1 ){
      return 1;
    }
    zDate += 2;
    if( *zDate=='.' && sqlite3Isdigit(zDate[1]) ){
      double rScale = 1.0;
      zDate++;
      while( sqlite3Isdigit(*zDate) ){
        ms = ms*10.0 + *zDate - '0';
        rScale *= 10.0;
        zDate++;
      }
      ms /= rScale;
      /* Truncate to avoid problems with sub-milliseconds
      ** rounding. https://sqlite.org/forum/forumpost/766a2c9231 */
      if( ms>0.999 ) ms = 0.999;
    }
  }else{
    s = 0;
  }
  if( h==24 && (m!=0 || s!=0 || ms!=0.0) ) return 1;
  p->validJD = 0;
  p->rawS = 0;
  p->validHMS = 1;
  p->h = h;
  p->m = m;
  p->s = s + ms;
  if( parseTimezone(zDate, p) ) return 1;
  return 0;
}
/* …（同文件无关代码省略）… */
  }
}

/*
** Given the YYYY-MM-DD information current in p, determine if there
** is day-of-month overflow and set nFloor to the number of days that
** would need to be subtracted from the date in order to bring the
** date back to the end of the month.
*/
static void computeFloor(DateTime *p){
  assert( p->validYMD || p->isError );
  assert( p->D>=0 && p->D<=31 );
  assert( p->M>=0 && p->M<=12 );
  if( p->validHMS && p->h==24 ){
    p->h = 0;
    p->D++;
  }
  if( p->D<=28 ){
    p->nFloor = 0;
  }else if( (1<<p->M) & 0x15aa ){
    p->nFloor = 0;
  }else if( p->M!=2 ){
    p->nFloor = (p->D==31);
  }else if( p->Y%4!=0 || (p->Y%100==0 && p->Y%400!=0) ){
    p->nFloor = p->D - 28;
  }else{
    p->nFloor = p->D - 29;
  }
}

/*
/* …（同文件无关代码省略）… */
  zDate += 10;
  while( sqlite3Isspace(*zDate) || 'T'==*(u8*)zDate ){ zDate++; }
  if( parseHhMmSs(zDate, p)==0 ){
    /* We got the time */
  }else if( *zDate==0 ){
    p->validHMS = 0;
  }else{
