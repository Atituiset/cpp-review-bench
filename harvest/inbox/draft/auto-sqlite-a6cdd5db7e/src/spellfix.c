// AUTO-DRAFT from sqlite/sqlite PR #f99a038a08cdd366676ffb14deaa405c75a28800
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
#define SPELLFIX_MX_HASH  32
/* …（同文件无关代码省略）… */
typedef struct spellfix1_cursor spellfix1_cursor;
/* …（同文件无关代码省略）… */
  struct spellfix1_row {       /* For each row of content */
    sqlite3_int64 iRowid;         /* Rowid for this row */
    char *zWord;                  /* Text for this row */
    int iRank;                    /* Rank for this row */
    int iDistance;                /* Distance from pattern for this row */
    int iScore;                   /* Score for sorting */
    int iMatchlen;                /* Value of matchlen column (or -1) */
    char zHash[SPELLFIX_MX_HASH]; /* the phonehash used for this match */
  } *a; 
/* …（同文件无关代码省略）… */
/*
** Resize the cursor to hold up to N rows of content
*/
static void spellfix1ResizeCursor(spellfix1_cursor *pCur, int N){
  struct spellfix1_row *aNew;
  assert( N>=pCur->nRow );
  aNew = sqlite3_realloc64(pCur->a, sizeof(pCur->a[0])*N);
/* …（同文件无关代码省略）… */
#define SPELLFIX_IDXNUM_TOP    0x04         /* top = $top */
/* …（同文件无关代码省略）… */
    if( p->iMaxDist>=0 ){
      if( iDist>p->iMaxDist ) continue;
      if( pCur->nRow>=pCur->nAlloc && (pCur->idxNum & SPELLFIX_IDXNUM_TOP)==0 ){
        spellfix1ResizeCursor(pCur, pCur->nAlloc*2 + 10);
        if( pCur->a==0 ) break;
      }
    }
/* …（同文件无关代码省略）… */
  if( idxNum&4 ){
    iLimit = sqlite3_value_int(argv[idx++]);
    if( iLimit<1 ) iLimit = 1;
  }
  if( idxNum&8 ){
    x.iScope = sqlite3_value_int(argv[idx++]);
