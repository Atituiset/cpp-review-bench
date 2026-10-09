// AUTO-DRAFT from sqlite/sqlite PR #bfb858c596f617c7ae74fd74672fae0d101a1588
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
  pSub = pSub1;
  for(pParent=p; pParent; pParent=pParent->pPrior, pSub=pSub->pPrior){
    int nSubSrc;
    u8 jointype = pSubitem->fg.jointype;
    assert( pSub!=0 );
    pSubSrc = pSub->pSrc;     /* FROM clause of subquery */
    nSubSrc = pSubSrc->nSrc;  /* Number of terms in subquery FROM clause */
/* …（同文件无关代码省略）… */
      pSrc = sqlite3SrcListEnlarge(pParse, pSrc, nSubSrc-1,iFrom+1);
      if( pSrc==0 ) break;
      pParent->pSrc = pSrc;
      pSubitem = &pSrc->a[iFrom];
    }

    /* Transfer the FROM clause terms from the subquery into the
/* …（同文件无关代码省略）… */
      pItem->fg.jointype |= (jointype & JT_LTORJ);
      memset(&pSubSrc->a[i], 0, sizeof(pSubSrc->a[i]));
    }
    pSubitem->fg.jointype |= jointype;
 
    /* Begin substituting subquery result set expressions for
    ** references to the iParent in the outer query.
