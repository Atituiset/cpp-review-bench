// AUTO-DRAFT from sqlite/sqlite PR #6de9972df6d8bca1e642feadb438b066375ef072
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Fts5ExprTerm Fts5ExprTerm;
/* …（同文件无关代码省略）… */
struct Fts5Expr {
  Fts5Index *pIndex;
  Fts5Config *pConfig;
  Fts5ExprNode *pRoot;
  int bDesc;                      /* Iterate in descending rowid order */
  int nPhrase;                    /* Number of phrases in expression */
  Fts5ExprPhrase **apExprPhrase;  /* Pointers to phrase objects */
};
/* …（同文件无关代码省略）… */
struct Fts5ExprNode {
  int eType;                      /* Node type */
  int bEof;                       /* True at EOF */
  int bNomatch;                   /* True if entry is not a match */
  int iHeight;                    /* Distance to tree leaf nodes */

  /* Next method for this node. */
  int (*xNext)(Fts5Expr*, Fts5ExprNode*, int, i64);

  i64 iRowid;                     /* Current rowid */
  Fts5ExprNearset *pNear;         /* For FTS5_STRING - cluster of phrases */

  /* Child nodes. For a NOT node, this array always contains 2 entries. For 
  ** AND or OR nodes, it contains 2 or more entries.  */
  int nChild;                     /* Number of child nodes */
  Fts5ExprNode *apChild[FLEXARRAY]; /* Array of child nodes */
};
/* …（同文件无关代码省略）… */
struct Fts5ExprPhrase {
  Fts5ExprNode *pNode;            /* FTS5_STRING node this phrase is part of */
  Fts5Buffer poslist;             /* Current position list */
  int nTerm;                      /* Number of entries in aTerm[] */
  Fts5ExprTerm aTerm[FLEXARRAY];  /* Terms that make up this phrase */
};
/* …（同文件无关代码省略）… */
struct Fts5ExprNearset {
  int nNear;                      /* NEAR parameter */
  Fts5Colset *pColset;            /* Columns to search (NULL -> all columns) */
  int nPhrase;                    /* Number of entries in aPhrase[] array */
  Fts5ExprPhrase *apPhrase[FLEXARRAY]; /* Array of phrase pointers */
};
/* …（同文件无关代码省略）… */
typedef struct Fts5LookaheadReader Fts5LookaheadReader;
/* …（同文件无关代码省略）… */
#define FTS5_LOOKAHEAD_EOF (((i64)1) << 62)
/* …（同文件无关代码省略）… */
static int fts5LookaheadReaderNext(Fts5LookaheadReader *p){
  p->iPos = p->iLookahead;
  if( sqlite3Fts5PoslistNext64(p->a, p->n, &p->i, &p->iLookahead) ){
    p->iLookahead = FTS5_LOOKAHEAD_EOF;
  }
  return (p->iPos==FTS5_LOOKAHEAD_EOF);
}
/* …（同文件无关代码省略）… */
static int fts5LookaheadReaderInit(
  const u8 *a, int n,             /* Buffer to read position list from */
  Fts5LookaheadReader *p          /* Iterator object to initialize */
){
  memset(p, 0, sizeof(Fts5LookaheadReader));
  p->a = a;
  p->n = n;
  fts5LookaheadReaderNext(p);
  return fts5LookaheadReaderNext(p);
}
/* …（同文件无关代码省略）… */
typedef struct Fts5NearTrimmer Fts5NearTrimmer;
/* …（同文件无关代码省略）… */
static int fts5ExprNearIsMatch(int *pRc, Fts5ExprNearset *pNear){
  Fts5NearTrimmer aStatic[4];
  Fts5NearTrimmer *a = aStatic;
  Fts5ExprPhrase **apPhrase = pNear->apPhrase;

  int i;
  int rc = *pRc;
  int bMatch;

  assert( pNear->nPhrase>1 );

  /* If the aStatic[] array is not large enough, allocate a large array
  ** using sqlite3_malloc(). This approach could be improved upon. */
  if( pNear->nPhrase>ArraySize(aStatic) ){
    sqlite3_int64 nByte = sizeof(Fts5NearTrimmer) * pNear->nPhrase;
    a = (Fts5NearTrimmer*)sqlite3Fts5MallocZero(&rc, nByte);
  }else{
    memset(aStatic, 0, sizeof(aStatic));
  }
  if( rc!=SQLITE_OK ){
    *pRc = rc;
    return 0;
  }

  /* Initialize a lookahead iterator for each phrase. After passing the
  ** buffer and buffer size to the lookaside-reader init function, zero
  ** the phrase poslist buffer. The new poslist for the phrase (containing
  ** the same entries as the original with some entries removed on account 
  ** of the NEAR constraint) is written over the original even as it is
  ** being read. This is safe as the entries for the new poslist are a
  ** subset of the old, so it is not possible for data yet to be read to
  ** be overwritten.  */
  for(i=0; i<pNear->nPhrase; i++){
    Fts5Buffer *pPoslist = &apPhrase[i]->poslist;
    fts5LookaheadReaderInit(pPoslist->p, pPoslist->n, &a[i].reader);
    pPoslist->n = 0;
    a[i].pOut = pPoslist;
  }

  while( 1 ){
    int iAdv;
    i64 iMin;
    i64 iMax;

    /* This block advances the phrase iterators until they point to a set of
    ** entries that together comprise a match.  */
    iMax = a[0].reader.iPos;
    do {
      bMatch = 1;
      for(i=0; i<pNear->nPhrase; i++){
        Fts5LookaheadReader *pPos = &a[i].reader;
        iMin = iMax - pNear->apPhrase[i]->nTerm - pNear->nNear;
        if( pPos->iPos<iMin || pPos->iPos>iMax ){
          bMatch = 0;
          while( pPos->iPos<iMin ){
            if( fts5LookaheadReaderNext(pPos) ) goto ismatch_out;
          }
          if( pPos->iPos>iMax ) iMax = pPos->iPos;
        }
      }
    }while( bMatch==0 );

    /* Add an entry to each output position list */
    for(i=0; i<pNear->nPhrase; i++){
      i64 iPos = a[i].reader.iPos;
      Fts5PoslistWriter *pWriter = &a[i].writer;
      if( a[i].pOut->n==0 || iPos!=pWriter->iPrev ){
        sqlite3Fts5PoslistSafeAppend(a[i].pOut, &pWriter->iPrev, iPos);
      }
    }

    iAdv = 0;
    iMin = a[0].reader.iLookahead;
    for(i=0; i<pNear->nPhrase; i++){
      if( a[i].reader.iLookahead < iMin ){
        iMin = a[i].reader.iLookahead;
        iAdv = i;
      }
    }
    if( fts5LookaheadReaderNext(&a[iAdv].reader) ) goto ismatch_out;
  }

  ismatch_out: {
    int bRet = a[0].pOut->n>0;
    *pRc = rc;
    if( a!=aStatic ) sqlite3_free(a);
    return bRet;
  }
}
/* …（同文件无关代码省略）… */
    if( i==pNear->nPhrase && (i==1 || fts5ExprNearIsMatch(pRc, pNear)) ){
      return 1;
    }
    return 0;
  }
}
