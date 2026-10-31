// AUTO-DRAFT from sqlite/sqlite PR #49a32e906c18cc061752f88503019efb4a5c8ea0
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdint.h>
  // <<< BUG ANCHOR
#  define UINT32_TYPE uint32_t
/* …（同文件无关代码省略）… */
#  define UINT16_TYPE uint16_t
/* …（同文件无关代码省略）… */
typedef sqlite3_int64 i64;
/* …（同文件无关代码省略）… */
typedef UINT32_TYPE u32;           /* 4-byte unsigned integer */
typedef UINT16_TYPE u16;           /* 2-byte unsigned integer */
/* …（同文件无关代码省略）… */
typedef struct ZipfileCDS ZipfileCDS;
/* …（同文件无关代码省略）… */
static u32 zipfileMtime(ZipfileCDS *pCDS){
  int Y,M,D,X1,X2,A,B,sec,min,hr;
  i64 JDsec;
  Y = (1980 + ((pCDS->mDate >> 9) & 0x7F));
  M = ((pCDS->mDate >> 5) & 0x0F);
  D = (pCDS->mDate & 0x1F);
  sec = (pCDS->mTime & 0x1F)*2;
  min = (pCDS->mTime >> 5) & 0x3F;
  hr = (pCDS->mTime >> 11) & 0x1F;
  if( M<=2 ){
    Y--;
    M += 12;
  }
  X1 = 36525*(Y+4716)/100;
  X2 = 306001*(M+1)/10000;
  A = Y/100;
  B = 2 - A + (A/4);
  JDsec = (i64)((X1 + X2 + D + B - 1524.5)*86400) + hr*3600 + min*60 + sec;
  return (u32)(JDsec - (i64)24405875*(i64)8640);
}
/* …（同文件无关代码省略）… */
  int yr, mon, day;
  int hr, min, sec;

  A = (int)((JD - 1867216.25)/36524.25);
  A = (int)(JD + 1 + A - (A/4));
  B = A + 1524;
/* …（同文件无关代码省略）… */
  min = (mUnixTime % (60*60)) / 60;
  sec = (mUnixTime % 60);

  if( yr>=1980 ){
    pCds->mDate = (u16)(day + (mon << 5) + ((yr-1980) << 9));
    pCds->mTime = (u16)(sec/2 + (min<<5) + (hr<<11));
  }else{
    pCds->mDate = pCds->mTime = 0;
  }

  assert( mUnixTime<315507600 
       || mUnixTime==zipfileMtime(pCds) 
       || ((mUnixTime % 2) && mUnixTime-1==zipfileMtime(pCds)) 
       /* || (mUnixTime % 2) */
  );
}
