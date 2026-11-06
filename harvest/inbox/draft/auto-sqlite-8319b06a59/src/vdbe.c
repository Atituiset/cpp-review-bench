// AUTO-DRAFT from sqlite/sqlite PR #bf331a4d0c8066ea2a7bfc9cacf51e924725cdef
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
  // <<< BUG ANCHOR
static int alsoAnInt(Mem *pRec, double rValue, i64 *piValue){
  i64 iValue;
  iValue = sqlite3RealToI64(rValue);
  if( sqlite3RealSameAsInt(rValue,iValue) ){
    *piValue = iValue;
    return 1;
  }
  return 0==sqlite3Atoi64(pRec->z, piValue, pRec->n, pRec->enc);
}
/* …（同文件无关代码省略）… */
static void applyNumericAffinity(Mem *pRec, int bTryForInt){
  double rValue;
  int rc;
  assert( (pRec->flags & (MEM_Str|MEM_Int|MEM_Real|MEM_IntReal))==MEM_Str );
  rc = sqlite3MemRealValueRC(pRec, &rValue);
  if( rc<=0 ) return;
  if( (rc&2)==0 && alsoAnInt(pRec, rValue, &pRec->u.i) ){
    pRec->flags |= MEM_Int;
  }else{
    pRec->u.r = rValue;
    pRec->flags |= MEM_Real;
    if( bTryForInt ) sqlite3VdbeIntegerAffinity(pRec, 1);
  }
  /* TEXT->NUMERIC is many->one.  Hence, it is important to invalidate the
  ** string representation after computing a numeric equivalent, because the
  ** string representation might not be the canonical representation for the
  ** numeric value.  Ticket [343634942dd54ab57b7024] 2018-01-31. */
  pRec->flags &= ~MEM_Str;
}
/* …（同文件无关代码省略）… */
static void applyAffinity(
  Mem *pRec,          /* The value to apply affinity to */
  char affinity,      /* The affinity to be applied */
  u8 enc              /* Use this text encoding */
){
  if( affinity>=SQLITE_AFF_NUMERIC ){
    assert( affinity==SQLITE_AFF_INTEGER || affinity==SQLITE_AFF_REAL
             || affinity==SQLITE_AFF_NUMERIC || affinity==SQLITE_AFF_FLEXNUM );
    if( (pRec->flags & MEM_Int)==0 ){ /*OPTIMIZATION-IF-FALSE*/
      if( (pRec->flags & (MEM_Real|MEM_IntReal))==0 ){
        if( pRec->flags & MEM_Str ) applyNumericAffinity(pRec,1);
      }else if( affinity<=SQLITE_AFF_REAL ){
        sqlite3VdbeIntegerAffinity(pRec, 0);
      }
    }
  }else if( affinity==SQLITE_AFF_TEXT ){
    /* Only attempt the conversion to TEXT if there is an integer or real
    ** representation (blob and NULL do not get converted) but no string
    ** representation.  It would be harmless to repeat the conversion if
    ** there is already a string rep, but it is pointless to waste those
    ** CPU cycles. */
    if( 0==(pRec->flags&MEM_Str) ){ /*OPTIMIZATION-IF-FALSE*/
      if( (pRec->flags&(MEM_Real|MEM_Int|MEM_IntReal)) ){
        testcase( pRec->flags & MEM_Int );
        testcase( pRec->flags & MEM_Real );
        testcase( pRec->flags & MEM_IntReal );
        sqlite3VdbeMemStringify(pRec, enc, 1);
      }
    }
    pRec->flags &= ~(MEM_Real|MEM_Int|MEM_IntReal);
  }
}
/* …（同文件无关代码省略）… */
void sqlite3VdbeMemPrettyPrint(Mem *pMem, StrAccum *pStr){
  int f = pMem->flags;
  static const char *const encnames[] = {"(X)", "(8)", "(16LE)", "(16BE)"};
  if( f&MEM_Blob ){
    int i;
    char c;
    if( f & MEM_Dyn ){
      c = 'z';
      assert( (f & (MEM_Static|MEM_Ephem))==0 );
    }else if( f & MEM_Static ){
      c = 't';
      assert( (f & (MEM_Dyn|MEM_Ephem))==0 );
    }else if( f & MEM_Ephem ){
      c = 'e';
      assert( (f & (MEM_Static|MEM_Dyn))==0 );
    }else{
      c = 's';
    }
    sqlite3_str_appendf(pStr, "%cx[", c);
    for(i=0; i<25 && i<pMem->n; i++){
      sqlite3_str_appendf(pStr, "%02X", ((int)pMem->z[i] & 0xFF));
    }
    sqlite3_str_appendf(pStr, "|");
    for(i=0; i<25 && i<pMem->n; i++){
      char z = pMem->z[i];
      sqlite3_str_appendchar(pStr, 1, (z<32||z>126)?'.':z);
    }
    sqlite3_str_appendf(pStr,"]");
    if( f & MEM_Zero ){
      sqlite3_str_appendf(pStr, "+%dz",pMem->u.nZero);
    }
  }else if( f & MEM_Str ){
    int j;
    u8 c;
    if( f & MEM_Dyn ){
      c = 'z';
      assert( (f & (MEM_Static|MEM_Ephem))==0 );
    }else if( f & MEM_Static ){
      c = 't';
      assert( (f & (MEM_Dyn|MEM_Ephem))==0 );
    }else if( f & MEM_Ephem ){
      c = 'e';
      assert( (f & (MEM_Static|MEM_Dyn))==0 );
    }else{
      c = 's';
    }
    sqlite3_str_appendf(pStr, " %c%d[", c, pMem->n);
    for(j=0; j<25 && j<pMem->n; j++){
      c = pMem->z[j];
      sqlite3_str_appendchar(pStr, 1, (c>=0x20&&c<=0x7f) ? c : '.');
    }
    sqlite3_str_appendf(pStr, "]%s", encnames[pMem->enc]);
    if( f & MEM_Term ){
      sqlite3_str_appendf(pStr, "(0-term)");
    }
  }
}
/* …（同文件无关代码省略）… */
static void memTracePrint(Mem *p){
  if( p->flags & MEM_Undefined ){
    printf(" undefined");
  }else if( p->flags & MEM_Null ){
    printf(p->flags & MEM_Zero ? " NULL-nochng" : " NULL");
  }else if( (p->flags & (MEM_Int|MEM_Str))==(MEM_Int|MEM_Str) ){
    printf(" si:%lld", p->u.i);
  }else if( (p->flags & (MEM_IntReal))!=0 ){
    printf(" ir:%lld", p->u.i);
  }else if( p->flags & MEM_Int ){
    printf(" i:%lld", p->u.i);
#ifndef SQLITE_OMIT_FLOATING_POINT
  }else if( p->flags & MEM_Real ){
    printf(" r:%.17g", p->u.r);
#endif
  }else if( sqlite3VdbeMemIsRowSet(p) ){
    printf(" (rowset)");
  }else{
    StrAccum acc;
    char zBuf[1000];
    sqlite3StrAccumInit(&acc, 0, zBuf, sizeof(zBuf), 0);
    sqlite3VdbeMemPrettyPrint(p, &acc);
    printf(" %s", sqlite3StrAccumFinish(&acc));
  }
  if( p->flags & MEM_Subtype ) printf(" subtype=0x%02x", p->eSubtype);
}
static void registerTrace(int iReg, Mem *p){
  printf("R[%d] = ", iReg);
  memTracePrint(p);
  if( p->pScopyFrom ){
    assert( p->pScopyFrom->bScopy );
    printf(" <== R[%d]", (int)(p->pScopyFrom - &p[-iReg]));
  }
  printf("\n");
  sqlite3VdbeCheckMemInvariants(p);
}
/* …（同文件无关代码省略）… */
#  define REGISTER_TRACE(R,M) if(db->flags&SQLITE_VdbeTrace)registerTrace(R,M)
/* …（同文件无关代码省略）… */
            testcase( pIn1->u.i==-140737488355329LL );
            if( pIn1->u.i<=140737488355327LL && pIn1->u.i>=-140737488355328LL){
              pIn1->flags |= MEM_IntReal;
              pIn1->flags &= ~MEM_Int;
            }else{
              pIn1->u.r = (double)pIn1->u.i;
              pIn1->flags |= MEM_Real;
              pIn1->flags &= ~MEM_Int;
            }
          }else if( (pIn1->flags & (MEM_Real|MEM_IntReal))==0 ){
            goto vdbe_type_error;
/* …（同文件无关代
