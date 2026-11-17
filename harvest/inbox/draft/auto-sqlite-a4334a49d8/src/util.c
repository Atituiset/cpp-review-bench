// AUTO-DRAFT from sqlite/sqlite PR #bcbbcee64ee7edd0b9883029da7251f9af1745d2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
#define SQLITE_USE_UINT128
/* …（同文件无关代码省略）… */
static u64 sqlite3Multiply128(u64 a, u64 b, u64 *pLo){
#if defined(SQLITE_USE_UINT128)
  __uint128_t r = (__uint128_t)a * b;
  *pLo = (u64)r;
  return (u64)(r>>64);
#elif defined(_WIN64) && !defined(SQLITE_DISABLE_INTRINSIC)
  *pLo = a*b;
  return __umulh(a, b);
#else
  u64 a0 = (u32)a;
  u64 a1 = a >> 32;
  u64 b0 = (u32)b;
  u64 b1 = b >> 32;
  u64 a0b0 = a0 * b0;
  u64 a1b1 = a1 * b1;
  u64 a0b1 = a0 * b1;
  u64 a1b0 = a1 * b0;
  u64 t = (a0b0 >> 32) + (u32)a0b1 + (u32)a1b0;
  *pLo = (a0b0 & UINT64_C(0xffffffff)) | (t << 32);
  return a1b1 + (a0b1>>32) + (a1b0>>32) + (t>>32);
#endif
}
/* …（同文件无关代码省略）… */
static u64 sqlite3Multiply160(u64 a, u32 aLo, u64 b, u32 *pLo){
#if defined(SQLITE_USE_UINT128)
  __uint128_t r = (__uint128_t)a * b;
  r += ((__uint128_t)aLo * b) >> 32;
  *pLo = (r>>32)&0xffffffff;
  return r>>64;
#elif defined(_WIN64) && !defined(SQLITE_DISABLE_INTRINSIC)
  u64 r1_hi = __umulh(a,b);
  u64 r1_lo = a*b;
  u64 r2 = (__umulh((u64)aLo,b)<<32) + ((aLo*b)>>32);
  u64 t = r1_lo + r2;
  if( t<r1_lo ) r1_hi++;
  *pLo = t>>32;
  return r1_hi;
#else
  u64 x2 = a>>32;
  u64 x1 = a&0xffffffff;
  u64 x0 = aLo;
  u64 y1 = b>>32;
  u64 y0 = b&0xffffffff;
  u64 x2y1 = x2*y1;
  u64 r4 = x2y1>>32;
  u64 x2y0 = x2*y0;
  u64 x1y1 = x1*y1;
  u64 r3 = (x2y1 & 0xffffffff) + (x2y0 >>32) + (x1y1 >>32);
  u64 x1y0 = x1*y0;
  u64 x0y1 = x0*y1;
  u64 r2 = (x2y0 & 0xffffffff) + (x1y1 & 0xffffffff) +
             (x1y0 >>32) + (x0y1>>32);
  u64 x0y0 = x0*y0;
  u64 r1 = (x1y0 & 0xffffffff) + (x0y1 & 0xffffffff) +
             (x0y0 >>32);
  r2 += r1>>32;
  r3 += r2>>32;
  *pLo = r2&0xffffffff;
  return (r4<<32) + r3;
#endif
}
/* …（同文件无关代码省略）… */
#define U64_BIT(N)  (((u64)1)<<(N))
/* …（同文件无关代码省略）… */
#define POWERSOF10_FIRST (-348)
#define POWERSOF10_LAST  (+347)
/* …（同文件无关代码省略）… */
static u64 powerOfTen(int p, u32 *pLo){
  static const u64 aBase[] = {
    UINT64_C(0x8000000000000000), /*  0: 1.0e+0 << 63 */
    UINT64_C(0xa000000000000000), /*  1: 1.0e+1 << 60 */
    UINT64_C(0xc800000000000000), /*  2: 1.0e+2 << 57 */
    UINT64_C(0xfa00000000000000), /*  3: 1.0e+3 << 54 */
    UINT64_C(0x9c40000000000000), /*  4: 1.0e+4 << 50 */
    UINT64_C(0xc350000000000000), /*  5: 1.0e+5 << 47 */
    UINT64_C(0xf424000000000000), /*  6: 1.0e+6 << 44 */
    UINT64_C(0x9896800000000000), /*  7: 1.0e+7 << 40 */
    UINT64_C(0xbebc200000000000), /*  8: 1.0e+8 << 37 */
    UINT64_C(0xee6b280000000000), /*  9: 1.0e+9 << 34 */
    UINT64_C(0x9502f90000000000), /* 10: 1.0e+10 << 30 */
    UINT64_C(0xba43b74000000000), /* 11: 1.0e+11 << 27 */
    UINT64_C(0xe8d4a51000000000), /* 12: 1.0e+12 << 24 */
    UINT64_C(0x9184e72a00000000), /* 13: 1.0e+13 << 20 */
    UINT64_C(0xb5e620f480000000), /* 14: 1.0e+14 << 17 */
    UINT64_C(0xe35fa931a0000000), /* 15: 1.0e+15 << 14 */
    UINT64_C(0x8e1bc9bf04000000), /* 16: 1.0e+16 << 10 */
    UINT64_C(0xb1a2bc2ec5000000), /* 17: 1.0e+17 << 7 */
    UINT64_C(0xde0b6b3a76400000), /* 18: 1.0e+18 << 4 */
    UINT64_C(0x8ac7230489e80000), /* 19: 1.0e+19 >> 0 */
    UINT64_C(0xad78ebc5ac620000), /* 20: 1.0e+20 >> 3 */
    UINT64_C(0xd8d726b7177a8000), /* 21: 1.0e+21 >> 6 */
    UINT64_C(0x878678326eac9000), /* 22: 1.0e+22 >> 10 */
    UINT64_C(0xa968163f0a57b400), /* 23: 1.0e+23 >> 13 */
    UINT64_C(0xd3c21bcecceda100), /* 24: 1.0e+24 >> 16 */
    UINT64_C(0x84595161401484a0), /* 25: 1.0e+25 >> 20 */
    UINT64_C(0xa56fa5b99019a5c8), /* 26: 1.0e+26 >> 23 */
  };
  static const u64 aScale[] = {
    UINT64_C(0x8049a4ac0c5811ae), /*  0: 1.0e-351 << 1229 */
    UINT64_C(0xcf42894a5dce35ea), /*  1: 1.0e-324 << 1140 */
    UINT64_C(0xa76c582338ed2621), /*  2: 1.0e-297 << 1050 */
    UINT64_C(0x873e4f75e2224e68), /*  3: 1.0e-270 << 960 */
    UINT64_C(0xda7f5bf590966848), /*  4: 1.0e-243 << 871 */
    UINT64_C(0xb080392cc4349dec), /*  5: 1.0e-216 << 781 */
    UINT64_C(0x8e938662882af53e), /*  6: 1.0e-189 << 691 */
    UINT64_C(0xe65829b3046b0afa), /*  7: 1.0e-162 << 602 */
    UINT64_C(0xba121a4650e4ddeb), /*  8: 1.0e-135 << 512 */
    UINT64_C(0x964e858c91ba2655), /*  9: 1.0e-108 << 422 */
    UINT64_C(0xf2d56790ab41c2a2), /* 10: 1.0e-81 << 333 */
    UINT64_C(0xc428d05aa4751e4c), /* 11: 1.0e-54 << 243 */
    UINT64_C(0x9e74d1b791e07e48), /* 12: 1.0e-27 << 153 */
    UINT64_C(0xcccccccccccccccc), /* 13: 1.0e-1 << 67 (special case) */
    UINT64_C(0xcecb8f27f4200f3a), /* 14: 1.0e+27 >> 26 */
    UINT64_C(0xa70c3c40a64e6c51), /* 15: 1.0e+54 >> 116 */
    UINT64_C(0x86f0ac99b4e8dafd), /* 16: 1.0e+81 >> 206 */
    UINT64_C(0xda01ee641a708de9), /* 17: 1.0e+108 >> 295 */
    UINT64_C(0xb01ae745b101e9e4), /* 18: 1.0e+135 >> 385 */
    UINT64_C(0x8e41ade9fbebc27d), /* 19: 1.0e+162 >> 475 */
    UINT64_C(0xe5d3ef282a242e81), /* 20: 1.0e+189 >> 564 */
    UINT64_C(0xb9a74a0637ce2ee1), /* 21: 1.0e+216 >> 654 */
    UINT64_C(0x95f83d0a1fb69cd9), /* 22: 1.0e+243 >> 744 */
    UINT64_C(0xf24a01a73cf2dccf), /* 23: 1.0e+270 >> 833 */
    UINT64_C(0xc3b8358109e84f07), /* 24: 1.0e+297 >> 923 */
    UINT64_C(0x9e19db92b4e31ba9), /* 25: 1.0e+324 >> 1013 */
  };
  static const unsigned int aScaleLo[] = {
    0x205b896d, /*  0: 1.0e-351 << 1229 */
    0x52064cad, /*  1: 1.0e-324 << 1140 */
    0xaf2af2b8, /*  2: 1.0e-297 << 1050 */
    0x5a7744a7, /*  3: 1.0e-270 << 960 */
    0xaf39a475, /*  4: 1.0e-243 << 871 */
    0xbd8d794e, /*  5: 1.0e-216 << 781 */
    0x547eb47b, /*  6: 1.0e-189 << 691 */
    0x0cb4a5a3, /*  7: 1.0e-162 << 602 */
    0x92f34d62, /*  8: 1.0e-135 << 512 */
    0x3a6a07f9, /*  9: 1.0e-108 << 422 */
    0xfae27299, /* 10: 1.0e-81 << 333 */
    0xaa97e14c, /* 11: 1.0e-54 << 243 */
    0x775ea265, /* 12: 1.0e-27 << 153 */
    0xcccccccc, /* 13: 1.0e-1 << 67 (special case) */
    0x00000000, /* 14: 1.0e+27 >> 26 */
    0x999090b6, /* 15: 1.0e+54 >> 116 */
    0x69a028bb, /* 16: 1.0e+81 >> 206 */
    0xe80e6f48, /* 17: 1.0e+108 >> 295 */
    0x5ec05dd0, /* 18: 1.0e+135 >> 385 */
    0x14588f14, /* 19: 1.0e+162 >> 475 */
    0x8f1668c9, /* 20: 1.0e+189 >> 564 */
    0x6d953e2c, /* 21: 1.0e+216 >> 654 */
    0x4abdaf10, /* 22: 1.0e+243 >> 744 */
    0xbc633b39, /* 23: 1.0e+270 >> 833 */
    0x0a862f81, /* 24: 1.0e+297 >> 923 */
    0x6c07a2c2, /* 25: 1.0e+324 >> 1013 */
  };
  int g, n;
  u64 s, x;
  u32 lo;

  assert( p>=POWERSOF10_FIRST && p<=POWERSOF10_LAST );
  if( p<0 ){
    if( p==(-1) ){
      *pLo = aScaleLo[13];
      return aScale[13];
    }
    g = p/27;
    n = p%27;
    if( n ){
      g--;
      n += 27;
    }
  }else if( p<27 ){
    *pLo = 0;
    return aBase[p];
  }else{
    g = p/27;
    n = p%27;
  }
  s = aScale[g+13];
  if( n==0 ){
    *pLo = aScaleLo[g+13];
    return s;
  }
  x = sqlite3Multiply160(s,aScaleLo[g+13],aBase[n],&lo);
  if( (U64_BIT(63) & x)==0 ){
    x  = x<<1 | ((lo>>31)&1);
    lo = (lo<<1) | 1;
  }
  *pLo = lo;
  return x;
}
/* …（同文件无关代码省略）… */
static int pwr10to2(int p){ return (p*108853) >> 15; }
static int pwr2to10(int p){ return (p*78913) >> 18; }
/* …（同文件无关代码省略）… */
static void sqlite3Fp2Convert10(u64 m, int e, int n, u64 *pD, int *pP){
  int p;
  u64 h, d1;
  u32 d2;
  assert( n>=1 && n<=18 );
  p = n - 1 - pwr2to10(e+63);
  h = sqlite3Multiply128(m, powerOfTen(p,&d2), &d1);
  assert( -(e + pwr10to2(p) + 2) >= 0  );
  assert( -(e + pwr10to2(p) + 1) <= 63 );
  if( n==18 ){
    h >>= -(e + pwr10to2(p) + 2);
    *pD = (h + ((h<<1)&2))>>1;
  }else{
    *pD = h >> -(e + pwr10to2(p) + 1);
  }
  *pP = -p;
}
/* …（同文件无关代码省略）… */
    v = (v<<11) | U64_BIT(63);
    e -= 1086;
  }
  sqlite3Fp2Convert10(v, e, (iRound<=0||iRound>=18)?18:iRound+1, &v, &exp);  

  /* Extract significant digits, start at the right-most slot in p->zBuf
  ** and working back to the right.  "i" keeps track of the next slot in
