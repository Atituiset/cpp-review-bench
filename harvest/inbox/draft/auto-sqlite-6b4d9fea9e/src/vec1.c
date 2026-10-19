// AUTO-DRAFT from sqlite/sqlite PR #948ad6e798467aa18b523d866a35dc41fcdd4548
typedef sqlite3_int64  i64;
typedef sqlite3_uint64 u64;
typedef unsigned char   u8;
typedef unsigned int   u32;
/* …（同文件无关代码省略）… */
static u32 vec1GetU32(const u8 *aBuf){
  return (((u32)aBuf[0]) << 24)
       + (((u32)aBuf[1]) << 16)
       + (((u32)aBuf[2]) <<  8)
       + (((u32)aBuf[3]) <<  0);
}
/* …（同文件无关代码省略）… */
static u64 vec1GetU64(const u8 *aBuf){
  return (((u64)aBuf[0]) << 56)
       + (((u64)aBuf[1]) << 48)
       + (((u64)aBuf[2]) << 40)
       + (((u64)aBuf[3]) << 32) 
       + (((u64)aBuf[4]) << 24)
       + (((u64)aBuf[5]) << 16)
       + (((u64)aBuf[6]) <<  8)
       + (((u64)aBuf[7]) <<  0);
}
/* …（同文件无关代码省略）… */
typedef struct Vec1ListBuilder Vec1ListBuilder;
/* …（同文件无关代码省略）… */
  ** in main memory. */
  if( pTab->pWriter ){
    Vec1ListBuilder *pBld = &pTab->pWriter->aBld[iBucket];
    int ii;
    for(ii=0; ii<pBld->bufRowid.n; ii+=pBld->szRowid){
      i64 iRead;
      if( pBld->szRowid==4 ){  // <<< BUG ANCHOR
        iRead = vec1GetU32(&pBld->bufRowid.a[ii]);
      }else{
        iRead = vec1GetU64(&pBld->bufRowid.a[ii]);
      }
      if( iRead==iRowid ){
        pOut->iEntry = (ii / pBld->szRowid);
        pOut->nEntry = (pBld->bufRowid.n / pBld->szRowid);
        pOut->nTombstone = pBld->nTombstone;
        pOut->szRowid = pBld->szRowid;
        pOut->pList = pBld;
        return SQLITE_OK;
      }
    }
  }
