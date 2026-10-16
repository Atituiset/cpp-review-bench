// AUTO-DRAFT from sqlite/sqlite PR #848cad0ec0b9008690af0b3e95b95814875a8c27
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
  // <<< BUG ANCHOR
# define UNUSED_PARAMETER(X)  (void)(X)
/* …（同文件无关代码省略）… */
#define IsSpace(X)  isspace((unsigned char)X)
/* …（同文件无关代码省略）… */
# define SQLITE_DECIMAL_MAX_DIGIT 10000000
/* …（同文件无关代码省略）… */
typedef struct Decimal Decimal;
/* …（同文件无关代码省略）… */
static void decimal_clear(Decimal *p){
  sqlite3_free(p->a);
}
/* …（同文件无关代码省略）… */
static void decimal_free(Decimal *p){
  if( p ){
    decimal_clear(p);
    sqlite3_free(p);
  }
}
/* …（同文件无关代码省略）… */
static Decimal *decimalNewFromText(const char *zIn, int n){
  Decimal *p = 0;
  int i;
  int iExp = 0;

  if( zIn==0 ) goto new_from_text_failed;
  p = sqlite3_malloc64( sizeof(*p) );
  if( p==0 ) goto new_from_text_failed;
  p->sign = 0;
  p->oom = 0;
  p->isInit = 1;
  p->isNull = 0;
  p->nDigit = 0;
  p->nFrac = 0;
  p->a = sqlite3_malloc64( n+1 );
  if( p->a==0 ) goto new_from_text_failed;
  for(i=0; i<n && IsSpace(zIn[i]); i++){}
  if( i<n && zIn[i]=='-' ){
    p->sign = 1;
    i++;
  }else if( i<n && zIn[i]=='+' ){
    i++;
  }
  while( i<n && zIn[i]=='0' ) i++;
  while( i<n ){
    char c = zIn[i];
    if( c>='0' && c<='9' ){
      p->a[p->nDigit++] = c - '0';
    }else if( c=='.' ){
      p->nFrac = p->nDigit + 1;
    }else if( c=='e' || c=='E' ){
      int j = i+1;
      int neg = 0;
      if( j>=n ) break;
      if( zIn[j]=='-' ){
        neg = 1;
        j++;
      }else if( zIn[j]=='+' ){
        j++;
      }
      while( j<n && iExp<1000000 ){
        if( zIn[j]>='0' && zIn[j]<='9' ){
          iExp = iExp*10 + zIn[j] - '0';
        }
        j++;
      }
      if( neg ) iExp = -iExp;
      break;
    }
    i++;
  }
  if( p->nFrac ){
    p->nFrac = p->nDigit - (p->nFrac - 1);
  }
  if( iExp>0 ){
    if( p->nFrac>0 ){
      if( iExp<=p->nFrac ){
        p->nFrac -= iExp;
        iExp = 0;
      }else{
        iExp -= p->nFrac;
        p->nFrac = 0;
      }
    }
    if( iExp>0 ){   
      signed char *a = sqlite3_realloc64(p->a, (sqlite3_int64)p->nDigit
                                     + (sqlite3_int64)iExp + 1 );
      if( a==0 ) goto new_from_text_failed;
      p->a = a;
      memset(p->a+p->nDigit, 0, iExp);
      p->nDigit += iExp;
    }
  }else if( iExp<0 ){
    int nExtra;
    iExp = -iExp;
    nExtra = p->nDigit - p->nFrac - 1;
    if( nExtra ){
      if( nExtra>=iExp ){
        p->nFrac += iExp;
        iExp  = 0;
      }else{
        iExp -= nExtra;
        p->nFrac = p->nDigit - 1;
      }
    }
    if( iExp>0 ){
      signed char *a = sqlite3_realloc64(p->a, (sqlite3_int64)p->nDigit
                                     + (sqlite3_int64)iExp + 1 );
      if( a==0 ) goto new_from_text_failed;
      p->a = a;
      memmove(p->a+iExp, p->a, p->nDigit);
      memset(p->a, 0, iExp);
      p->nDigit += iExp;
      p->nFrac += iExp;
    }
  }
  if( p->sign ){
    for(i=0; i<p->nDigit && p->a[i]==0; i++){}
    if( i>=p->nDigit ) p->sign = 0;
  }
  if( p->nDigit>SQLITE_DECIMAL_MAX_DIGIT ) goto new_from_text_failed;
  return p;

new_from_text_failed:
  if( p ){
    if( p->a ) sqlite3_free(p->a);
    sqlite3_free(p);
  }
  return 0;
}
/* …（同文件无关代码省略）… */
static Decimal *decimal_new(
  sqlite3_context *pCtx,       /* Report error here, if not null */
  sqlite3_value *pIn,          /* Construct the decimal object from this */
  int bTextOnly                /* Always interpret pIn as text if true */
){
  Decimal *p = 0;
  int eType = sqlite3_value_type(pIn);
  if( bTextOnly && (eType==SQLITE_FLOAT || eType==SQLITE_BLOB) ){
    eType = SQLITE_TEXT;
  }
  switch( eType ){
    case SQLITE_TEXT:
    case SQLITE_INTEGER: {
      const char *zIn = (const char*)sqlite3_value_text(pIn);
      int n = sqlite3_value_bytes(pIn);
      p = decimalNewFromText(zIn, n);
      if( p==0 ) goto new_failed;
      break;
    }

    case SQLITE_FLOAT: {
      p = decimalFromDouble(sqlite3_value_double(pIn));
      break;
    }

    case SQLITE_BLOB: {
      const unsigned char *x;
      unsigned int i;
      sqlite3_uint64 v = 0;
      double r;

      if( sqlite3_value_bytes(pIn)!=sizeof(r) ) break;
      x = sqlite3_value_blob(pIn);
      for(i=0; i<sizeof(r); i++){
        v = (v<<8) | x[i];
      }
      memcpy(&r, &v, sizeof(r));
      p = decimalFromDouble(r);
      break;
    }

    case SQLITE_NULL: {
      break;
    }
  }
  return p;

new_failed:
  if( pCtx ) sqlite3_result_error_nomem(pCtx);
  sqlite3_free(p);
  return 0;
}
/* …（同文件无关代码省略）… */
static void decimal_round(Decimal *p, int N){
  int i;
  int nZero;  /* Number of leading zeros */
  if( N<1 ) return;
  if( p==0 ) return;
  if( p->nDigit<=N ) return;
  for(nZero=0; nZero<p->nDigit && p->a[nZero]==0; nZero++){}
  N += nZero;
  if( p->nDigit<=N ) return;
  if( p->a[N]>=5 ){
    /* If all leading digits are 9, increase the number of digits
    ** by adding a new 0 to the front */
    for(i=0; i<N && p->a[i]==9; i++){}
    if( i==N ){
      decimal_expand(p, p->nDigit+1, p->nFrac);
      if( p->oom ) return;
    }

    /* Do the rounding */
    p->a[N-1]++;
    for(i=N-1; i>0 && p->a[i]>9; i--){
      p->a[i] = 0;
      p->a[i-1]++;
    }
    assert( p->a[0]<=9 );
  }
  memset(&p->a[N], 0, p->nDigit - N);
}
/* …（同文件无关代码省略）… */
static void decimal_result_sci(sqlite3_context *pCtx, Decimal *p, int N){
  char *z;       /* The output buffer */
  int i;         /* Loop counter */
  int nZero;     /* Number of leading zeros */
  int nDigit;    /* Number of digits not counting trailing zeros */
  int nFrac;     /* Digits to the right of the decimal point */
  int exp;       /* Exponent value */
  signed char zero;     /* Zero value */
  signed char *a;       /* Array of digits */

  if( p==0 || p->oom ){
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  if( p->isNull ){
    sqlite3_result_null(pCtx);
    return;
  }
  if( N<1 ) N = 0;
  for(nDigit=p->nDigit; nDigit>N && p->a[nDigit-1]==0; nDigit--){}
  for(nZero=0; nZero<nDigit && p->a[nZero]==0; nZero++){}
  nFrac = p->nFrac + (nDigit - p->nDigit);
  nDigit -= nZero;
  z = sqlite3_malloc64( (sqlite3_int64)nDigit+20 );
  if( z==0 ){
    sqlite3_result_error_nomem(pCtx);
    return;
  }
  if( nDigit==0 ){
    zero = 0;
    a = &zero;
    nDigit = 1;
    nFrac = 0;
  }else{
    a = &p->a[nZero];
  }
  if( p->sign && nDigit>0 ){
    z[0] = '-';
  }else{
    z[0] = '+';
  }
  z[1] = a[0]+'0';
  z[2] = '.';
  if( nDigit==1 ){
    z[3] = '0';
    i = 4;
  }else{
    for(i=1; i<nDigit; i++){
      z[2+i] = a[i]+'0';
    }
    i = nDigit+2;
  }
  exp = nDigit - nFrac - 1;
  sqlite3_snprintf(nDigit+20-i, &z[i], "e%+03d", exp);
  sqlite3_result_text(pCtx, z, -1, sqlite3_free);
}
/* …（同文件无关代码省略）… */
static void decimal_expand(Decimal *p, int nDigit, int nFrac){
  int nAddSig;
  int nAddFrac;
  signed char *a;
  if( p==0 ) return;
  nAddFrac = nFrac - p->nFrac;
  assert( nAddFrac>=0 );
  nAddSig = (nDigit - p->nDigit) - nAddFrac;
  if( nAddFrac==0 && nAddSig==0 ) return;
  if( nDigit+1>SQLITE_DECIMAL_MAX_DIGIT ){ p->oom = 1; return; }
  a = sqlite3_realloc64(p->a, nDigit+1);
  if( a==0 ){
    p->oom = 1;
    return;
  }
  p->a = a;
  if( nAddSig ){
    memmove(p->a+nAddSig, p->a, p->nDigit);
    memset(p->a, 0, nAddSig);
    p->nDigit += nAddSig;
  }
  if( nAddFrac ){
    memset(p->a+p->nDigit, 0, nAddFrac);
    p->nDigit += nAddFrac;
    p->nFrac += nAddFrac;
  }
}
/* …（同文件无关代码省略）… */
static void decimalMul(Decimal *pA, Decimal *pB){
  signed char *acc = 0;
  int i, j, k;
  int minFrac;
  sqlite3_int64 sumDigit;

  if( pA==0 || pA->oom || pA->isNull
   || pB==0 || pB->oom || pB->isNull 
  ){
    goto mul_end;
  }
  sumDigit = pA->nDigit;
  sumDigit += pB->nDigit;
  sumDigit += 2;
  if( sumDigit>SQLITE_DECIMAL_MAX_DIGIT ){ pA->oom = 1; return; }
  acc = sqlite3_malloc64( sumDigit );
  if( acc==0 ){
    pA->oom = 1;
    goto mul_end;
  }
  memset(acc, 0, pA->nDigit + pB->nDigit + 2);
  minFrac = pA->nFrac;
  if( pB->nFrac<minFrac ) minFrac = pB->nFrac;
  for(i=pA->nDigit-1; i>=0; i--){
    signed char f = pA->a[i];
    int carry = 0, x;
    for(j=pB->nDigit-1, k=i+j+3; j>=0; j--, k--){
      x = acc[k] + f*pB->a[j] + carry;
      acc[k] = x%10;
      carry = x/10;
    }
    x = acc[k] + carry;
    acc[k] = x%10;
    acc[k-1] += x/10;
  }
  sqlite3_free(pA->a);
  pA->a = acc;
  acc = 0;
  pA->nDigit += pB->nDigit + 2;
  pA->nFrac += pB->nFrac;
  pA->sign ^= pB->sign;
  while( pA->nFrac>minFrac && pA->a[pA->nDigit-1]==0 ){
    pA->nFrac--;
    pA->nDigit--;
  }

mul_end:
  sqlite3_free(acc);
}
/* …（同文件无关代码省略）… */
static Decimal *decimalPow2(int N){
  Decimal *pA = 0;      /* The result to be returned */
  Decimal *pX = 0;      /* Multiplier */
  if( N<-20000 || N>20000 ) goto pow2_fault;
  pA = decimalNewFromText("1.0", 3);
  if( pA==0 || pA->oom ) goto pow2_fault;
  if( N==0 ) return pA;
  if( N>0 ){
    pX = decimalNewFromText("2.0", 3);
  }else{
    N = -N;
    pX = decimalNewFromText("0.5", 3);
  }
  if( pX==0 || pX->oom ) goto pow2_fault;
  while( 1 /* Exit by break */ ){
    if( N & 1 ){
      decimalMul(pA, pX);
      if( pA->oom ) goto pow2_fault;
    }
    N >>= 1;
    if( N==0 ) break;
    decimalMul(pX, pX);
  }
  decimal_free(pX);
  return pA;

pow2_fault:
  decimal_free(pA);
  decimal_free(pX);
  return 0;
}
/* …（同文件无关代码省略）… */
static Decimal *decimalFromDouble(double r){
  sqlite3_int64 m, a;
  int e;
  int isNeg;
  Decimal *pA;
  Decimal *pX;
  char zNum[100];
  if( r<0.0 ){
    isNeg = 1;
    r = -r;
  }else{
    isNeg = 0;
  }
  memcpy(&a,&r,sizeof(a));
  if( a==0 || a==(sqlite3_int64)0x8000000000000000LL){
    e = 0;
    m = 0;
  }else{
    e = a>>52;
    m = a & ((((sqlite3_int64)1)<<52)-1);
    if( e==0 ){
      m <<= 1;
    }else{
      m |= ((sqlite3_int64)1)<<52;
    }
    while( e<1075 && m>0 && (m&1)==0 ){
      m >>= 1;
      e++;
    }
    if( isNeg ) m = -m;
    e = e - 1075;
    if( e>971 ){
      return 0;  /* A NaN or an Infinity */
    }
  }

  /* At this point m is the integer significand and e is the exponent */
  sqlite3_snprintf(sizeof(zNum), zNum, "%lld", m);
  pA = decimalNewFromText(zNum, (int)strlen(zNum));
  pX = decimalPow2(e);
  decimalMul(pA, pX);
  decimal_free(pX);
  return pA;
}

/*
** SQL Function:   decimal(X)
** OR:             decimal_exp(X)
/* …（同文件无关代码省略）… */
  Decimal *p =  decimal_new(context, argv[0], 0);
  int N;
  if( argc==2 ){
    N = sqlite3_value_int(argv[1]);
    if( N>0 ) decimal_round(p, N);
  }else{
    N = 0;
  }
/* …（同文件无关代码省略）… */
){
  UNUSED_PARAMETER(argc);
  if( sqlite3_value_type(argv[0])==SQLITE_INTEGER ){
    Decimal *pA = decimalPow2(sqlite3_value_int(argv[0]));
    decimal_result_sci(context, pA, 0);
    decimal_free(pA);
  }
}
