// AUTO-DRAFT from sqlite/sqlite PR #f47440713d2751286d8ca27306019f80bd7ae769
typedef struct Qrf Qrf;
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
** Store string zUtf to pOut as w characters.  If w is negative,
** then right-justify the text.  W is the width in display characters, not
** in bytes.  Double-width unicode characters count as two characters.
** VT100 escape sequences count as zero.  And so forth.
*/
static void qrfWidthPrint(Qrf *p, sqlite3_str *pOut, int w, const char *zUtf){
  const unsigned char *a = (const unsigned char*)zUtf;
/* …（同文件无关代码省略）… */
      n += x;
    }else if( c==0x1b && (k = qrfIsVt100(&a[i]))>0 ){
      i += k;       
    }else if( n>=aw ){
      break;
    }else{
