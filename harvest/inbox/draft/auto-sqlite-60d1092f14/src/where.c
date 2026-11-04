// AUTO-DRAFT from sqlite/sqlite PR #2a9537b719e4e5fc3d5b5ef1bcd6fff8a7387df2
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
  // <<< BUG ANCHOR
  WhereLoop *pLoop;           /* The Loop object */
  char *zNotUsed;             /* Extra space on the end of pIdx */
  Bitmask idxCols;            /* Bitmap of columns used for indexing */
  Bitmask extraCols;          /* Bitmap of additional columns */
  u8 sentWarning = 0;         /* True if a warning has been issued */
  u8 useBloomFilter = 1;      /* True to also add a Bloom filter */
/* …（同文件无关代码省略）… */
  pWCEnd = &pWC->a[pWC->nTerm];
  pLoop = pLevel->pWLoop;
  idxCols = 0;
  for(pTerm=pWC->a; pTerm<pWCEnd; pTerm++){
    Expr *pExpr = pTerm->pExpr;
    /* Make the automatic index a partial index if there are terms in the
/* …（同文件无关代码省略）… */
        }
        pLoop->aLTerm[nKeyCol++] = pTerm;
        idxCols |= cMask;
      }
    }
  }
/* …（同文件无关代码省略）… */
  }
  if( !HasRowid(pTable) ){
    /* For WITHOUT ROWID tables, ensure that all PRIMARY KEY columns are
    ** either in the idxCols mask or in the extraCols mask */
    for(i=0; i<pTable->nCol; i++){
      if( (pTable->aCol[i].colFlags & COLFLAG_PRIMKEY)==0 ) continue;
      if( i>=BMS-1 ){
        extraCols |= MASKBIT(BMS-1);
        break;
      }
      if( idxCols & MASKBIT(i) ) continue;
      extraCols |= MASKBIT(i);
    }
  }
/* …（同文件无关代码省略）… */
  }

  /* Construct the Index object to describe this index */
  assert( nKeyCol <= pTable->nCol + MAX(0, pTable->nCol - BMS + 1) );
  /* ^-- This guarantees that the number of index columns will fit in the u16 */
  pIdx = sqlite3AllocateIndexObject(pParse->db, nKeyCol+HasRowid(pTable),
                                    0, &zNotUsed);
