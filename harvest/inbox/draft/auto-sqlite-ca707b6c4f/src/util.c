// AUTO-DRAFT from sqlite/sqlite PR #e0a6d50fa4eef0de4079902df9325a75cc254318
** The (current) algorithm is adapted from the work of Ross Cox at
** https://github.com/rsc/fpfmt
*/
static double sqlite3Fp10Convert2(u64 d, int p){  // <<< BUG ANCHOR
  int b, lp, e, adj, s;
  u32 pwr10l, mid1;
  u64 pwr10h, x, hi, lo, sticky, u, m;
/* …（同文件无关代码省略）… */
  const unsigned char *z = (const unsigned char*)zIn;
  int neg = 0;       /* True for a negative value */
  u64 s = 0;         /* mantissa */
  int d = 0;         /* Value is s * pow(10,d) */
  int mState = 0;    /* 1: digit seen 2: fp 4: hard-zero */
  unsigned v;        /* Value of a single digit */

/* …（同文件无关代码省略）… */
    }
    /* copy digits to exponent */
    if( (v = (unsigned)z[0] - '0')<10 ){
      int exp = v;
      z++;
      mState |= 2;
      while( (v = (unsigned)z[0] - '0')<10 ){
        exp = exp<10000 ? (exp*10 + v) : 10000;
        z++;
      }
      d += esign*exp;
    }else{
      z--;  /* Leave z[0] at 'e' or '+' or '-',
            ** so that the return is 0 or -1 */
