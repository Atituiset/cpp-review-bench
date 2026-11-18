// AUTO-DRAFT from sqlite/sqlite PR #90ba7148c6d943ec5233002927c18e57cc8a5589
#  define deliberate_fall_through __attribute__((fallthrough));
/* …（同文件无关代码省略）… */
#define PC 0x80 /* pad character */
#define WS 0x81 /* whitespace */
#define ND 0x82 /* Not above or digit-value */
#define PAD_CHAR '='
/* …（同文件无关代码省略）… */
typedef unsigned char u8;
/* …（同文件无关代码省略）… */
#define BX_DV_PROTO(c) \
  ((((u8)(c))<0x80)? (u8)(b64DigitValues[(u8)(c)]) : 0x80)
#define IS_BX_DIGIT(bdp) (((u8)(bdp))<0x80)
/* …（同文件无关代码省略）… */
static char * skipNonB64( char *s, int nc ){
  char c;
  while( nc-- > 0 && (c = *s) && !IS_BX_DIGIT(BX_DV_PROTO(c)) ) ++s;
  return s;
}
  // <<< BUG ANCHOR
/* Decode base64 text into a byte buffer. */
static u8* fromBase64( char *pIn, int ncIn, u8 *pOut ){
  if( ncIn>0 && pIn[ncIn-1]=='\n' ) --ncIn;
  while( ncIn>0 && *pIn!=PAD_CHAR ){
    static signed char nboi[] = { 0, 0, 1, 2, 3 };
    char *pUse = skipNonB64(pIn, ncIn);
    unsigned long qv = 0L;
    int nti, nbo, nac;
    ncIn -= (pUse - pIn);
    pIn = pUse;
    nti = (ncIn>4)? 4 : ncIn;
    ncIn -= nti;
    nbo = nboi[nti];
    if( nbo==0 ) break;
    for( nac=0; nac<4; ++nac ){
      char c = (nac<nti)? *pIn++ : b64Numerals[0];
      u8 bdp = BX_DV_PROTO(c);
      switch( bdp ){
      case ND:
        /*  Treat dark non-digits as pad, but they terminate decode too. */
        ncIn = 0;
        deliberate_fall_through; /* FALLTHRU */
      case WS:
        /* Treat whitespace as pad and terminate this group.*/
        nti = nac;
        deliberate_fall_through; /* FALLTHRU */
      case PC:
        bdp = 0;
        --nbo;
        deliberate_fall_through; /* FALLTHRU */
      default: /* bdp is the digit value. */
        qv = qv<<6 | bdp;
        break;
      }
    }
    switch( nbo ){
    case 3:
      pOut[2] = (qv) & 0xff;
