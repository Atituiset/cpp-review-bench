// AUTO-DRAFT from sqlite/sqlite PR #cbe0ed9f17583dc90cfb37dc0b6c6abd3cedd864
#define VEC1_VECSIZE_MAX   4096
/* …（同文件无关代码省略）… */
#define VEC1_MAX_BUCKET            65536
/* …（同文件无关代码省略）… */

  /* Check header fields are within the acceptable ranges. This also serves
  ** to ensure that the 32-bit arithmetic used to calculate offsets below
  ** does not overflow.  */
  if( pMod->hdr.nElem>VEC1_VECSIZE_MAX 
   || pMod->hdr.nBucket>VEC1_MAX_BUCKET 
   || pMod->hdr.nCodebook>pMod->hdr.nElem
  ){
    *pzErr = sqlite3_mprintf("vec1: model is corrupt");
    return SQLITE_ERROR;
