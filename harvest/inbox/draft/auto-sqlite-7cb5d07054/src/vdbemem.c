// AUTO-DRAFT from sqlite/sqlite PR #12b80e71b6d1d4a9b39bc8b402be52f8f818f676
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define ISPOWEROF2(X)  (((X)&((X)-1))==0)
/* …（同文件无关代码省略）… */
int sqlite3VdbeCheckMemInvariants(Mem *p){
  /* If MEM_Dyn is set then Mem.xDel!=0.  
  ** Mem.xDel might not be initialized if MEM_Dyn is clear.
  */
  assert( (p->flags & MEM_Dyn)==0 || p->xDel!=0 );

  /* MEM_Dyn may only be set if Mem.szMalloc==0.  In this way we
  ** ensure that if Mem.szMalloc>0 then it is safe to do
  ** Mem.z = Mem.zMalloc without having to check Mem.flags&MEM_Dyn.
  ** That saves a few cycles in inner loops. */
  assert( (p->flags & MEM_Dyn)==0 || p->szMalloc==0 );

  /* Cannot have more than one of MEM_Int, MEM_Real, or MEM_IntReal */
  assert( ISPOWEROF2(p->flags & (MEM_Int|MEM_Real|MEM_IntReal)) );

  if( p->flags & MEM_Null ){
    /* Cannot be both MEM_Null and some other type */
    assert( (p->flags & (MEM_Int|MEM_Real|MEM_Str|MEM_Blob|MEM_Agg))==0 );

    /* If MEM_Null is set, then either the value is a pure NULL (the usual
    ** case) or it is a pointer set using sqlite3_bind_pointer() or
    ** sqlite3_result_pointer().  If a pointer, then MEM_Term must also be
    ** set.
    */
    if( (p->flags & (MEM_Term|MEM_Subtype))==(MEM_Term|MEM_Subtype) ){
      /* This is a pointer type.  There may be a flag to indicate what to
      ** do with the pointer. */
      assert( ((p->flags&MEM_Dyn)!=0 ? 1 : 0) +
              ((p->flags&MEM_Ephem)!=0 ? 1 : 0) +
              ((p->flags&MEM_Static)!=0 ? 1 : 0) <= 1 );

      /* No other bits set */
      assert( (p->flags & ~(MEM_Null|MEM_Term|MEM_Subtype|MEM_FromBind
                           |MEM_Dyn|MEM_Ephem|MEM_Static))==0 );
    }else{
      /* A pure NULL might have other flags, such as MEM_Static, MEM_Dyn,
      ** MEM_Ephem, MEM_Cleared, or MEM_Subtype */
    }
  }else{
    /* The MEM_Cleared bit is only allowed on NULLs */
    assert( (p->flags & MEM_Cleared)==0 );
  }

  /* The szMalloc field holds the correct memory allocation size */
  assert( p->szMalloc==0
       || (p->flags==MEM_Undefined 
           && p->szMalloc<=sqlite3DbMallocSize(p->db,p->zMalloc))
       || p->szMalloc==sqlite3DbMallocSize(p->db,p->zMalloc));

  /* If p holds a string or blob, the Mem.z must point to exactly
  ** one of the following:
  **
  **   (1) Memory in Mem.zMalloc and managed by the Mem object
  **   (2) Memory to be freed using Mem.xDel
  **   (3) An ephemeral string or blob
  **   (4) A static string or blob
  */
  if( (p->flags & (MEM_Str|MEM_Blob)) && p->n>0 ){
    assert( 
      ((p->szMalloc>0 && p->z==p->zMalloc)? 1 : 0) +
      ((p->flags&MEM_Dyn)!=0 ? 1 : 0) +
      ((p->flags&MEM_Ephem)!=0 ? 1 : 0) +
      ((p->flags&MEM_Static)!=0 ? 1 : 0) == 1
    );
  }
  return 1;
}
/* …（同文件无关代码省略）… */
SQLITE_NOINLINE int sqlite3VdbeMemGrow(Mem *pMem, int n, int bPreserve){
  assert( sqlite3VdbeCheckMemInvariants(pMem) );
  assert( !sqlite3VdbeMemIsRowSet(pMem) );
  testcase( pMem->db==0 );

  /* If the bPreserve flag is set to true, then the memory cell must already
  ** contain a valid string or blob value.  */
  assert( bPreserve==0 || pMem->flags&(MEM_Blob|MEM_Str) );
  testcase( bPreserve && pMem->z==0 );

  assert( pMem->szMalloc==0
       || (pMem->flags==MEM_Undefined 
           && pMem->szMalloc<=sqlite3DbMallocSize(pMem->db,pMem->zMalloc))
       || pMem->szMalloc==sqlite3DbMallocSize(pMem->db,pMem->zMalloc));
  if( pMem->szMalloc>0 && bPreserve && pMem->z==pMem->zMalloc ){
    if( pMem->db ){
      pMem->z = pMem->zMalloc = sqlite3DbReallocOrFree(pMem->db, pMem->z, n);
    }else{
      pMem->zMalloc = sqlite3Realloc(pMem->z, n);
      if( pMem->zMalloc==0 ) sqlite3_free(pMem->z);
      pMem->z = pMem->zMalloc;
    }
    bPreserve = 0;
  }else{
    if( pMem->szMalloc>0 ) sqlite3DbFreeNN(pMem->db, pMem->zMalloc);
    pMem->zMalloc = sqlite3DbMallocRaw(pMem->db, n);
  }
  if( pMem->zMalloc==0 ){
    sqlite3VdbeMemSetNull(pMem);
    pMem->z = 0;
    pMem->szMalloc = 0;
    return SQLITE_NOMEM_BKPT;
  }else{
    pMem->szMalloc = sqlite3DbMallocSize(pMem->db, pMem->zMalloc);
  }

  if( bPreserve && pMem->z ){
    assert( pMem->z!=pMem->zMalloc );
    memcpy(pMem->zMalloc, pMem->z, pMem->n);
  }
  if( (pMem->flags&MEM_Dyn)!=0 ){
    assert( pMem->xDel!=0 && pMem->xDel!=SQLITE_DYNAMIC );
    pMem->xDel((void *)(pMem->z));
  }

  pMem->z = pMem->zMalloc;
  pMem->flags &= ~(MEM_Dyn|MEM_Ephem|MEM_Static);
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
int sqlite3VdbeMemClearAndResize(Mem *pMem, int szNew){
  assert( CORRUPT_DB || szNew>0 );
  assert( (pMem->flags & MEM_Dyn)==0 || pMem->szMalloc==0 );
  if( pMem->szMalloc<szNew ){
    return sqlite3VdbeMemGrow(pMem, szNew, 0);
  }
  assert( (pMem->flags & MEM_Dyn)==0 );
  pMem->z = pMem->zMalloc;
  pMem->flags &= (MEM_Null|MEM_Int|MEM_Real|MEM_IntReal);
  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
int sqlite3VdbeMemFinalize(Mem *pMem, FuncDef *pFunc){
  sqlite3_context ctx;
  Mem t;
  assert( pFunc!=0 );
  assert( pMem!=0 );
  assert( pMem->db!=0 );
  assert( pFunc->xFinalize!=0 );
  assert( (pMem->flags & MEM_Null)!=0 || pFunc==pMem->u.pDef );
  assert( sqlite3_mutex_held(pMem->db->mutex) );
  memset(&ctx, 0, sizeof(ctx));
  memset(&t, 0, sizeof(t));
  t.flags = MEM_Null;
  t.db = pMem->db;
  ctx.pOut = &t;
  ctx.pMem = pMem;
  ctx.pFunc = pFunc;
  ctx.enc = ENC(t.db);
  pFunc->xFinalize(&ctx); /* IMP: R-24505-23230 */
  assert( (pMem->flags & MEM_Dyn)==0 );
  if( pMem->szMalloc>0 ) sqlite3DbFreeNN(pMem->db, pMem->zMalloc);
  memcpy(pMem, &t, sizeof(t));
  return ctx.isError;
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE void vdbeMemClearExternAndSetNull(Mem *p){
  assert( p->db==0 || sqlite3_mutex_held(p->db->mutex) );
  assert( VdbeMemDynamic(p) );
  if( p->flags&MEM_Agg ){
    sqlite3VdbeMemFinalize(p, p->u.pDef);
    assert( (p->flags & MEM_Agg)==0 );
    testcase( p->flags & MEM_Dyn );
  }
  if( p->flags&MEM_Dyn ){
    assert( p->xDel!=SQLITE_DYNAMIC && p->xDel!=0 );
    p->xDel((void *)p->z);
  }
  p->flags = MEM_Null;
}
/* …（同文件无关代码省略）… */
static SQLITE_NOINLINE void vdbeMemClear(Mem *p){
  if( VdbeMemDynamic(p) ){
    vdbeMemClearExternAndSetNull(p);
  }
  if( p->szMalloc ){
    sqlite3DbFreeNN(p->db, p->zMalloc);
    p->szMalloc = 0;
  }
  p->z = 0;
}
/* …（同文件无关代码省略）… */
void sqlite3VdbeMemRelease(Mem *p){
  assert( sqlite3VdbeCheckMemInvariants(p) );
  if( VdbeMemDynamic(p) || p->szMalloc ){
    vdbeMemClear(p);
  }
}
/* …（同文件无关代码省略）… */
void sqlite3VdbeMemSetNull(Mem *pMem){
  if( VdbeMemDynamic(pMem) ){
    vdbeMemClearExternAndSetNull(pMem);
  }else{
    pMem->flags = MEM_Null;
  }
}
/* …（同文件无关代码省略）… */
int sqlite3VdbeMemIsRowSet(const Mem *pMem){
  return (pMem->flags&(MEM_Blob|MEM_Dyn))==(MEM_Blob|MEM_Dyn)
         && pMem->xDel==sqlite3RowSetDelete;
}
/* …（同文件无关代码省略）… */
int sqlite3VdbeMemSetStr(
  Mem *pMem,          /* Memory cell to set to string value */
  const char *z,      /* String pointer */
  i64 n,              /* Bytes in string, or negative */
  u8 enc,             /* Encoding of z.  0 for BLOBs */
  void (*xDel)(void*) /* Destructor function */
){
  i64 nByte = n;      /* New value for pMem->n */
  int iLimit;         /* Maximum allowed string or blob size */
  u16 flags;          /* New value for pMem->flags */

  assert( pMem!=0 );
  assert( pMem->db==0 || sqlite3_mutex_held(pMem->db->mutex) );
  assert( !sqlite3VdbeMemIsRowSet(pMem) );
  assert( enc!=0 || n>=0 );

  /* If z is a NULL pointer, set pMem to contain an SQL NULL. */
  if( !z ){
    sqlite3VdbeMemSetNull(pMem);
    return SQLITE_OK;
  }

  if( pMem->db ){
    iLimit = pMem->db->aLimit[SQLITE_LIMIT_LENGTH];
  }else{
    iLimit = SQLITE_MAX_LENGTH;
  }
  if( nByte<0 ){
    assert( enc!=0 );
    if( enc==SQLITE_UTF8 ){
      nByte = strlen(z);
    }else{
      for(nByte=0; nByte<=iLimit && (z[nByte] | z[nByte+1]); nByte+=2){}
    }
    flags= MEM_Str|MEM_Term;
  }else if( enc==0 ){
    flags = MEM_Blob;
    enc = SQLITE_UTF8;
  }else{
    flags = MEM_Str;
  }
  if( nByte>iLimit ){
    if( xDel && xDel!=SQLITE_TRANSIENT ){
      if( xDel==SQLITE_DYNAMIC ){
        sqlite3DbFree(pMem->db, (void*)z);
      }else{
        xDel((void*)z);
      }
    }
    sqlite3VdbeMemSetNull(pMem);
    return sqlite3ErrorToParser(pMem->db, SQLITE_TOOBIG);
  }

  /* The following block sets the new values of Mem.z and Mem.xDel. It
  ** also sets a flag in local variable "flags" to indicate the memory
  ** management (one of MEM_Dyn or MEM_Static).
  */
  if( xDel==SQLITE_TRANSIENT ){
    i64 nAlloc = nByte;
    if( flags&MEM_Term ){
      nAlloc += (enc==SQLITE_UTF8?1:2);
    }
    testcase( nAlloc==0 );
    testcase( nAlloc==31 );
    testcase( nAlloc==32 );
    if( sqlite3VdbeMemClearAndResize(pMem, (int)MAX(nAlloc,32)) ){
      return SQLITE_NOMEM_BKPT;
    }
    assert( pMem->z!=0 );
    memcpy(pMem->z, z, nAlloc);
  }else{
    sqlite3VdbeMemRelease(pMem);
    pMem->z = (char *)z;
    if( xDel==SQLITE_DYNAMIC ){
      pMem->zMalloc = pMem->z;
      pMem->szMalloc = sqlite3DbMallocSize(pMem->db, pMem->zMalloc);
    }else{
      pMem->xDel = xDel;
      flags |= ((xDel==SQLITE_STATIC)?MEM_Static:MEM_Dyn);
    }
  }

  pMem->n = (int)(nByte & 0x7fffffff);
  pMem->flags = flags;
  pMem->enc = enc;

#ifndef SQLITE_OMIT_UTF16
  if( enc>SQLITE_UTF8 && sqlite3VdbeMemHandleBom(pMem) ){
    return SQLITE_NOMEM_BKPT;
  }
#endif


  return SQLITE_OK;
}
/* …（同文件无关代码省略）… */
        zVal = sqlite3MPrintf(db, "%s%s", zNeg, pExpr->u.zToken);
        if( zVal==0 ) goto no_mem;
        sqlite3ValueSetStr(pVal, -1, zVal, SQLITE_UTF8, SQLITE_DYNAMIC);
      }
    }
    if( affinity==SQLITE_AFF_BLOB ){
      if( op==TK_FLOAT ){
        assert( pVal && pVal->z && pVal->flags==(MEM_Str|MEM_Term) );
        sqlite3AtoF(pVal->z, &pVal->u.r);
        pVal->flags = MEM_Real;
      }else if( op==TK_INTEGER ){
        /* This case is required by -9223372036854775808 and other strings
        ** that look like integers but cannot be handled by the
        ** sqlite3DecOrHexToI64() call above.  */
/* …（同文件无关代码省略）… */
void sqlite3ValueSetStr(
  sqlite3_value *v,     /* Value to be set */
  int n,                /* Length of string z */
  const void *z,        /* Text of the new string */
  u8 enc,               /* Encoding to use */
  void (*xDel)(void*)   /* Destructor for the string */
){
  if( v ) sqlite3VdbeMemSetStr((Mem *)v, z, n, enc, xDel);
}
