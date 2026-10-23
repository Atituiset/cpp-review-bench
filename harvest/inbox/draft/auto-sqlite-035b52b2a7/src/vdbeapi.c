// AUTO-DRAFT from sqlite/sqlite PR #bc766d966a3690f848e8c9ace9f01380f98ab9cd
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <assert.h>
#include <string.h>
  // <<< BUG ANCHOR
static UnpackedRecord *vdbeUnpackRecord(
  KeyInfo *pKeyInfo,
  int nKey,
  const void *pKey
){
  UnpackedRecord *pRet;           /* Return value */

  pRet = sqlite3VdbeAllocUnpackedRecord(pKeyInfo);
  if( pRet ){
    memset(pRet->aMem, 0, sizeof(Mem)*(pKeyInfo->nKeyField+1));
    sqlite3VdbeRecordUnpack(nKey, pKey, pRet);
  }
  return pRet;
}
/* …（同文件无关代码省略）… */

      assert( p->pCsr->eCurType==CURTYPE_BTREE );
      nRec = sqlite3BtreePayloadSize(p->pCsr->uc.pCursor);
      aRec = sqlite3DbMallocRaw(db, nRec);
      if( !aRec ) goto preupdate_old_out;
      rc = sqlite3BtreePayload(p->pCsr->uc.pCursor, 0, nRec, aRec);
      if( rc==SQLITE_OK ){
        p->pUnpacked = vdbeUnpackRecord(p->pKeyinfo, nRec, aRec);
        if( !p->pUnpacked ) rc = SQLITE_NOMEM;
      }
