// AUTO-DRAFT from sqlite/sqlite PR #e4facb4585c7ef4d1e314e1314b81512408aeab1
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
  // <<< BUG ANCHOR
static const char *explainIndexColumnName(Index *pIdx, int i){
  i = pIdx->aiColumn[i];
  if( i==XN_EXPR ) return "<expr>";
  if( i==XN_ROWID ) return "rowid";
  return pIdx->pTable->aCol[i].zCnName;
}
/* …（同文件无关代码省略）… */
static void disableTerm(WhereLevel *pLevel, WhereTerm *pTerm){
  int nLoop = 0;
  assert( pTerm!=0 );
  while( (pTerm->wtFlags & TERM_CODED)==0
      && (pLevel->iLeftJoin==0 || ExprHasProperty(pTerm->pExpr, EP_OuterON))
      && (pLevel->notReady & pTerm->prereqAll)==0
  ){
    if( nLoop && (pTerm->wtFlags & TERM_LIKE)!=0 ){
      pTerm->wtFlags |= TERM_LIKECOND;
    }else{
      pTerm->wtFlags |= TERM_CODED;
    }
#ifdef WHERETRACE_ENABLED
    if( (sqlite3WhereTrace & 0x4001)==0x4001 ){
      sqlite3DebugPrintf("DISABLE-");
      sqlite3WhereTermPrint(pTerm, (int)(pTerm - (pTerm->pWC->a)));
    }
#endif
    if( pTerm->iParent<0 ) break;
    pTerm = &pTerm->pWC->a[pTerm->iParent];
    assert( pTerm!=0 );
    pTerm->nChild--;
    if( pTerm->nChild!=0 ) break;
    nLoop++;
  }
}
/* …（同文件无关代码省略）… */
static void adjustOrderByCol(ExprList *pOrderBy, ExprList *pEList){
  int i, j;
  if( pOrderBy==0 ) return;
  for(i=0; i<pOrderBy->nExpr; i++){
    int t = pOrderBy->a[i].u.x.iOrderByCol;
    if( t==0 ) continue;
    for(j=0; j<pEList->nExpr; j++){
      if( pEList->a[j].u.x.iOrderByCol==t ){
        pOrderBy->a[i].u.x.iOrderByCol = j+1;
        break;
      }
    }
    if( j>=pEList->nExpr ){
      pOrderBy->a[i].u.x.iOrderByCol = 0;
    }
  }
}
/* …（同文件无关代码省略）… */
static Expr *removeUnindexableInClauseTerms(
  Parse *pParse,        /* The parsing context */
  int iEq,              /* Look at loop terms starting here */
  WhereLoop *pLoop,     /* The current loop */
  Expr *pX              /* The IN expression to be reduced */
){
  sqlite3 *db = pParse->db;
  Select *pSelect;            /* Pointer to the SELECT on the RHS */
  Expr *pNew;
  pNew = sqlite3ExprDup(db, pX, 0);
  if( db->mallocFailed==0 ){
    for(pSelect=pNew->x.pSelect; pSelect; pSelect=pSelect->pPrior){
      ExprList *pOrigRhs;         /* Original unmodified RHS */
      ExprList *pOrigLhs = 0;     /* Original unmodified LHS */
      ExprList *pRhs = 0;         /* New RHS after modifications */
      ExprList *pLhs = 0;         /* New LHS after mods */
      int i;                      /* Loop counter */
      int nRhs = 0;               /* Number of RHS terms added so far */

      assert( ExprUseXSelect(pNew) );
      pOrigRhs = pSelect->pEList;
      assert( pNew->pLeft!=0 );
      assert( ExprUseXList(pNew->pLeft) );
      if( pSelect==pNew->x.pSelect ){
        pOrigLhs = pNew->pLeft->x.pList;
      }
      for(i=iEq; i<pLoop->nLTerm; i++){
        if( pLoop->aLTerm[i]->pExpr==pX ){
          int iField;
          assert( (pLoop->aLTerm[i]->eOperator & (WO_OR|WO_AND))==0 );
          iField = pLoop->aLTerm[i]->u.x.iField - 1;
          if( iField!=nRhs ){
            ExprClearProperty(pNew, EP_Subrtn);
          }
          if( NEVER(pOrigRhs->a[iField].pExpr==0) ){
            continue; /* Duplicate PK column */
          }
          pRhs = sqlite3ExprListAppend(pParse, pRhs, pOrigRhs->a[iField].pExpr);
          pOrigRhs->a[iField].pExpr = 0;
          if( pRhs ) pRhs->a[pRhs->nExpr-1].u.x.iOrderByCol = iField+1;
          if( pOrigLhs ){
            assert( pOrigLhs->a[iField].pExpr!=0 );
            pLhs = sqlite3ExprListAppend(pParse,pLhs,pOrigLhs->a[iField].pExpr);
            pOrigLhs->a[iField].pExpr = 0;
          }
          nRhs++;
        }
      }
      sqlite3ExprListDelete(db, pOrigRhs);
      if( pOrigLhs ){
        sqlite3ExprListDelete(db, pOrigLhs);
        pNew->pLeft->x.pList = pLhs;
      }
      pSelect->pEList = pRhs;
      pSelect->selId = ++pParse->nSelect; /* Req'd for SubrtnSig validity */
      if( pLhs && pLhs->nExpr==1 ){
        /* Take care here not to generate a TK_VECTOR containing only a
        ** single value. Since the parser never creates such a vector, some
        ** of the subroutines do not handle this case.  */
        Expr *p = pLhs->a[0].pExpr;
        pLhs->a[0].pExpr = 0;
        sqlite3ExprDelete(db, pNew->pLeft);
        pNew->pLeft = p;
      }

      /* If either the ORDER BY clause or the GROUP BY clause contains
      ** references to result-set columns, those references might now be
      ** obsolete.  So fix them up.
      */
      assert( pRhs!=0 || db->mallocFailed );
      if( pRhs ){
        adjustOrderByCol(pSelect->pOrderBy, pRhs);
        adjustOrderByCol(pSelect->pGroupBy, pRhs);
        for(i=0; i<pRhs->nExpr; i++) pRhs->a[i].u.x.iOrderByCol = 0;
      }

#if 0
      printf("For indexing, change the IN expr:\n");
      sqlite3TreeViewExpr(0, pX, 0);
      printf("Into:\n");
      sqlite3TreeViewExpr(0, pNew, 0);
#endif
    }
  }
  return pNew;
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE void codeINTerm(
  Parse *pParse,      /* The parsing context */
  WhereTerm *pTerm,   /* The term of the WHERE clause to be coded */
  WhereLevel *pLevel, /* The level of the FROM clause we are working on */
  int iEq,            /* Index of the equality term within this level */
  int bRev,           /* True for reverse-order IN operations */
  int iTarget         /* Attempt to leave results in this register */
){
  Expr *pX = pTerm->pExpr;
  int eType = IN_INDEX_NOOP;
  int iTab;
  struct InLoop *pIn;
  WhereLoop *pLoop = pLevel->pWLoop;
  Vdbe *v = pParse->pVdbe;
  int i;
  int nEq = 0;
  int *aiMap = 0;

  if( (pLoop->wsFlags & WHERE_VIRTUALTABLE)==0
    && pLoop->u.btree.pIndex!=0
    && pLoop->u.btree.pIndex->aSortOrder[iEq]
  ){
    testcase( iEq==0 );
    testcase( bRev );
    bRev = !bRev;
  }
  assert( pX->op==TK_IN );

  for(i=0; i<iEq; i++){
    if( pLoop->aLTerm[i] && pLoop->aLTerm[i]->pExpr==pX ){
      disableTerm(pLevel, pTerm);
      return;
    }
  }
  for(i=iEq; i<pLoop->nLTerm; i++){
    assert( pLoop->aLTerm[i]!=0 );
    if( pLoop->aLTerm[i]->pExpr==pX ) nEq++;
  }

  iTab = 0;
  if( !ExprUseXSelect(pX) || pX->x.pSelect->pEList->nExpr==1 ){
    eType = sqlite3FindInIndex(pParse, pX, IN_INDEX_LOOP, 0, 0, &iTab);
  }else{
    sqlite3 *db = pParse->db;
    Expr *pXMod = removeUnindexableInClauseTerms(pParse, iEq, pLoop, pX);
    if( nEq>1 ){
      /* If this IN(SELECT ...) expression drives more than one column of
      ** the index, disable the seek-scan optimization. The reasons for this
      ** are that (a) it is only possible to make this happen by populating
      ** the sqlite_stat1 table with inconsistent information, and (b) it
      ** would require sqlite3FindInIndex() to find an index that is not
      ** only unique for the columns in question, but also delivers them
      ** in sorted order (requires checking asc/desc, and rejecting cases
      ** where the indexed columns are not in the right order).  */
      pLoop->wsFlags &= ~WHERE_IN_SEEKSCAN;
    }
    if( !db->mallocFailed ){
      int nCol = pX->x.pSelect->pEList->nExpr;
      aiMap = (int*)sqlite3DbMallocZero(db, sizeof(int)*nCol);
      eType = sqlite3FindInIndex(pParse, pXMod, IN_INDEX_LOOP, 0, aiMap, &iTab);
    }
    sqlite3ExprDelete(db, pXMod);
  }

  if( eType==IN_INDEX_INDEX_DESC ){
    testcase( bRev );
    bRev = !bRev;
  }
  sqlite3VdbeAddOp2(v, bRev ? OP_Last : OP_Rewind, iTab, 0);
  VdbeCoverageIf(v, bRev);
  VdbeCoverageIf(v, !bRev);

  assert( (pLoop->wsFlags & WHERE_MULTI_OR)==0 );
  pLoop->wsFlags |= WHERE_IN_ABLE;
  if( pLevel->u.in.nIn==0 ){
    pLevel->addrNxt = sqlite3VdbeMakeLabel(pParse);
  }
  if( iEq>0 && (pLoop->wsFlags & WHERE_IN_SEEKSCAN)==0 ){
    pLoop->wsFlags |= WHERE_IN_EARLYOUT;
  }

  i = pLevel->u.in.nIn;
  pLevel->u.in.nIn += nEq;
  pLevel->u.in.aInLoop =
     sqlite3WhereRealloc(pTerm->pWC->pWInfo,
                         pLevel->u.in.aInLoop,
                         sizeof(pLevel->u.in.aInLoop[0])*pLevel->u.in.nIn);
  pIn = pLevel->u.in.aInLoop;
  if( pIn ){
    int iMap = 0;               /* Index in aiMap[] */
    pIn += i;
    for(i=iEq; i<pLoop->nLTerm; i++){
      if( pLoop->aLTerm[i]->pExpr==pX ){
        int iOut = iTarget + i - iEq;
        if( eType==IN_INDEX_ROWID ){
          pIn->addrInTop = sqlite3VdbeAddOp2(v, OP_Rowid, iTab, iOut);
        }else{
          int iCol = aiMap ? aiMap[iMap++] : 0;
          pIn->addrInTop = sqlite3VdbeAddOp3(v,OP_Column,iTab, iCol, iOut);
        }
        sqlite3VdbeAddOp1(v, OP_IsNull, iOut); VdbeCoverage(v);
        if( i==iEq ){
          pIn->iCur = iTab;
          pIn->eEndLoopOp = bRev ? OP_Prev : OP_Next;
          if( iEq>0 ){
            pIn->iBase = iTarget - i;
            pIn->nPrefix = i;
          }else{
            pIn->nPrefix = 0;
          }
        }else{
          pIn->eEndLoopOp = OP_Noop;
        }
        pIn++;
      }
    }
    testcase( iEq>0
              && (pLoop->wsFlags & WHERE_IN_SEEKSCAN)==0
              && (pLoop->wsFlags & WHERE_VIRTUALTABLE)!=0 );
    if( iEq>0
     && (pLoop->wsFlags & (WHERE_IN_SEEKSCAN|WHERE_VIRTUALTABLE))==0
    ){
      sqlite3VdbeAddOp3(v, OP_SeekHit, pLevel->iIdxCur, 0, iEq);
    }
  }else{
    pLevel->u.in.nIn = 0;
  }
  sqlite3DbFree(pParse->db, aiMap);
}
/* …（同文件无关代码省略）… */
static int codeEqualityTerm(
  Parse *pParse,      /* The parsing context */
  WhereTerm *pTerm,   /* The term of the WHERE clause to be coded */
  WhereLevel *pLevel, /* The level of the FROM clause we are working on */
  int iEq,            /* Index of the equality term within this level */
  int bRev,           /* True for reverse-order IN operations */
  int iTarget         /* Attempt to leave results in this register */
){
  Expr *pX = pTerm->pExpr;
  int iReg;                  /* Register holding results */

  assert( pLevel->pWLoop->aLTerm[iEq]==pTerm );
  assert( iTarget>0 );
  if( pX->op==TK_EQ || pX->op==TK_IS ){
    iReg = sqlite3ExprCodeTarget(pParse, pX->pRight, iTarget);
  }else if( pX->op==TK_ISNULL ){
    iReg = iTarget;
    sqlite3VdbeAddOp2(pParse->pVdbe, OP_Null, 0, iReg);
#ifndef SQLITE_OMIT_SUBQUERY
  }else{
    assert( pX->op==TK_IN );
    iReg = iTarget;
    codeINTerm(pParse, pTerm, pLevel, iEq, bRev, iTarget);
#endif
  }

  /* As an optimization, try to disable the WHERE clause term that is
  ** driving the index as it will always be true.  The correct answer is
  ** obtained regardless, but we might get the answer with fewer CPU cycles
  ** by omitting the term.
  **
  ** But do not disable the term unless we are certain that the term is
  ** not a transitive constraint.  For an example of where that does not
  ** work, see https://sqlite.org/forum/forumpost/eb8613976a (2021-05-04)
  */
  if( (pLevel->pWLoop->wsFlags & WHERE_TRANSCONS)==0
   || (pTerm->eOperator & WO_EQUIV)==0
  ){
    disableTerm(pLevel, pTerm);
  }

  return iReg;
}
/* …（同文件无关代码省略）… */
static int codeAllEqualityTerms(
  Parse *pParse,        /* Parsing context */
  WhereLevel *pLevel,   /* Which nested loop of the FROM we are coding */
  int bRev,             /* Reverse the order of IN operators */
  int nExtraReg,        /* Number of extra registers to allocate */
  char **pzAff          /* OUT: Set to point to affinity string */
){
  u16 nEq;                      /* The number of == or IN constraints to code */
  u16 nSkip;                    /* Number of left-most columns to skip */
  Vdbe *v = pParse->pVdbe;      /* The vm under construction */
  Index *pIdx;                  /* The index being used for this loop */
  WhereTerm *pTerm;             /* A single constraint term */
  WhereLoop *pLoop;             /* The WhereLoop object */
  int j;                        /* Loop counter */
  int regBase;                  /* Base register */
  int nReg;                     /* Number of registers to allocate */
  char *zAff;                   /* Affinity string to return */

  /* This module is only called on query plans that use an index. */
  pLoop = pLevel->pWLoop;
  assert( (pLoop->wsFlags & WHERE_VIRTUALTABLE)==0 );
  nEq = pLoop->u.btree.nEq;
  nSkip = pLoop->nSkip;
  pIdx = pLoop->u.btree.pIndex;
  assert( pIdx!=0 );

  /* Figure out how many memory cells we will need then allocate them.
  */
  regBase = pParse->nMem + 1;
  nReg = nEq + nExtraReg;
  pParse->nMem += nReg;

  zAff = sqlite3DbStrDup(pParse->db,sqlite3IndexAffinityStr(pParse->db,pIdx));
  assert( zAff!=0 || pParse->db->mallocFailed );

  if( nSkip ){
    int iIdxCur = pLevel->iIdxCur;
    sqlite3VdbeAddOp3(v, OP_Null, 0, regBase, regBase+nSkip-1);
    sqlite3VdbeAddOp1(v, (bRev?OP_Last:OP_Rewind), iIdxCur);
    VdbeCoverageIf(v, bRev==0);
    VdbeCoverageIf(v, bRev!=0);
    VdbeComment((v, "begin skip-scan on %s", pIdx->zName));
    j = sqlite3VdbeAddOp0(v, OP_Goto);
    assert( pLevel->addrSkip==0 );
    pLevel->addrSkip = sqlite3VdbeAddOp4Int(v, (bRev?OP_SeekLT:OP_SeekGT),
                            iIdxCur, 0, regBase, nSkip);
    VdbeCoverageIf(v, bRev==0);
    VdbeCoverageIf(v, bRev!=0);
    sqlite3VdbeJumpHere(v, j);
    for(j=0; j<nSkip; j++){
      sqlite3VdbeAddOp3(v, OP_Column, iIdxCur, j, regBase+j);
      testcase( pIdx->aiColumn[j]==XN_EXPR );
      VdbeComment((v, "%s", explainIndexColumnName(pIdx, j)));
    }
  }

  /* Evaluate the equality constraints
  */
  assert( zAff==0 || (int)strlen(zAff)>=nEq );
  for(j=nSkip; j<nEq; j++){
    int r1;
    pTerm = pLoop->aLTerm[j];
    assert( pTerm!=0 );
    /* The following testcase is true for indices with redundant columns.
    ** Ex: CREATE INDEX i1 ON t1(a,b,a); SELECT * FROM t1 WHERE a=0 AND b=0; */
    testcase( (pTerm->wtFlags & TERM_CODED)!=0 );
    testcase( pTerm->wtFlags & TERM_VIRTUAL );
    r1 = codeEqualityTerm(pParse, pTerm, pLevel, j, bRev, regBase+j);
    if( r1!=regBase+j ){
      sqlite3VdbeAddOp2(v, OP_Copy, r1, regBase+j);
    }
    if( pTerm->eOperator & WO_IN ){
      if( pTerm->pExpr->flags & EP_xIsSelect ){
        /* No affinity ever needs to be (or should be) applied to a value
        ** from the RHS of an "? IN (SELECT ...)" expression. The
        ** sqlite3FindInIndex() routine has already ensured that the
        ** affinity of the comparison has been applied to the value.  */
        if( zAff ) zAff[j] = SQLITE_AFF_BLOB;
      }
    }else if( (pTerm->eOperator & WO_ISNULL)==0 ){
      Expr *pRight = pTerm->pExpr->pRight;
      if( (pTerm->wtFlags & TERM_IS)==0 && sqlite3ExprCanBeNull(pRight) ){
        sqlite3VdbeAddOp2(v, OP_IsNull, regBase+j, pLevel->addrBrk);
        VdbeCoverage(v);
      }
      if( pParse->nErr==0 ){
        assert( pParse->db->mallocFailed==0 );
        if( sqlite3CompareAffinity(pRight, zAff[j])==SQLITE_AFF_BLOB ){
          zAff[j] = SQLITE_AFF_BLOB;
        }
        if( sqlite3ExprNeedsNoAffinityChange(pRight, zAff[j]) ){
          zAff[j] = SQLITE_AFF_BLOB;
        }
      }
    }
  }
  *pzAff = zAff;
  return regBase;
}
/* …（同文件无关代码省略）… */
# define codeCursorHint(A,B,C,D)  /* No-op */
/* …（同文件无关代码省略）… */
        assert( pRangeStart!=0 );                     /* LIKE opt constraints */
        assert( pRangeStart->wtFlags & TERM_LIKEOPT );   /* occur in pairs */
        pLevel->iLikeRepCntr = (u32)++pParse->nMem;
        sqlite3VdbeAddOp2(v, OP_Integer, 1, (int)pLevel->iLikeRepCntr);
        VdbeComment((v, "LIKE loop counter"));
        pLevel->addrLikeRep = sqlite3VdbeCurrentAddr(v);
        /* iLikeRepCntr actually stores 2x the counter register number.  The
        ** bottom bit indicates whether the search order is ASC or DESC. */
        testcase( bRev );
/* …（同文件无关代码省略）… */
    codeCursorHint(pTabItem, pWInfo, pLevel, pRangeEnd);
    regBase = codeAllEqualityTerms(pParse,pLevel,bRev,nExtraReg,&zStartAff);
    assert( zStartAff==0 || sqlite3Strlen30(zStartAff)>=nEq );
    if( zStartAff && nTop ){
      zEndAff = sqlite3DbStrDup(db, &zStartAff[nEq]);
    }
